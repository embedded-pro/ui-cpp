ui_add_library(ui.theme
    SOURCES
        Theme.cpp
        Theme.hpp
    LINK
        ui.core
)

add_subdirectory(test)
