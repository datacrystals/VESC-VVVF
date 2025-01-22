#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define TWO_PI 6.28318530718f
#define PULSE_MAX 127

// Generate a pulse wave sample
int8_t SPWMGenerator_GeneratePulse(float phase, float dutyCycle) {
    if (dutyCycle <= 0.0f || dutyCycle >= 1.0f) {
        dutyCycle = 0.03f; // Default to 3% duty cycle if invalid
    }

    float pulseWidth = TWO_PI * dutyCycle;

    if (phase < pulseWidth / 2) {
        return PULSE_MAX; // Rising edge to 127
    } else if (phase < pulseWidth) {
        return -PULSE_MAX; // Falling edge to -127
    } else {
        return 0; // Zero for the rest of the period
    }
}

int main() {
    float frequency = 440.0f; // Frequency in Hz
    float dutyCycle = 0.03f;  // 3% duty cycle
    float phase = 0.0f;
    float phaseIncrement = TWO_PI * frequency / SAMPLE_RATE;
    int numSamples = SAMPLE_RATE / 10; // Generate 0.1 seconds of data

    // Open a file to write the samples
    FILE *file = fopen("pulse_wave.csv", "w");
    if (!file) {
        fprintf(stderr, "Error opening file for writing.\n");
        return 1;
    }

    // Write the header
    fprintf(file, "Time (s),Amplitude\n");

    // Generate and write samples
    for (int i = 0; i < numSamples; i++) {
        int8_t sample = SPWMGenerator_GeneratePulse(phase, dutyCycle);
        float time = (float)i / SAMPLE_RATE;
        fprintf(file, "%.6f,%d\n", time, sample);

        // Increment phase
        phase += phaseIncrement;
        if (phase >= TWO_PI) {
            phase -= TWO_PI;
        }
    }

    // Close the file
    fclose(file);
    printf("Pulse wave data written to pulse_wave.csv\n");

    return 0;
}