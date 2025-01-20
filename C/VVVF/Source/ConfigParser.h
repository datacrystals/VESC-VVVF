#ifndef INVERTER_CONFIG_H
#define INVERTER_CONFIG_H

#include <stdint.h>
#include "Parameters.h"

typedef enum {
    SPWM_TYPE_NONE,          // Output disabled
    SPWM_TYPE_FIXED_ASYNC,   // Fixed frequency asynchronous SPWM
    SPWM_TYPE_RAMP_ASYNC,    // Ramp frequency asynchronous SPWM
    SPWM_TYPE_RSPWM,         // Random SPWM
    SPWM_TYPE_SYNC           // Synchronous SPWM
} SPWMType;

typedef struct {
    SPWMType type;           // Type of SPWM (fixed, ramp, RSPWM, sync)
    int carrierFrequencyStart; // Starting carrier frequency for a ramp, if not a ramp, we default to this
    int carrierFrequencyEnd; // Ending ramp value for carrier frequency, unused if not a ramp
    int numPulses;           // Number of pulses for synchronous SPWM (only used for SPWM_TYPE_SYNC)
} SPWMConfig;

typedef struct {
    SPWMConfig acceleration; // SPWM configuration for acceleration
    SPWMConfig coasting;     // SPWM configuration for coasting
    SPWMConfig deceleration; // SPWM configuration for deceleration
} SPWMBehaviorConfig;

typedef struct {
    float minSpeed;            // Minimum speed in km/h
    float maxSpeed;            // Maximum speed in km/h
    SPWMBehaviorConfig spwm;   // SPWM configuration for this speed range
} SpeedRange;

// Define the main configuration struct
typedef struct {
    float rpmToSpeedRatio;   // Used to convert from the motor's rpm to the speed in km/h
    float maxSpeed;            // Maximum speed - any value above this will be capped to the max speed in km/h
    float zeroSpeedCutoffMargin; // How close it should be to 0 kmh before cutting off
    SpeedRange speedRanges[MAX_SPEED_RANGES]; // Array of speed ranges
    int speedRangeCount;     // Number of valid speed ranges
} InverterConfig;

// Add a new enum to track the state of the rotor
typedef enum {
    ROTOR_STATE_ACCELERATING,
    ROTOR_STATE_COASTING,
    ROTOR_STATE_DECELERATING
} RotorState;


// Function to parse JSON and populate the InverterConfig struct
void InitializeConfiguration(InverterConfig* _Config);
SpeedRange GetSpeedRangeAtRPM(InverterConfig* _Source, float _MotorRPM, float _MotorCurrent);
void PrintInverterConfig(const InverterConfig* config);
void PrintSPWMConfig(const SPWMConfig* spwm);

// NOTE: THE SPEED RANGE VALUES *MUST* BE IN ASCENDING ORDER (Low index = closer to 0 speed, higher = higher speed)
SPWMConfig AddSPWM_AsyncFixed(int _Carrier);
SPWMConfig AddSPWM_AsyncRamp(int _CarrierFreqStart, int _CarrierFreqEnd);
SPWMConfig AddSPWM_RSPWM(int _CarrierMin, int _CarrierMax);
SPWMConfig AddSPWM_Sync(int _NumPulses);
SPWMConfig AddSPWM_Disabled();

void SetSPWM_Acceleration(InverterConfig* _Source, int _Index, SPWMConfig config);
void SetSPWM_Coasting(InverterConfig* _Source, int _Index, SPWMConfig config);
void SetSPWM_Deceleration(InverterConfig* _Source, int _Index, SPWMConfig config);
void SetSpeedRangeSpeed(InverterConfig* _Source, int _Index, float minSpeed, float maxSpeed);


#endif // INVERTER_CONFIG_H