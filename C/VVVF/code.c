#include "vesc_c_if.h"
#include <stdint.h>
#include <string.h>

// HEADER macro
HEADER

// Constants
#define SINE_TABLE_SIZE 100
#define INT8_SCALE 127
#define TWO_PI 6.28318530718f
#define BUFFER_LENGTH 400
#define SAMPLE_RATE_WARNING_THRESHOLD 1.2f
#define NUM_BUFFERS 3

// Sine lookup table
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
static int8_t *buffers[NUM_BUFFERS];  // Array of pointers to buffers allocated on the heap
static bool buffer_ready_for_consumption[NUM_BUFFERS];  // Flags to indicate if a buffer is ready for consumption
static int producer_index = 0;  // Index of the buffer currently being filled by the producer
static int consumer_index = 0;  // Index of the buffer currently being consumed by the consumer
static float phase = 0.0f;
static float amplitude = 0.0f;
static float sample_rate = 11025.0f;
static float frequency = 1000.0f;

// Statistics
static uint32_t samples_generated = 0;
static uint32_t last_samples_generated = 0;
static uint32_t samples_consumed = 0;
static uint32_t last_samples_comsumed = 0;
static float last_time = 0.0f;

// Thread data structure
typedef struct {
    lib_thread thread;
    bool running;
} thread_data;

static thread_data generator_thread_data;
static thread_data playback_thread_data;

// Custom sine function using the lookup table
static inline int8_t custom_sin(float phase) {
    int index = (int)(phase * SINE_TABLE_SIZE / TWO_PI) % SINE_TABLE_SIZE;
    if (index < 0) index += SINE_TABLE_SIZE;
    return sineLookupTable[index];
}

// Function to generate sine wave samples
static void generate_sine_wave(int8_t *buffer, float frequency, float *phase) {
    float phase_increment = (TWO_PI * frequency) / sample_rate;

    for (int i = 0; i < BUFFER_LENGTH; i++) {
        buffer[i] = (int8_t)(custom_sin(*phase) * amplitude);
        *phase += phase_increment;
        if (*phase >= TWO_PI) {
            *phase -= TWO_PI;
        }
    }
}

// Generator loop function
static void generator_loop(void *arg) {
    (void)arg;

    while (generator_thread_data.running) {
        // Wait until the current buffer is ready to be written to (i.e., not being consumed)
        while (buffer_ready_for_consumption[producer_index] && generator_thread_data.running) {
            VESC_IF->sleep_ms(1);  // Sleep briefly to avoid busy-waiting
        }

        if (!generator_thread_data.running) {
            break;
        }

        // Generate sine wave samples in the current buffer
        generate_sine_wave(buffers[producer_index], frequency, &phase);

        // Mark the buffer as ready for consumption
        buffer_ready_for_consumption[producer_index] = true;

        // Update statistics
        samples_generated += BUFFER_LENGTH;

        // Move to the next buffer in a circular manner
        producer_index = (producer_index + 1) % NUM_BUFFERS;

    }

    VESC_IF->printf("Generator loop thread terminated.\n");
}

// Playback loop function
static void playback_loop(void *arg) {
    (void)arg;

    while (playback_thread_data.running) {
        // Wait until the current buffer is ready for consumption
        while (!buffer_ready_for_consumption[consumer_index] && playback_thread_data.running) {
            VESC_IF->sleep_ms(1);  // Sleep briefly to avoid busy-waiting

			// If we're not just booting up, the buffer should be full. If it isn't log errors.
			if (VESC_IF->system_time() > 1) {
    			VESC_IF->printf("[ERROR] Playback Thread Starved For Sample Buffers!\n");
			}
        }

        if (!playback_thread_data.running) {
            break;
        }

        // Play the samples from the current buffer
		// The documentation says this blocks once two buffers have been produced, but it does not appear to do that.
        VESC_IF->foc_play_audio_samples(buffers[consumer_index], BUFFER_LENGTH, sample_rate, amplitude);
        samples_consumed += BUFFER_LENGTH;

		// TEMPORARY FIX!!!!!
        // -- THIS SHOULD NOT BE NEEDED -- THE 
        float sleep_time = (float)BUFFER_LENGTH / sample_rate * 1000.0f * 1000.0f;
        VESC_IF->sleep_us((uint32_t)sleep_time);

        // Mark the buffer as consumed
        buffer_ready_for_consumption[consumer_index] = false;

        // Move to the next buffer in a circular manner
        consumer_index = (consumer_index + 1) % NUM_BUFFERS;
    }

    VESC_IF->printf("Playback loop thread terminated.\n");
}

