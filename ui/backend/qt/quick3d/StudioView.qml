pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root

    default property alias content: scene.data
    readonly property alias view: view
    readonly property alias camera: camera
    readonly property alias orthographicCamera: orthographicCamera
    readonly property alias pivot: pivot

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
    // Off for a software renderer (WARP, llvmpipe), where MSAA and the probe cost seconds per frame.
    property bool antialiasing: true
    property bool lightProbeEnabled: true
    property bool keyboardShortcuts: true
    property bool orthographic: false

    property var bounds: null
    property real frameMargin: 0.08

    property bool gizmoVisible: true
    property real gizmoSize: 84
    property color gizmoTextColor: "#f2f4f7"
    property var gizmoAxes: [
        { label: "X", direction: Qt.vector3d(1, 0, 0), color: "#e53935" },
        { label: "Y", direction: Qt.vector3d(0, 1, 0), color: "#43a047" },
        { label: "Z", direction: Qt.vector3d(0, 0, 1), color: "#1e88e5" }
    ]

    property vector3d homeRotation: Qt.vector3d(-18, 35, 0)
    property real homeDistance: 2.4
    property var presets: [
        { name: "Front", rotation: Qt.vector3d(-10, 0, 0), distance: 2.9 },
        { name: "Side", rotation: Qt.vector3d(-10, 90, 0), distance: 2.9 },
        { name: "Top", rotation: Qt.vector3d(-89, 0, 0), distance: 3.3 },
        { name: "Iso", rotation: Qt.vector3d(-30, 45, 0), distance: 3.1 }
    ]

    readonly property vector3d homeFocus: Qt.vector3d(0, root.extent * root.focusHeight, 0)
    readonly property real tanHalfFieldOfView: Math.tan(camera.fieldOfView * Math.PI / 360)
    readonly property real aspect: view.height > 0 ? view.width / view.height : 1
    readonly property real minimumDistance: camera.clipNear * 4

    signal picked(var result)
    signal gizmoClicked(int axis)

    focus: true

    function nearestYaw(rotation) {
        const turns = Math.round((pivot.eulerRotation.y - rotation.y) / 360)
        return Qt.vector3d(rotation.x, rotation.y + 360 * turns, rotation.z)
    }

    function moveCamera(rotation, focus, distance) {
        rotationAnimation.to = root.nearestYaw(rotation)
        focusAnimation.to = focus
        distanceAnimation.to = Qt.vector3d(0, 0, Math.max(distance, root.minimumDistance))
        cameraMove.restart()
    }

    function fitDistance(min, max, rotation) {
        const q = Quaternion.fromEulerAngles(rotation)
        const right = q.times(Qt.vector3d(1, 0, 0))
        const up = q.times(Qt.vector3d(0, 1, 0))
        const back = q.times(Qt.vector3d(0, 0, 1))
        const centre = min.plus(max).times(0.5)
        const tanV = root.tanHalfFieldOfView * (1 - 2 * root.frameMargin)
        const tanH = tanV * root.aspect
        let perspective = 0
        let halfHeight = 0
        let depth = 0
        for (let corner = 0; corner < 8; ++corner) {
            const point = Qt.vector3d(corner & 1 ? max.x : min.x, corner & 2 ? max.y : min.y, corner & 4 ? max.z : min.z).minus(centre)
            const x = Math.abs(point.dotProduct(right))
            const y = Math.abs(point.dotProduct(up))
            const z = point.dotProduct(back)
            perspective = Math.max(perspective, z + x / tanH, z + y / tanV)
            halfHeight = Math.max(halfHeight, y, x / root.aspect)
            depth = Math.max(depth, z)
        }
        if (root.orthographic)
            return Math.max(halfHeight / tanV, depth + root.minimumDistance)
        return perspective
    }

    function frameBounds(min, max, rotation) {
        const towards = rotation === undefined ? pivot.eulerRotation : rotation
        root.moveCamera(towards, min.plus(max).times(0.5), root.fitDistance(min, max, towards))
    }

    function goTo(rotation, distance) {
        if (root.bounds)
            root.frameBounds(root.bounds.min, root.bounds.max, rotation)
        else
            root.moveCamera(rotation, root.homeFocus, distance * root.extent)
    }

    function focusOn(point, distance) {
        root.moveCamera(pivot.eulerRotation, point, distance === undefined ? camera.position.z : distance)
    }

    function lookAlong(direction) {
        const d = direction.normalized()
        const elevation = Math.asin(Math.max(-0.9998, Math.min(0.9998, d.y))) * 180 / Math.PI
        const yaw = Math.abs(d.y) > 0.9998 ? pivot.eulerRotation.y : Math.atan2(d.x, d.z) * 180 / Math.PI
        root.moveCamera(Qt.vector3d(-elevation, yaw, 0), pivot.position, camera.position.z)
    }

    function frame() {
        root.goTo(root.homeRotation, root.homeDistance)
    }

    function showPreset(index) {
        if (index >= 0 && index < root.presets.length)
            root.goTo(root.presets[index].rotation, root.presets[index].distance)
        else
            root.frame()
    }

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
    }

    Texture {
        id: probeTexture
        source: root.lightProbe
    }

    View3D {
        id: view
        anchors.fill: parent
        camera: root.orthographic ? orthographicCamera : camera

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: root.backgroundColor
            lightProbe: root.lightProbeEnabled ? probeTexture : null
            probeExposure: root.probeExposure
            tonemapMode: SceneEnvironment.TonemapModeFilmic
            antialiasingMode: root.antialiasing ? SceneEnvironment.MSAA : SceneEnvironment.NoAA
            antialiasingQuality: SceneEnvironment.High
            specularAAEnabled: root.antialiasing
            aoStrength: root.ambientOcclusion ? 55 : 0
            aoDistance: 5
            aoSoftness: 40
            aoDither: true
            depthPrePassEnabled: true
        }

        Node {
            id: pivot
            objectName: "studioPivot"
            position: root.homeFocus
            eulerRotation: root.homeRotation

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, root.extent * root.homeDistance)
                fieldOfView: 40
                clipNear: Math.max(0.5, root.extent * 0.05)
                clipFar: Math.max(1000, root.extent * 30)
            }

            OrthographicCamera {
                id: orthographicCamera
                objectName: "studioOrthographicCamera"
                readonly property real magnification: Math.max(1e-3, view.height / Math.max(1e-3, 2 * camera.position.z * root.tanHalfFieldOfView))
                position: Qt.vector3d(0, 0, Math.max(camera.position.z, root.extent * 4))
                horizontalMagnification: orthographicCamera.magnification
                verticalMagnification: orthographicCamera.magnification
                clipNear: camera.clipNear
                clipFar: camera.clipFar + root.extent * 4
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

    OrientationGizmo {
        objectName: "studioGizmo"
        visible: root.gizmoVisible
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: root.gizmoSize
        height: root.gizmoSize
        viewRotation: pivot.rotation
        axes: root.gizmoAxes
        textColor: root.gizmoTextColor
        onAxisClicked: (axis, direction) => {
            root.lookAlong(direction)
            root.gizmoClicked(axis)
        }
    }

    ParallelAnimation {
        id: cameraMove
        PropertyAnimation { id: rotationAnimation; target: pivot; property: "eulerRotation"; duration: 250; easing.type: Easing.OutCubic }
        PropertyAnimation { id: focusAnimation; target: pivot; property: "position"; duration: 250; easing.type: Easing.OutCubic }
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
