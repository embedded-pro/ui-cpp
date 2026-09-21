# A floor rather than the bare package: apt ships whatever the distribution pinned, and the
# per-event QMouseEvent::position() accessor this backend relies on is Qt 6 only.
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

ui_add_library(ui.backend.qt QT
    SOURCES
        QtAppShell.cpp
        QtAppShell.hpp
        QtCanvas.cpp
        QtCanvas.hpp
        QtConversions.hpp
        QtFormTable.cpp
        QtFormTable.hpp
        QtFormView.cpp
        QtFormView.hpp
        QtPaintedWidget.cpp
        QtPaintedWidget.hpp
        QtTheme.cpp
        QtTheme.hpp
    LINK
        ui.core
        ui.theme
        ui.shell
        Qt6::Widgets
)

add_subdirectory(test)
