#include "PulsePattern.h"


#define TWO_PI 6.28318530718f
#define PULSE_MAX 127


// Phase-shifted pulse wave generator
int8_t GeneratePulse(float phase, float dutyCycle) {
    if (dutyCycle <= 0.0f || dutyCycle >= 1.0f) {
        dutyCycle = 0.03f; // Default to 3% duty cycle if invalid
    }

    float pulseWidth = TWO_PI * dutyCycle;

    // Negative pulse at the start of the cycle
    if (phase < pulseWidth) {
        return -PULSE_MAX;
    }
    // Positive pulse halfway through the cycle
    else if (phase >= TWO_PI / 2 && phase < TWO_PI / 2 + pulseWidth) {
        return PULSE_MAX;
    }
    // Zero for the rest of the cycle
    else {
        return 0;
    }
}

// Sawtooth wave generator
int8_t GenerateSawtooth(float phase, float dutyCycle) {
    return (int8_t)((phase / TWO_PI) * 2 * PULSE_MAX - PULSE_MAX);
}

// Sine wave generator
int8_t GenerateSine(float phase, float dutyCycle) {
    return (int8_t)(sinf(phase) * PULSE_MAX);
}

// Square wave generator
int8_t GenerateSquare(float phase, float dutyCycle) {
    return (phase < TWO_PI / 2) ? PULSE_MAX : -PULSE_MAX;
}

// Triangle wave generator
int8_t GenerateTriangle(float phase, float dutyCycle) {
    float value = (phase < TWO_PI / 2) ?
                  (phase / (TWO_PI / 2)) * PULSE_MAX :
                  (1.0f - (phase - TWO_PI / 2) / (TWO_PI / 2)) * PULSE_MAX;
    return (int8_t)(2 * value - PULSE_MAX);
}