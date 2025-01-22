#include "ConfigParser.h"
#include "vesc_c_if.h"
#include "Parameters.h"
#include <string.h>

/**
 * @brief Calculate the conversion factor from wheel diameter (in mm) to km/h.
 * @param diameter_mm: Wheel diameter in millimeters.
 * @return Conversion factor. Multiply this by RPM to get speed in km/h.
 */
float wheel_diameter_to_kmh_factor(float diameter_mm) {
    const float mm_to_km = 1.0f / 1000000.0f; // Convert mm to km
    const float minutes_to_hours = 60.0f;     // Convert minutes to hours
    return (float)(3.14159265535 * diameter_mm * mm_to_km * minutes_to_hours);
}

    SPWMConfig AddSPWM_AsyncFixed(int _Carrier) {
    SPWMConfig config = {SPWM_TYPE_FIXED_ASYNC, _Carrier, _Carrier, 0};
    return config;
}

SPWMConfig AddSPWM_AsyncRamp(int _CarrierFreqStart, int _CarrierFreqEnd) {
    SPWMConfig config = {SPWM_TYPE_RAMP_ASYNC, _CarrierFreqStart, _CarrierFreqEnd, 0};
    return config;
}

SPWMConfig AddSPWM_RSPWM(int _CarrierMin, int _CarrierMax) {
    SPWMConfig config = {SPWM_TYPE_RSPWM, _CarrierMin, _CarrierMax, 0};
    return config;
}

SPWMConfig AddSPWM_Sync(int _NumPulses) {
    SPWMConfig config = {SPWM_TYPE_SYNC, 0, 0, _NumPulses};
    return config;
}

SPWMConfig AddSPWM_Disabled() {
    SPWMConfig config = {SPWM_TYPE_NONE, 0, 0, 0};
    return config;
}


void SetSPWM_Acceleration(InverterConfig* _Source, int _Index, SPWMConfig config) {
    if (_Index >= MAX_SPEED_RANGES) return;
    _Source->speedRanges[_Index].spwm.acceleration = config;
}

void SetSPWM_Coasting(InverterConfig* _Source, int _Index, SPWMConfig config) {
    if (_Index >= MAX_SPEED_RANGES) return;
    _Source->speedRanges[_Index].spwm.coasting = config;
}

void SetSPWM_Deceleration(InverterConfig* _Source, int _Index, SPWMConfig config) {
    if (_Index >= MAX_SPEED_RANGES) return;
    _Source->speedRanges[_Index].spwm.deceleration = config;
}

void SetSpeedRangeSpeed(InverterConfig* _Source, int _Index, float minSpeed, float maxSpeed) {
    if (_Index >= MAX_SPEED_RANGES) return;
    _Source->speedRanges[_Index].minSpeed = minSpeed;
    _Source->speedRanges[_Index].maxSpeed = maxSpeed;
    _Source->speedRangeCount += 1;
}

