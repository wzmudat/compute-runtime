/*
 * Copyright (C) 2018-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/command_stream/device_command_stream.h"
#include "shared/source/execution_environment/execution_environment.h"
#include "shared/source/helpers/aligned_memory.h"
#include "shared/source/os_interface/linux/drm_buffer_object.h"
#include "shared/source/os_interface/linux/drm_command_stream.h"
#include "shared/source/os_interface/linux/drm_gem_close_worker.h"
#include "shared/source/os_interface/linux/drm_memory_manager.h"
#include "shared/source/os_interface/linux/drm_memory_operations_handler.h"
#include "shared/source/os_interface/os_interface.h"
#include "shared/test/common/mocks/mock_execution_environment.h"

#include "opencl/source/mem_obj/buffer.h"
#include "opencl/test/unit_test/os_interface/linux/device_command_stream_fixture.h"
#include "test.h"

#include "drm/i915_drm.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <iostream>
#include <memory>
#include <mutex>
#include <sched.h>
#include <thread>

using namespace NEO;

class DrmMockForWorker : public Drm {
  public:
    std::mutex mutex;
    std::atomic<int> gem_close_cnt;
    std::atomic<int> gem_close_expected;
    std::atomic<std::thread::id> ioctl_caller_thread_id;
    DrmMockForWorker() : Drm(std::make_unique<HwDeviceIdDrm>(mockFd, mockPciPath), *platform()->peekExecutionEnvironment()->rootDeviceEnvironments[0]) {
    }
    int ioctl(unsigned long request, void *arg) override {
        if (_IOC_TYPE(request) == DRM_IOCTL_BASE) {
            //when drm ioctl is called, try acquire mutex
            //main thread can hold mutex, to prevent ioctl handling
            std::lock_guard<std::mutex> lock(mutex);
        }
        if (request == DRM_IOCTL_GEM_CLOSE)
            gem_close_cnt++;

        ioctl_caller_thread_id = std::this_thread::get_id();

        return 0;
    };
};

class DrmGemCloseWorkerFixture {
  public:
    DrmGemCloseWorkerFixture() : executionEnvironment(defaultHwInfo.get()){};
    //max loop count for while
    static const uint32_t deadCntInit = 10 * 1000 * 1000;

    DrmMemoryManager *mm;
    DrmMockForWorker *drmMock;
    uint32_t deadCnt = deadCntInit;

    void SetUp() {
        this->drmMock = new DrmMockForWorker;

        executionEnvironment.rootDeviceEnvironments[0]->osInterface = std::make_unique<OSInterface>();
        executionEnvironment.rootDeviceEnvironments[0]->osInterface->setDriverModel(std::unique_ptr<DriverModel>(drmMock));
        executionEnvironment.rootDeviceEnvironments[0]->memoryOperationsInterface = DrmMemoryOperationsHandler::create(*drmMock, 0u);

        this->mm = new DrmMemoryManager(gemCloseWorkerMode::gemCloseWorkerInactive,
                                        false,
                                        false,
                                        executionEnvironment);

        this->drmMock->gem_close_cnt = 0;
        this->drmMock->gem_close_expected = 0;
    }

    void TearDown() {
        if (this->drmMock->gem_close_expected >= 0) {
            EXPECT_EQ(this->drmMock->gem_close_expected, this->drmMock->gem_close_cnt);
        }

        delete this->mm;
    }

  protected:
    class DrmAllocationWrapper : public DrmAllocation {
      public:
        DrmAllocationWrapper(BufferObject *bo)
            : DrmAllocation(0, GraphicsAllocation::AllocationType::UNKNOWN, bo, nullptr, 0, (osHandle)0u, MemoryPool::MemoryNull) {
        }
    };
    MockExecutionEnvironment executionEnvironment;
};

typedef Test<DrmGemCloseWorkerFixture> DrmGemCloseWorkerTests;

TEST_F(DrmGemCloseWorkerTests, WhenClosingGemThenSucceeds) {
    this->drmMock->gem_close_expected = 1;

    auto worker = new DrmGemCloseWorker(*mm);
    auto bo = new BufferObject(this->drmMock, 1, 0, 1);

    worker->push(bo);

    delete worker;
}

TEST_F(DrmGemCloseWorkerTests, GivenMultipleThreadsWhenClosingGemThenSucceeds) {
    this->drmMock->gem_close_expected = -1;

    auto worker = new DrmGemCloseWorker(*mm);
    auto bo = new BufferObject(this->drmMock, 1, 0, 1);

    worker->push(bo);

    //wait for worker to complete or deadCnt drops
    while (!worker->isEmpty() && (deadCnt-- > 0))
        sched_yield(); //yield to another threads

    worker->close(false);

    //and check if GEM was closed
    EXPECT_EQ(1, this->drmMock->gem_close_cnt.load());

    delete worker;
}

TEST_F(DrmGemCloseWorkerTests, GivenMultipleThreadsAndCloseFalseWhenClosingGemThenSucceeds) {
    this->drmMock->gem_close_expected = -1;

    auto worker = new DrmGemCloseWorker(*mm);
    auto bo = new BufferObject(this->drmMock, 1, 0, 1);

    worker->push(bo);
    worker->close(false);

    //wait for worker to complete or deadCnt drops
    while (!worker->isEmpty() && (deadCnt-- > 0))
        sched_yield(); //yield to another threads

    //and check if GEM was closed
    EXPECT_EQ(1, this->drmMock->gem_close_cnt.load());

    delete worker;
}
TEST_F(DrmGemCloseWorkerTests, givenAllocationWhenAskedForUnreferenceWithForceFlagSetThenAllocationIsReleasedFromCallingThread) {
    this->drmMock->gem_close_expected = 1;

    auto worker = new DrmGemCloseWorker(*mm);
    auto bo = new BufferObject(this->drmMock, 1, 0, 1);

    bo->reference();
    worker->push(bo);

    auto r = mm->unreference(bo, true);
    EXPECT_EQ(1u, r);
    EXPECT_EQ(drmMock->ioctl_caller_thread_id, std::this_thread::get_id());

    delete worker;
}

TEST_F(DrmGemCloseWorkerTests, givenDrmGemCloseWorkerWhenCloseIsCalledWithBlockingFlagThenThreadIsClosed) {
    struct mockDrmGemCloseWorker : DrmGemCloseWorker {
        using DrmGemCloseWorker::DrmGemCloseWorker;
        using DrmGemCloseWorker::thread;
    };

    std::unique_ptr<mockDrmGemCloseWorker> worker(new mockDrmGemCloseWorker(*mm));
    EXPECT_NE(nullptr, worker->thread);
    worker->close(true);
    EXPECT_EQ(nullptr, worker->thread);
}

TEST_F(DrmGemCloseWorkerTests, givenDrmGemCloseWorkerWhenCloseIsCalledMultipleTimeWithBlockingFlagThenThreadIsClosed) {
    struct mockDrmGemCloseWorker : DrmGemCloseWorker {
        using DrmGemCloseWorker::DrmGemCloseWorker;
        using DrmGemCloseWorker::thread;
    };

    std::unique_ptr<mockDrmGemCloseWorker> worker(new mockDrmGemCloseWorker(*mm));
    worker->close(true);
    worker->close(true);
    worker->close(true);
    EXPECT_EQ(nullptr, worker->thread);
}
