ui_add_library(ui.scene
    SOURCES
        Camera3D.cpp
        Camera3D.hpp
        SceneGizmos.cpp
        SceneGizmos.hpp
        Vector3.hpp
    LINK
        ui.core
        ui.theme
)

add_subdirectory(test)
