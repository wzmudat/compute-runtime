/*
 * Copyright (C) 2018-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/device/device.h"
#include "shared/source/helpers/blit_commands_helper.h"
#include "shared/source/helpers/local_memory_access_modes.h"
#include "shared/test/common/helpers/debug_manager_state_restore.h"
#include "shared/test/common/helpers/variable_backup.h"
#include "shared/test/common/mocks/mock_deferred_deleter.h"
#include "shared/test/common/mocks/mock_device.h"
#include "shared/test/common/mocks/mock_memory_manager.h"

#include "opencl/source/command_queue/command_queue.h"
#include "opencl/source/context/context.inl"
#include "opencl/source/device_queue/device_queue.h"
#include "opencl/source/mem_obj/buffer.h"
#include "opencl/source/sharings/sharing.h"
#include "opencl/test/unit_test/fixtures/platform_fixture.h"
#include "opencl/test/unit_test/mocks/mock_cl_device.h"
#include "opencl/test/unit_test/mocks/mock_command_queue.h"
#include "opencl/test/unit_test/mocks/mock_context.h"
#include "opencl/test/unit_test/mocks/mock_platform.h"
#include "opencl/test/unit_test/test_macros/test_checks_ocl.h"
#include "test.h"

using namespace NEO;

class WhiteBoxContext : public Context {
  public:
    MemoryManager *getMM() {
        return this->memoryManager;
    }

    const cl_context_properties *getProperties() const {
        return properties;
    }

    size_t getNumProperties() const {
        return numProperties;
    }

    WhiteBoxContext(void(CL_CALLBACK *pfnNotify)(const char *, const void *, size_t, void *), void *userData) : Context(pfnNotify, userData){};
};

struct ContextTest : public PlatformFixture,
                     public ::testing::Test {

    using PlatformFixture::SetUp;

    void SetUp() override {
        PlatformFixture::SetUp();

        cl_platform_id platform = pPlatform;
        properties = new cl_context_properties[3];
        properties[0] = CL_CONTEXT_PLATFORM;
        properties[1] = (cl_context_properties)platform;
        properties[2] = 0;

        context = Context::create<WhiteBoxContext>(properties, ClDeviceVector(devices, num_devices), nullptr, nullptr, retVal);
        ASSERT_NE(nullptr, context);
    }

    void TearDown() override {
        delete[] properties;
        delete context;
        PlatformFixture::TearDown();
    }

    cl_int retVal = CL_SUCCESS;
    WhiteBoxContext *context = nullptr;
    cl_context_properties *properties = nullptr;
};

TEST_F(ContextTest, WhenCreatingContextThenDevicesAllDevicesExist) {
    for (size_t deviceOrdinal = 0; deviceOrdinal < context->getNumDevices(); ++deviceOrdinal) {
        EXPECT_NE(nullptr, context->getDevice(deviceOrdinal));
    }
}

TEST_F(ContextTest, WhenCreatingContextThenMemoryManagerForContextIsSet) {
    EXPECT_NE(nullptr, context->getMM());
}

TEST_F(ContextTest, WhenCreatingContextThenPropertiesAreCopied) {
    auto contextProperties = context->getProperties();
    EXPECT_NE(properties, contextProperties);
}

TEST_F(ContextTest, WhenCreatingContextThenPropertiesAreValid) {
    auto contextProperties = context->getProperties();
    ASSERT_NE(nullptr, contextProperties);
    EXPECT_EQ(3u, context->getNumProperties());

    while (*contextProperties) {
        switch (*contextProperties) {
        case CL_CONTEXT_PLATFORM:
            ++contextProperties;
            break;
        default:
            ASSERT_FALSE(!"Unknown context property");
            break;
        }
        ++contextProperties;
    }
}

TEST_F(ContextTest, WhenCreatingContextThenSpecialQueueIsAvailable) {
    auto specialQ = context->getSpecialQueue(0u);
    EXPECT_NE(specialQ, nullptr);
}

TEST_F(ContextTest, WhenSettingSpecialQueueThenQueueIsAvailable) {
    MockContext context((ClDevice *)devices[0], true);

    auto specialQ = context.getSpecialQueue(0u);
    EXPECT_EQ(specialQ, nullptr);

    auto cmdQ = new MockCommandQueue(&context, (ClDevice *)devices[0], 0, false);
    context.setSpecialQueue(cmdQ, 0u);
    specialQ = context.getSpecialQueue(0u);
    EXPECT_NE(specialQ, nullptr);
}

TEST_F(ContextTest, WhenSettingDefaultQueueThenQueueIsAvailable) {
    REQUIRE_DEVICE_ENQUEUE_OR_SKIP(context);
    EXPECT_EQ(nullptr, context->getDefaultDeviceQueue());
    auto dq = new DeviceQueue();
    context->setDefaultDeviceQueue(dq);
    EXPECT_EQ(dq, context->getDefaultDeviceQueue());
    delete dq;
}

TEST_F(ContextTest, givenCmdQueueWithoutContextWhenBeingCreatedNextDeletedThenContextRefCountShouldNeitherBeIncrementedNorNextDecremented) {
    MockContext context((ClDevice *)devices[0]);
    EXPECT_EQ(1, context.getRefInternalCount());

    auto cmdQ1 = new MockCommandQueue();
    EXPECT_EQ(1, context.getRefInternalCount());

    delete cmdQ1;
    EXPECT_EQ(1, context.getRefInternalCount());

    auto cmdQ2 = new MockCommandQueue(nullptr, (ClDevice *)devices[0], 0, false);
    EXPECT_EQ(1, context.getRefInternalCount());

    delete cmdQ2;
    EXPECT_EQ(1, context.getRefInternalCount());
}

TEST_F(ContextTest, givenDeviceQueueWithoutContextWhenBeingCreatedNextDeletedThenContextRefCountShouldNeitherBeIncrementedNorNextDecremented) {
    REQUIRE_DEVICE_ENQUEUE_OR_SKIP(context);
    MockContext context((ClDevice *)devices[0]);
    EXPECT_EQ(1, context.getRefInternalCount());

    auto cmdQ1 = new DeviceQueue();
    EXPECT_EQ(1, context.getRefInternalCount());

    delete cmdQ1;
    EXPECT_EQ(1, context.getRefInternalCount());

    cl_queue_properties properties = 0;
    auto cmdQ2 = new DeviceQueue(nullptr, (ClDevice *)devices[0], properties);
    EXPECT_EQ(1, context.getRefInternalCount());

    delete cmdQ2;
    EXPECT_EQ(1, context.getRefInternalCount());
}

TEST_F(ContextTest, givenCmdQueueWithContextWhenBeingCreatedNextDeletedThenContextRefCountShouldBeIncrementedNextDecremented) {
    MockContext context((ClDevice *)devices[0]);
    EXPECT_EQ(1, context.getRefInternalCount());

    auto cmdQ = new MockCommandQueue(&context, (ClDevice *)devices[0], 0, false);
    EXPECT_EQ(2, context.getRefInternalCount());

    delete cmdQ;
    EXPECT_EQ(1, context.getRefInternalCount());
}

TEST_F(ContextTest, givenDeviceCmdQueueWithContextWhenBeingCreatedNextDeletedThenContextRefCountShouldBeIncrementedNextDecremented) {
    REQUIRE_DEVICE_ENQUEUE_OR_SKIP(context);
    MockContext context((ClDevice *)devices[0]);
    EXPECT_EQ(1, context.getRefInternalCount());

    cl_queue_properties properties = 0;
    auto cmdQ = new DeviceQueue(&context, (ClDevice *)devices[0], properties);
    EXPECT_EQ(2, context.getRefInternalCount());

    delete cmdQ;
    EXPECT_EQ(1, context.getRefInternalCount());
}

TEST_F(ContextTest, givenDefaultDeviceCmdQueueWithContextWhenBeingCreatedNextDeletedThenContextRefCountShouldBeIncrementedNextDecremented) {
    REQUIRE_DEVICE_ENQUEUE_OR_SKIP(context);
    MockContext context((ClDevice *)devices[0]);
    EXPECT_EQ(1, context.getRefInternalCount());

    cl_queue_properties properties = 0;
    auto cmdQ = new DeviceQueue(&context, (ClDevice *)devices[0], properties);
    context.setDefaultDeviceQueue(cmdQ);
    EXPECT_EQ(2, context.getRefInternalCount());

    delete cmdQ;
    EXPECT_EQ(1, context.getRefInternalCount());
}

TEST_F(ContextTest, givenContextWhenItIsCreatedFromDeviceThenItAddsRefCountToThisDevice) {
    auto device = castToObject<ClDevice>(devices[0]);
    EXPECT_EQ(2, device->getRefInternalCount());
    cl_device_id deviceID = devices[0];
    std::unique_ptr<Context> context(Context::create<Context>(0, ClDeviceVector(&deviceID, 1), nullptr, nullptr, retVal));
    EXPECT_EQ(3, device->getRefInternalCount());
    context.reset(nullptr);
    EXPECT_EQ(2, device->getRefInternalCount());
}

TEST_F(ContextTest, givenContextWhenItIsCreatedFromMultipleDevicesThenItAddsRefCountToThoseDevices) {
    auto device = castToObject<ClDevice>(devices[0]);
    EXPECT_EQ(2, device->getRefInternalCount());

    ClDeviceVector devicesVector;
    devicesVector.push_back(device);
    devicesVector.push_back(device);

    std::unique_ptr<Context> context(Context::create<Context>(0, devicesVector, nullptr, nullptr, retVal));
    EXPECT_EQ(4, device->getRefInternalCount());
    context.reset(nullptr);
    EXPECT_EQ(2, device->getRefInternalCount());
}

TEST_F(ContextTest, givenSpecialCmdQueueWithContextWhenBeingCreatedNextAutoDeletedThenContextRefCountShouldNeitherBeIncrementedNorNextDecremented) {
    MockContext context((ClDevice *)devices[0], true);
    EXPECT_EQ(1, context.getRefInternalCount());

    auto cmdQ = new MockCommandQueue(&context, (ClDevice *)devices[0], 0, false);
    context.overrideSpecialQueueAndDecrementRefCount(cmdQ, 0u);
    EXPECT_EQ(1, context.getRefInternalCount());

    //special queue is to be deleted implicitly by context
}

TEST_F(ContextTest, givenSpecialCmdQueueWithContextWhenBeingCreatedNextDeletedThenContextRefCountShouldNeitherBeIncrementedNorNextDecremented) {
    MockContext context((ClDevice *)devices[0], true);
    EXPECT_EQ(1, context.getRefInternalCount());

    auto cmdQ = new MockCommandQueue(&context, (ClDevice *)devices[0], 0, false);
    context.overrideSpecialQueueAndDecrementRefCount(cmdQ, 0u);
    EXPECT_EQ(1, context.getRefInternalCount());

    delete cmdQ;
    EXPECT_EQ(1, context.getRefInternalCount());

    context.setSpecialQueue(nullptr, 0u);
}

TEST_F(ContextTest, GivenInteropSyncParamWhenCreateContextThenSetContextParam) {
    cl_device_id deviceID = devices[0];
    auto pPlatform = NEO::platform();
    cl_platform_id pid[1];
    pid[0] = pPlatform;

    cl_context_properties validProperties[5] = {CL_CONTEXT_PLATFORM, (cl_context_properties)pid[0],
                                                CL_CONTEXT_INTEROP_USER_SYNC, 1, 0};
    cl_int retVal = CL_SUCCESS;
    auto context = Context::create<Context>(validProperties, ClDeviceVector(&deviceID, 1), nullptr, nullptr, retVal);
    EXPECT_EQ(CL_SUCCESS, retVal);
    EXPECT_NE(nullptr, context);
    EXPECT_TRUE(context->getInteropUserSyncEnabled());
    delete context;

    validProperties[3] = 0; // false
    context = Context::create<Context>(validProperties, ClDeviceVector(&deviceID, 1), nullptr, nullptr, retVal);
    EXPECT_EQ(CL_SUCCESS, retVal);
    EXPECT_NE(nullptr, context);
    EXPECT_FALSE(context->getInteropUserSyncEnabled());
    delete context;
}

class MockSharingFunctions : public SharingFunctions {
  public:
    uint32_t getId() const override {
        return sharingId;
    }
    static const uint32_t sharingId = 0;
};

TEST_F(ContextTest, givenContextWhenSharingTableEmptyThenReturnsNullptr) {
    MockContext context;
    context.clearSharingFunctions();
    auto *sharingF = context.getSharing<MockSharingFunctions>();
    EXPECT_EQ(sharingF, nullptr);
}

TEST_F(ContextTest, givenNullptrWhenRegisteringSharingToContextThenAbortExecution) {
    MockContext context;
    context.clearSharingFunctions();
    EXPECT_THROW(context.registerSharing<MockSharingFunctions>(nullptr), std::exception);
}

TEST_F(ContextTest, givenContextWhenSharingTableIsNotEmptyThenReturnsSharingFunctionPointer) {
    MockContext context;
    MockSharingFunctions *sharingFunctions = new MockSharingFunctions;
    context.registerSharing<MockSharingFunctions>(sharingFunctions);
    auto *sharingF = context.getSharing<MockSharingFunctions>();
    EXPECT_EQ(sharingF, sharingFunctions);
}

TEST(Context, givenFtrSvmFalseWhenContextIsCreatedThenSVMAllocsManagerIsNotCreated) {
    ExecutionEnvironment *executionEnvironment = platform()->peekExecutionEnvironment();
    executionEnvironment->prepareRootDeviceEnvironments(1u);
    auto hwInfo = executionEnvironment->rootDeviceEnvironments[0]->getMutableHardwareInfo();
    hwInfo->capabilityTable.ftrSvm = false;

    auto device = std::make_unique<MockClDevice>(MockDevice::createWithExecutionEnvironment<MockDevice>(hwInfo, executionEnvironment, 0));

    cl_device_id clDevice = device.get();
    cl_int retVal = CL_SUCCESS;
    auto context = std::unique_ptr<MockContext>(Context::create<MockContext>(nullptr, ClDeviceVector(&clDevice, 1), nullptr, nullptr, retVal));
    ASSERT_NE(nullptr, context);
    auto svmManager = context->getSVMAllocsManager();
    EXPECT_EQ(nullptr, svmManager);
}

TEST(Context, whenCreateContextThenSpecialQueueUsesInternalEngine) {
    auto device = std::make_unique<MockClDevice>(MockDevice::createWithNewExecutionEnvironment<MockDevice>(defaultHwInfo.get()));
    cl_device_id clDevice = device.get();
    cl_int retVal = CL_SUCCESS;

    auto context = std::unique_ptr<MockContext>(Context::create<MockContext>(nullptr, ClDeviceVector(&clDevice, 1), nullptr, nullptr, retVal));
    ASSERT_NE(nullptr, context);
    EXPECT_EQ(CL_SUCCESS, retVal);

    auto specialQueueEngine = context->getSpecialQueue(device->getRootDeviceIndex())->getGpgpuEngine();
    auto internalEngine = device->getInternalEngine();
    EXPECT_EQ(internalEngine.commandStreamReceiver, specialQueueEngine.commandStreamReceiver);
}

TEST(MultiDeviceContextTest, givenContextWithMultipleDevicesWhenGettingInfoAboutSubDevicesThenCorrectValueIsReturned) {
    MockSpecializedContext context1;
    MockUnrestrictiveContext context2;
    MockDefaultContext context3;

    EXPECT_EQ(2u, context1.getNumDevices());
    EXPECT_TRUE(context1.containsMultipleSubDevices(0));

    EXPECT_EQ(3u, context2.getNumDevices());
    EXPECT_TRUE(context2.containsMultipleSubDevices(0));

    EXPECT_EQ(3u, context3.getNumDevices());
    EXPECT_FALSE(context3.containsMultipleSubDevices(0));
    EXPECT_FALSE(context3.containsMultipleSubDevices(1));
    EXPECT_FALSE(context3.containsMultipleSubDevices(2));
}

class ContextWithAsyncDeleterTest : public ::testing::WithParamInterface<bool>,
                                    public ::testing::Test {
  public:
    void SetUp() override {
        memoryManager = new MockMemoryManager();
        device = new MockClDevice{MockDevice::createWithNewExecutionEnvironment<MockDevice>(defaultHwInfo.get())};
        deleter = new MockDeferredDeleter();

        device->engines.clear();
        device->injectMemoryManager(memoryManager);
        device->createEngines();
        memoryManager->setDeferredDeleter(deleter);
    }
    void TearDown() override {
        delete device;
    }
    Context *context;
    MockMemoryManager *memoryManager;
    MockDeferredDeleter *deleter;
    MockClDevice *device;
};

TEST_P(ContextWithAsyncDeleterTest, givenContextWithMemoryManagerWhenAsyncDeleterIsEnabledThenUsesDeletersMethods) {
    cl_device_id clDevice = device;
    cl_int retVal;
    ClDeviceVector deviceVector(&clDevice, 1);
    bool asyncDeleterEnabled = GetParam();
    memoryManager->overrideAsyncDeleterFlag(asyncDeleterEnabled);

    EXPECT_EQ(0, deleter->getClientsNum());
    context = Context::create<Context>(0, deviceVector, nullptr, nullptr, retVal);

    if (asyncDeleterEnabled) {
        EXPECT_EQ(1, deleter->getClientsNum());
    } else {
        EXPECT_EQ(0, deleter->getClientsNum());
    }
    delete context;

    EXPECT_EQ(0, deleter->getClientsNum());
}

INSTANTIATE_TEST_CASE_P(ContextTests,
                        ContextWithAsyncDeleterTest,
                        ::testing::Bool());

TEST(DefaultContext, givenDefaultContextWhenItIsQueriedForTypeThenDefaultTypeIsReturned) {
    MockContext context;
    EXPECT_EQ(ContextType::CONTEXT_TYPE_DEFAULT, context.peekContextType());
}

TEST(Context, givenContextWhenCheckIfAllocationsAreMultiStorageThenReturnProperValueAccordingToContextType) {
    MockContext context;
    EXPECT_TRUE(context.areMultiStorageAllocationsPreferred());

    context.contextType = ContextType::CONTEXT_TYPE_SPECIALIZED;
    EXPECT_FALSE(context.areMultiStorageAllocationsPreferred());

    context.contextType = ContextType::CONTEXT_TYPE_UNRESTRICTIVE;
    EXPECT_TRUE(context.areMultiStorageAllocationsPreferred());
}

TEST(Context, givenContextWhenIsDeviceAssociatedIsCalledWithItsDeviceThenTrueIsReturned) {
    MockContext context;
    EXPECT_TRUE(context.isDeviceAssociated(*context.getDevice(0)));
}

TEST(Context, givenContextWhenIsDeviceAssociatedIsCalledWithNotAssociatedDeviceThenFalseIsReturned) {
    MockContext context0;
    MockContext context1;
    EXPECT_FALSE(context0.isDeviceAssociated(*context1.getDevice(0)));
    EXPECT_FALSE(context1.isDeviceAssociated(*context0.getDevice(0)));
}
TEST(Context, givenContextWithSingleDevicesWhenGettingDeviceBitfieldForAllocationThenDeviceBitfieldForDeviceIsReturned) {
    UltClDeviceFactory deviceFactory{1, 3};
    auto device = deviceFactory.subDevices[1];
    auto expectedDeviceBitfield = device->getDeviceBitfield();
    MockContext context(device);
    EXPECT_EQ(expectedDeviceBitfield.to_ulong(), context.getDeviceBitfieldForAllocation(device->getRootDeviceIndex()).to_ulong());
}
TEST(Context, givenContextWithMultipleSubDevicesWhenGettingDeviceBitfieldForAllocationThenMergedDeviceBitfieldIsReturned) {
    UltClDeviceFactory deviceFactory{1, 3};
    cl_int retVal;
    cl_device_id devices[]{deviceFactory.subDevices[0], deviceFactory.subDevices[2]};
    ClDeviceVector deviceVector(devices, 2);
    auto expectedDeviceBitfield = deviceFactory.subDevices[0]->getDeviceBitfield() | deviceFactory.subDevices[2]->getDeviceBitfield();
    auto context = Context::create<Context>(0, deviceVector, nullptr, nullptr, retVal);
    EXPECT_NE(nullptr, context);
    EXPECT_EQ(CL_SUCCESS, retVal);
    EXPECT_EQ(expectedDeviceBitfield.to_ulong(), context->getDeviceBitfieldForAllocation(deviceFactory.rootDevices[0]->getRootDeviceIndex()).to_ulong());
    context->release();
}

TEST(MultiDeviceContextTest, givenContextWithTwoDifferentSubDevicesFromDifferentRootDevicesWhenGettingDeviceBitfieldForAllocationThenSeparatedDeviceBitfieldsAreReturned) {
    DebugManagerStateRestore restorer;

    DebugManager.flags.EnableMultiRootDeviceContexts.set(true);
    UltClDeviceFactory deviceFactory{2, 2};
    cl_int retVal;
    cl_device_id devices[]{deviceFactory.subDevices[1], deviceFactory.subDevices[2]};
    ClDeviceVector deviceVector(devices, 2);

    auto expectedDeviceBitfieldForRootDevice0 = deviceFactory.subDevices[1]->getDeviceBitfield();
    auto expectedDeviceBitfieldForRootDevice1 = deviceFactory.subDevices[2]->getDeviceBitfield();

    auto context = Context::create<Context>(0, deviceVector, nullptr, nullptr, retVal);
    EXPECT_NE(nullptr, context);
    EXPECT_EQ(CL_SUCCESS, retVal);

    EXPECT_EQ(expectedDeviceBitfieldForRootDevice0.to_ulong(), context->getDeviceBitfieldForAllocation(deviceFactory.rootDevices[0]->getRootDeviceIndex()).to_ulong());
    EXPECT_EQ(expectedDeviceBitfieldForRootDevice1.to_ulong(), context->getDeviceBitfieldForAllocation(deviceFactory.rootDevices[1]->getRootDeviceIndex()).to_ulong());

    context->release();
}

TEST(Context, WhenSettingContextDestructorCallbackThenCallOrderIsPreserved) {
    struct UserDataType {
        cl_context expectedContext;
        std::vector<size_t> &vectorToModify;
        size_t valueToAdd;
    };
    auto callback = [](cl_context context, void *userData) -> void {
        auto pUserData = reinterpret_cast<UserDataType *>(userData);
        EXPECT_EQ(pUserData->expectedContext, context);
        pUserData->vectorToModify.push_back(pUserData->valueToAdd);
    };

    auto pContext = new MockContext{};
    std::vector<size_t> callbacksReturnValues;
    UserDataType userDataArray[]{
        {pContext, callbacksReturnValues, 1},
        {pContext, callbacksReturnValues, 2},
        {pContext, callbacksReturnValues, 3}};

    for (auto &userData : userDataArray) {
        cl_int retVal = clSetContextDestructorCallback(pContext, callback, &userData);
        ASSERT_EQ(CL_SUCCESS, retVal);
    }
    delete pContext;

    ASSERT_EQ(3u, callbacksReturnValues.size());
    EXPECT_EQ(3u, callbacksReturnValues[0]);
    EXPECT_EQ(2u, callbacksReturnValues[1]);
    EXPECT_EQ(1u, callbacksReturnValues[2]);
}

TEST(Context, givenContextAndDevicesWhenIsTileOnlyThenProperValueReturned) {
    UltClDeviceFactory deviceFactoryWithSubDevices{1, 2};
    UltClDeviceFactory deviceFactoryWithMultipleDevices{2, 0};
    cl_device_id devices[] = {deviceFactoryWithMultipleDevices.rootDevices[0], deviceFactoryWithMultipleDevices.rootDevices[1]};

    MockContext tileOnlyContext(deviceFactoryWithMultipleDevices.rootDevices[0]);
    MockContext subDevicesContext(deviceFactoryWithSubDevices.rootDevices[0]);
    MockContext multipleDevicesContext(ClDeviceVector(devices, 2));

    EXPECT_TRUE(tileOnlyContext.isSingleDeviceContext());
    EXPECT_FALSE(subDevicesContext.isSingleDeviceContext());
    EXPECT_FALSE(multipleDevicesContext.isSingleDeviceContext());
}

TEST(InvalidExtraPropertiesTests, givenInvalidExtraPropertiesWhenCreatingContextThenContextIsNotCreated) {
    constexpr cl_context_properties INVALID_PROPERTY_TYPE = (1 << 31);
    constexpr cl_context_properties INVALID_CONTEXT_FLAG = (1 << 31);

    auto device = std::make_unique<MockClDevice>(MockDevice::createWithNewExecutionEnvironment<MockDevice>(nullptr));
    cl_device_id deviceID = device.get();
    cl_int retVal = 0;
    std::unique_ptr<Context> context;

    {
        cl_context_properties properties[] = {INVALID_PROPERTY_TYPE, INVALID_CONTEXT_FLAG, 0};
        context.reset(Context::create<Context>(properties, ClDeviceVector(&deviceID, 1), nullptr, nullptr, retVal));
        EXPECT_EQ(CL_INVALID_PROPERTY, retVal);
        EXPECT_EQ(nullptr, context.get());
    }
}

using ContextCreateTests = ::testing::Test;

HWCMDTEST_F(IGFX_XE_HP_CORE, ContextCreateTests, givenLocalMemoryAllocationWhenBlitMemoryToAllocationIsCalledThenSuccessIsReturned) {
    if (is32bit) {
        GTEST_SKIP();
    }

    DebugManagerStateRestore restore;
    DebugManager.flags.EnableLocalMemory.set(true);
    DebugManager.flags.ForceLocalMemoryAccessMode.set(static_cast<int32_t>(LocalMemoryAccessMode::Default));
    UltClDeviceFactory deviceFactory{1, 2};

    ClDevice *devicesToTest[] = {deviceFactory.rootDevices[0], deviceFactory.subDevices[0], deviceFactory.subDevices[1]};

    for (const auto &testedDevice : devicesToTest) {

        MockContext context(testedDevice);
        cl_int retVal;
        auto buffer = std::unique_ptr<Buffer>(Buffer::create(&context, {}, 1, nullptr, retVal));
        auto memory = buffer->getGraphicsAllocation(testedDevice->getRootDeviceIndex());
        uint8_t hostMemory[1];
        auto executionEnv = testedDevice->getExecutionEnvironment();
        executionEnv->rootDeviceEnvironments[0]->getMutableHardwareInfo()->capabilityTable.blitterOperationsSupported = false;

        const auto &hwInfo = testedDevice->getHardwareInfo();
        auto isBlitterRequired = HwHelper::get(hwInfo.platform.eRenderCoreFamily).isBlitCopyRequiredForLocalMemory(hwInfo, *memory);

        auto expectedStatus = isBlitterRequired ? BlitOperationResult::Success : BlitOperationResult::Unsupported;

        EXPECT_EQ(expectedStatus, BlitHelper::blitMemoryToAllocation(buffer->getContext()->getDevice(0)->getDevice(), memory, buffer->getOffset(), hostMemory, {1, 1, 1}));

        executionEnv->rootDeviceEnvironments[0]->getMutableHardwareInfo()->capabilityTable.blitterOperationsSupported = true;
        EXPECT_EQ(BlitOperationResult::Success, BlitHelper::blitMemoryToAllocation(buffer->getContext()->getDevice(0)->getDevice(), memory, buffer->getOffset(), hostMemory, {1, 1, 1}));
    }
}