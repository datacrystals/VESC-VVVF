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

void AddSPWM_AsyncFixed(InverterConfig* _Source, int _Index, float _SpeedStart, float _SpeedEnd, int _Carrier) {
    if (_Index >= MAX_SPEED_RANGES) {
        return;
    }

    _Source->speedRanges[_Index].spwm.type = SPWM_TYPE_FIXED_ASYNC;
    _Source->speedRanges[_Index].spwm.carrierFrequencyStart = _Carrier;
    _Source->speedRanges[_Index].spwm.carrierFrequencyEnd = _Carrier; // Same as start for fixed
    _Source->speedRanges[_Index].minSpeed = _SpeedStart;
    _Source->speedRanges[_Index].maxSpeed = _SpeedEnd;
    _Source->speedRangeCount++;
}

void AddSPWM_AsyncRamp(InverterConfig* _Source, int _Index, float _MinSpeed, float _MaxSpeed, int _CarrierFreqStart, int _CarrierFreqEnd) {
    if (_Index >= MAX_SPEED_RANGES) {
        return;
    }

    _Source->speedRanges[_Index].spwm.type = SPWM_TYPE_RAMP_ASYNC;
    _Source->speedRanges[_Index].spwm.carrierFrequencyStart = _CarrierFreqStart;
    _Source->speedRanges[_Index].spwm.carrierFrequencyEnd = _CarrierFreqEnd;
    _Source->speedRanges[_Index].minSpeed = _MinSpeed;
    _Source->speedRanges[_Index].maxSpeed = _MaxSpeed;
    _Source->speedRangeCount++;
}

void AddSPWM_RSPWM(InverterConfig* _Source, int _Index, float _MinSpeed, float _MaxSpeed, int _CarrierMin, int _CarrierMax) {
    if (_Index >= MAX_SPEED_RANGES) {
        return;
    }

    _Source->speedRanges[_Index].spwm.type = SPWM_TYPE_RSPWM;
    _Source->speedRanges[_Index].spwm.carrierFrequencyStart = _CarrierMin;
    _Source->speedRanges[_Index].spwm.carrierFrequencyEnd = _CarrierMax;
    _Source->speedRanges[_Index].minSpeed = _MinSpeed;
    _Source->speedRanges[_Index].maxSpeed = _MaxSpeed;
    _Source->speedRangeCount++;
}

void AddSPWM_Sync(InverterConfig* _Source, int _Index, float _MinSpeed, float _MaxSpeed, int _NumPulses) {
    if (_Index >= MAX_SPEED_RANGES) {
        return;
    }

    _Source->speedRanges[_Index].spwm.type = SPWM_TYPE_SYNC;
    _Source->speedRanges[_Index].spwm.numPulses = _NumPulses;
    _Source->speedRanges[_Index].minSpeed = _MinSpeed;
    _Source->speedRanges[_Index].maxSpeed = _MaxSpeed;
    _Source->speedRangeCount++;
}

void InitializeConfiguration(InverterConfig* _Config) {

    // Setup basic parameters
    _Config->maxSpeed = MAX_SPEED_KMH; // km/h
    _Config->rpmToSpeedRatio = wheel_diameter_to_kmh_factor(WHEEL_DIAMETER_MM);
    _Config->zeroSpeedCutoffMargin = ZERO_CUTOFF_MARGIN_KMH;

    // Now add ranges
    // AddSPWM_RSPWM(_Config, 0, 0., 22., 4000, 6000); // RSPWM from 10-20km/h @ 2khz-3khz carrier
    // AddSPWM_AsyncFixed(_Config, 0, 0., 3.0, 1000); // Async SPWM from 0-5km/h @ 1khz carrier
    AddSPWM_AsyncRamp (_Config, 0, 0.0, 4, 250, 500); // Async SPWM from 5-10km/h @ 1khz-2khz carrier
    AddSPWM_AsyncFixed(_Config, 1, 4, 7, 500); // Async SPWM from 0-5km/h @ 1khz carrier
    AddSPWM_AsyncRamp (_Config, 2, 7., 7.5, 500, 300); // Async SPWM from 5-10km/h @ 1khz-2khz carrier
    // AddSPWM_AsyncFixed(_Config, 2, 6., 9.0, 3000); // Async SPWM from 0-5km/h @ 1khz carrier
    // AddSPWM_AsyncFixed(_Config, 3, 9., 12.0, 4000); // Async SPWM from 0-5km/h @ 1khz carrier
    // AddSPWM_AsyncFixed(_Config, 4, 12., 15.0, 5000); // Async SPWM from 0-5km/h @ 1khz carrier
    // AddSPWM_AsyncRamp (_Config, 1, 10.0, 13.0, 1000, 2000); // Async SPWM from 5-10km/h @ 1khz-2khz carrier
    // AddSPWM_AsyncFixed(_Config, 2, 13.0, 20., 2000); // Async SPWM from 0-5km/h @ 1khz carrier
    AddSPWM_Sync      (_Config, 3, 7.5, 12., 7); // Sync SPWM from 20-30km/h with 4 pulses
    AddSPWM_Sync      (_Config, 4, 12., 18., 5); // Sync SPWM from 20-30km/h with 4 pulses
    AddSPWM_Sync      (_Config, 5, 18., 22., 3); // Sync SPWM from 20-30km/h with 4 pulses
    AddSPWM_Sync      (_Config, 6, 22., 27., 1); // Sync SPWM from 20-30km/h with 4 pulses
    // AddSPWM_Sync      (_Config, 3, 55., 9999., 4); // Sync SPWM from 20-30km/h with 4 pulses

}


