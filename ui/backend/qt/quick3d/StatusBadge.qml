import QtQuick

Rectangle {
    id: root

    property string text
    property string symbol: "⚠"
    property color tone: "#f2c25c"
    property color textColor: "#1d1f21"
    property real toneOpacity: 0.85

    visible: root.text.length > 0
    width: label.implicitWidth + 24
    height: label.implicitHeight + 12
    radius: 4
    color: Qt.rgba(root.tone.r, root.tone.g, root.tone.b, root.toneOpacity)

    Accessible.role: Accessible.AlertMessage
    Accessible.name: root.text

    Text {
        id: label
        anchors.centerIn: parent
        text: root.symbol.length > 0 ? root.symbol + " " + root.text : root.text
        color: root.textColor
        font.bold: true
    }
}
