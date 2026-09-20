cmake_minimum_required(VERSION 3.24)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(UI_STANDALONE On)
endif()

option(CMAKE_COMPILE_WARNING_AS_ERROR "Enable warnings-as-error" On)
option(UI_BUILD_TESTS "Enable build of the tests" Off)

# Deliberately NOT inferred from a consumer's *_BUILD_SIMULATOR flag: e-foc configures tools/
# (and therefore Qt) on every host build, while the toolboxes only do so under their simulator
# option. The Qt backend is opted into explicitly.
option(UI_BUILD_QT_BACKEND "Enable build of the Qt backend" Off)
option(UI_ENABLE_COVERAGE "Instrument this project's targets for gcov" Off)

if (UI_BUILD_TESTS)
    set(BUILD_TESTING On)
endif()

set(UI_INCLUDE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(UiTarget)

project(ui LANGUAGES C CXX VERSION 0.1.0) # x-release-please-version

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS_DEBUG "-Og -g" CACHE STRING "C Debug flags" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "-Og -g" CACHE STRING "C++ Debug flags" FORCE)
endif()

set_directory_properties(PROPERTY USE_FOLDERS ON)

if (UI_STANDALONE)
    set(UI_EXCLUDE_FROM_ALL "")
else()
    set(UI_EXCLUDE_FROM_ALL "EXCLUDE_FROM_ALL")
endif()

if (UI_BUILD_TESTS)
    include(CTest)
    ui_fetch_googletest()
endif()

add_subdirectory(ui)

if (COMMAND emil_clangformat_directories)
    emil_clangformat_directories(ui DIRECTORIES .)
endif()

if (COMMAND emil_coverage_targets)
    emil_coverage_targets(DIRECTORIES .)
endif()

if (UI_STANDALONE AND COMMAND emil_folderize_all_targets)
    emil_folderize_all_targets()
endif()
