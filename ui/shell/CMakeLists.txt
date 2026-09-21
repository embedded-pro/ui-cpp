ui_add_library(ui.shell
    SOURCES
        AppShell.cpp
        AppShell.hpp
        FormView.hpp
    LINK
        ui.core
        ui.model
        ui.theme
)

add_subdirectory(test)
