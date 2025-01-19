#include "vesc_c_if.h"
#include <stdint.h>
#include <string.h>

// TODO:
// Low Pass filter on amplitude (we don't want *any* sharp changes in amplitude, that makes the motor lose tracking)
// other spwm modes (rspwm, etc)
// change spwm mode based on speed
// Open bug report on bldc firmware for play-samples being nonblocking


// HEADER macro
HEADER

// Constants
#define SINE_TABLE_SIZE 100
#define INT8_SCALE 127
#define TWO_PI 6.28318530718f
#define BUFFER_LENGTH 400
#define SAMPLE_RATE_WARNING_THRESHOLD 1.2f
#define SAWTOOTH_MAX 127        // Maximum value for the sawtooth (int8 range: 0-127)
#define NUM_BUFFERS 3
#define NUM_MOTOR_STAT_SAMPLES 20

// SPWM modes
#define SPWM_MODE_FIXED 0
#define SPWM_MODE_RAMP 1
#define SPWM_MODE_SYNC 2


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
static float amplitude = 0.0f;
static float sample_rate = 25000.0f;

static float current_samples[NUM_MOTOR_STAT_SAMPLES]; // Array of last n current values used to average them
static int active_current_index = 0; // Index of current sample to be replaced
static float hz_samples[NUM_MOTOR_STAT_SAMPLES]; // Array of last n hz values used to average them
static int active_hz_index = 0; // Index of current sample to be replaced

static float inverter_current = 0.; // Number of phase amps pushed into the motor from the vesc
static float inverter_hz = 0.; // Current freqency of the inverter in hz
static int motor_poles = 0; // Number of poles of the motor

// Motor Sound Config
static float min_current = 3.0f; // Amperes - defines linear ramp min current
static float max_current = 10.0f; // Amperes - defines linear ramp max current
static float min_sound_voltage = 0.05; // Volts - defines linear ramp min voltage
static float max_sound_voltage = 0.3; // Volts - defines linear ramp max voltage

// SPWM variables
static float carrier_phase = 0.0f; // Phase of the current carrier sin wave
static float command_phase = 0.0f; // Phase of the current command sin wave
static float carrier_frequency = 2000.0f;  // Example carrier frequency
static float command_frequency = 1.0f;   // Example command frequency
static float modulation_index = 1.0f;       // Modulation index for wide pulse mode
static int spwm_mode = SPWM_MODE_FIXED;     // SPWM mode (fixed, ramp, sync)


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


// Wrapping helper function 
static inline float wrap_phase(float input_phase) {
	if (input_phase >= TWO_PI) {
        input_phase -= TWO_PI;
	}
	return input_phase;
}

// Custom sine function using the lookup table
static inline int8_t generate_sin(float phase) {
    int index = (int)(phase * SINE_TABLE_SIZE / TWO_PI) % SINE_TABLE_SIZE;
    if (index < 0) index += SINE_TABLE_SIZE;
    return sineLookupTable[index];
}

// Function to generate a sawtooth waveform in int8 range (0-127)
static int8_t generate_sawtooth(float phase_offset) {

    // Generate the sawtooth value
    // The sawtooth waveform is a linear ramp from 0 to 127 based on the phase
    float sawtooth_value = (phase_offset / TWO_PI) * SAWTOOTH_MAX;

	if (sawtooth_value > SAWTOOTH_MAX) {
		sawtooth_value = SAWTOOTH_MAX;
	}

    // Cast to int8 and return
    return (int8_t)sawtooth_value;
}


// Function to generate SPWM samples
static void generate_spwm(int8_t *buffer, float command_frequency, float carrier_frequency) {


    for (int i = 0; i < BUFFER_LENGTH; i++) {

		// Update phase offsets of both carrier and command based on frequency and sample rate
		command_phase += (TWO_PI * command_frequency) / sample_rate;
    	carrier_phase += (TWO_PI * carrier_frequency) / sample_rate;

        // Now generate the carrier and command based on the current phase
        int8_t command = generate_sin(command_phase);
        int8_t carrier = generate_sawtooth(carrier_phase);

        // SPWM logic
        int8_t output;
        if (command > 0 && command > carrier) {
            output = 127;
        } else if (command < 0 && command < -carrier) {
            output = -127;
        } else {
            output = 0;
        }

        // Set output value
        buffer[i] = carrier;

        // Wrap the phases at 2*pi
        command_phase = wrap_phase(command_phase);
        carrier_phase = wrap_phase(carrier_phase);
    }
}