SpeedRange GetSpeedRangeAtRPM(InverterConfig* _Source, float _MotorRPM, float _MotorCurrent) {

    SpeedRange defaultRange = {0}; // Default range if no match is found
    defaultRange.minSpeed = 0;
    defaultRange.maxSpeed = 99999; 
    defaultRange.spwm.type = SPWM_TYPE_NONE;
    defaultRange.spwm.carrierFrequencyStart = 0;
    defaultRange.spwm.carrierFrequencyEnd = 0;
    defaultRange.spwm.numPulses = 0;

    if (_Source == NULL || _Source->speedRangeCount == 0) {
        // VESC_IF->printf("Error! Invalid Config")    ;
        return defaultRange; // Return an empty range if the config is invalid
    }

    // Calculate the speed in km/h using the rpmToSpeedRatio
    float speedKmh = _MotorRPM * _Source->rpmToSpeedRatio;
    // if (speedKmh < 0.) {
    //     speedKmh = -speedKmh;
    // }

    // Cap the speed to the maximum allowed speed
    if (speedKmh > _Source->maxSpeed) {
        speedKmh = _Source->maxSpeed;
    }

    if (speedKmh < _Source->zeroSpeedCutoffMargin && _MotorCurrent < 3.) {
        return defaultRange;
    }

    // Iterate through the speed ranges to find the appropriate range
    for (int i = 0; i < _Source->speedRangeCount; i++) {
        // We put in extra logic to ensure that the code works even for negative values
        float BottomSpeed = _Source->speedRanges[i].minSpeed;
        if (i == 0) {
            BottomSpeed -= 1.;
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
    VESC_IF->printf("  RPM to Speed Ratio: %.6f\n", (double)config->rpmToSpeedRatio);
    VESC_IF->printf("  Max Speed: %f km/h\n", (double)config->maxSpeed);
    VESC_IF->printf("  Number of Speed Ranges: %d\n", config->speedRangeCount);

    // Print each speed range and its SPWM configuration
    for (int i = 0; i < config->speedRangeCount; i++) {
        const SpeedRange* range = &config->speedRanges[i];
        VESC_IF->printf("\nSpeed Range %d:\n", i + 1);
        VESC_IF->printf("  Min Speed: %f km/h\n", (double)range->minSpeed);
        VESC_IF->printf("  Max Speed: %f km/h\n", (double)range->maxSpeed);

        // Print SPWM configuration
        const SPWMConfig* spwm = &range->spwm;
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
        VESC_IF->printf("  SPWM Type: %s\n", spwmTypeStr);

        if (spwm->type == SPWM_TYPE_FIXED_ASYNC || spwm->type == SPWM_TYPE_RAMP_ASYNC || spwm->type == SPWM_TYPE_RSPWM) {
            VESC_IF->printf("  Carrier Frequency Start: %d Hz\n", spwm->carrierFrequencyStart);
            if (spwm->type == SPWM_TYPE_RAMP_ASYNC || spwm->type == SPWM_TYPE_RSPWM) {
                VESC_IF->printf("  Carrier Frequency End: %d Hz\n", spwm->carrierFrequencyEnd);
            }
        } else if (spwm->type == SPWM_TYPE_SYNC) {
            VESC_IF->printf("  Number of Pulses: %d\n", spwm->numPulses);
        }
    }
}