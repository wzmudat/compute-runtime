/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/aub_mem_dump/definitions/aub_services.h"
#include "shared/source/debug_settings/debug_settings_manager.h"
#include "shared/source/helpers/constants.h"
#include "shared/source/helpers/hw_info.h"
#include "shared/source/xe_hp_core/hw_cmds.h"

#include "engine_node.h"

namespace NEO {

const char *HwMapper<IGFX_XE_HP_SDV>::abbreviation = "xe_hp_sdv";

bool isSimulationXEHP(unsigned short deviceId) {
    return false;
};

const PLATFORM XE_HP_SDV::platform = {
    IGFX_XE_HP_SDV,
    PCH_UNKNOWN,
    IGFX_XE_HP_CORE,
    IGFX_XE_HP_CORE,
    PLATFORM_NONE, // default init
    0,             // usDeviceID
    0,             // usRevId. 0 sets the stepping to A0
    0,             // usDeviceID_PCH
    0,             // usRevId_PCH
    GTTYPE_UNDEFINED};

const RuntimeCapabilityTable XE_HP_SDV::capabilityTable{
    EngineDirectSubmissionInitVec{
        {aub_stream::ENGINE_CCS, {true, true, false, true}},
        {aub_stream::ENGINE_CCS1, {true, false, true, true}},
        {aub_stream::ENGINE_CCS2, {true, false, true, true}},
        {aub_stream::ENGINE_CCS3, {true, false, true, true}}}, // directSubmissionEngines
    {0, 0, 0, false, false, false},                            // kmdNotifyProperties
    MemoryConstants::max48BitAddress,                          // gpuAddressSpace
    83.333,                                                    // defaultProfilingTimerResolution
    MemoryConstants::pageSize,                                 // requiredPreemptionSurfaceSize
    &isSimulationXEHP,                                         // isSimulation
    PreemptionMode::ThreadGroup,                               // defaultPreemptionMode
    aub_stream::ENGINE_CCS,                                    // defaultEngineType
    0,                                                         // maxRenderFrequency
    30,                                                        // clVersionSupport
    CmdServicesMemTraceVersion::DeviceValues::XeHP_SDV,        // aubDeviceId
    0,                                                         // extraQuantityThreadsPerEU
    64,                                                        // slmSize
    sizeof(XE_HP_SDV::GRF),                                    // grfSize
    36u,                                                       // timestampValidBits
    32u,                                                       // kernelTimestampValidBits
    false,                                                     // blitterOperationsSupported
    true,                                                      // ftrSupportsInteger64BitAtomics
    true,                                                      // ftrSupportsFP64
    true,                                                      // ftrSupports64BitMath
    true,                                                      // ftrSvm
    false,                                                     // ftrSupportsCoherency
    false,                                                     // ftrSupportsVmeAvcTextureSampler
    false,                                                     // ftrSupportsVmeAvcPreemption
    false,                                                     // ftrRenderCompressedBuffers
    false,                                                     // ftrRenderCompressedImages
    true,                                                      // ftr64KBpages
    true,                                                      // instrumentationEnabled
    true,                                                      // forceStatelessCompilationFor32Bit
    "core",                                                    // platformType
    "",                                                        // deviceName
    true,                                                      // sourceLevelDebuggerSupported
    false,                                                     // supportsVme
    true,                                                      // supportCacheFlushAfterWalker
    true,                                                      // supportsImages
    false,                                                     // supportsDeviceEnqueue
    false,                                                     // supportsPipes
    true,                                                      // supportsOcl21Features
    false,                                                     // supportsOnDemandPageFaults
    false,                                                     // supportsIndependentForwardProgress
    false,                                                     // hostPtrTrackingEnabled
    true,                                                      // levelZeroSupported
    false,                                                     // isIntegratedDevice
    true,                                                      // supportsMediaBlock
    true                                                       // fusedEuEnabled
};

WorkaroundTable XE_HP_SDV::workaroundTable = {};
FeatureTable XE_HP_SDV::featureTable = {};

void XE_HP_SDV::setupFeatureAndWorkaroundTable(HardwareInfo *hwInfo) {
    FeatureTable *featureTable = &hwInfo->featureTable;
    WorkaroundTable *workaroundTable = &hwInfo->workaroundTable;

    featureTable->ftrL3IACoherency = true;
    featureTable->ftrFlatPhysCCS = true;
    featureTable->ftrPPGTT = true;
    featureTable->ftrSVM = true;
    featureTable->ftrIA32eGfxPTEs = true;
    featureTable->ftrStandardMipTailFormat = true;
    featureTable->ftrTranslationTable = true;
    featureTable->ftrUserModeTranslationTable = true;
    featureTable->ftrTileMappedResource = true;
    featureTable->ftrEnableGuC = true;
    featureTable->ftrFbc = true;
    featureTable->ftrFbc2AddressTranslation = true;
    featureTable->ftrFbcBlitterTracking = true;
    featureTable->ftrAstcHdr2D = true;
    featureTable->ftrAstcLdr2D = true;

    featureTable->ftr3dMidBatchPreempt = true;
    featureTable->ftrGpGpuMidBatchPreempt = true;
    featureTable->ftrGpGpuThreadGroupLevelPreempt = true;
    featureTable->ftrPerCtxtPreemptionGranularityControl = true;

    featureTable->ftrTileY = false;
    featureTable->ftrLocalMemory = true;
    featureTable->ftrLinearCCS = true;
    featureTable->ftrE2ECompression = true;
    featureTable->ftrCCSNode = true;
    featureTable->ftrCCSRing = true;
    featureTable->ftrMultiTileArch = true;
    featureTable->ftrCCSMultiInstance = true;

    workaroundTable->wa4kAlignUVOffsetNV12LinearSurface = true;
    workaroundTable->waEnablePreemptionGranularityControlByUMD = true;
};

const HardwareInfo XE_HP_SDV_CONFIG::hwInfo = {
    &XE_HP_SDV::platform,
    &XE_HP_SDV::featureTable,
    &XE_HP_SDV::workaroundTable,
    &XE_HP_SDV_CONFIG::gtSystemInfo,
    XE_HP_SDV::capabilityTable,
};
GT_SYSTEM_INFO XE_HP_SDV_CONFIG::gtSystemInfo = {0};
void XE_HP_SDV_CONFIG::setupHardwareInfo(HardwareInfo *hwInfo, bool setupFeatureTableAndWorkaroundTable) {
    XE_HP_SDV_CONFIG::setupHardwareInfoMultiTile(hwInfo, setupFeatureTableAndWorkaroundTable, false);
}

void XE_HP_SDV_CONFIG::setupHardwareInfoMultiTile(HardwareInfo *hwInfo, bool setupFeatureTableAndWorkaroundTable, bool setupMultiTile) {
    GT_SYSTEM_INFO *gtSysInfo = &hwInfo->gtSystemInfo;
    gtSysInfo->CsrSizeInMb = 8;
    gtSysInfo->IsL3HashModeEnabled = false;
    gtSysInfo->IsDynamicallyPopulated = false;

    // non-zero values for unit tests
    if (gtSysInfo->SliceCount == 0) {
        gtSysInfo->SliceCount = 2;
        gtSysInfo->SubSliceCount = 8;
        gtSysInfo->EUCount = 40;
        gtSysInfo->MaxEuPerSubSlice = gtSysInfo->EUCount / gtSysInfo->SubSliceCount;
        gtSysInfo->MaxSlicesSupported = gtSysInfo->SliceCount;
        gtSysInfo->MaxSubSlicesSupported = gtSysInfo->SubSliceCount;

        gtSysInfo->L3BankCount = 1;

        gtSysInfo->CCSInfo.IsValid = true;
        gtSysInfo->CCSInfo.NumberOfCCSEnabled = 1;

        hwInfo->featureTable.ftrBcsInfo = 1;
    }

    if (setupFeatureTableAndWorkaroundTable) {
        XE_HP_SDV::setupFeatureAndWorkaroundTable(hwInfo);
    }
};
#include "hw_info_setup_xehp.inl"
} // namespace NEO