// Function to print statistics
static void print_stats(void) {
    float current_time = VESC_IF->system_time();
    if (current_time - last_time >= 1.0f) {
        uint32_t samples_per_second = samples_generated - last_samples_generated;
		uint32_t samples_consumed_per_second = samples_consumed - last_samples_comsumed;
        last_samples_generated = samples_generated;
		last_samples_comsumed = samples_consumed;
        last_time = current_time;

        float actual_sample_rate = (float)samples_per_second;
		float sample_consume_rate = (float)samples_consumed_per_second;

        if (actual_sample_rate > sample_rate * SAMPLE_RATE_WARNING_THRESHOLD) {
            VESC_IF->printf("WARNING: Sample rate too high! Actual: %.1f, Expected: %.1f\n",
                            actual_sample_rate, sample_rate);
        }

        VESC_IF->printf("(Generated samples/s: %.1f) (Consumed samples/s: %.1f)\n",
                        actual_sample_rate, sample_consume_rate);
    }
}

// Extension function to start the audio loop
static lbm_value ext_start_audio_loop(lbm_value *args, lbm_uint argn) {
    (void)args;
    (void)argn;

    if (!generator_thread_data.running && !playback_thread_data.running) {
        // Allocate buffers on the heap
        for (int i = 0; i < NUM_BUFFERS; i++) {
            buffers[i] = (int8_t *)VESC_IF->malloc(BUFFER_LENGTH * sizeof(int8_t));
            if (buffers[i] == NULL) {
                VESC_IF->printf("Failed to allocate buffer %d\n", i);
                return VESC_IF->lbm_enc_sym_eerror;
            }
            buffer_ready_for_consumption[i] = false;  // Initialize all buffers as not ready for consumption
        }

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

    if (generator_thread_data.running && playback_thread_data.running) {
        generator_thread_data.running = false;
        playback_thread_data.running = false;

        VESC_IF->request_terminate(generator_thread_data.thread);
        VESC_IF->request_terminate(playback_thread_data.thread);

        // Free the allocated buffers
        for (int i = 0; i < NUM_BUFFERS; i++) {
            if (buffers[i] != NULL) {
                VESC_IF->free(buffers[i]);
                buffers[i] = NULL;
            }
        }

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

    if (generator_thread_data.running && playback_thread_data.running) {
        generator_thread_data.running = false;
        playback_thread_data.running = false;

        VESC_IF->request_terminate(generator_thread_data.thread);
        VESC_IF->request_terminate(playback_thread_data.thread);

        // Free the allocated buffers
        for (int i = 0; i < NUM_BUFFERS; i++) {
            if (buffers[i] != NULL) {
                VESC_IF->free(buffers[i]);
                buffers[i] = NULL;
            }
        }

        VESC_IF->printf("Generator and playback threads terminated in stop function.\n");
    }
}

INIT_FUN(lib_info *info) {
    INIT_START

    generator_thread_data.running = false;
    playback_thread_data.running = false;

    VESC_IF->lbm_add_extension("ext-start-audio-loop", ext_start_audio_loop);
    VESC_IF->lbm_add_extension("ext-stop-audio-loop", ext_stop_audio_loop);
    VESC_IF->lbm_add_extension("ext-set-amplitude", ext_set_amplitude);
    VESC_IF->lbm_add_extension("ext-set-frequency", ext_set_frequency);

    info->stop_fun = stop;
    info->arg = NULL;

    VESC_IF->printf("Audio module initialized.\n");

    return true;
}