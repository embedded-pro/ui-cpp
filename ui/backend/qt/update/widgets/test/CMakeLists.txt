ui_add_test(ui.backend.qt.update_widgets_test CUSTOM_MAIN
    SOURCES
        DialogAnswerer.hpp
        Main.cpp
        TestUpdateAssistant.cpp
        TestUpdateBanner.cpp
    LINK
        ui.backend.qt.update_widgets
)

if (TARGET ui.backend.qt.update_widgets_test)
    set_target_properties(ui.backend.qt.update_widgets_test PROPERTIES AUTOMOC ON)
endif()
