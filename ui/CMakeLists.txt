add_subdirectory(core)
add_subdirectory(theme)
add_subdirectory(model)
add_subdirectory(shell)
add_subdirectory(charts)
add_subdirectory(scope)
add_subdirectory(scene)
add_subdirectory(backend/recording)

if (UI_BUILD_QT_BACKEND)
    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/backend/qt")
        message(FATAL_ERROR
            "UI_BUILD_QT_BACKEND is on, but ui/backend/qt does not exist yet. The Qt backend "
            "arrives with the QtCanvas work; until then build the portable layer without it.")
    endif()

    add_subdirectory(backend/qt)
endif()
