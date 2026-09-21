ui_add_library(ui.terminal
    SOURCES
        AnsiPalette.hpp
        TerminalScreen.cpp
        TerminalScreen.hpp
        TerminalTypes.hpp
        Vt100Parser.cpp
        Vt100Parser.hpp
        Vt100Terminal.cpp
        Vt100Terminal.hpp
    LINK
        ui.core
)

add_subdirectory(test)
