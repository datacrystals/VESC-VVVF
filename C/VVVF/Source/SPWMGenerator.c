#include "SPWMGenerator.h"
#include "vesc_c_if.h"
#include <math.h>


// Sine lookup table
const int8_t SineLookupTable[SINE_TABLE_SIZE] = {
    64, 76, 88, 100, 111, 123, 134, 145, 156, 166,
    176, 186, 195, 203, 211, 219, 225, 231, 237, 242,
    246, 249, 252, 253, 255, 255, 255, 253, 252, 249,
    246, 242, 237, 231, 225, 219, 211, 203, 195, 186,
    176, 166, 156, 145, 134, 123, 111, 100, 88, 76,
    64, 52, 40, 28, 17, 5, -6, -17, -28, -38,
    -48, -58, -67, -75, -83, -91, -97, -103, -109, -114,
    -118, -121, -124, -125, -127, -127, -127, -125, -124, -121,
    -118, -114, -109, -103, -97, -91, -83, -75, -67, -58,
    -48, -38, -28, -17, -6, 5, 17, 28, 40, 52
};

// -- Helpers for RSPWM 
// Larger lookup table with 256 precomputed random values
static const uint16_t random_lookup_table[256] = {
    42, 17, 89, 63, 31, 55, 10, 72, 94, 28, 5, 36, 81, 22, 68, 14,
    77, 49, 90, 33, 8, 45, 99, 12, 60, 25, 70, 40, 83, 19, 3, 50,
    88, 7, 65, 29, 74, 52, 96, 21, 1, 38, 79, 16, 57, 34, 85, 47,
    92, 26, 9, 44, 98, 13, 61, 24, 71, 39, 84, 18, 4, 51, 87, 6,
    66, 30, 75, 53, 95, 20, 2, 37, 80, 15, 58, 35, 86, 48, 91, 27,
    11, 46, 97, 23, 62, 32, 76, 54, 93, 41, 0, 43, 78, 17, 59, 36,
    82, 49, 89, 22, 123, 234, 189, 145, 67, 201, 132, 98, 177, 210,
    55, 120, 33, 200, 88, 150, 44, 99, 111, 66, 77, 22, 199, 180,
    30, 155, 43, 166, 92, 188, 10, 144, 78, 199, 65, 122, 87, 33,
    210, 99, 45, 177, 66, 88, 150, 44, 123, 234, 189, 145, 67, 201,
    132, 98, 177, 210, 55, 120, 33, 200, 88, 150, 44, 99, 111, 66,
    77, 22, 199, 180, 30, 155, 43, 166, 92, 188, 10, 144, 78, 199,
    65, 122, 87, 33, 210, 99, 45, 177, 66, 88, 150, 44, 123, 234,
    189, 145, 67, 201, 132, 98, 177, 210, 55, 120, 33, 200, 88, 150,
    44, 99, 111, 66, 77, 22, 199, 180, 30, 155, 43, 166, 92, 188, 10,
    144, 78, 199, 65, 122, 87, 33, 210, 99, 45, 177, 66, 88, 150, 44
};

// Index to track the current position in the lookup table
static uint16_t lookup_index = 0;

// LFSR state for additional randomness
static uint16_t lfsr = 0xACE1u; // Example seed

// Get the next random number from the lookup table and LFSR
uint16_t get_enhanced_random() {
    // Get a value from the lookup table
    uint16_t lookup_value = random_lookup_table[lookup_index];
    lookup_index = (lookup_index + 1) % 256; // Wrap around after 256 values

    // Update the LFSR for additional randomness
    uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    lfsr = (lfsr >> 1) | (bit << 15);

    // Combine the lookup value and LFSR output
    return (lookup_value ^ lfsr); // XOR for mixing
}

// Generate a random number between min and max (inclusive)
int random_range(int min, int max) {
    uint16_t rand_val = get_enhanced_random();
    int range = max - min + 1;
    return min + (rand_val % range);
}

// Initialize the SPWM generator
void SPWMGenerator_Init(SPWMGenerator* generator) {
    generator->CarrierPhase = 0.0f;
    generator->CommandPhase = 0.0f;
    generator->CarrierFrequency = 1000.0f; // Default carrier frequency
    generator->CommandFrequency = 1.0f;    // Default command frequency
    generator->ModulationIndex = 1.0f;     // Default modulation index
    generator->Amplitude = 0.0f;           // Default amplitude
}

