#define WHEEL_DIAMETER_MM 550
#define MAX_SPEED_KMH 100

#define MAX_SPEED_RANGES 16

#define SAMPLE_RATE 25000 // Should be equal to your foc zero vector frequency

#define ZERO_CUTOFF_MARGIN_KMH 1 // Speed which the inverter turns off when slowing down

#define INVERTER_CURRENT_RAMP_START 3.0
#define INVERTER_CURRENT_RAMP_END 100.0

#define INVERTER_AMPLITUDE_BASE 0.03
#define INVERTER_AMPLITUDE_RAMP_START 0.05
#define INVERTER_AMPLITUDE_RAMP_END 1.0