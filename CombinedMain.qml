import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Extras 1.4
import Vedder.vesc.utility 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

// Root element of the file
Item {
    id: rootItem
    anchors.fill: parent

    Component {
    id: gaugeBarComponent
    Item {
        id: root
        property string title: "Title"
        property real value: 0
        property real minValue: -100
        property real maxValue: 100
        property string unit: "unit"
        property color valueColor: "white"
        property color unitColor: "#ff6700"
        property color negativeColor: "#6a7aff"
        property int titleFontSize: 20
        property int valueFontSize: 28
        property int unitFontSize: 14
        property bool bold: true
        property real tickmarkStepSize: 20.0
        property int minorTickmarkCount: 4
        property int gaugeWidth: 150
        property int barWidth: 40
        property int bottomMargin: 15
        property int topMargin: 10
        property int leftMargin: 10
        property real totalWidth: arrowCanvas.width + gaugeBackground.width + root.leftMargin + 20

        // Calculate zero position (center of gauge)
        property real zeroPosition: (0 - minValue) / (maxValue - minValue) * gaugeBackground.height

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
                Layout.topMargin: root.topMargin
            }

            // Gauge
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: root.totalWidth
                Layout.topMargin: root.topMargin
                Layout.alignment: Qt.AlignHCenter

                // Background of the gauge
                Rectangle {
                    id: gaugeBackground
                    width: root.barWidth
                    height: parent.height - root.bottomMargin - root.topMargin
                    anchors.left: arrowCanvas.right
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: root.bottomMargin
                    color: "transparent"
                    border.color: "white"
                    border.width: 1

                    // Vertical bar representing the gauge value
Rectangle {
    id: valueBar
    width: parent.width
    color: root.value >= 0 ? root.valueColor : root.negativeColor
    radius: 3

    // Calculate the position of zero in the gauge
    property real zeroPosition: gaugeBackground.height - (0 - root.minValue) / (root.maxValue - root.minValue) * gaugeBackground.height

    // Calculate bar height based on absolute value
    property real barHeight: Math.abs(root.value) / (root.maxValue - root.minValue) * gaugeBackground.height

    // Position and size based on value sign
    y: {
        if (root.value >= 0) {
            // For positive values: from zeroPosition - barHeight to zeroPosition
            return zeroPosition - barHeight;
        } else {
            // For negative values: from zeroPosition to zeroPosition + barHeight
            return zeroPosition;
        }
    }
    height: barHeight
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
                        anchors.left: gaugeBackground.right
                        anchors.leftMargin: 5
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
                        anchors.left: gaugeBackground.right
                        anchors.leftMargin: 5
                    }
                }

                // Labels (positioned to the right of the bar)
                Repeater {
                    model: Math.floor((root.maxValue - root.minValue) / root.tickmarkStepSize) + 1

                    Text {
                        text: {
                            var value = root.minValue + index * root.tickmarkStepSize;
                            if (value % 1 !== 0) {
                                return value.toFixed(1);
                            } else {
                                return value.toFixed(0);
                            }
                        }
                        color: "white"
                        font.pixelSize: 12
                        anchors.bottom: gaugeBackground.bottom
                        anchors.bottomMargin: (index * root.tickmarkStepSize / (root.maxValue - root.minValue)) * gaugeBackground.height - height/2
                        anchors.left: gaugeBackground.right
                        anchors.leftMargin: 20
                    }
                }

                // Triangular arrow pointing to the value
                Canvas {
                    id: arrowCanvas
                    width: 20
                    height: 20
                    anchors.left: parent.left
                    anchors.leftMargin: leftMargin
                    anchors.bottom: gaugeBackground.bottom
                    anchors.bottomMargin: {
                        var pos = (root.value - root.minValue) / (root.maxValue - root.minValue) * gaugeBackground.height;
                        return Math.max(0, Math.min(gaugeBackground.height - height, pos - height/2));
                    }

                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.beginPath();
                        ctx.moveTo(width, height / 2);
                        ctx.lineTo(0, 0);
                        ctx.lineTo(0, height);
                        ctx.closePath();
                        ctx.fillStyle = root.valueColor;
                        ctx.fill();
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
                Layout.alignment: Qt.AlignHCenter

                Text {
                    text: {
                        var formattedValue = root.value.toFixed(1);
                        if (root.value < 0) {
                            formattedValue = "-" + Math.abs(root.value).toFixed(1).padStart(4, '0');
                        } else {
                            formattedValue = formattedValue.padStart(5, '0');
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

    // Define the TitledDecimalDisplay component
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

    // Main application logic
    Item {
        id: mainItem
        anchors.fill: parent

        property Commands mCommands: VescIf.commands()
        property ConfigParams mMcConf: VescIf.mcConfig()

        // Battery configuration
        property int sCells: 26  // Number of series cells (26S battery)
        property real batteryAH: 30.0 // capacity of battery in amp hours
        property real fullVoltage: sCells * 4.2  // 4.2V per cell when full
        property real emptyVoltage: sCells * 3.0  // 3.0V per cell when empty
        property real nominalVoltage: sCells * 3.7  // nominal voltage of traction battery


        // SOC voltage lookup table (per cell voltage -> SOC)
        property var socLookup: [
            {voltage: 4.2, soc: 1.00},  // 100% charged
            {voltage: 4.15, soc: 0.99},
            {voltage: 4.11, soc: 0.95},
            {voltage: 4.08, soc: 0.90},
            {voltage: 4.02, soc: 0.80},
            {voltage: 3.98, soc: 0.70},
            {voltage: 3.95, soc: 0.60},
            {voltage: 3.91, soc: 0.50},
            {voltage: 3.87, soc: 0.40},
            {voltage: 3.85, soc: 0.30},
            {voltage: 3.84, soc: 0.20},
            {voltage: 3.82, soc: 0.15},
            {voltage: 3.79, soc: 0.10},
            {voltage: 3.70, soc: 0.05},
            {voltage: 3.60, soc: 0.02},
            {voltage: 3.30, soc: 0.00}   // 0% charged
        ]


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
            height: parent.height * 0.65 // Reduced to make space for new row
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
                Loader {
                    id: speedGaugeLoader
                    sourceComponent: gaugeBarComponent
                    onLoaded: {
                        // Bind properties to the loaded component
                        item.title = "Velocity"
                        item.valueColor = "#00FF00"
                        item.unit = VescIf.useImperialUnits() ? "mph" : "km/h"
                        item.tickmarkStepSize = 10
                        item.minorTickmarkCount = 4
                        item.gaugeWidth = 150
                        item.barWidth = 25
                        item.minValue = 0
                        item.maxValue = 100
                    }
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
                Loader {
                    id: phaseAmpsGaugeLoader
                    sourceComponent: gaugeBarComponent
                    onLoaded: {
                        // Bind properties to the loaded component
                        item.title = "PhCurrent"
                        item.valueColor = "#FFFF00"
                        item.unit = "A"
                        item.tickmarkStepSize = 50
                        item.minorTickmarkCount = 4
                        item.gaugeWidth = 150
                        item.barWidth = 25
                        item.minValue = -100
                        item.maxValue = 300
                        item.value = -50
                    }
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
                Loader {
                    id: powerGaugeLoader
                    sourceComponent: gaugeBarComponent
                    onLoaded: {
                        // Bind properties to the loaded component
                        item.title = "TrPower"
                        item.valueColor = "#FF0000"
                        item.unit = "kW"
                        item.tickmarkStepSize = 2
                        item.minorTickmarkCount = 4
                        item.gaugeWidth = 150
                        item.barWidth = 25
                        item.minValue = -4
                        item.maxValue = 16
                    }
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
                Loader {
                    id: throttleGaugeLoader
                    sourceComponent: gaugeBarComponent
                    onLoaded: {
                        // Bind properties to the loaded component
                        item.title = "TrEffort"
                        item.valueColor = "#00FFFF"
                        item.unit = "%"
                        item.tickmarkStepSize = 10
                        item.minorTickmarkCount = 4
                        item.gaugeWidth = 150
                        item.barWidth = 25
                        item.minValue = 0
                        item.maxValue = 100
                    }
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
            Loader {
                id: motorTempDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    // Bind properties to the loaded component
                    item.label = "Motor Temp"
                    item.value = "00.0"
                    item.unit = "°C"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Inverter Temperature
            Loader {
                id: inverterTempDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    // Bind properties to the loaded component
                    item.label = "Inverter Temp"
                    item.value = "00.0"
                    item.unit = "°C"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Efficiency (Wh/mi)
            Loader {
                id: efficiencyDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    // Bind properties to the loaded component
                    item.label = "Efficiency"
                    item.value = "00.0"
                    item.unit = VescIf.useImperialUnits() ? "Wh/mi" : "Wh/km"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Traction Battery Volts
            Loader {
                id: batteryVoltsDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    // Bind properties to the loaded component
                    item.label = "Battery Volts"
                    item.value = "00.0"
                    item.unit = "V"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Used Amp Hours
            Loader {
                id: usedAhDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    // Bind properties to the loaded component
                    item.label = "Used Ah"
                    item.value = "00.0"
                    item.unit = "Ah"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Distance
            Loader {
                id: distanceDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    // Bind properties to the loaded component
                    item.label = "Distance"
                    item.value = "00.0"
                    item.unit = VescIf.useImperialUnits() ? "mi" : "km"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }




            // Battery Current
            Loader {
                id: batteryCurrentDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    item.label = "Batt Current"
                    item.value = "00.0"
                    item.unit = "A"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Used Watt Hours
            Loader {
                id: usedWhDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    item.label = "Used Wh"
                    item.value = "00.0"
                    item.unit = "Wh"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Range Estimate
            Loader {
                id: rangeDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    item.label = "Range Est"
                    item.value = "00"
                    item.unit = VescIf.useImperialUnits() ? "mi" : "km"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Range Estimate
            Loader {
                id: socDisplayLoader
                sourceComponent: titledDecimalDisplayComponent
                onLoaded: {
                    item.label = "SoC Est"
                    item.value = "00"
                    item.unit = "%"
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        Timer {
            running: true
            repeat: true
            interval: 200

            onTriggered: {
                mainItem.mCommands.getValues(); // Fetch standard values (current, voltage, duty cycle, etc.)
                mainItem.mCommands.getValuesSetup(); // Fetch setup values (speed, odometer, etc.)
            }
        }

        Connections {
            target: mainItem.mCommands

            function onValuesSetupReceived(values, mask) {
                // Setup Imperial/Metric
                var useImperial = VescIf.useImperialUnits()
                var impFact = useImperial ? 0.621371192 : 1.0

                // Use speed directly from values.speed instead of RPM
                speedGaugeLoader.item.value = values.speed * 3.6 * impFact; // Convert m/s to km/h or mi/h
                phaseAmpsGaugeLoader.item.value = values.current_motor; // Example: Phase current
                powerGaugeLoader.item.value = (values.current_in * values.v_in) / 1000; // Convert to kW
                throttleGaugeLoader.item.value = values.duty_now * 100; // Example: Throttle (assuming duty cycle is 0-1)

                // Update bottom grid values
                motorTempDisplayLoader.item.value = values.temp_motor.toFixed(1);
                inverterTempDisplayLoader.item.value = values.temp_mos.toFixed(1);
                batteryVoltsDisplayLoader.item.value = values.v_in.toFixed(1);
                usedAhDisplayLoader.item.value = values.amp_hours.toFixed(1);

                // Update odometer (distance) from values.odometer
                distanceDisplayLoader.item.value = (values.odometer * impFact / 1000.0).toFixed(1);
                distanceDisplayLoader.item.unit = useImperial ? "mi" : "km";

                // Calculate efficiency
                var dist = values.tachometer_abs / 1000.0;
                var wh_consume = values.watt_hours - values.watt_hours_charged;
                var wh_km_total = wh_consume / Math.max(dist, 1e-6);
                efficiencyDisplayLoader.item.value = (wh_km_total / impFact).toFixed(1);
                efficiencyDisplayLoader.item.unit = useImperial ? "Wh/mi" : "Wh/km";

      // Calculate battery SOC using voltage lookup table
var cellVoltage = values.v_in / mainItem.sCells;
var soc = 0;

// Find the voltage range in the lookup table
for (var i = 0; i < mainItem.socLookup.length - 1; i++) {
    var lower = mainItem.socLookup[i];
    var upper = mainItem.socLookup[i+1];

    // Changed this condition - we need to check if cellVoltage is between upper and lower
    if (cellVoltage <= lower.voltage && cellVoltage >= upper.voltage) {
        // Linear interpolation between points
        var voltageRange = lower.voltage - upper.voltage;
        var voltagePos = cellVoltage - upper.voltage;
        var socRange = lower.soc - upper.soc;
        soc = upper.soc + (voltagePos / voltageRange) * socRange;
        break;
    }
}

// Handle voltages outside table range
if (cellVoltage > mainItem.socLookup[0].voltage) {
    soc = 1.00; // Above max voltage = 100%
} else if (cellVoltage < mainItem.socLookup[mainItem.socLookup.length-1].voltage) {
    soc = 0.00; // Below min voltage = 0%
}

// Update SOC display
socDisplayLoader.item.value = (soc * 100).toFixed(0);

// Calculate range estimate
var range = 0;
if (wh_km_total > 1e-3) {


    // Calculate remaining energy more accurately
    var totalCapacityWh = mainItem.batteryAH * mainItem.nominalVoltage;
    var usedWh = values.watt_hours - values.watt_hours_charged;
    var remainingWh = totalCapacityWh - usedWh;

    // Calculate current efficiency (Wh/km or Wh/mi)
    var currentEfficiency = wh_km_total / impFact;

    // Calculate range
    range = remainingWh / currentEfficiency;

    // Limit to reasonable values
    range = Math.min(range, 999);
}

                // Update new row displays
                batteryCurrentDisplayLoader.item.value = values.current_in.toFixed(1);
                usedWhDisplayLoader.item.value = wh_consume.toFixed(1);
                rangeDisplayLoader.item.value = range.toFixed(0);
            }
        }
    }

}