void InitializeConfiguration(InverterConfig* _Config) {
    // Setup basic parameters
    _Config->maxSpeed = MAX_SPEED_KMH; // km/h
    // _Config->rpmToSpeedRatio = wheel_diameter_to_kmh_factor(WHEEL_DIAMETER_MM);
    _Config->zeroSpeedCutoffMargin = ZERO_CUTOFF_MARGIN_KMH;

    // Now add ranges
    SPWMConfig config;

    // Range 0: -1.0 km/h to 5.0 km/h
    config = AddSPWM_AsyncFixed(6000);
    SetSPWM_Acceleration(_Config, 0, config);
    SetSPWM_Coasting(_Config, 0, config);
    SetSPWM_Deceleration(_Config, 0, config);
    SetSpeedRangeSpeed(_Config, 0, -1.0f, 31.0f);


    // config = AddSPWM_AsyncFixed(2000);
    // SetSPWM_Acceleration(_Config, 1, config);
    // SetSPWM_Coasting(_Config, 1, config);
    // SetSPWM_Deceleration(_Config, 1, config);
    // SetSpeedRangeSpeed(_Config, 1, 8.0f, 14.0f);


    // config = AddSPWM_AsyncFixed(4000);
    // SetSPWM_Acceleration(_Config, 2, config);
    // SetSPWM_Coasting(_Config, 2, config);
    // SetSPWM_Deceleration(_Config, 2, config);
    // SetSpeedRangeSpeed(_Config, 2, 14.0f, 999.0f);
    // // Range 0: -1.0 km/h to 5.0 km/h
    // config = AddSPWM_AsyncRamp(250, 500); // Ramp from 250 Hz to 500 Hz
    // SetSPWM_Acceleration(_Config, 0, config);
    // SetSPWM_Coasting(_Config, 0, config);
    // SetSPWM_Deceleration(_Config, 0, config);
    // SetSpeedRangeSpeed(_Config, 0, -1.0f, 5.0f);

    // // Range 1: 5.0 km/h to 20.0 km/h
    // config = AddSPWM_AsyncFixed(500); // Fixed at 500 Hz
    // SetSPWM_Acceleration(_Config, 1, config);
    // SetSPWM_Coasting(_Config, 1, config);
    // SetSPWM_Deceleration(_Config, 1, config);
    // SetSpeedRangeSpeed(_Config, 1, 5.0f, 20.0f);

    // // Range 2: 20.0 km/h to 21.0 km/h
    // config = AddSPWM_AsyncRamp(500, 300); // Ramp from 500 Hz to 300 Hz
    // SetSPWM_Acceleration(_Config, 2, config);
    // SetSPWM_Coasting(_Config, 2, config);
    // config = AddSPWM_AsyncFixed(500); // Fixed at 500 Hz for braking
    // SetSPWM_Deceleration(_Config, 2, config);
    // SetSpeedRangeSpeed(_Config, 2, 20.0f, 21.0f);

    // // Range 3: 21.0 km/h to 27.0 km/h
    // config = AddSPWM_Sync(11); // Sync with 11 pulses
    // SetSPWM_Acceleration(_Config, 3, config);
    // SetSPWM_Coasting(_Config, 3, config);
    // SetSPWM_Deceleration(_Config, 3, config);
    // SetSpeedRangeSpeed(_Config, 3, 21.0f, 27.0f);

    // // Range 4: 27.0 km/h to 40.0 km/h
    // config = AddSPWM_Sync(7); // Sync with 7 pulses
    // SetSPWM_Acceleration(_Config, 4, config);
    // SetSPWM_Coasting(_Config, 4, config);
    // SetSPWM_Deceleration(_Config, 4, config);
    // SetSpeedRangeSpeed(_Config, 4, 27.0f, 40.0f);

    // // Range 5: 40.0 km/h to 48.0 km/h
    // config = AddSPWM_Sync(3); // Sync with 3 pulses, no wide pulse
    // SetSPWM_Acceleration(_Config, 5, config);
    // SetSPWM_Coasting(_Config, 5, config);
    // SetSPWM_Deceleration(_Config, 5, config);
    // SetSpeedRangeSpeed(_Config, 5, 40.0f, 48.0f);

    // // Range 6: 48.0 km/h to 55.0 km/h
    // config = AddSPWM_Sync(3); // Sync with 3 pulses, wide pulse enabled
    // // config.modulationIndex = 1.1f; // Modulation index for W3P mode
    // SetSPWM_Acceleration(_Config, 6, config);
    // SetSPWM_Coasting(_Config, 6, config);
    // SetSPWM_Deceleration(_Config, 6, config);
    // SetSpeedRangeSpeed(_Config, 6, 48.0f, 55.0f);

    // // Range 7: 55.0 km/h to 150.0 km/h
    // config = AddSPWM_Sync(1); // Sync with 1 pulse, no wide pulse
    // SetSPWM_Acceleration(_Config, 7, config);
    // config = AddSPWM_Sync(3); // Sync with 3 pulses, wide pulse enabled
    // // config.modulationIndex = 1.15f; // Modulation index for W3P mode
    // SetSPWM_Coasting(_Config, 7, config);
    // config = AddSPWM_Sync(1); // Sync with 1 pulse, no wide pulse
    // SetSPWM_Deceleration(_Config, 7, config);
    // SetSpeedRangeSpeed(_Config, 7, 55.0f, 150.0f);

    // // Range 0: 0.0 km/h to 4.0 km/h
    // config = AddSPWM_AsyncFixed(1235);
    // SetSPWM_Acceleration(_Config, 0, config);
    // SetSPWM_Coasting(_Config, 0, config);
    // SetSPWM_Deceleration(_Config, 0, AddSPWM_AsyncFixed(1160));
    // SetSpeedRangeSpeed(_Config, 0, 0.0f, 10.0f);

    // // Range 1: 4.0 km/h to 8.0 km/h
    // config = AddSPWM_AsyncFixed(1190);
    // SetSPWM_Acceleration(_Config, 1, config);
    // SetSPWM_Coasting(_Config, 1, config);
    // SetSPWM_Deceleration(_Config, 1, config);
    // SetSpeedRangeSpeed(_Config, 1, 10.0f, 15.0f);

    // // Range 2: 8.0 km/h to 10.0 km/h
    // config = AddSPWM_AsyncFixed(1210);
    // SetSPWM_Acceleration(_Config, 2, config);
    // SetSPWM_Coasting(_Config, 2, config);
    // SetSPWM_Deceleration(_Config, 2, config);
    // SetSpeedRangeSpeed(_Config, 2, 15.0f, 20.0f);

    // // Range 3: 10.0 km/h to 14.0 km/h
    // config = AddSPWM_AsyncFixed(1235);
    // SetSPWM_Acceleration(_Config, 3, config);
    // SetSPWM_Coasting(_Config, 3, config);
    // SetSPWM_Deceleration(_Config, 3, config);
    // SetSpeedRangeSpeed(_Config, 3, 20.0f, 23.0f);

    // // Range 4: 14.0 km/h to 16.0 km/h
    // config = AddSPWM_AsyncFixed(1460);
    // SetSPWM_Acceleration(_Config, 4, config);
    // SetSPWM_Coasting(_Config, 4, config);
    // SetSPWM_Deceleration(_Config, 4, config);
    // SetSpeedRangeSpeed(_Config, 4, 23.0f, 27.0f);

    // // Range 5: 16.0 km/h to 19.0 km/h
    // config = AddSPWM_AsyncFixed(1210);
    // SetSPWM_Acceleration(_Config, 5, config);
    // SetSPWM_Coasting(_Config, 5, config);
    // SetSPWM_Deceleration(_Config, 5, config);
    // SetSpeedRangeSpeed(_Config, 5, 27.0f, 32.0f);

    // // Range 6: 19.0 km/h to 999.0 km/h
    // config = AddSPWM_AsyncFixed(1230);
    // SetSPWM_Acceleration(_Config, 6, config);
    // SetSPWM_Coasting(_Config, 6, config);
    // SetSPWM_Deceleration(_Config, 6, config);
    // SetSpeedRangeSpeed(_Config, 6, 32.0f, 999.0f);

}


