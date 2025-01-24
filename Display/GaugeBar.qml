import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

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
    property real tickmarkStepSize: 20.0 // Changed to real for floating-point values
    property int minorTickmarkCount: 4
    property int gaugeWidth: 150 // New property to control gauge width
    property int barWidth: 40 // New property to control the width of the vertical bar
    property int bottomMargin: 15 // Default bottom margin set to 15
    property int topMargin: 10 // New property to control the margin at the top of the gauge

    // Calculate the total width from the arrow to the labels
    property real totalWidth: arrowCanvas.width + gaugeBackground.width + 20 // Arrow + Bar + Label spacing

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
            Layout.topMargin: root.topMargin // Apply top margin to the title
        }

        // Gauge
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: root.totalWidth // Set the width of the gauge based on totalWidth
            Layout.topMargin: root.topMargin // Apply top margin to the gauge
            Layout.alignment: Qt.AlignHCenter // Center the gauge horizontally

            // Background of the gauge
            Rectangle {
                id: gaugeBackground
                width: root.barWidth
                height: parent.height - root.bottomMargin - root.topMargin // Account for top and bottom margins
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: root.bottomMargin // Apply bottom margin here
                color: "transparent"
                border.color: "white"
                border.width: 1

                // Vertical bar representing the gauge value
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: (root.value - root.minValue) / (root.maxValue - root.minValue) * parent.height
                    color: root.valueColor
                    radius: 3 // Rounded corners for the bar
                }
            }

            // Triangular arrow pointing to the value (on the left side of the bar)
            Canvas {
                id: arrowCanvas
                width: 20 // 2x bigger (20 pixels wide)
                height: 20
                anchors.right: gaugeBackground.left // Position to the left of the bar
                anchors.rightMargin: 0 // Move the arrow left so its rightmost tip touches the bar
                anchors.verticalCenter: gaugeBackground.bottom
                anchors.verticalCenterOffset: -(root.value - root.minValue) / (root.maxValue - root.minValue) * gaugeBackground.height

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.beginPath();
                    ctx.moveTo(width, height / 2); // Start at the right middle
                    ctx.lineTo(0, 0); // Draw to the top left
                    ctx.lineTo(0, height); // Draw to the bottom left
                    ctx.closePath();
                    ctx.fillStyle = root.valueColor; // Use the same color as the bar
                    ctx.fill();
                }
            }

            // Tick marks (positioned to the right of the bar)
            Repeater {
                model: Math.floor((root.maxValue - root.minValue) / root.tickmarkStepSize) + 1

                Rectangle {
                    width: 10
                    height: 1
                    color: "white"
                    anchors.bottom: gaugeBackground.bottom
                    anchors.bottomMargin: (index * root.tickmarkStepSize / (root.maxValue - root.minValue)) * gaugeBackground.height
                    anchors.left: gaugeBackground.right // Align to the right of the bar
                    anchors.leftMargin: 5 // Space between bar and tick marks
                }
            }

            // Minor tick marks (positioned to the right of the bar)
            Repeater {
                model: Math.floor((root.maxValue - root.minValue) / (root.tickmarkStepSize / root.minorTickmarkCount)) + 1

                Rectangle {
                    width: 5
                    height: 1
                    color: "white"
                    anchors.bottom: gaugeBackground.bottom
                    anchors.bottomMargin: (index * (root.tickmarkStepSize / root.minorTickmarkCount) / (root.maxValue - root.minValue)) * gaugeBackground.height
                    anchors.left: gaugeBackground.right // Align to the right of the bar
                    anchors.leftMargin: 5 // Space between bar and minor tick marks
                }
            }

            // Labels (positioned to the right of the bar, centered vertically on the tick marks)
            Repeater {
                model: Math.floor((root.maxValue - root.minValue) / root.tickmarkStepSize) + 1

                Text {
                    text: {
                        var value = root.minValue + index * root.tickmarkStepSize;
                        // Show decimals only if the value requires it
                        if (value % 1 !== 0) {
                            return value.toFixed(1); // Show 1 decimal place
                        } else {
                            return value.toFixed(0); // Show no decimals
                        }
                    }
                    color: "white"
                    font.pixelSize: 12
                    anchors.verticalCenter: gaugeBackground.bottom // Center vertically on the tick mark
                    anchors.verticalCenterOffset: -(index * root.tickmarkStepSize / (root.maxValue - root.minValue)) * gaugeBackground.height
                    anchors.left: gaugeBackground.right
                    anchors.leftMargin: 20 // Space between bar and labels
                }
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
                text: root.value.toFixed(1).padStart(5, '0') // 000.0 format
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