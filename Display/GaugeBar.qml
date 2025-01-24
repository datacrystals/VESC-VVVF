import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Extras 1.4 // Import for Gauge

Item {
    id: root
    property string title: "Title"
    property real value: 0
    property real minValue: 0
    property real maxValue: 100
    property string unit: "unit"
    property color valueColor: "white"
    property color unitColor: "#ff6700"
    property int titleFontSize: 20
    property int valueFontSize: 28
    property int unitFontSize: 14
    property bool bold: true
    property int tickmarkStepSize: 20
    property int minorTickmarkCount: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Title
        Text {
            text: root.title
            color: "white"
            font.pixelSize: root.titleFontSize
            font.bold: root.bold
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 30
        }

        // Gauge
        Gauge {
            id: gauge
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical
            minimumValue: root.minValue
            maximumValue: root.maxValue
            value: root.value
            tickmarkStepSize: root.tickmarkStepSize
            minorTickmarkCount: root.minorTickmarkCount
            tickmarkAlignment: Qt.AlignRight
            formatValue: function(value) {
                return value.toFixed(1); // Display value with 1 decimal place
            }
        }

        // Grey horizontal line
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 2
            color: "grey"
        }

        // Decimal value and unit label
        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            spacing: 5
            Layout.alignment: Qt.AlignHCenter // Center the content

            Text {
                text: gauge.value.toFixed(1).padStart(5, '0') // 000.0 format
                color: root.valueColor
                font.family: "Monospace"
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