SpeedRange GetSpeedRangeAtSpeed(InverterConfig* _Source, float _Speed, float _MotorCurrent) {
    SpeedRange defaultRange = {0}; // Default range if no match is found
    defaultRange.minSpeed = 0;
    defaultRange.maxSpeed = 99999;
    defaultRange.spwm.acceleration.type = SPWM_TYPE_NONE;
    defaultRange.spwm.acceleration.carrierFrequencyStart = 0;
    defaultRange.spwm.acceleration.carrierFrequencyEnd = 0;
    defaultRange.spwm.acceleration.numPulses = 0;

    defaultRange.spwm.coasting.type = SPWM_TYPE_NONE;
    defaultRange.spwm.coasting.carrierFrequencyStart = 0;
    defaultRange.spwm.coasting.carrierFrequencyEnd = 0;
    defaultRange.spwm.coasting.numPulses = 0;

    defaultRange.spwm.deceleration.type = SPWM_TYPE_NONE;
    defaultRange.spwm.deceleration.carrierFrequencyStart = 0;
    defaultRange.spwm.deceleration.carrierFrequencyEnd = 0;
    defaultRange.spwm.deceleration.numPulses = 0;

    if (_Source == NULL || _Source->speedRangeCount == 0) {
        return defaultRange; // Return an empty range if the config is invalid
    }

    // Calculate the speed in km/h using the rpmToSpeedRatio
    float speedKmh = _Speed;

    // Cap the speed to the maximum allowed speed
    if (speedKmh > _Source->maxSpeed) {
        speedKmh = _Source->maxSpeed;
    }

    // If the speed is below the cutoff margin and the current is low, return the default range (disabled)
    if (speedKmh < _Source->zeroSpeedCutoffMargin && _MotorCurrent < 3.0f) {
        return defaultRange;
    }

    // Iterate through the speed ranges to find the appropriate range
    for (int i = 0; i < _Source->speedRangeCount; i++) {
        // We put in extra logic to ensure that the code works even for negative values
        float BottomSpeed = _Source->speedRanges[i].minSpeed;
        if (i == 0) {
            BottomSpeed -= 1.0f; // Allow some margin for the first range
        }

        if (speedKmh >= BottomSpeed && speedKmh <= _Source->speedRanges[i].maxSpeed) {
            return _Source->speedRanges[i]; // Return the matching speed range
        }
    }

    // If no range matches, return the default range
    return defaultRange;
}


