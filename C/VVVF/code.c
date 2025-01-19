#include "vesc_c_if.h"
#include <stdint.h>  // For fixed-width integer types
#include <string.h>  // For memcpy

// HEADER macro
HEADER

// Constants
#define SINE_TABLE_SIZE 100  // Size of the sine lookup table
#define INT8_SCALE 127       // Scaling factor for int8_t (sine table is scaled to -127 to 127)
#define TWO_PI 6.28318530718f  // 2 * PI for phase wrapping
#define BUFFER_LENGTH 400     // Define buffer length as a macro
#define SAMPLE_RATE_WARNING_THRESHOLD 1.2f  // 120% of the sample rate

// Sine lookup table (100 elements, int8_t, scaled from -127 to 127)
static const int8_t sineLookupTable[SINE_TABLE_SIZE] = {
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

// Global variables
static int8_t buffer1[BUFFER_LENGTH];  // Buffer 1 for audio samples
static int8_t buffer2[BUFFER_LENGTH];  // Buffer 2 for audio samples
static bool use_buffer1 = true;        // Flag to alternate between buffers
static bool buffer_ready = false;      // Flag to indicate if a buffer is ready for playback
static float phase = 0.0f;             // Phase for the sine wave
static float amplitude = 0.0f;         // Amplitude of the sine wave (float for precision)
static float sample_rate = 11025.0f;   // Sample rate (Hz)
static float frequency = 1000.0f;      // Frequency of the sine wave (1 kHz)

// Statistics
static uint32_t samples_generated = 0;  // Total samples generated
static uint32_t last_samples_generated = 0;  // Samples generated in the last interval
static float last_time = 0.0f;  // Last time statistics were printed (in seconds)

// Thread data structure
typedef struct {
    lib_thread thread;  // Thread handle
    bool running;       // Flag to indicate if the thread is running
} thread_data;

static thread_data generator_thread_data;  // Thread data for the generator loop
static thread_data playback_thread_data;   // Thread data for the playback loop

// Custom sine function using the lookup table
static inline int8_t custom_sin(float phase) {
    // Scale the phase to the lookup table size (100 elements)
    int index = (int)(phase * SINE_TABLE_SIZE / TWO_PI) % SINE_TABLE_SIZE;
    if (index < 0) index += SINE_TABLE_SIZE;  // Ensure positive index
    return sineLookupTable[index];  // Return the sine value directly (scaled to int8_t)
}

// Function to generate sine wave samples
static void generate_sine_wave(int8_t *buffer, float frequency, float *phase) {
    // Calculate phase increment
    float phase_increment = (TWO_PI * frequency) / sample_rate;

    for (int i = 0; i < BUFFER_LENGTH; i++) {
        // Generate sine wave sample
        buffer[i] = (int8_t)(custom_sin(*phase) * amplitude);

        // Increment phase
        *phase += phase_increment;

        // Wrap phase to avoid overflow
        if (*phase >= TWO_PI) {
            *phase -= TWO_PI;
        }
    }
}

// Generator loop function (runs in a separate thread)
static void generator_loop(void *arg) {
    (void)arg;

    while (generator_thread_data.running) {
        // Wait until the buffer is consumed
        while (buffer_ready) {
            VESC_IF->sleep_ms(1);  // Sleep briefly to avoid busy-waiting
        }

        // Generate sine wave samples in the active buffer
        if (use_buffer1) {
            generate_sine_wave(buffer1, frequency, &phase);
        } else {
            generate_sine_wave(buffer2, frequency, &phase);
        }

        // Signal that the buffer is ready for playback
        buffer_ready = true;

        // Update statistics
        samples_generated += BUFFER_LENGTH;

        // Switch to the other buffer
        use_buffer1 = !use_buffer1;

        // Sleep to throttle the generator to the expected sample rate
        float sleep_time = (float)BUFFER_LENGTH / sample_rate * 1000.0f;  // Convert to milliseconds
        VESC_IF->sleep_ms((uint32_t)sleep_time);
    }

    VESC_IF->printf("Generator loop thread terminated.\n");
}

// Playback loop function (runs in a separate thread)
static void playback_loop(void *arg) {
    (void)arg;

    while (playback_thread_data.running) {
        // Wait until a buffer is ready for playback
        while (!buffer_ready) {
            VESC_IF->sleep_ms(1);  // Sleep briefly to avoid busy-waiting
    		VESC_IF->printf("Playback thread starved!\n");

        }

        // Play the samples from the active buffer
        if (use_buffer1) {
            VESC_IF->foc_play_audio_samples(buffer2, BUFFER_LENGTH, sample_rate, amplitude);
        } else {
            VESC_IF->foc_play_audio_samples(buffer1, BUFFER_LENGTH, sample_rate, amplitude);
        }

        // Signal that the buffer has been consumed
        buffer_ready = false;
    }

    VESC_IF->printf("Playback loop thread terminated.\n");
}

// Function to print statistics
static void print_stats(void) {
    float current_time = VESC_IF->system_time() / 1000.0f;  // Convert to seconds
    if (current_time - last_time >= 1.0f) {  // Print stats every second
        uint32_t samples_per_second = samples_generated - last_samples_generated;
        last_samples_generated = samples_generated;
        last_time = current_time;

        // Calculate the actual sample rate
        float actual_sample_rate = (float)samples_per_second;

        // Check if the actual sample rate exceeds the warning threshold
        if (actual_sample_rate > sample_rate * SAMPLE_RATE_WARNING_THRESHOLD) {
            VESC_IF->printf("WARNING: Sample rate too high! Actual: %.1f, Expected: %.1f\n",
                            actual_sample_rate, sample_rate);
        }

        VESC_IF->printf("Samples generated: %u (samples/s: %.1f)\n",
                        samples_generated, actual_sample_rate);
    }
}

// Extension function to start the audio loop
static lbm_value ext_start_audio_loop(lbm_value *args, lbm_uint argn) {
    (void)args;
    (void)argn;

    // Start the generator and playback threads if they're not already running
    if (!generator_thread_data.running && !playback_thread_data.running) {
        generator_thread_data.running = true;
        playback_thread_data.running = true;

        generator_thread_data.thread = VESC_IF->spawn(generator_loop, 1024, "generator_loop", NULL);
        playback_thread_data.thread = VESC_IF->spawn(playback_loop, 1024, "playback_loop", NULL);

        VESC_IF->printf("Generator and playback threads started.\n");
    } else {
        VESC_IF->printf("Generator and playback threads are already running.\n");
    }

    return VESC_IF->lbm_enc_sym_true;
}

// Extension function to stop the audio loop
static lbm_value ext_stop_audio_loop(lbm_value *args, lbm_uint argn) {
    (void)args;
    (void)argn;

    // Stop the generator and playback threads if they're running
    if (generator_thread_data.running && playback_thread_data.running) {
        generator_thread_data.running = false;
        playback_thread_data.running = false;

        VESC_IF->request_terminate(generator_thread_data.thread);
        VESC_IF->request_terminate(playback_thread_data.thread);

        VESC_IF->printf("Generator and playback threads stopped.\n");
    } else {
        VESC_IF->printf("Generator and playback threads are not running.\n");
    }

    return VESC_IF->lbm_enc_sym_true;
}

// Extension function to set amplitude
static lbm_value ext_set_amplitude(lbm_value *args, lbm_uint argn) {
    if (argn != 1 || !VESC_IF->lbm_is_number(args[0])) {
        return VESC_IF->lbm_enc_sym_eerror;
    }

    amplitude = VESC_IF->lbm_dec_as_float(args[0]);
	print_stats();
    return VESC_IF->lbm_enc_sym_true;
}

// Extension function to set frequency
static lbm_value ext_set_frequency(lbm_value *args, lbm_uint argn) {
    if (argn != 1 || !VESC_IF->lbm_is_number(args[0])) {
        return VESC_IF->lbm_enc_sym_eerror;
    }

    frequency = VESC_IF->lbm_dec_as_float(args[0]);
    VESC_IF->printf("Frequency set to: %f\n", frequency);
    return VESC_IF->lbm_enc_sym_true;
}

// Called when the code is stopped
static void stop(void *arg) {
    (void)arg;

    // Stop the generator and playback threads if they're running
    if (generator_thread_data.running && playback_thread_data.running) {
        generator_thread_data.running = false;
        playback_thread_data.running = false;

        VESC_IF->request_terminate(generator_thread_data.thread);
        VESC_IF->request_terminate(playback_thread_data.thread);

        VESC_IF->printf("Generator and playback threads terminated in stop function.\n");
    }
}

INIT_FUN(lib_info *info) {
    INIT_START

    // Initialize thread data
    generator_thread_data.running = false;
    playback_thread_data.running = false;

    // Register extension functions
    VESC_IF->lbm_add_extension("ext-start-audio-loop", ext_start_audio_loop);
    VESC_IF->lbm_add_extension("ext-stop-audio-loop", ext_stop_audio_loop);
    VESC_IF->lbm_add_extension("ext-set-amplitude", ext_set_amplitude);
    VESC_IF->lbm_add_extension("ext-set-frequency", ext_set_frequency);

    // Set the stop function
    info->stop_fun = stop;
    info->arg = NULL;

    VESC_IF->printf("Audio module initialized.\n");

    return true;
}