#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(RUNTIME_SRCS_SCHEDULER
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/scheduler.cl
    ${CMAKE_CURRENT_SOURCE_DIR}/scheduler_kernel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scheduler_kernel.h
)
target_sources(${NEO_STATIC_LIB_NAME} PRIVATE ${RUNTIME_SRCS_SCHEDULER})
set_property(GLOBAL PROPERTY RUNTIME_SRCS_SCHEDULER ${RUNTIME_SRCS_SCHEDULER})

if(NOT NEO_DISABLE_BUILTINS_COMPILATION AND NOT (TARGET ${SCHEDULER_BINARY_LIB_NAME}))
  include(scheduler_binary.cmake)
endif()