void PrintInverterConfig(const InverterConfig* config) {
    if (!config) {
        VESC_IF->printf("Error: Config pointer is NULL.\n");
        return;
    }

    // Print basic configuration
    VESC_IF->printf("Inverter Configuration:\n");
    // VESC_IF->printf("  RPM to Speed Ratio: %.6f\n", (double)config->rpmToSpeedRatio);
    VESC_IF->printf("  Max Speed: %f km/h\n", (double)config->maxSpeed);
    VESC_IF->printf("  Number of Speed Ranges: %d\n", config->speedRangeCount);

    // Print each speed range and its SPWM configuration
    for (int i = 0; i < config->speedRangeCount; i++) {
        const SpeedRange* range = &config->speedRanges[i];
        VESC_IF->printf("\nSpeed Range %d:\n", i + 1);
        VESC_IF->printf("  Min Speed: %f km/h\n", (double)range->minSpeed);
        VESC_IF->printf("  Max Speed: %f km/h\n", (double)range->maxSpeed);

        // Print SPWM configuration for acceleration
        VESC_IF->printf("  Acceleration SPWM Configuration:\n");
        PrintSPWMConfig(&range->spwm.acceleration);

        // Print SPWM configuration for coasting
        VESC_IF->printf("  Coasting SPWM Configuration:\n");
        PrintSPWMConfig(&range->spwm.coasting);

        // Print SPWM configuration for deceleration
        VESC_IF->printf("  Deceleration SPWM Configuration:\n");
        PrintSPWMConfig(&range->spwm.deceleration);
    }
}

void PrintSPWMConfig(const SPWMConfig* spwm) {
    const char* spwmTypeStr = "Unknown";
    switch (spwm->type) {
        case SPWM_TYPE_FIXED_ASYNC:
            spwmTypeStr = "Fixed Async";
            break;
        case SPWM_TYPE_RAMP_ASYNC:
            spwmTypeStr = "Ramp Async";
            break;
        case SPWM_TYPE_RSPWM:
            spwmTypeStr = "Random SPWM";
            break;
        case SPWM_TYPE_SYNC:
            spwmTypeStr = "Synchronous";
            break;
        case SPWM_TYPE_NONE:
            spwmTypeStr = "Output Disabled (all 0s)";
            break;
    }
    VESC_IF->printf("    SPWM Type: %s\n", spwmTypeStr);

    if (spwm->type == SPWM_TYPE_FIXED_ASYNC || spwm->type == SPWM_TYPE_RAMP_ASYNC || spwm->type == SPWM_TYPE_RSPWM) {
        VESC_IF->printf("    Carrier Frequency Start: %d Hz\n", spwm->carrierFrequencyStart);
        if (spwm->type == SPWM_TYPE_RAMP_ASYNC || spwm->type == SPWM_TYPE_RSPWM) {
            VESC_IF->printf("    Carrier Frequency End: %d Hz\n", spwm->carrierFrequencyEnd);
        }
    } else if (spwm->type == SPWM_TYPE_SYNC) {
        VESC_IF->printf("    Number of Pulses: %d\n", spwm->numPulses);
    }
}