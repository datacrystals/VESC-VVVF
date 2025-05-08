import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Extras 1.4
import Vedder.vesc.utility 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import "qrc:/mobile"

Item {
    id: mainItem
    anchors.fill: parent

    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()

    // Configurable color for units
    property color unitColor: "#ff6700" // Neon indigo

    // Black background for the entire display
    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    // Container for the bars
    Rectangle {
        id: container
        width: parent.width
        height: parent.height * 0.70 // Use 60% of the screen height
        color: "transparent"
        border.color: "white"
        border.width: 2
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter

        RowLayout {
            id: barRow
            anchors.fill: parent
            spacing: 0 // No spacing between gauges, as we'll add dividers manually

            // Speed Gauge
            GaugeBar {
                id: speedGauge
                title: "Velocity"
                value: 0
                minValue: 0
                maxValue: 100
                unit: VescIf.useImperialUnits() ? "mph" : "km/h"
                valueColor: "#00FF00" // Bright green
                tickmarkStepSize: 10
                minorTickmarkCount: 4
                gaugeWidth: 150 // Set the width of the gauge
                barWidth: 40 // Set the width of the vertical bar (2x wider)
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // White divider line
            Rectangle {
                width: 2
                Layout.fillHeight: true
                color: "white"
            }

            // Phase Current Gauge
            GaugeBar {
                id: phaseAmpsGauge
                title: "PhCurrent"
                value: 0
                minValue: 0
                maxValue: 200
                unit: "A"
                valueColor: "#FFFF00" // Bright yellow
                tickmarkStepSize: 25
                minorTickmarkCount: 4
                gaugeWidth: 150 // Set the width of the gauge
                barWidth: 40 // Set the width of the vertical bar (2x wider)
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // White divider line
            Rectangle {
                width: 2
                Layout.fillHeight: true
                color: "white"
            }

            // Tractive Power Gauge
            GaugeBar {
                id: powerGauge
                title: "TrPower"
                value: 0
                minValue: 0
                maxValue: 4
                unit: "kW"
                valueColor: "#FF0000" // Bright red
                tickmarkStepSize: 0.5
                minorTickmarkCount: 4
                gaugeWidth: 150 // Set the width of the gauge
                barWidth: 40 // Set the width of the vertical bar (2x wider)
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // White divider line
            Rectangle {
                width: 2
                Layout.fillHeight: true
                color: "white"
            }

            // Requested Tractive Effort Gauge
            GaugeBar {
                id: throttleGauge
                title: "TrEffort"
                value: 0
                minValue: 0
                maxValue: 100
                unit: "%"
                valueColor: "#00FFFF" // Bright cyan
                tickmarkStepSize: 10
                minorTickmarkCount: 4
                gaugeWidth: 150 // Set the width of the gauge
                barWidth: 40 // Set the width of the vertical bar (2x wider)
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    // Bottom grid layout for additional data
    GridLayout {
        id: bottomGrid
        anchors.top: container.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        columns: 2
        rows: 3
        columnSpacing: 0 // Remove spacing between columns
        rowSpacing: 0 // Remove spacing between rows

        // Motor Temperature
        TitledDecimalDisplay {
            id: motorTempDisplay
            label: "Motor Temp"
            unit: "°C"
            value: "00.0"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Inverter Temperature
        TitledDecimalDisplay {
            id: inverterTempDisplay
            label: "Inverter Temp"
            unit: "°C"
            value: "00.0"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Efficiency (Wh/mi)
        TitledDecimalDisplay {
            id: efficiencyDisplay
            label: "Efficiency"
            unit: VescIf.useImperialUnits() ? "Wh/mi" : "Wh/km"
            value: "00.0"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Traction Battery Volts
        TitledDecimalDisplay {
            id: batteryVoltsDisplay
            label: "Battery Volts"
            unit: "V"
            value: "00.0"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Used Amp Hours
        TitledDecimalDisplay {
            id: usedAhDisplay
            label: "Used Ah"
            unit: "Ah"
            value: "00.0"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Distance
        TitledDecimalDisplay {
            id: distanceDisplay
            label: "Distance"
            unit: VescIf.useImperialUnits() ? "mi" : "km"
            value: "00.0"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    Timer {
        running: true
        repeat: true
        interval: 100

        onTriggered: {
            mCommands.getValues(); // Fetch standard values (current, voltage, duty cycle, etc.)
            mCommands.getValuesSetup(); // Fetch setup values (speed, odometer, etc.)
        }
    }

    Connections {
        target: mCommands

        function onValuesSetupReceived(values, mask) {
            // Setup Imperial/Metric
            var useImperial = VescIf.useImperialUnits()
            var impFact = useImperial ? 0.621371192 : 1.0

            // Use speed directly from values.speed instead of RPM
            speedGauge.value = values.speed * 3.6 * impFact; // Convert m/s to km/h or mi/h
            phaseAmpsGauge.value = values.current_motor; // Example: Phase current
            powerGauge.value = (values.current_in * values.v_in) / 1000; // Convert to kW
            throttleGauge.value = values.duty_now * 100; // Example: Throttle (assuming duty cycle is 0-1)

            // Update bottom grid values
            motorTempDisplay.value = values.temp_motor.toFixed(1);
            inverterTempDisplay.value = values.temp_mos.toFixed(1);
            batteryVoltsDisplay.value = values.v_in.toFixed(1);
            usedAhDisplay.value = values.amp_hours.toFixed(1);

            // Update odometer (distance) from values.odometer
            distanceDisplay.value = ((values.odometer * impFact) / 1000.0).toFixed(1); // Convert meters to km or mi

            // Calculate efficiency
            //var alpha = 0.05
            //var efficiencyNow = Math.max(Math.min(values.current_in * values.v_in / Math.max(values.speed * 3.6 * impFact, 1e-6), 60), -60)
            //var efficiency_lpf = (1.0 - alpha) * efficiency_lpf + alpha * efficiencyNow
            //efficiencyDisplay.value = efficiency_lpf.toFixed(1);
        }
    }

    // GaugeBar Component
    Component {
        id: gaugeBarComponent
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
                        text: {
                            var formattedValue = root.value.toFixed(1);
                            if (root.value < 0) {
                                // Add a negative sign to the left of the value
                                formattedValue = "-" + Math.abs(root.value).toFixed(1).padStart(4, '0'); // Ensure 4 digits after the negative sign
                            } else {
                                formattedValue = formattedValue.padStart(5, '0'); // 000.0 format
                            }
                            return formattedValue;
                        }
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
    }

    // TitledDecimalDisplay Component
    Component {
        id: titledDecimalDisplayComponent
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
    }
}