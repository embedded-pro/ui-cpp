ui_add_library(ui.scene
    SOURCES
        Camera3D.cpp
        Camera3D.hpp
        Clip3.cpp
        Clip3.hpp
        Matrix3.hpp
        SceneGizmos.cpp
        SceneGizmos.hpp
        Transform3.hpp
        Vector3.hpp
    LINK
        ui.core
        ui.theme
)

add_subdirectory(test)
