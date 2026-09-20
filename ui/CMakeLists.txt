add_subdirectory(core)
add_subdirectory(theme)
add_subdirectory(charts)
add_subdirectory(backend/recording)

if (UI_BUILD_QT_BACKEND)
    add_subdirectory(backend/qt)
endif()
