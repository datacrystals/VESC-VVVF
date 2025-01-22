import matplotlib.pyplot as plt
import pandas as pd

# Read the CSV file
data = pd.read_csv("pulse_wave.csv")

# Plot the data
plt.figure(figsize=(10, 6))
plt.plot(data["Time (s)"], data["Amplitude"], label="Pulse Wave", color="blue")
plt.title("Pulse Wave (Frequency: 440 Hz, Duty Cycle: 3%)")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")
plt.grid(True)
plt.legend()
plt.show()