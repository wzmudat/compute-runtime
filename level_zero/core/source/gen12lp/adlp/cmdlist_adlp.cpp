/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/core/source/cmdlist/cmdlist_hw.inl"
#include "level_zero/core/source/cmdlist/cmdlist_hw_base.inl"
#include "level_zero/core/source/cmdlist/cmdlist_hw_immediate.inl"
#include "level_zero/core/source/gen12lp/cmdlist_gen12lp.h"

#include "cache_flush_gen12lp.inl"
#include "cmdlist_extended.inl"

namespace L0 {
template struct CommandListCoreFamily<IGFX_GEN12LP_CORE>;

static CommandListPopulateFactory<IGFX_ALDERLAKE_P, CommandListProductFamily<IGFX_ALDERLAKE_P>>
    populateADLP;

static CommandListImmediatePopulateFactory<IGFX_ALDERLAKE_P, CommandListImmediateProductFamily<IGFX_ALDERLAKE_P>>
    populateADLPImmediate;

} // namespace L0