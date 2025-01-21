#include "vesc_c_if.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ConfigParser.h"
#include "SPWMGenerator.h"
#include "Parameters.h"

// TODO:
// Open bug report on bldc firmware for play-samples being nonblocking


// HEADER macro
HEADER

// Constants
#define SINE_TABLE_SIZE 100
#define INT8_SCALE 127
#define TWO_PI 6.28318530718f
#define SAWTOOTH_MAX 127        // Maximum value for the sawtooth (int8 range: 0-127)


// SPWM modes
#define SPWM_MODE_FIXED 0
#define SPWM_MODE_RAMP 1
#define SPWM_MODE_SYNC 2


// Global variables
static int8_t *buffers[NUM_BUFFERS];  // Array of pointers to buffers allocated on the heap
static bool buffer_ready_for_consumption[NUM_BUFFERS];  // Flags to indicate if a buffer is ready for consumption
static int producer_index = 0;  // Index of the buffer currently being filled by the producer
static int consumer_index = 0;  // Index of the buffer currently being consumed by the consumer
static float amplitude = 0.0f;
static float sample_rate = SAMPLE_RATE;

static float current_samples[NUM_MOTOR_STAT_SAMPLES]; // Array of last n current values used to average them
static int active_current_index = 0; // Index of current sample to be replaced
static float hz_samples[NUM_MOTOR_STAT_SAMPLES]; // Array of last n hz values used to average them
static int active_hz_index = 0; // Index of current sample to be replaced
static float rpm_samples[RPM_SAMPLE_COUNT];
static int active_rpm_index = 0;

static float inverter_current = 0.; // Number of phase amps pushed into the motor from the vesc
static float inverter_hz = 0.; // Current freqency of the inverter in hz
static int motor_poles = 0; // Number of poles of the motor
static int inverter_enabled = false; // Enable or disable the inverter doing stuff

static InverterConfig Conf = {0}; // Configuration of the inverter from the json file
static SpeedRange ActiveSpeedRange = {0}; // Currently active speed range that should be used for motor sound generation
static SPWMGenerator generator;
static RotorState rotor_state = ROTOR_STATE_COASTING;

// Motor Sound Config
static float min_current = INVERTER_CURRENT_RAMP_START; // Amperes - defines linear ramp min current
static float max_current = INVERTER_CURRENT_RAMP_END; // Amperes - defines linear ramp max current
static float min_sound_voltage = INVERTER_AMPLITUDE_RAMP_START; // Volts - defines linear ramp min voltage
static float max_sound_voltage = INVERTER_AMPLITUDE_RAMP_END; // Volts - defines linear ramp max voltage

// SPWM variables
// static float carrier_phase = 0.0f; // Phase of the current carrier sin wave
// static float command_phase = 0.0f; // Phase of the current command sin wave


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


// Function to map a value from one range to another
float map_value(float value, float in_min, float in_max, float out_min, float out_max) {
    // Ensure the input range is not zero to avoid division by zero
    if (in_min == in_max) {
        return out_min;  // If input range is zero, return the minimum output value
    }

    // Clamp the input value to the input range
    if (value < in_min) {
        return out_min;  // If value is below in_min, clamp to out_min
    } else if (value > in_max) {
        return out_max;  // If value is above in_max, clamp to out_max
    }

    // Map the value from the input range to the output range
    float val = out_min + ((value - in_min) / (in_max - in_min)) * (out_max - out_min);

    return val;
}


// Function to update the rotor state based on the last n RPM values
static void update_rotor_state(float current_rpm) {
    // Store the current RPM in the samples array
    rpm_samples[active_rpm_index] = current_rpm;
    active_rpm_index = (active_rpm_index + 1) % RPM_SAMPLE_COUNT;

    // Calculate the average RPM over the last n samples
    float total_rpm = 0;
    for (int i = 0; i < RPM_SAMPLE_COUNT; i++) {
        total_rpm += rpm_samples[i];
    }
    float average_rpm = total_rpm / RPM_SAMPLE_COUNT;

    // Determine the rotor state based on the average RPM
    float abs_rpm_val = average_rpm - current_rpm;
    if (abs_rpm_val < 0) {
        abs_rpm_val = -abs_rpm_val;
    }
    // VESC_IF->printf("avg: %.1f, absval: %.1f.\n", average_rpm, abs_rpm_val);
    if (abs_rpm_val <= COASTING_RPM_THRESHOLD) {
        rotor_state = ROTOR_STATE_COASTING;
    } else if (current_rpm > average_rpm) {
        rotor_state = ROTOR_STATE_ACCELERATING;
    } else {
        rotor_state = ROTOR_STATE_DECELERATING;
    }
}


static void update_spwm_settings() {
    // Calculate current speed
    float CurrentSpeed_KMH = (inverter_hz / (float)motor_poles) * Conf.rpmToSpeedRatio;

    // Update the rotor state based on the current RPM
    update_rotor_state(inverter_hz / (float)motor_poles);

    // Define amplitude based on current, speed
    amplitude = map_value(inverter_current, min_current, max_current, min_sound_voltage, max_sound_voltage);
    float AmplitudeScaleFactor = map_value(
        CurrentSpeed_KMH, 
        INVERTER_AMPLITUDE_SPEED_RAMP_START_KMH,
        INVERTER_AMPLITUDE_SPEED_RAMP_END_KMH,
        INVERTER_AMPLITUDE_SPEED_SCALAR_START,
        INVERTER_AMPLITUDE_SPEED_SCALAR_END
    );
    amplitude += INVERTER_AMPLITUDE_BASE;
    // VESC_IF->printf("Amplitude Scale Value: %.1f.\n", AmplitudeScaleFactor);
    amplitude = amplitude * AmplitudeScaleFactor;

    // Get the active speed range
    ActiveSpeedRange = GetSpeedRangeAtRPM(&Conf, inverter_hz / (float)motor_poles, inverter_current);

    // Select the appropriate SPWM configuration based on the rotor state
    SPWMConfig* spwm_config = NULL;
    switch (rotor_state) {
        case ROTOR_STATE_ACCELERATING:
            spwm_config = &ActiveSpeedRange.spwm.acceleration;
            break;
        case ROTOR_STATE_COASTING:
            spwm_config = &ActiveSpeedRange.spwm.coasting;
            break;
        case ROTOR_STATE_DECELERATING:
            spwm_config = &ActiveSpeedRange.spwm.deceleration;
            break;
    }

    // Update the generator with the selected SPWM configuration
    if (spwm_config) {
        generator.CarrierFrequency = spwm_config->carrierFrequencyStart;
        generator.ModulationIndex = 1.0f; // Adjust as needed
    }
}


