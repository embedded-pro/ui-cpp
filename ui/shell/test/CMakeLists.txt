ui_add_test(ui.shell_test
    SOURCES
        TestRecordingFormView.cpp
        TestRecordingShell.cpp
    LINK
        ui.shell
        ui.backend.recording
)
