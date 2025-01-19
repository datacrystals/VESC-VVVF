#include "vesc_c_if.h"
#include <math.h>

HEADER

// Sine lookup table (100 elements, uint8_t, scaled from -127 to 127)
static const int8_t sineLookupTable[] = {
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
static int8_t buffer1[400];  // Buffer 1 for audio samples
static int8_t buffer2[400];  // Buffer 2 for audio samples
static bool use_buffer1 = true;  // Flag to alternate between buffers
static float command_phase = 0.0f;  // Phase for command signal
static float amplitude = 0.05f;  // Amplitude of the sine wave
static float sample_rate = 11025.0f;  // Sample rate (Hz)
static int chunk_size = 400;  // Chunk size (number of samples per buffer)
static float command_frequency = 1000.0f;  // Command frequency (Hz)

// Thread data structure
typedef struct {
    lib_thread thread;  // Thread handle
    bool running;       // Flag to indicate if the thread is running
} thread_data;

static thread_data audio_thread_data;  // Thread data for the audio loop

// Function to generate sine wave samples
static void generate_sine_wave(int8_t *buffer, float frequency, float *phase) {
    // Phase increment (fixed-point arithmetic)
    uint16_t phase_increment = (uint16_t)((frequency / sample_rate) * 100);  // 100 = size of sine table
    uint16_t phase_accumulator = (uint16_t)(*phase * 100);  // Initial phase

    for (int i = 0; i < chunk_size; i++) {
        // Fetch sine value from lookup table
        uint8_t phase_index = phase_accumulator % 100;  // Wrap around after 100 elements
        int8_t sine_value = sineLookupTable[phase_index];

        // Store the sample in the buffer (no amplitude scaling here)
        buffer[i] = sine_value;

        // Increment phase accumulator and wrap around
        phase_accumulator += phase_increment;
        if (phase_accumulator >= 100) {
            phase_accumulator -= 100;  // Wrap around after completing one period
        }
    }

    // Update the phase for the next chunk
    *phase = (float)(phase_accumulator) / 100.0f;
}

// Main loop function (runs in a separate thread)
static void audio_loop(void *arg) {
    (void)arg;

    while (audio_thread_data.running) {
        // Generate sine wave samples in the active buffer
        if (use_buffer1) {
            generate_sine_wave(buffer1, command_frequency, &command_phase);
        } else {
            generate_sine_wave(buffer2, command_frequency, &command_phase);
        }

        // Play the samples from the active buffer
        if (use_buffer1) {
            //VESC_IF->printf("Playing buffer1 with amplitude: %f\n", amplitude);
            VESC_IF->foc_play_audio_samples(buffer1, chunk_size, sample_rate, amplitude);
        } else {
            //VESC_IF->printf("Playing buffer2 with amplitude: %f\n", amplitude);
            VESC_IF->foc_play_audio_samples(buffer2, chunk_size, sample_rate, amplitude);
        }

        // Alternate buffers
        use_buffer1 = !use_buffer1;

        // Sleep for a short time to avoid overloading the system
        VESC_IF->sleep_ms(10);
    }

    VESC_IF->printf("Audio loop thread terminated.\n");
}

// Extension function to start the audio loop
static lbm_value ext_start_audio_loop(lbm_value *args, lbm_uint argn) {
    (void)args;
    (void)argn;

    // Start the audio loop thread if it's not already running
    if (!audio_thread_data.running) {
        audio_thread_data.running = true;
        audio_thread_data.thread = VESC_IF->spawn(audio_loop, 1024, "audio_loop", NULL);
        VESC_IF->printf("Audio loop thread started.\n");
    } else {
        VESC_IF->printf("Audio loop thread is already running.\n");
    }

    return VESC_IF->lbm_enc_sym_true;
}

// Extension function to stop the audio loop
static lbm_value ext_stop_audio_loop(lbm_value *args, lbm_uint argn) {
    (void)args;
    (void)argn;

    // Stop the audio loop thread if it's running
    if (audio_thread_data.running) {
        audio_thread_data.running = false;
        VESC_IF->request_terminate(audio_thread_data.thread);
        VESC_IF->printf("Audio loop thread stopped.\n");
    } else {
        VESC_IF->printf("Audio loop thread is not running.\n");
    }

    return VESC_IF->lbm_enc_sym_true;
}

// Extension function to set amplitude
static lbm_value ext_set_amplitude(lbm_value *args, lbm_uint argn) {
    if (argn != 1 || !VESC_IF->lbm_is_number(args[0])) {
        return VESC_IF->lbm_enc_sym_eerror;
    }

    amplitude = VESC_IF->lbm_dec_as_float(args[0]);
    VESC_IF->printf("Amplitude set to: %f\n", amplitude);
    return VESC_IF->lbm_enc_sym_true;
}

// Extension function to set command frequency
static lbm_value ext_set_command_frequency(lbm_value *args, lbm_uint argn) {
    if (argn != 1 || !VESC_IF->lbm_is_number(args[0])) {
        return VESC_IF->lbm_enc_sym_eerror;
    }

    command_frequency = VESC_IF->lbm_dec_as_float(args[0]);
    VESC_IF->printf("Command frequency set to: %f\n", command_frequency);
    return VESC_IF->lbm_enc_sym_true;
}

// Called when the code is stopped
static void stop(void *arg) {
    (void)arg;

    // Stop the audio loop thread if it's running
    if (audio_thread_data.running) {
        audio_thread_data.running = false;
        VESC_IF->request_terminate(audio_thread_data.thread);
        VESC_IF->printf("Audio loop thread terminated in stop function.\n");
    }
}

INIT_FUN(lib_info *info) {
    INIT_START

    // Initialize thread data
    audio_thread_data.running = false;

    // Register extension functions
    VESC_IF->lbm_add_extension("ext-start-audio-loop", ext_start_audio_loop);
    VESC_IF->lbm_add_extension("ext-stop-audio-loop", ext_stop_audio_loop);
    VESC_IF->lbm_add_extension("ext-set-amplitude", ext_set_amplitude);
    VESC_IF->lbm_add_extension("ext-set-command-frequency", ext_set_command_frequency);

    // Set the stop function
    info->stop_fun = stop;
    info->arg = NULL;

    VESC_IF->printf("Audio module initialized.\n");

    return true;
}