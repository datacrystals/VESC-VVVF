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
            GaugeBar {
                id: speedGauge
                title: "Velocity"
                value: 0
                minValue: 0
                maxValue: 100
                unit: VescIf.useImperialUnits() ? "mph" : "km/h"
                valueColor: "#00FF00" // Bright green
                tickmarkStepSize: 20
                minorTickmarkCount: 4
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
                title: "PhCur"
                value: 0
                minValue: 0
                maxValue: 200
                unit: "A"
                valueColor: "#FFFF00" // Bright yellow
                tickmarkStepSize: 40
                minorTickmarkCount: 4
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
                title: "TrPwr"
                value: 0
                minValue: 0
                maxValue: 4
                unit: "kW"
                valueColor: "#FF0000" // Bright red
                tickmarkStepSize: 1
                minorTickmarkCount: 3
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
                title: "MtrDty"
                value: 0
                minValue: 0
                maxValue: 100
                unit: "%"
                valueColor: "#00FFFF" // Bright cyan
                tickmarkStepSize: 20
                minorTickmarkCount: 4
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
            var alpha = 0.05
            var efficiencyNow = Math.max(Math.min(values.current_in * values.v_in / Math.max(values.speed * 3.6 * impFact, 1e-6), 60), -60)
            efficiency_lpf = (1.0 - alpha) * efficiency_lpf + alpha * efficiencyNow
            efficiencyDisplay.value = efficiency_lpf.toFixed(1);
        }
    }
}