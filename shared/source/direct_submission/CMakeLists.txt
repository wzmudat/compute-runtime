#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(NEO_CORE_DIRECT_SUBMISSION
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_prefetcher_base.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_prefetch_mitigation_base.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_hw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_hw.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_hw_diagnostic_mode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_hw_diagnostic_mode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_properties.h
)

if(SUPPORT_XEHP_AND_LATER)
  list(APPEND NEO_CORE_DIRECT_SUBMISSION
       ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_prefetcher_xe_hp_core_and_later.inl
       ${CMAKE_CURRENT_SOURCE_DIR}/direct_submission_prefetch_mitigation_xe_hp_core_and_later.inl
  )
endif()

set_property(GLOBAL PROPERTY NEO_CORE_DIRECT_SUBMISSION ${NEO_CORE_DIRECT_SUBMISSION})

add_subdirectories()
