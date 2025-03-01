/*
 * Copyright (C) 2018-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/source_level_debugger/source_level_debugger.h"
#include "shared/test/common/helpers/debug_manager_state_restore.h"
#include "shared/test/common/mocks/mock_device.h"
#include "shared/test/common/mocks/mock_os_library.h"
#include "shared/test/common/mocks/mock_source_level_debugger.h"

#include "opencl/test/unit_test/mocks/mock_cl_device.h"
#include "opencl/test/unit_test/mocks/mock_platform.h"
#include "test.h"

using PreambleTest = ::testing::Test;
using namespace NEO;

TEST(DeviceWithSourceLevelDebugger, givenDeviceWithSourceLevelDebuggerActiveWhenDeviceIsDestructedThenSourceLevelDebuggerIsNotified) {
    ExecutionEnvironment *executionEnvironment = platform()->peekExecutionEnvironment();
    auto gmock = new ::testing::NiceMock<GMockSourceLevelDebugger>(new MockOsLibrary);

    executionEnvironment->rootDeviceEnvironments[0]->debugger.reset(gmock);
    auto device = std::make_unique<MockClDevice>(MockDevice::create<MockDeviceWithDebuggerActive>(executionEnvironment, 0u));

    EXPECT_CALL(*gmock, notifyDeviceDestruction()).Times(1);
}

TEST(DeviceWithSourceLevelDebugger, givenDeviceWithSourceLevelDebuggerActiveWhenDeviceIsCreatedThenPreemptionIsDisabled) {
    ExecutionEnvironment *executionEnvironment = platform()->peekExecutionEnvironment();
    executionEnvironment->rootDeviceEnvironments[0]->debugger.reset(new MockActiveSourceLevelDebugger(new MockOsLibrary));
    auto device = std::unique_ptr<MockDevice>(MockDevice::create<MockDeviceWithDebuggerActive>(executionEnvironment, 0u));

    EXPECT_EQ(PreemptionMode::Disabled, device->getPreemptionMode());
}
