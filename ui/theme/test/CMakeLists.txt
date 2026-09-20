ui_add_test(ui.theme_test
    SOURCES
        TestRecordingCanvas.cpp
        TestTheme.cpp
    LINK
        ui.theme
        ui.backend.recording
)
