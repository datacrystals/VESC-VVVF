/*
	Copyright 2024 Thomas Liao, Benjamin Vedder	benjamin@vedder.se

	This file is (was) part of the VESC firmware.

	The VESC firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The VESC firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vesc_c_if.h"
#include <math.h>

HEADER

static lbm_value ext_test(lbm_value *args, lbm_uint argn) {
	if (argn != 1 || !VESC_IF->lbm_is_number(args[0])) {
		return VESC_IF->lbm_enc_sym_eerror;
	}
	
	return VESC_IF->lbm_enc_i(VESC_IF->lbm_dec_as_i32(args[0]) * 3);
}

// Faster than using the sin function
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
-48, -38, -28, -17, -6, 5, 17, 28, 40, 52};

// Function to generate samples and play them
static lbm_value ext_generate_and_play_samples(lbm_value *args, lbm_uint argn) {
    // Check the number of arguments and their types
    if (argn != 6 || 
        !VESC_IF->lbm_is_number(args[0]) ||  // phase
        !VESC_IF->lbm_is_number(args[1]) ||  // frequency
        !VESC_IF->lbm_is_number(args[2]) ||  // amplitude
        !VESC_IF->lbm_is_number(args[3]) ||  // sample rate
        !VESC_IF->lbm_is_byte_array(args[4]) ||  // buffer
        !VESC_IF->lbm_is_number(args[5])) {  // chunk size
        return VESC_IF->lbm_enc_sym_eerror;  // Return an error if types are invalid
    }

    // Decode LBM values into C types
    float phase = VESC_IF->lbm_dec_as_float(args[0]);
    float frequency = VESC_IF->lbm_dec_as_float(args[1]);
    float amplitude = VESC_IF->lbm_dec_as_float(args[2]);
    float sample_rate = VESC_IF->lbm_dec_as_float(args[3]);
    lbm_flat_value_t *buffer = (lbm_flat_value_t *)args[4];  // Access the buffer
    int chunk_size = VESC_IF->lbm_dec_as_i32(args[5]);

    // Debug: Print input parameters
    // VESC_IF->printf("Phase: %f, Frequency: %f, Amplitude: %f, Sample Rate: %f, Chunk Size: %d\n",
    //                 phase, frequency, amplitude, sample_rate, chunk_size);

    // Create a separate buffer for testing
    int8_t test_buffer[chunk_size];

    // Phase increment (fixed-point arithmetic)
    uint16_t phase_increment = (uint16_t)((frequency / sample_rate) * 100);  // 100 = size of sine table
    uint16_t phase_accumulator = (uint16_t)(phase * 100);  // Initial phase

    // Generate sine wave samples
    for (int i = 0; i < chunk_size; i++) {
        // Fetch sine value from lookup table
        uint8_t phase_index = phase_accumulator % 100;  // Wrap around after 100 elements
        int8_t sine_value = sineLookupTable[phase_index];

        // Scale to amplitude (using bit shift instead of multiplication)
        // int8_t sample = (int8_t)((sine_value) >> 7);  // amplitude is assumed to be <= 1.0

        // Store the sample in the test buffer
        test_buffer[i] = sine_value;

        // Debug: Print the sample
        // VESC_IF->printf("Sample %i: Actual: %i, Stored: %i\n", i, sine_value, test_buffer[i]);

        // Increment phase accumulator and wrap around
        phase_accumulator += phase_increment;
        if (phase_accumulator >= 100) {
            phase_accumulator -= 100;  // Wrap around after completing one period
        }
    }

    // // Debug: Print the first few samples from the test buffer
    // for (int i = 0; i < 10; i++) {
    //     VESC_IF->printf("Test Buffer %i: %i\n", i, test_buffer[i]);
    // }

    // Play the samples from the test buffer
    // VESC_IF->printf("Playing samples...\n");
    VESC_IF->foc_play_audio_samples(test_buffer, chunk_size, sample_rate, amplitude);

    // Return new phase value (wrapped around)
    return VESC_IF->lbm_enc_float((float)(phase_accumulator) / 100.0f);
}


INIT_FUN(lib_info *info) {
	INIT_START

	(void)info;
	VESC_IF->lbm_add_extension("ext-test", ext_test);	
	VESC_IF->lbm_add_extension("ext-generate-and-play-samples", ext_generate_and_play_samples);	
	return true;
}

