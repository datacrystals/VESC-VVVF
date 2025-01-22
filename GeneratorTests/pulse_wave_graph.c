#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define TWO_PI 6.28318530718f
#define PULSE_MAX 127

// Function pointer type for waveform generators
typedef int8_t (*WaveformGenerator)(float phase, float dutyCycle);

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

int main() {
    float frequency = 440.0f; // Frequency in Hz
    float dutyCycle = 0.03f;  // 3% duty cycle
    float phase = 0.0f;
    float phaseIncrement = TWO_PI * frequency / SAMPLE_RATE;
    int numSamples = SAMPLE_RATE / 10; // Generate 0.1 seconds of data

    // List of waveform generators to test
    WaveformGenerator generators[] = {
        GeneratePulse,
        GenerateSawtooth,
        GenerateSine,
        GenerateSquare,
        GenerateTriangle
    };
    const char *waveformNames[] = {
        "Pulse",
        "Sawtooth",
        "Sine",
        "Square",
        "Triangle"
    };
    int numWaveforms = sizeof(generators) / sizeof(generators[0]);

    // Open a file to write the JSON data
    FILE *file = fopen("waveforms.json", "w");
    if (!file) {
        fprintf(stderr, "Error opening file for writing.\n");
        return 1;
    }

    // Write the JSON header
    fprintf(file, "{\n");
    fprintf(file, "  \"sample_rate\": %d,\n", SAMPLE_RATE);
    fprintf(file, "  \"frequency\": %.1f,\n", frequency);
    fprintf(file, "  \"duty_cycle\": %.2f,\n", dutyCycle);
    fprintf(file, "  \"waveforms\": [\n");

    // Generate and write samples for each waveform
    for (int w = 0; w < numWaveforms; w++) {
        fprintf(file, "    {\n");
        fprintf(file, "      \"name\": \"%s\",\n", waveformNames[w]);
        fprintf(file, "      \"data\": [\n");

        phase = 0.0f; // Reset phase for each waveform
        for (int i = 0; i < numSamples; i++) {
            int8_t sample = generators[w](phase, dutyCycle);
            float time = (float)i / SAMPLE_RATE;
            fprintf(file, "        { \"time\": %.6f, \"amplitude\": %d }%s\n",
                    time, sample, (i == numSamples - 1) ? "" : ",");

            // Increment phase
            phase += phaseIncrement;
            if (phase >= TWO_PI) {
                phase -= TWO_PI;
            }
        }

        fprintf(file, "      ]\n");
        fprintf(file, "    }%s\n", (w == numWaveforms - 1) ? "" : ",");
    }

    // Write the JSON footer
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");

    // Close the file
    fclose(file);
    printf("Waveform data written to waveforms.json\n");

    return 0;
}