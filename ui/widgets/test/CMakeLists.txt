ui_add_test(ui.widgets_test
    SOURCES
        TestHexagonCore.cpp
    LINK
        ui.widgets
        ui.backend.recording
)
