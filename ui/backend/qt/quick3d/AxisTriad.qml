pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D

Node {
    id: root

    property real length: 10
    property real thickness: 0.4
    property color xColor: "#e53935"
    property color yColor: "#43a047"
    property color zColor: "#1e88e5"

    Repeater3D {
        model: [
            { color: root.xColor, rotation: Qt.vector3d(0, 0, -90) },
            { color: root.yColor, rotation: Qt.vector3d(0, 0, 0) },
            { color: root.zColor, rotation: Qt.vector3d(90, 0, 0) }
        ]

        Node {
            id: axis
            required property var modelData
            eulerRotation: axis.modelData.rotation

            Model {
                source: "#Cylinder"
                position: Qt.vector3d(0, root.length / 2, 0)
                scale: Qt.vector3d(root.thickness / 100, root.length / 100, root.thickness / 100)
                castsShadows: false
                receivesShadows: false
                pickable: false
                materials: DefaultMaterial {
                    lighting: DefaultMaterial.NoLighting
                    diffuseColor: axis.modelData.color
                }
            }
        }
    }
}
