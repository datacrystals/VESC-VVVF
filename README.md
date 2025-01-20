# VESC Inverter Sound Simulator

This project simulates the switching pattern sound of arbitrary traction inverters on a VESC (Vedder Electronic Speed Controller) using SPWM (Sinusoidal Pulse Width Modulation) techniques. 

---

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Example Configurations](#example-configurations)
- [Important Notes](#important-notes)
- [Contributing](#contributing)
- [License](#license)

---

## Installation

### Prerequisites
- A VESC with firmware that supports custom C code.
- A compatible development environment (e.g., VESC Tool, VESC firmware source code).
- Basic knowledge of compiling and flashing firmware to the VESC.

### Steps
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yourusername/vesc-inverter-sound-simulator.git
   cd vesc-inverter-sound-simulator
   ```

2. **Build the Code**:
   - Ensure you have the necessary toolchain installed (e.g., `gcc-arm-none-eabi`).
   - Run the build script:
     ```bash
     ./Build.py
     ```
   - This will compile the C code and generate a Lisp binary (`VVVF_COMPILED.lisp`) that can be loaded onto the VESC.


3. **Load the Lisp Code**:
   - Open the VESC Tool and connect to your VESC.
   - Go to the "LispBM" tab and load the `VVVF_COMPILED.lisp` file.

4. **Stream or Upload**:
   - Use either the `stream` or `upload` button to push the LispBM script to your VESC.


## Configuration

Unfortunately due to memory limitations the parameters are hardcoded, I tried to use JSON but there was not enough RAM on the STM32 for that to work. So, you'll have to write the code in C and compile it.


### General Parameters

The behavior of the inverter sound simulation can be fine-tuned by modifying the following parameters in the `Parameters.h` file:

### Wheel and Speed Settings
- **`WHEEL_DIAMETER_MM`**: The diameter of the wheel in millimeters. This is used to calculate the speed in km/h based on the motor's RPM.
  ```c
  #define WHEEL_DIAMETER_MM 550  // Example: 550mm wheel diameter
  ```

- **`MAX_SPEED_KMH`**: The maximum speed (in km/h) that the inverter sound simulation will support. Speeds above this value will be capped.
  ```c
  #define MAX_SPEED_KMH 100  // Example: Maximum speed of 100 km/h
  ```

- **`MAX_SPEED_RANGES`**: The maximum number of speed ranges that can be configured. Each range defines a different SPWM behavior.
  ```c
  #define MAX_SPEED_RANGES 16  // Example: Up to 16 speed ranges
  ```

### Sample Rate
- **`SAMPLE_RATE`**: The sample rate for the audio generation. This should match the FOC (Field-Oriented Control) zero vector frequency of your VESC.
  ```c
  #define SAMPLE_RATE 25000  // Example: 25kHz sample rate
  ```

### Inverter Cutoff
- **`ZERO_CUTOFF_MARGIN_KMH`**: The speed (in km/h) below which the inverter sound will be disabled when slowing down. This prevents the inverter sound from playing at very low speeds.
  ```c
  #define ZERO_CUTOFF_MARGIN_KMH 1  // Example: Disable inverter sound below 1 km/h
  ```

### Current Ramp Settings
- **`INVERTER_CURRENT_RAMP_START`**: The minimum motor current (in amps) at which the inverter sound starts ramping up.
  ```c
  #define INVERTER_CURRENT_RAMP_START 3.0  // Example: Start ramping at 3A
  ```

- **`INVERTER_CURRENT_RAMP_END`**: The maximum motor current (in amps) at which the inverter sound reaches its full amplitude.
  ```c
  #define INVERTER_CURRENT_RAMP_END 120.0  // Example: Full amplitude at 120A
  ```

### Amplitude Ramp Settings
- **`INVERTER_AMPLITUDE_RAMP_START`**: The minimum amplitude (in volts) of the inverter sound when the motor current is at `INVERTER_CURRENT_RAMP_START`.
  ```c
  #define INVERTER_AMPLITUDE_RAMP_START 0.00  // Example: Start with 0V amplitude
  ```

- **`INVERTER_AMPLITUDE_RAMP_END`**: The maximum amplitude (in volts) of the inverter sound when the motor current is at `INVERTER_CURRENT_RAMP_END`.
  ```c
  #define INVERTER_AMPLITUDE_RAMP_END 0.35  // Example: Max amplitude of 0.35V
  ```

### Speed-Based Amplitude Scaling
- **`INVERTER_AMPLITUDE_SPEED_RAMP_START_KMH`**: The speed (in km/h) at which the amplitude scaling starts.
  ```c
  #define INVERTER_AMPLITUDE_SPEED_RAMP_START_KMH 15.0  // Example: Start scaling at 15 km/h
  ```

- **`INVERTER_AMPLITUDE_SPEED_RAMP_END_KMH`**: The speed (in km/h) at which the amplitude scaling reaches its maximum.
  ```c
  #define INVERTER_AMPLITUDE_SPEED_RAMP_END_KMH 40.0  // Example: Full scaling at 40 km/h
  ```

- **`INVERTER_AMPLITUDE_SPEED_SCALAR_START`**: The starting scalar value for amplitude scaling at `INVERTER_AMPLITUDE_SPEED_RAMP_START_KMH`.
  ```c
  #define INVERTER_AMPLITUDE_SPEED_SCALAR_START 1.0  // Example: Start with 1.0x scaling
  ```

- **`INVERTER_AMPLITUDE_SPEED_SCALAR_END`**: The ending scalar value for amplitude scaling at `INVERTER_AMPLITUDE_SPEED_RAMP_END_KMH`.
  ```c
  #define INVERTER_AMPLITUDE_SPEED_SCALAR_END 2.0  // Example: End with 2.0x scaling
  ```

---

### Example Configuration
Here’s an example configuration for a typical setup:
```c
#define WHEEL_DIAMETER_MM 550
#define MAX_SPEED_KMH 100
#define MAX_SPEED_RANGES 16
#define SAMPLE_RATE 25000
#define ZERO_CUTOFF_MARGIN_KMH 1
#define INVERTER_CURRENT_RAMP_START 3.0
#define INVERTER_CURRENT_RAMP_END 120.0
#define INVERTER_AMPLITUDE_RAMP_START 0.00
#define INVERTER_AMPLITUDE_RAMP_END 0.35
#define INVERTER_AMPLITUDE_SPEED_RAMP_START_KMH 15.0
#define INVERTER_AMPLITUDE_SPEED_RAMP_END_KMH 40.0
#define INVERTER_AMPLITUDE_SPEED_SCALAR_START 1.0
#define INVERTER_AMPLITUDE_SPEED_SCALAR_END 2.0
```

This configuration is designed for a 550mm wheel, with a maximum speed of 100 km/h, and amplitude scaling based on both motor current and speed. Adjust these values to match your specific motor and pev setup.


### Switching Pattern Configuraiton

The inverter sound simulation can be configured by modifying the `ConfigParser.c` file. The configuration is divided into speed ranges, each with its own SPWM (Sinusoidal Pulse Width Modulation) settings.

#### Speed Ranges
Each speed range defines the behavior of the inverter sound within a specific speed range (in km/h). You can configure the following parameters for each range:

- **SPWM Type**: 
  - `SPWM_TYPE_FIXED_ASYNC`: Fixed frequency asynchronous SPWM.
  - `SPWM_TYPE_RAMP_ASYNC`: Ramp frequency asynchronous SPWM.
  - `SPWM_TYPE_RSPWM`: Random SPWM.
  - `SPWM_TYPE_SYNC`: Synchronous SPWM.
  - `SPWM_TYPE_NONE`: Disable inverter sound.

- **Carrier Frequency**: The frequency of the carrier wave used in SPWM.
- **Number of Pulses**: For synchronous SPWM, the number of pulses per cycle.

#### Example Configuration
Here’s an example configuration that defines three speed ranges:

```c
void InitializeConfiguration(InverterConfig* _Config) {
    _Config->maxSpeed = MAX_SPEED_KMH; // km/h
    _Config->rpmToSpeedRatio = wheel_diameter_to_kmh_factor(WHEEL_DIAMETER_MM);
    _Config->zeroSpeedCutoffMargin = ZERO_CUTOFF_MARGIN_KMH;

    // Add speed ranges
    AddSPWM_AsyncFixed(_Config, 0, 0., 22.0, 1000); // Async SPWM from 0-22 km/h @ 1kHz carrier
    AddSPWM_AsyncRamp(_Config, 1, 22.0, 44.0, 1000, 2000); // Async SPWM from 22-44 km/h @ 1kHz-2kHz carrier
    AddSPWM_Sync(_Config, 2, 44.0, 9999.0, 12); // Sync SPWM from 44 km/h and above with 12 pulses
}
```

---

### Example Switching Pattern Configurations

### Fixed Frequency Async SPWM
This configuration uses a fixed carrier frequency for all speeds:
```c
AddSPWM_AsyncFixed(_Config, 0, 0., 9999.0, 1000); // Fixed 1kHz carrier for all speeds
```

### Ramp Frequency Async SPWM
This configuration ramps the carrier frequency from 1kHz to 2kHz as speed increases:
```c
AddSPWM_AsyncRamp(_Config, 0, 0., 50.0, 1000, 2000); // Ramp from 1kHz to 2kHz between 0-50 km/h
```

### Random SPWM
This configuration uses random carrier frequencies within a range:
```c
AddSPWM_RSPWM(_Config, 0, 0., 50.0, 1000, 5000); // Random carrier frequency between 1kHz and 5kHz
```

### Synchronous SPWM
This configuration uses synchronous SPWM with a fixed number of pulses:
```c
AddSPWM_Sync(_Config, 0, 0., 9999.0, 12); // 12 pulses per cycle for all speeds
```

---

## Important Notes

1. **Avoid Modifying the Lisp Code**:
   - The Lisp code (`VVVF_COMPILED.lisp`) is automatically generated from the C code. **Do not modify the Lisp code directly, your changes will be overwritten when you recompile!** Instead, make your changes in the C code and recompile using the `Build.py` script. If you need to modify the lisp code, use the file `Main.lisp` in the `Lisp` directory.

2. **Motor Stuttering Issues**:
   - **Low Frequencies**: Using carrier frequencies that are too low can cause large hub motors (e.g., QS205) to stutter. Ensure the carrier frequency is high enough to avoid this issue.
   - **High Amplitude**: Setting the amplitude too high can also cause stuttering. Start with a low amplitude and gradually increase it while monitoring motor performance.

3. **Amplitude and Frequency Tuning**:
   - Experiment with different carrier frequencies and amplitudes to find the optimal settings for your motor. Larger motors may require higher frequencies and lower amplitudes to avoid stuttering.

---

## Contributing

Contributions are welcome! If you have any improvements, bug fixes, or new features, feel free to open a pull request. Please ensure your code follows the existing style and includes appropriate documentation.

---

## License

This project is licensed under the **GPLv3 License**. See the [LICENSE.md](LICENSE.md) file for details.

---

Enjoy simulating inverter sounds on your VESC! If you have any questions or run into issues, feel free to open an issue on GitHub.

## Contact

If you have questions or need assistance, you can find me over at [my website](https://tliao.net).