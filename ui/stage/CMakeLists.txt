ui_add_library(ui.stage
    SOURCES
        Ids.hpp
        Mesh.cpp
        Mesh.hpp
        Primitives.cpp
        Primitives.hpp
        Stl.cpp
        Stl.hpp
    LINK
        ui.core
        ui.theme
        ui.scene
)

add_subdirectory(test)
