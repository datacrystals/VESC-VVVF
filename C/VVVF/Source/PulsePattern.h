#pragma once

#include "stdint.h"

// Phase-shifted pulse wave generator
int8_t GeneratePulse(float phase, float dutyCycle);

// Sawtooth wave generator
int8_t GenerateSawtooth(float phase, float dutyCycle);

// Sine wave generator
int8_t GenerateSine(float phase, float dutyCycle);

// Square wave generator
int8_t GenerateSquare(float phase, float dutyCycle);

// Triangle wave generator
int8_t GenerateTriangle(float phase, float dutyCycle);
