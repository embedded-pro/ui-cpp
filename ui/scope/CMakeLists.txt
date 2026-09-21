ui_add_library(ui.scope
    SOURCES
        RingBuffer.cpp
        RingBuffer.hpp
        ScopeCore.cpp
        ScopeCore.hpp
        Trigger.hpp
    LINK
        ui.core
        ui.theme
)

add_subdirectory(test)
