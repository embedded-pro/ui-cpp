pragma ComponentBehavior: Bound

import QtQuick
import QtQuick3D

Item {
    id: root

    property quaternion viewRotation: Qt.quaternion(1, 0, 0, 0)
    property var axes: []
    property color textColor: "#f2f4f7"
    property real tipSize: 20

    readonly property vector3d viewRight: root.viewRotation.times(Qt.vector3d(1, 0, 0))
    readonly property vector3d viewUp: root.viewRotation.times(Qt.vector3d(0, 1, 0))
    readonly property vector3d viewBack: root.viewRotation.times(Qt.vector3d(0, 0, 1))
    readonly property real reach: Math.min(root.width, root.height) / 2 - root.tipSize / 2
    readonly property int endCount: ends.count

    signal axisClicked(int axis, vector3d direction)

    Accessible.role: Accessible.Grouping
    Accessible.name: qsTr("View orientation")

    Repeater {
        id: ends
        model: root.axes.length * 2

        Item {
            id: end
            required property int index

            readonly property int axis: Math.floor(end.index / 2)
            readonly property bool positive: end.index % 2 === 0
            readonly property var spec: root.axes[end.axis]
            readonly property vector3d direction: end.positive ? end.spec.direction : end.spec.direction.times(-1)
            readonly property real screenX: end.direction.dotProduct(root.viewRight) * root.reach
            readonly property real screenY: -end.direction.dotProduct(root.viewUp) * root.reach
            readonly property real depth: end.direction.dotProduct(root.viewBack)

            anchors.fill: parent
            z: end.depth

            Rectangle {
                visible: end.positive
                x: root.width / 2
                y: root.height / 2 - height / 2
                width: Math.hypot(end.screenX, end.screenY)
                height: 2
                antialiasing: true
                color: end.spec.color
                transformOrigin: Item.Left
                rotation: Math.atan2(end.screenY, end.screenX) * 180 / Math.PI
            }

            Rectangle {
                id: tip
                objectName: "gizmo" + (end.positive ? "+" : "-") + end.spec.label
                x: root.width / 2 + end.screenX - width / 2
                y: root.height / 2 + end.screenY - height / 2
                width: end.positive ? root.tipSize : root.tipSize * 0.6
                height: tip.width
                radius: tip.width / 2
                antialiasing: true
                color: end.positive ? end.spec.color : Qt.rgba(end.spec.color.r, end.spec.color.g, end.spec.color.b, 0.45)
                border.color: Qt.darker(end.spec.color, 1.4)
                border.width: end.positive ? 0 : 1
                opacity: end.depth < -0.2 ? 0.75 : 1.0

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("View along %1%2").arg(end.positive ? "+" : "−").arg(end.spec.label)
                Accessible.onPressAction: root.axisClicked(end.axis, end.direction)

                Text {
                    visible: end.positive
                    anchors.centerIn: parent
                    text: end.spec.label
                    color: root.textColor
                    font.bold: true
                    font.pixelSize: root.tipSize * 0.6
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -2
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.axisClicked(end.axis, end.direction)
                }
            }
        }
    }
}
