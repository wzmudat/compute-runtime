#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

link_libraries(${ASAN_LIBS} ${TSAN_LIBS})

set(TARGET_NAME ${TARGET_NAME_L0}_exp_tests)

include(${NEO_SOURCE_DIR}/cmake/setup_ult_global_flags.cmake)

function(ADD_SUPPORTED_TEST_PRODUCT_FAMILIES_DEFINITION)
  set(L0_TESTED_PRODUCT_FAMILIES ${ALL_TESTED_PRODUCT_FAMILY})
  string(REPLACE ";" "," L0_TESTED_PRODUCT_FAMILIES
         "${L0_TESTED_PRODUCT_FAMILIES}"
  )
  add_definitions(
                  -DSUPPORTED_TEST_PRODUCT_FAMILIES=${L0_TESTED_PRODUCT_FAMILIES}
  )
endfunction()

add_supported_test_product_families_definition()

add_executable(
               ${TARGET_NAME}
               ${NEO_SOURCE_DIR}/level_zero/core/source/dll/disallow_deferred_deleter.cpp
               ${NEO_SOURCE_DIR}/shared/test/common/mocks/ult_device_factory.cpp
               ${NEO_SOURCE_DIR}/shared/test/common/mocks/ult_device_factory.h
               ${NEO_SOURCE_DIR}/level_zero/core/test/unit_tests/sources/builtin/create_ult_builtin_functions_lib.cpp
               ${NEO_SOURCE_DIR}/level_zero/tools/test/unit_tests/sources/debug/debug_session_helper.cpp
)

target_sources(
               ${TARGET_NAME}
               PRIVATE
               ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
               ${NEO_SOURCE_DIR}/level_zero/core/test/unit_tests/main.cpp
               ${NEO_SOURCE_DIR}/level_zero/core/test/unit_tests/mock.h
               ${NEO_SOURCE_DIR}/level_zero/core/test/unit_tests/white_box.h
               ${NEO_SHARED_TEST_DIRECTORY}/unit_test/tests_configuration.h
               ${NEO_SHARED_TEST_DIRECTORY}/common/test_configuration/unit_tests/test_mode.h
               ${NEO_SHARED_TEST_DIRECTORY}/common/test_configuration/unit_tests/ult_configuration.cpp
)

target_sources(
               ${TARGET_NAME}
               PRIVATE
               $<TARGET_OBJECTS:${L0_MOCKABLE_LIB_NAME}>
               $<TARGET_OBJECTS:igdrcl_libult_common>
               $<TARGET_OBJECTS:neo_libult_cs>
)

if(UNIX)
  target_sources(
                 ${TARGET_NAME}
                 PRIVATE
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/linux/create_drm_memory_manager.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/linux/drm_neo_create.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/linux/options.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/linux/sys_calls_linux_ult.cpp
  )
  if(NOT DISABLE_WDDM_LINUX)
    target_sources(
                   ${TARGET_NAME}
                   PRIVATE
                   ${NEO_SHARED_DIRECTORY}/gmm_helper/windows/gmm_memory.cpp
                   ${NEO_SHARED_DIRECTORY}/os_interface/windows/wddm/wddm_create.cpp
    )
  endif()
else()
  target_sources(
                 ${TARGET_NAME}
                 PRIVATE
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/windows/create_wddm_memory_manager.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/windows/options.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/windows/os_memory_virtual_alloc_ult.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/windows/sys_calls.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/windows/ult_dxcore_factory.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/windows/ult_dxgi_factory.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/windows/wddm_calls.cpp
                 ${COMPUTE_RUNTIME_DIR}/opencl/test/unit_test/os_interface/windows/wddm_create.cpp
  )
endif()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER ${TARGET_NAME_L0})

add_subdirectoriesl0(${CMAKE_CURRENT_SOURCE_DIR} "*")

target_compile_definitions(
                           ${TARGET_NAME}
                           PRIVATE
                           $<TARGET_PROPERTY:${L0_MOCKABLE_LIB_NAME},INTERFACE_COMPILE_DEFINITIONS>
)

target_include_directories(
                           ${TARGET_NAME} BEFORE
                           PRIVATE
                           $<TARGET_PROPERTY:${TARGET_NAME_L0},SOURCE_DIR>/experimental
                           ${NEO_SHARED_TEST_DIRECTORY}/common/test_macros/header${BRANCH_DIR_SUFFIX}
                           ${NEO_SHARED_TEST_DIRECTORY}/common/test_configuration/unit_tests
)

if(UNIX)
  target_link_libraries(${TARGET_NAME} pthread rt)
else()
  target_link_libraries(${TARGET_NAME} dbghelp)
  add_dependencies(${TARGET_NAME} mock_gdi)
endif()

target_link_libraries(
                      ${TARGET_NAME} ${NEO_STATICALLY_LINKED_LIBRARIES_MOCKABLE}
                      compute_runtime_mockable_extra ${HW_LIBS_ULT} gmock-gtest
)

target_sources(${TARGET_NAME} PRIVATE $<TARGET_OBJECTS:mock_gmm>
               $<TARGET_OBJECTS:${TARGET_NAME_L0}_mocks>
)

option(L0_ULT_VERBOSE "Use the default/verbose test output" OFF)
if(NOT L0_ULT_VERBOSE)
  set(L0_TESTS_LISTENER_OPTION "--disable_default_listener")
else()
  set(L0_TESTS_LISTENER_OPTION "--enable_default_listener")
endif()

if(MSVC)
  set_target_properties(
                        ${TARGET_NAME}
                        PROPERTIES
                        VS_DEBUGGER_COMMAND_ARGUMENTS
                        "${L0_TESTS_FILTER_OPTION} --gtest_catch_exceptions=0 ${L0_TESTS_LISTENER_OPTION}"
                        VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir
  )")
endif()

add_dependencies(unit_tests ${TARGET_NAME})

create_source_tree(${TARGET_NAME} ${L0_ROOT_DIR}/..)
