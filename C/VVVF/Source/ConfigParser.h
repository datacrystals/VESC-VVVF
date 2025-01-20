#ifndef INVERTER_CONFIG_H
#define INVERTER_CONFIG_H

#include <stdint.h>
#include "Parameters.h"

// Define an enum for SPWM types
typedef enum {
    SPWM_TYPE_NONE,          // Output disabled
    SPWM_TYPE_FIXED_ASYNC,   // Fixed frequency asynchronous SPWM
    SPWM_TYPE_RAMP_ASYNC,    // Ramp frequency asynchronous SPWM
    SPWM_TYPE_RSPWM,         // Resonant SPWM
    SPWM_TYPE_SYNC           // Synchronous SPWM
} SPWMType;

// Define a struct for the SPWM configuration
typedef struct {
    SPWMType type;           // Type of SPWM (fixed, ramp, RSPWM, sync)
    int carrierFrequencyStart; // Starting carrier frequency for a ramp, if not a ramp, we default to this
    int carrierFrequencyEnd; // Ending ramp value for carrier frequency, unused if not a ramp
    int numPulses;           // Number of pulses for synchronous SPWM (only used for SPWM_TYPE_SYNC)
} SPWMConfig;

// Define a struct for each speed range
typedef struct {
    float minSpeed;            // Minimum speed in km/h
    float maxSpeed;            // Maximum speed in km/h
    SPWMConfig spwm;         // SPWM configuration for this speed range
} SpeedRange;

// Define the main configuration struct
typedef struct {
    float rpmToSpeedRatio;   // Used to convert from the motor's rpm to the speed in km/h
    float maxSpeed;            // Maximum speed - any value above this will be capped to the max speed in km/h
    float zeroSpeedCutoffMargin; // How close it should be to 0 kmh before cutting off
    SpeedRange speedRanges[MAX_SPEED_RANGES]; // Array of speed ranges
    int speedRangeCount;     // Number of valid speed ranges
} InverterConfig;

// Function to parse JSON and populate the InverterConfig struct
void InitializeConfiguration(InverterConfig* _Config);
SpeedRange GetSpeedRangeAtRPM(InverterConfig* _Source, float _MotorRPM, float _MotorCurrent);
void PrintInverterConfig(const InverterConfig* config);

// NOTE: THE SPEED RANGE VALUES *MUST* BE IN ASCENDING ORDER (Low index = closer to 0 speed, higher = higher speed)
void AddSPWM_AsyncFixed(InverterConfig* _Source, int _Index, float _MinSpeed, float _MaxSpeed, int _CarrierFreq);
void AddSPWM_AsyncRamp(InverterConfig* _Source, int _Index, float _MinSpeed, float _MaxSpeed, int _CarrierFreqStart, int _CarrierFreqEnd);
void AddSPWM_RSPWM(InverterConfig* _Source, int _Index, float _MinSpeed, float _MaxSpeed, int _CarrierMin, int _CarrierMax);
void AddSPWM_Sync(InverterConfig* _Source, int _Index, float _MinSpeed, float _MaxSpeed, int _NumPulses);

#endif // INVERTER_CONFIG_H