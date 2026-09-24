ui_add_test(ui.backend.qt_test CUSTOM_MAIN
    SOURCES
        Main.cpp
        TestQtAppShell.cpp
        TestQtCanvas.cpp
        TestQtConformance.cpp
        TestQtConversions.cpp
        TestQtFormConformance.cpp
        TestQtFormTable.cpp
        TestQtFormView.cpp
        TestQtPaintedWidget.cpp
        TestQtStageView.cpp
        TestQtTheme.cpp
    LINK
        ui.backend.qt
        ui.backend.recording
        ui.charts
        ui.stage
)

if (TARGET ui.backend.qt_test)
    set_target_properties(ui.backend.qt_test PROPERTIES AUTOMOC ON)
endif()
