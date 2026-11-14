pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root

    default property alias content: scene.data
    readonly property alias view: view
    readonly property alias camera: camera

    property real extent: 100
    property real focusHeight: 0.45
    property real floorSize: 300
    property real gridSpacing: 10

    property color backgroundColor: "#434b56"
    property color floorColor: "#656a72"
    property color gridColor: "#838a95"
    property real gridOpacity: 0.5

    property url lightProbe: Qt.resolvedUrl("studio.hdr")
    property real probeExposure: 1.4
    property real keyLightBrightness: 1.6
    property real fillLightBrightness: 0.6
    property real rimLightBrightness: 0.9

    property bool floorVisible: true
    property bool gridVisible: true
    property bool shadows: true
    property bool ambientOcclusion: true
    property bool keyboardShortcuts: true

    property vector3d homeRotation: Qt.vector3d(-18, 35, 0)
    property real homeDistance: 2.4
    property var presets: [
        { name: "Front", rotation: Qt.vector3d(-10, 0, 0), distance: 2.9 },
        { name: "Side", rotation: Qt.vector3d(-10, 90, 0), distance: 2.9 },
        { name: "Top", rotation: Qt.vector3d(-89, 0, 0), distance: 3.3 },
        { name: "Iso", rotation: Qt.vector3d(-30, 45, 0), distance: 3.1 }
    ]

    signal picked(var result)

    focus: true

    function goTo(rotation, distance) {
        rotationAnimation.to = rotation
        distanceAnimation.to = Qt.vector3d(0, 0, distance * root.extent)
        cameraMove.restart()
    }

    function frame() {
        goTo(root.homeRotation, root.homeDistance)
    }

    function showPreset(index) {
        if (index >= 0 && index < root.presets.length)
            goTo(root.presets[index].rotation, root.presets[index].distance)
        else
            frame()
    }

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
    }

    View3D {
        id: view
        anchors.fill: parent
        camera: camera

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: root.backgroundColor
            lightProbe: Texture { source: root.lightProbe }
            probeExposure: root.probeExposure
            tonemapMode: SceneEnvironment.TonemapModeFilmic
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            specularAAEnabled: true
            aoStrength: root.ambientOcclusion ? 55 : 0
            aoDistance: 5
            aoSoftness: 40
            aoDither: true
            depthPrePassEnabled: true
        }

        Node {
            id: pivot
            position: Qt.vector3d(0, root.extent * root.focusHeight, 0)
            eulerRotation: root.homeRotation

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, root.extent * root.homeDistance)
                fieldOfView: 40
                clipNear: Math.max(0.5, root.extent * 0.05)
                clipFar: Math.max(1000, root.extent * 30)
            }
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(-50, -35, 0)
            brightness: root.keyLightBrightness
            castsShadow: root.shadows
            shadowMapQuality: Light.ShadowMapQualityVeryHigh
            shadowMapFar: root.floorSize * 2
            shadowFactor: 70
            shadowFilter: 10
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(-15, 140, 0)
            brightness: root.fillLightBrightness
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(-60, 180, 0)
            brightness: root.rimLightBrightness
        }

        Model {
            objectName: "studioFloor"
            visible: root.floorVisible
            source: "#Rectangle"
            eulerRotation.x: -90
            scale: Qt.vector3d(root.floorSize / 100, root.floorSize / 100, 1)
            receivesShadows: true
            castsShadows: false
            pickable: false
            materials: PrincipledMaterial {
                baseColor: root.floorColor
                metalness: 0.0
                roughness: 0.9
            }
        }

        Repeater3D {
            id: grid
            readonly property int halfCount: Math.floor(root.floorSize / 2 / root.gridSpacing)
            model: root.gridVisible ? 2 * grid.halfCount + 1 : 0

            Node {
                id: gridLine
                required property int index
                readonly property real offset: (gridLine.index - grid.halfCount) * root.gridSpacing

                Model {
                    source: "#Cube"
                    position: Qt.vector3d(gridLine.offset, 0.05, 0)
                    scale: Qt.vector3d(0.0015, 0.0005, root.floorSize / 100)
                    castsShadows: false
                    pickable: false
                    materials: DefaultMaterial {
                        lighting: DefaultMaterial.NoLighting
                        diffuseColor: root.gridColor
                        opacity: root.gridOpacity
                    }
                }

                Model {
                    source: "#Cube"
                    position: Qt.vector3d(0, 0.05, gridLine.offset)
                    scale: Qt.vector3d(root.floorSize / 100, 0.0005, 0.0015)
                    castsShadows: false
                    pickable: false
                    materials: DefaultMaterial {
                        lighting: DefaultMaterial.NoLighting
                        diffuseColor: root.gridColor
                        opacity: root.gridOpacity
                    }
                }
            }
        }

        Node {
            id: scene
        }
    }

    OrbitCameraController {
        anchors.fill: parent
        origin: pivot
        camera: camera
    }

    TapHandler {
        onTapped: (eventPoint) => {
            root.forceActiveFocus()
            root.picked(view.pick(eventPoint.position.x, eventPoint.position.y))
        }
        onDoubleTapped: root.frame()
    }

    ParallelAnimation {
        id: cameraMove
        PropertyAnimation { id: rotationAnimation; target: pivot; property: "eulerRotation"; duration: 250; easing.type: Easing.OutCubic }
        PropertyAnimation { id: distanceAnimation; target: camera; property: "position"; duration: 250; easing.type: Easing.OutCubic }
    }

    Keys.enabled: root.keyboardShortcuts
    Keys.onPressed: (event) => {
        const index = event.key - Qt.Key_1
        if (index >= 0 && index < root.presets.length && index < 9) {
            root.showPreset(index)
            event.accepted = true
        } else if (event.key === Qt.Key_F) {
            root.frame()
            event.accepted = true
        }
    }
}
