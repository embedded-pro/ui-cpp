ui_add_test(ui.backend.qt_test CUSTOM_MAIN
    SOURCES
        Main.cpp
        TestQtCanvas.cpp
        TestQtConformance.cpp
        TestQtPaintedWidget.cpp
        TestQtTheme.cpp
    LINK
        ui.backend.qt
        ui.backend.recording
        ui.charts
)

if (TARGET ui.backend.qt_test)
    set_target_properties(ui.backend.qt_test PROPERTIES AUTOMOC ON)
endif()
