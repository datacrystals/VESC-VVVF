#ifndef SPWM_GENERATOR_H
#define SPWM_GENERATOR_H

#include <stdint.h>
#include "ConfigParser.h" // For SpeedRange and other dependencies
#include "Parameters.h"

// Constants
#define SINE_TABLE_SIZE 100
#define INT8_SCALE 127
#define TWO_PI 6.28318530718f
#define SAWTOOTH_MAX 127 // Maximum value for the sawtooth (int8 range: 0-127)

// Sine lookup table
extern const int8_t SineLookupTable[SINE_TABLE_SIZE];

// SPWM Generator Struct
typedef struct {
    float CarrierPhase;       // Phase of the carrier waveform
    float CommandPhase;       // Phase of the command waveform
    float CarrierFrequency;   // Current carrier frequency
    float CommandFrequency;   // Current command frequency
    float ModulationIndex;    // Modulation index for SPWM
    float Amplitude;          // Output amplitude scaling
} SPWMGenerator;

// Function Prototypes
void SPWMGenerator_Init(SPWMGenerator* generator);
int SPWMGenerator_GenerateSamples(SPWMGenerator* generator, int8_t* buffer, int bufferLength, const SpeedRange* speedRange, float CommandHZ, int NumPoles, float HzToKmhFactor);
float SPWMGenerator_MapValue(float value, float inMin, float inMax, float outMin, float outMax);
int8_t SPWMGenerator_GenerateSin(float phase);
int8_t SPWMGenerator_GenerateSawtooth(float phase);

#endif // SPWM_GENERATOR_H