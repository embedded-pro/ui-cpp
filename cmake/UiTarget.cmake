# Helpers replacing the warning/AUTOMOC/include boilerplate copy-pasted 28 times across the
# consumer repos. emil is optional here: when ui is nested inside a repo that provides
# emil its helpers are used, otherwise plain CMake equivalents are.

function(ui_add_library target)
    cmake_parse_arguments(ARG "QT" "" "SOURCES;LINK" ${ARGN})

    add_library(${target} STATIC ${UI_EXCLUDE_FROM_ALL})

    target_include_directories(${target} PUBLIC
        "$<BUILD_INTERFACE:${UI_INCLUDE_ROOT}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )

    target_sources(${target} PRIVATE ${ARG_SOURCES})

    if (ARG_LINK)
        target_link_libraries(${target} PUBLIC ${ARG_LINK})
    endif()

    if (ARG_QT)
        set_target_properties(${target} PROPERTIES AUTOMOC ON)
    endif()

    ui_set_warnings(${target})
    ui_enable_coverage(${target})
endfunction()

# CUSTOM_MAIN is for suites that must own a process-wide object across every test. The Qt backend
# needs it: a QApplication destroyed from an exit handler outlives Qt's own statics and crashes on
# the way out, so it has to be constructed and destroyed inside main.
function(ui_add_test target)
    cmake_parse_arguments(ARG "CUSTOM_MAIN" "" "SOURCES;LINK" ${ARGN})

    if (NOT UI_BUILD_TESTS)
        return()
    endif()

    if (ARG_CUSTOM_MAIN)
        set(entrypoint GTest::gmock)
    else()
        set(entrypoint GTest::gmock_main)
    endif()

    add_executable(${target})
    target_sources(${target} PRIVATE ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE ${ARG_LINK} ${entrypoint})
    ui_set_warnings(${target})
    ui_enable_coverage(${target})

    add_test(NAME ${target} COMMAND ${target})
endfunction()

# emil supplies coverage instrumentation in the sibling repos; this repo keeps emil optional, so
# it has to instrument its own targets. Deliberately not applied to vendored googletest.
function(ui_enable_coverage target)
    if (NOT UI_ENABLE_COVERAGE)
        return()
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PUBLIC --coverage -O0)
        target_link_options(${target} PUBLIC --coverage)
    endif()
endfunction()

function(ui_set_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

function(ui_fetch_googletest)
    if (TARGET GTest::gmock_main)
        return()
    endif()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0
    )
    set(gtest_force_shared_crt On CACHE BOOL "" FORCE)
    set(INSTALL_GTEST Off CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # This project builds warnings-as-error, but that policy is ours and does not belong to a
    # vendored dependency: AppleClang 21 rejects googletest's own char8_t handling in
    # gtest-printers.h under -Wcharacter-conversion.
    foreach(vendored gtest gtest_main gmock gmock_main)
        if (TARGET ${vendored})
            set_target_properties(${vendored} PROPERTIES COMPILE_WARNING_AS_ERROR Off)
        endif()
    endforeach()
endfunction()
