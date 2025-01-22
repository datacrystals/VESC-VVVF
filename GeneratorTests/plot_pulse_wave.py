import json
import matplotlib.pyplot as plt
import sounddevice as sd
import numpy as np
import threading
import time

# Read the JSON file
with open("waveforms.json", "r") as file:
    data = json.load(file)

# Extract metadata
sample_rate = data["sample_rate"]
frequency = data["frequency"]
duty_cycle = data["duty_cycle"]

# Prepare waveform data
waveforms = []
for waveform in data["waveforms"]:
    time_data = [point["time"] for point in waveform["data"]]
    amplitude = [point["amplitude"] for point in waveform["data"]]
    waveforms.append({
        "name": waveform["name"],
        "time": time_data,
        "amplitude": amplitude
    })

# Plot all waveforms
plt.figure(figsize=(10, 6))
for waveform in waveforms:
    plt.plot(waveform["time"], waveform["amplitude"], label=waveform["name"])

# Add labels and legend
plt.title(f"Waveforms (Frequency: {frequency} Hz, Duty Cycle: {duty_cycle * 100}%)")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")
plt.grid(True)
plt.legend()

# Show the plot
plt.show()

# Global variable to control playback
is_playing = False

# Function to play a waveform as audio on repeat
def play_waveform_on_repeat(waveform_index, device_id):
    global is_playing
    waveform = waveforms[waveform_index]
    print(f"Playing {waveform['name']} waveform on repeat...")

    # Normalize amplitude to the range [-1, 1] for audio playback
    audio_data = np.array(waveform["amplitude"], dtype=np.float32) / 127.0

    # Play the audio on repeat
    is_playing = True
    while is_playing:
        try:
            sd.play(audio_data, samplerate=sample_rate, device=device_id)
            sd.wait()  # Wait until the audio is finished playing
        except Exception as e:
            print(f"Error during playback: {e}")
            break

# List available output devices
def list_output_devices():
    devices = sd.query_devices()
    print("Available output devices:")
    for device_id, device_info in enumerate(devices):
        if device_info["max_output_channels"] > 0:
            print(f"{device_id}: {device_info['name']} (Sample rate: {device_info['default_samplerate']} Hz)")

# Select an output device
def select_output_device():
    list_output_devices()
    while True:
        try:
            device_id = int(input("Select an output device by ID: "))
            device_info = sd.query_devices(device_id)
            if device_info["max_output_channels"] > 0:
                return device_id
            else:
                print("Error: Selected device does not support output.")
        except ValueError:
            print("Invalid input. Please enter a number.")
        except sd.PortAudioError:
            print("Invalid device ID. Please try again.")

# Keyboard input loop
print("Press a key to play a waveform (or 'q' to quit):")
for i, waveform in enumerate(waveforms):
    print(f"{i + 1}: {waveform['name']}")

# Select the output device
device_id = select_output_device()

while True:
    key = input("Select a waveform (1-5) or 'q' to quit: ")
    if key == 'q':
        is_playing = False  # Stop playback
        break
    try:
        waveform_index = int(key) - 1
        if 0 <= waveform_index < len(waveforms):
            # Stop any existing playback
            is_playing = False
            time.sleep(0.1)  # Allow the playback thread to stop

            # Start playback in a new thread
            playback_thread = threading.Thread(target=play_waveform_on_repeat, args=(waveform_index, device_id))
            playback_thread.start()
        else:
            print("Invalid selection. Please try again.")
    except ValueError:
        print("Invalid input. Please enter a number or 'q' to quit.")