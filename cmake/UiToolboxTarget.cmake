# Helpers replacing the warning/AUTOMOC/include boilerplate copy-pasted 28 times across the
# consumer repos. emil is optional here: when ui-toolbox is nested inside a repo that provides
# emil its helpers are used, otherwise plain CMake equivalents are.

function(ui_toolbox_add_library target)
    cmake_parse_arguments(ARG "QT" "" "SOURCES;LINK" ${ARGN})

    add_library(${target} STATIC ${UI_TOOLBOX_EXCLUDE_FROM_ALL})

    target_include_directories(${target} PUBLIC
        "$<BUILD_INTERFACE:${UI_TOOLBOX_INCLUDE_ROOT}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )

    target_sources(${target} PRIVATE ${ARG_SOURCES})

    if (ARG_LINK)
        target_link_libraries(${target} PUBLIC ${ARG_LINK})
    endif()

    if (ARG_QT)
        set_target_properties(${target} PROPERTIES AUTOMOC ON)
    endif()

    ui_toolbox_set_warnings(${target})
endfunction()

function(ui_toolbox_add_test target)
    cmake_parse_arguments(ARG "" "" "SOURCES;LINK" ${ARGN})

    if (NOT UI_TOOLBOX_BUILD_TESTS)
        return()
    endif()

    add_executable(${target})
    target_sources(${target} PRIVATE ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE ${ARG_LINK} GTest::gmock_main)
    ui_toolbox_set_warnings(${target})

    add_test(NAME ${target} COMMAND ${target})
endfunction()

function(ui_toolbox_set_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

function(ui_toolbox_fetch_googletest)
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
endfunction()