// Function to map a value from one range to another
float map_value(float value, float in_min, float in_max, float out_min, float out_max) {
    // Ensure the input range is not zero to avoid division by zero
    if (in_min == in_max) {
        return out_min;  // If input range is zero, return the minimum output value
    }

    // Map the value from the input range to the output range
    return out_min + ((value - in_min) / (in_max - in_min)) * (out_max - out_min);
}

// Code to update the amplitude and other functions from the provided rpm and current)
static void update_spwm_settings() {
	
	// Define amplitude based on current
	amplitude = map_value(inverter_current, min_current, max_current, min_sound_voltage, max_sound_voltage);
	if (amplitude < 0.) {
		amplitude = 0.;
	}
	amplitude += 0.05;

	carrier_frequency = 1000.f;



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

        // Generate SPWM samples in the current buffer
        generate_spwm(buffers[producer_index], command_frequency, carrier_frequency);

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
        VESC_IF->foc_play_audio_samples(buffers[consumer_index], BUFFER_LENGTH, sample_rate, amplitude);
        samples_consumed += BUFFER_LENGTH;

        // TEMPORARY FIX!!!!!
        // -- THIS SHOULD NOT BE NEEDED -- THE PLAY AUDIO SAMPLES CODE SHOULD BLOCK BUT IT DOESNT!
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
                            (double)actual_sample_rate, (double)sample_rate);
        }

        VESC_IF->printf("(Generated samples/s: %.1f) (Consumed samples/s: %.1f)\n",
                        (double)actual_sample_rate, (double)sample_consume_rate);

		VESC_IF->printf("Command Frequency: %.1f, Carrier Frequency: %.1f", (double) command_frequency, carrier_frequency);

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


static lbm_value ext_set_motor_current(lbm_value *args, lbm_uint argn) {
    if (argn != 1 || !VESC_IF->lbm_is_number(args[0])) {
        return VESC_IF->lbm_enc_sym_eerror;
    }

	float new_current = VESC_IF->lbm_dec_as_float(args[0]);

	// Update array of samples with new value, update current value pointer
	current_samples[active_current_index] = new_current;
	active_current_index = (active_current_index + 1) % NUM_MOTOR_STAT_SAMPLES;

	// Now calculate average over the array, and use that as the actual amplitude
	float total = 0;
	for (unsigned int i = 0; i < NUM_MOTOR_STAT_SAMPLES; i++) {
		total += current_samples[i];
	}
	inverter_current = total / NUM_MOTOR_STAT_SAMPLES;

	update_spwm_settings();

    return VESC_IF->lbm_enc_sym_true;
}

static lbm_value ext_set_motor_hz(lbm_value *args, lbm_uint argn) {
    if (argn != 1 || !VESC_IF->lbm_is_number(args[0])) {
        return VESC_IF->lbm_enc_sym_eerror;
    }

	float new_freq = VESC_IF->lbm_dec_as_float(args[0]);

	// Update array of samples with new value, update current value pointer
	hz_samples[active_hz_index] = new_freq;
	active_hz_index = (active_hz_index + 1) % NUM_MOTOR_STAT_SAMPLES;

	// Now calculate average over the array, and use that as the actual amplitude
	float total = 0;
	for (unsigned int i = 0; i < NUM_MOTOR_STAT_SAMPLES; i++) {
		total += hz_samples[i];
	}
	inverter_hz = total / NUM_MOTOR_STAT_SAMPLES;

	update_spwm_settings();


    return VESC_IF->lbm_enc_sym_true;
}

static lbm_value ext_set_motor_poles(lbm_value *args, lbm_uint argn) {
    if (argn != 1 || !VESC_IF->lbm_is_number(args[0])) {
        return VESC_IF->lbm_enc_sym_eerror;
    }

    motor_poles = VESC_IF->lbm_dec_as_float(args[0]);

	update_spwm_settings();

    return VESC_IF->lbm_enc_sym_true;
}

static lbm_value ext_get_stats(lbm_value *args, lbm_uint argn) {
    if (argn != 0) {
        return VESC_IF->lbm_enc_sym_eerror;
    }

    print_stats();


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
    VESC_IF->lbm_add_extension("ext-get-stats", ext_get_stats);
    VESC_IF->lbm_add_extension("ext-set-motor-current", ext_set_motor_current);
    VESC_IF->lbm_add_extension("ext-set-motor-hz", ext_set_motor_hz);
    VESC_IF->lbm_add_extension("ext-set-motor-poles", ext_set_motor_poles);

    info->stop_fun = stop;
    info->arg = NULL;

    VESC_IF->printf("Audio module initialized.\n");

    return true;
}