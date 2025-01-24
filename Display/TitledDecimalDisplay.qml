import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

Rectangle {
    id: root
    property string label: "Label"
    property string value: "00.0"
    property string unit: "unit"
    property color valueColor: "white"
    property color unitColor: "#ff6700"
    property int labelFontSize: 16
    property int valueFontSize: 24
    property int unitFontSize: 16
    property bool bold: true
    border.color: "white"
    border.width: 1
    color: "transparent"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 5

        Text {
            text: root.label
            color: "white"
            font.pixelSize: root.labelFontSize
            font.bold: root.bold
            horizontalAlignment: Text.AlignHCenter
        }

        RowLayout {
            spacing: 5
            Layout.alignment: Qt.AlignHCenter

            Text {
                text: root.value
                color: root.valueColor
                font.pixelSize: root.valueFontSize
                font.bold: root.bold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                text: root.unit
                color: root.unitColor
                font.pixelSize: root.unitFontSize
                font.bold: root.bold
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}