ui_add_library(ui.backend.recording
    SOURCES
        RecordingCanvas.cpp
        RecordingCanvas.hpp
        RecordingFormView.cpp
        RecordingFormView.hpp
        RecordingPaintedHost.cpp
        RecordingPaintedHost.hpp
        RecordingShell.cpp
        RecordingShell.hpp
    LINK
        ui.core
        ui.shell
)

add_subdirectory(test)
