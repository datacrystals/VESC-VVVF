import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Extras 1.4 // Import for Gauge
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
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Text {
                    text: "Velocity"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true // Bold text
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                }

                Gauge {
                    id: speedGauge
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignCenter // Center the gauge
                    orientation: Qt.Vertical
                    minimumValue: 0
                    maximumValue: 100
                    value: 0 // Initial value
                    tickmarkStepSize: 20
                    minorTickmarkCount: 4
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

                // Decimal value and unit label for Speed
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter // Center the content

                    Text {
                        text: speedGauge.value.toFixed(1).padStart(5, '0') // 000.0 format
                        color: "#00FF00" // Bright green
                        font.family: "Monospace"
                        font.pixelSize: 28 // Increased font size
                        font.bold: true // Bold text
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "mph"
                        color: unitColor // Neon indigo
                        font.pixelSize: 14
                        font.bold: true // Bold text
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // White divider line
            Rectangle {
                width: 2
                Layout.fillHeight: true
                color: "white"
            }

            // Phase Current Gauge
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Text {
                    text: "PhCur"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true // Bold text
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                }

                Gauge {
                    id: phaseAmpsGauge
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignCenter // Center the gauge
                    orientation: Qt.Vertical
                    minimumValue: 0
                    maximumValue: 200
                    value: 0 // Initial value
                    tickmarkStepSize: 40
                    minorTickmarkCount: 4
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

                // Decimal value and unit label for Phase Current
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter // Center the content

                    Text {
                        text: phaseAmpsGauge.value.toFixed(1).padStart(5, '0') // 000.0 format
                        color: "#FFFF00" // Bright yellow
                        font.family: "Monospace"
                        font.pixelSize: 28 // Increased font size
                        font.bold: true // Bold text
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "A"
                        color: unitColor // Neon indigo
                        font.pixelSize: 14
                        font.bold: true // Bold text
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // White divider line
            Rectangle {
                width: 2
                Layout.fillHeight: true
                color: "white"
            }

            // Tractive Power Gauge
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Text {
                    text: "TrPwr"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true // Bold text
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                }

                Gauge {
                    id: powerGauge
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignCenter // Center the gauge
                    orientation: Qt.Vertical
                    minimumValue: 0
                    maximumValue: 4
                    value: 0 // Initial value, centered at 0
                    tickmarkStepSize: 1
                    minorTickmarkCount: 3
                    tickmarkAlignment: Qt.AlignRight
                    formatValue: function(value) {
                        return value.toFixed(2); // Display value with 2 decimal places
                    }
                }

                // Grey horizontal line
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 2
                    color: "grey"
                }

                // Decimal value and unit label for Tractive Power
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter // Center the content

                    Text {
                        text: powerGauge.value.toFixed(1).padStart(4, '0') // 00.00 format
                        color: "#FF0000" // Bright red
                        font.family: "Monospace"
                        font.pixelSize: 28 // Increased font size
                        font.bold: true // Bold text
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "kW"
                        color: unitColor // Neon indigo
                        font.pixelSize: 14
                        font.bold: true // Bold text
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            // White divider line
            Rectangle {
                width: 2
                Layout.fillHeight: true
                color: "white"
            }

            // Requested Tractive Effort Gauge
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Text {
                    text: "MtrDty"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true // Bold text
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                }

                Gauge {
                    id: throttleGauge
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignCenter // Center the gauge
                    orientation: Qt.Vertical
                    minimumValue: 0
                    maximumValue: 100
                    value: 0 // Initial value
                    tickmarkStepSize: 20
                    minorTickmarkCount: 4
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

                // Decimal value and unit label for Requested Tractive Effort
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter // Center the content

                    Text {
                        text: throttleGauge.value.toFixed(1).padStart(5, '0') // 000.0 format
                        color: "#00FFFF" // Bright cyan
                        font.family: "Monospace"
                        font.pixelSize: 28 // Increased font size
                        font.bold: true // Bold text
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "%"
                        color: unitColor // Neon indigo
                        font.pixelSize: 14
                        font.bold: true // Bold text
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
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
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            border.color: "white"
            border.width: 1

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 5

                Text {
                    text: "Motor Temp"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        id: motorTempValue
                        text: "00.0"
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "°C"
                        color: unitColor // Neon indigo
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        // Inverter Temperature
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            border.color: "white"
            border.width: 1

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 5

                Text {
                    text: "Inverter Temp"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        id: inverterTempValue
                        text: "00.0"
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "°C"
                        color: unitColor // Neon indigo
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        // Efficiency (Wh/mi)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            border.color: "white"
            border.width: 1

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 5

                Text {
                    text: "Efficiency"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        id: efficiencyValue
                        text: "00.0"
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "Wh/mi"
                        color: unitColor // Neon indigo
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        // Traction Battery Volts
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            border.color: "white"
            border.width: 1

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 5

                Text {
                    text: "Battery Volts"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        id: batteryVoltsValue
                        text: "00.0"
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "V"
                        color: unitColor // Neon indigo
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        // Used Amp Hours
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            border.color: "white"
            border.width: 1

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 5

                Text {
                    text: "Used Ah"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        id: usedAhValue
                        text: "00.0"
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "Ah"
                        color: unitColor // Neon indigo
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        // Distance
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            border.color: "white"
            border.width: 1

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 5

                Text {
                    text: "Distance"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        id: distanceValue
                        text: "00.0"
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        text: "mi"
                        color: unitColor // Neon indigo
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
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

    function onValuesReceived(values, mask) {
        phaseAmpsGauge.value = values.current_motor; // Example: Phase current
        powerGauge.value = (values.current_in * values.v_in) / 1000; // Convert to kW
        throttleGauge.value = values.duty_now * 100; // Example: Throttle (assuming duty cycle is 0-1)

        // Update bottom grid values
        motorTempValue.text = values.temp_motor.toFixed(1);
        inverterTempValue.text = values.temp_mos.toFixed(1);
        batteryVoltsValue.text = values.v_in.toFixed(1);
        usedAhValue.text = values.amp_hours.toFixed(1);
    }

    function onValuesSetupReceived(values, mask) {
        // Use speed directly from values.speed instead of RPM
        speedGauge.value = values.speed * 0.621371; // Convert km/h to mph

        // Update odometer (distance) from values.odometer
        distanceValue.text = (values.odometer / 1000).toFixed(1); // Convert meters to kilometers

        // Calculate efficiency (Wh/mi)
        var powerWatts = values.current_in * values.v_in;
        var speedMph = values.speed * 0.621371; // Convert km/h to mph
        var efficiency = speedMph > 0 ? (powerWatts / speedMph) : 0;
        efficiencyValue.text = efficiency.toFixed(1);
    }
}

}