// Generator loop function
static void generator_loop(void *arg) {
    (void)arg;

    SPWMGenerator_Init(&generator);

    while (generator_thread_data.running) {
        // Wait until the current buffer is ready to be written to
        while (buffer_ready_for_consumption[producer_index] && generator_thread_data.running) {
            VESC_IF->sleep_ms(1);
        }

        if (!generator_thread_data.running) break;

        // Generate SPWM samples
        inverter_enabled = SPWMGenerator_GenerateSamples(&generator, rotor_state, buffers[producer_index], BUFFER_LENGTH, &ActiveSpeedRange, inverter_hz, motor_poles, Conf.rpmToSpeedRatio);

        // Mark the buffer as ready for consumption
        buffer_ready_for_consumption[producer_index] = true;

        // Update statistics
        samples_generated += BUFFER_LENGTH;

        // Move to the next buffer
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
        if (inverter_enabled) {
            VESC_IF->foc_play_audio_samples(buffers[consumer_index], BUFFER_LENGTH, sample_rate, amplitude);
        }
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

static void print_stats(void) {
    float current_time = VESC_IF->system_time();
    if (current_time - last_time >= 1.0f) {
        // Calculate samples generated and consumed per second
        uint32_t samples_per_second = samples_generated - last_samples_generated;
        uint32_t samples_consumed_per_second = samples_consumed - last_samples_comsumed;
        last_samples_generated = samples_generated;
        last_samples_comsumed = samples_consumed;
        last_time = current_time;

        // Calculate actual sample rates
        float actual_sample_rate = (float)samples_per_second;
        float sample_consume_rate = (float)samples_consumed_per_second;

        // Print warning if the sample rate is too high
        if (actual_sample_rate > sample_rate * SAMPLE_RATE_WARNING_THRESHOLD) {
            VESC_IF->printf("WARNING: Sample rate too high! Actual: %.1f, Expected: %.1f\n",
                            (double)actual_sample_rate, (double)sample_rate);
        }

        // Print sample generation and consumption rates
        VESC_IF->printf("(Generated samples/s: %.1f) (Consumed samples/s: %.1f)\n",
                        (double)actual_sample_rate, (double)sample_consume_rate);

        // Calculate the current speed in km/h
        float current_speed_kmh = inverter_hz / (float)motor_poles * Conf.rpmToSpeedRatio;

        // Print the current speed and active speed range
        VESC_IF->printf("Current Speed: %.1f km/h\n", (double)current_speed_kmh);
        VESC_IF->printf("Active Speed Range: %f km/h to %f km/h\n",
                        (double)ActiveSpeedRange.minSpeed, (double)ActiveSpeedRange.maxSpeed);

        // Print the rotor state
        const char* rotor_state_str = "Unknown";
        switch (rotor_state) {
            case ROTOR_STATE_ACCELERATING:
                rotor_state_str = "Accelerating";
                break;
            case ROTOR_STATE_COASTING:
                rotor_state_str = "Coasting";
                break;
            case ROTOR_STATE_DECELERATING:
                rotor_state_str = "Decelerating";
                break;
        }
        VESC_IF->printf("Rotor State: %s\n", rotor_state_str);

        // Print SPWM mode and carrier frequency
        const char* spwm_mode_str = "Unknown";
        SPWMConfig ActiveSPWM = ActiveSpeedRange.spwm.acceleration;
        switch (rotor_state) {
            case ROTOR_STATE_ACCELERATING:
                ActiveSPWM = ActiveSpeedRange.spwm.acceleration;
                break;
            case ROTOR_STATE_COASTING:
                ActiveSPWM = ActiveSpeedRange.spwm.coasting;
                break;
            case ROTOR_STATE_DECELERATING:
                ActiveSPWM = ActiveSpeedRange.spwm.deceleration;
                break;
        }
        switch (ActiveSPWM.type) {
            case SPWM_TYPE_FIXED_ASYNC:
                spwm_mode_str = "Fixed Async";
                break;
            case SPWM_TYPE_RAMP_ASYNC:
                spwm_mode_str = "Ramp Async";
                break;
            case SPWM_TYPE_RSPWM:
                spwm_mode_str = "Random SPWM";
                break;
            case SPWM_TYPE_SYNC:
                spwm_mode_str = "Synchronous";
                break;
            case SPWM_TYPE_NONE:
                spwm_mode_str = "Disabled";
                break;
        }
        VESC_IF->printf("SPWM Mode: %s, Carrier Frequency: %.1fHz, Amplitude %.3fV\n",
                        spwm_mode_str, (double)generator.CarrierFrequency, (double)amplitude);
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
    (void)args;
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

    // Load the config
    InitializeConfiguration(&Conf);
    PrintInverterConfig(&Conf);

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