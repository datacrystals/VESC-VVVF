#!/bin/bash

# Script to compile, run, and plot the pulse wave generator

# Step 1: Compile the C program
echo "Compiling pulse_wave_graph.c..."
gcc -o pulse_wave_graph pulse_wave_graph.c -lm
if [ $? -ne 0 ]; then
    echo "Compilation failed. Please check the code and try again."
    exit 1
fi
echo "Compilation successful."

# Step 2: Run the C program to generate the CSV file
echo "Generating pulse wave data..."
./pulse_wave_graph
if [ $? -ne 0 ]; then
    echo "Failed to generate pulse wave data. Please check the program."
    exit 1
fi
echo "Pulse wave data written to pulse_wave.csv."

# Step 3: Check if Python and matplotlib are installed
echo "Checking for Python and matplotlib..."
if ! command -v python3 &> /dev/null; then
    echo "Python3 is not installed. Please install Python3 and try again."
    exit 1
fi

if ! python3 -c "import matplotlib" &> /dev/null; then
    echo "matplotlib is not installed. Installing matplotlib..."
    pip3 install matplotlib
    if [ $? -ne 0 ]; then
        echo "Failed to install matplotlib. Please install it manually and try again."
        exit 1
    fi
fi
echo "Python and matplotlib are ready."

# Step 4: Plot the data using Python
echo "Plotting the pulse wave..."
python3 plot_pulse_wave.py
if [ $? -ne 0 ]; then
    echo "Failed to plot the data. Please check the Python script."
    exit 1
fi
echo "Plotting complete. Check the graph window."

echo "All done!"