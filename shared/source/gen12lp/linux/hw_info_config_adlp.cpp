/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/helpers/hw_info.h"
#include "shared/source/helpers/pipeline_select_helper.h"
#include "shared/source/os_interface/hw_info_config.h"
#include "shared/source/os_interface/hw_info_config.inl"
#include "shared/source/os_interface/hw_info_config_bdw_and_later.inl"

namespace NEO {
constexpr static auto gfxProduct = IGFX_ALDERLAKE_P;

#include "shared/source/gen12lp/os_agnostic_hw_info_config_adlp.inl"
#include "shared/source/gen12lp/os_agnostic_hw_info_config_gen12lp.inl"

template <>
int HwInfoConfigHw<gfxProduct>::configureHardwareCustom(HardwareInfo *hwInfo, OSInterface *osIface) {
    GT_SYSTEM_INFO *gtSystemInfo = &hwInfo->gtSystemInfo;
    gtSystemInfo->SliceCount = 1;
    const auto &hwInfoConfig = *HwInfoConfig::get(hwInfo->platform.eProductFamily);
    hwInfo->featureTable.ftrGpGpuMidThreadLevelPreempt = (hwInfo->platform.usRevId >= hwInfoConfig.getHwRevIdFromStepping(REVISION_B, *hwInfo));

    enableBlitterOperationsSupport(hwInfo);

    return 0;
}

template class HwInfoConfigHw<gfxProduct>;
} // namespace NEO