// Generate SPWM samples
void SPWMGenerator_GenerateSamples(SPWMGenerator* generator, int8_t* buffer, int bufferLength, const SpeedRange* speedRange, float CommandHZ, int NumPoles, float HzToKmhFactor) {
    if (!generator || !buffer || !speedRange) return;

    // Firstly check if disabled, if so, set audio to none
    if (speedRange->spwm.type == SPWM_TYPE_NONE) {
        for (int i = 0; i < bufferLength; i++) {
            buffer[i] = 0;
        }
        return;
    }

    // Convert command frequency to speed
    float speedKmh = (CommandHZ / (float)NumPoles) * HzToKmhFactor;


    // Update carrier frequency based on the active speed range
    if (speedRange->spwm.type == SPWM_TYPE_FIXED_ASYNC) {
        generator->CarrierFrequency = speedRange->spwm.carrierFrequencyStart;
        // VESC_IF->printf("Fixed %.1f\n", generator->CarrierFrequency);
    } else if (speedRange->spwm.type == SPWM_TYPE_RAMP_ASYNC) {
        // Linear interpolation for ramp mode
        float speedRatio = (speedKmh - speedRange->minSpeed) / (speedRange->maxSpeed - speedRange->minSpeed);
        generator->CarrierFrequency = speedRange->spwm.carrierFrequencyStart + (speedRange->spwm.carrierFrequencyEnd - speedRange->spwm.carrierFrequencyStart) * speedRatio;
        // VESC_IF->printf("Ramp %.1f\n", generator->CarrierFrequency);
    } else if (speedRange->spwm.type == SPWM_TYPE_SYNC) {
        generator->CarrierFrequency = (CommandHZ / (float)NumPoles) * speedRange->spwm.numPulses;
        // VESC_IF->printf("Sync %.1f\n", generator->CarrierFrequency);
    } else if (speedRange->spwm.type == SPWM_TYPE_RSPWM) {
        generator->CarrierFrequency = random_range(speedRange->spwm.carrierFrequencyStart, speedRange->spwm.carrierFrequencyEnd);
        // VESC_IF->printf("Random %.1f\n", generator->CarrierFrequency);
    }

    // Generate SPWM samples
    for (int i = 0; i < bufferLength; i++) {

        // Some modes require updating carrier constantly
        if (i % 10 == 0) {
             if (speedRange->spwm.type == SPWM_TYPE_RSPWM) {
                // lookup_index = (lookup_index + i) % 256;
                generator->CarrierFrequency = random_range(speedRange->spwm.carrierFrequencyStart, speedRange->spwm.carrierFrequencyEnd);
            }
        }

        // Update phases
        generator->CommandPhase += (TWO_PI * generator->CommandFrequency) / SAMPLE_RATE;
        generator->CarrierPhase += (TWO_PI * generator->CarrierFrequency) / SAMPLE_RATE;

        // Wrap phases
        if (generator->CommandPhase >= TWO_PI) generator->CommandPhase -= TWO_PI;
        if (generator->CarrierPhase >= TWO_PI) generator->CarrierPhase -= TWO_PI;

        // Generate command and carrier signals
        //int8_t command = SPWMGenerator_GenerateSin(generator->CommandPhase);
        int8_t carrier = SPWMGenerator_GenerateSawtooth(generator->CarrierPhase);

        buffer[i] = carrier;

        // SPWM logic
        // if (command > 0 && command > carrier) {
        //     buffer[i] = INT8_SCALE;
        // } else if (command < 0 && command < -carrier) {
        //     buffer[i] = -INT8_SCALE;
        // } else {
        //     buffer[i] = 0;
        // }
    }
}

// Map a value from one range to another
float SPWMGenerator_MapValue(float value, float inMin, float inMax, float outMin, float outMax) {
    if (inMin == inMax) return outMin; // Avoid division by zero
    return outMin + ((value - inMin) / (inMax - inMin)) * (outMax - outMin);
}

// Generate a sine wave sample using the lookup table
int8_t SPWMGenerator_GenerateSin(float phase) {
    int index = (int)(phase * SINE_TABLE_SIZE / TWO_PI) % SINE_TABLE_SIZE;
    if (index < 0) index += SINE_TABLE_SIZE;
    return SineLookupTable[index];
}

// Generate a sawtooth wave sample
int8_t SPWMGenerator_GenerateSawtooth(float phase) {
    float sawtoothValue = (phase / TWO_PI) * SAWTOOTH_MAX;
    if (sawtoothValue > SAWTOOTH_MAX) sawtoothValue = SAWTOOTH_MAX;
    return (int8_t)sawtoothValue;
}