ui_add_test(ui.backend.qt.update_widgets_test CUSTOM_MAIN
    SOURCES
        Main.cpp
        TestUpdateAssistant.cpp
        TestUpdateBanner.cpp
    LINK
        ui.backend.qt.update_widgets
)
