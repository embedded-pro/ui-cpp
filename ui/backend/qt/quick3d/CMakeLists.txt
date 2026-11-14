# Opt-in: it needs Qt Quick 3D (qt6-quick3d-dev, qt6-declarative-dev, qt6-shadertools-dev), which a
# Widgets-only consumer should not have to install. The QML module is Ui.Quick3D.
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick Quick3D)

ui_add_library(ui.backend.quick3d QT
    LINK
        ui.stage
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Quick3D
)

qt_add_qml_module(ui.backend.quick3d
    URI Ui.Quick3D
    VERSION 1.0
    PLUGIN_TARGET ui.backend.quick3d_plugin
    CLASS_NAME UiQuick3DPlugin
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/Ui/Quick3D
    DEPENDENCIES QtQuick QtQuick3D
    NO_CACHEGEN
    SOURCES
        GltfInspection.cpp
        GltfInspection.hpp
        MeshCatalog.cpp
        MeshCatalog.hpp
        PolylineGeometry.cpp
        PolylineGeometry.hpp
        ShadedMeshGeometry.cpp
        ShadedMeshGeometry.hpp
    QML_FILES
        AxisTriad.qml
        StatusBadge.qml
        StudioView.qml
    RESOURCES
        studio.hdr
)

add_subdirectory(test)
