/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/command_stream/linear_stream.h"
#include "shared/source/memory_manager/graphics_allocation.h"
#include "shared/test/common/cmd_parse/gen_cmd_parse.h"

#include "test.h"

#include "level_zero/core/test/unit_tests/fixtures/device_fixture.h"
#include "level_zero/core/test/unit_tests/mocks/mock_cmdlist.h"
#include "level_zero/core/test/unit_tests/mocks/mock_cmdqueue.h"
#include "level_zero/core/test/unit_tests/mocks/mock_device.h"
#include "level_zero/core/test/unit_tests/mocks/mock_memory_manager.h"
#include <level_zero/ze_api.h>

#include "gtest/gtest.h"

namespace L0 {
namespace ult {

using CommandQueueExecuteCommandListsXE_HP_CORE = Test<DeviceFixture>;

XE_HP_CORE_TEST_F(CommandQueueExecuteCommandListsXE_HP_CORE, WhenExecutingCmdListsThenPipelineSelectAndCfeStateAreAddedToCmdBuffer) {
    const ze_command_queue_desc_t desc = {};
    ze_result_t returnValue;
    auto commandQueue = whitebox_cast(CommandQueue::create(
        productFamily,
        device, neoDevice->getDefaultEngine().commandStreamReceiver, &desc, false, false, returnValue));
    ASSERT_NE(nullptr, commandQueue->commandStream);
    auto usedSpaceBefore = commandQueue->commandStream->getUsed();

    ze_command_list_handle_t commandLists[] = {
        CommandList::create(productFamily, device, NEO::EngineGroupType::RenderCompute, 0u, returnValue)->toHandle()};
    uint32_t numCommandLists = sizeof(commandLists) / sizeof(commandLists[0]);
    auto result = commandQueue->executeCommandLists(numCommandLists, commandLists, nullptr, true);

    ASSERT_EQ(ZE_RESULT_SUCCESS, result);

    auto usedSpaceAfter = commandQueue->commandStream->getUsed();
    ASSERT_GT(usedSpaceAfter, usedSpaceBefore);

    GenCmdList cmdList;
    ASSERT_TRUE(FamilyType::PARSE::parseCommandBuffer(
        cmdList, ptrOffset(commandQueue->commandStream->getCpuBase(), 0), usedSpaceAfter));

    using CFE_STATE = typename FamilyType::CFE_STATE;
    auto itorCFE = find<CFE_STATE *>(cmdList.begin(), cmdList.end());
    ASSERT_NE(itorCFE, cmdList.end());

    // Should have a PS before a CFE
    using PIPELINE_SELECT = typename FamilyType::PIPELINE_SELECT;
    auto itorPS = find<PIPELINE_SELECT *>(cmdList.begin(), itorCFE);
    ASSERT_NE(itorPS, itorCFE);
    {
        auto cmd = genCmdCast<PIPELINE_SELECT *>(*itorPS);
        EXPECT_EQ(cmd->getMaskBits() & 3u, 3u);
        EXPECT_EQ(cmd->getPipelineSelection(), PIPELINE_SELECT::PIPELINE_SELECTION_GPGPU);
    }

    CommandList::fromHandle(commandLists[0])->destroy();
    commandQueue->destroy();
}

XE_HP_CORE_TEST_F(CommandQueueExecuteCommandListsXE_HP_CORE, WhenExecutingCmdListsThenStateBaseAddressForGeneralStateBaseAddressIsNotAdded) {
    const ze_command_queue_desc_t desc = {};
    ze_result_t returnValue;
    auto commandQueue = whitebox_cast(CommandQueue::create(
        productFamily,
        device, neoDevice->getDefaultEngine().commandStreamReceiver, &desc, false, false, returnValue));
    ASSERT_NE(nullptr, commandQueue->commandStream);
    auto usedSpaceBefore = commandQueue->commandStream->getUsed();

    ze_command_list_handle_t commandLists[] = {
        CommandList::create(productFamily, device, NEO::EngineGroupType::RenderCompute, 0u, returnValue)->toHandle()};
    uint32_t numCommandLists = sizeof(commandLists) / sizeof(commandLists[0]);
    auto result = commandQueue->executeCommandLists(numCommandLists, commandLists, nullptr, true);

    ASSERT_EQ(ZE_RESULT_SUCCESS, result);

    auto usedSpaceAfter = commandQueue->commandStream->getUsed();
    ASSERT_GT(usedSpaceAfter, usedSpaceBefore);

    GenCmdList cmdList;
    ASSERT_TRUE(FamilyType::PARSE::parseCommandBuffer(
        cmdList, ptrOffset(commandQueue->commandStream->getCpuBase(), 0), usedSpaceAfter));
    using STATE_BASE_ADDRESS = typename FamilyType::STATE_BASE_ADDRESS;

    auto itorSba = find<STATE_BASE_ADDRESS *>(cmdList.begin(), cmdList.end());
    EXPECT_EQ(itorSba, cmdList.end());

    CommandList::fromHandle(commandLists[0])->destroy();
    commandQueue->destroy();
}

} // namespace ult
} // namespace L0
