ui_add_library(ui.backend.qt.update_widgets QT
    SOURCES
        UpdateAssistant.cpp
        UpdateAssistant.hpp
        UpdateBanner.cpp
        UpdateBanner.hpp
        UpdateProduct.hpp
    LINK
        ui.backend.qt
        ui.backend.qt.update
        Qt6::Widgets
)

add_subdirectory(test)
