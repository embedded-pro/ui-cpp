ui_toolbox_add_library(ui.core
    SOURCES
        Callback.hpp
        Canvas.hpp
        Color.hpp
        Font.hpp
        Format.cpp
        Format.hpp
        Geometry.hpp
        Input.cpp
        Input.hpp
        PaintedView.cpp
        PaintedView.hpp
        Pen.hpp
)

add_subdirectory(test)
