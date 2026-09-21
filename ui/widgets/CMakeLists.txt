ui_add_library(ui.widgets
    SOURCES
        HexagonCore.cpp
        HexagonCore.hpp
    LINK
        ui.core
)

add_subdirectory(test)
