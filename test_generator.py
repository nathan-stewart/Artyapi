import numpy as np
import matplotlib.pyplot as plt
import soundfile as sf
import scipy.signal as signal

def generate_pink_noise(duration, samplerate):
    # Generate white noise
    white_noise = np.random.normal(0, 1, int(samplerate * duration))
    
    # Apply a filter to convert white noise to pink noise
    b = [0.049922035, -0.095993537, 0.050612699, -0.004408786]
    a = [1, -2.494956002, 2.017265875, -0.522189400]
    pink_noise = signal.lfilter(b, a, white_noise)
    
    return pink_noise

def generate_test_signal(duration, samplerate, noise_intensity_sweep=True, repetitive_signal=True):
    t = np.linspace(0, duration, int(samplerate * duration), endpoint=False)
    
    # Generate pink noise
    noise = generate_pink_noise(duration, samplerate)
    
    # Sweep intensity of the noise from -90 dB to +12 dB
    if noise_intensity_sweep:
        intensity_sweep = np.linspace(-90, 12, len(t))
        noise = noise * 10**(intensity_sweep / 20)
    
    # Generate a repetitive signal (e.g., sine waves at specified times and frequencies)
    if repetitive_signal:
        signal = noise.copy()
        sine_wave_frequencies = [440, 880]  # Frequencies in Hz
        sine_wave_duration = 2  # Duration in seconds
        for freq in sine_wave_frequencies:
            start_time = int((duration / len(sine_wave_frequencies)) * sine_wave_frequencies.index(freq) * samplerate)
            end_time = start_time + int(sine_wave_duration * samplerate)
            sine_wave = 0.5 * np.sin(2 * np.pi * freq * t[start_time:end_time])
            signal[start_time:end_time] += sine_wave
    else:
        signal = noise
    
    return signal

# Parameters
duration = 10  # Duration of the signal in seconds
samplerate = 48000  # Sample rate in Hz

# Generate the test signal
test_signal = generate_test_signal(duration, samplerate)

# Calculate dB levels
db_levels = 20 * np.log10(np.abs(test_signal) + 1e-10)  # Add a small value to avoid log(0)

# Plot the dB levels
plt.plot(db_levels[:1000])  # Plot the first 1000 samples for visualization
plt.title("dB Levels of Test Signal")
plt.xlabel("Sample")
plt.ylabel("dB")
plt.show()

# Determine the dB range
db_range = (np.min(db_levels), np.max(db_levels))
print(f"dB Range: {db_range}")

# Save the test signal to a WAV file (optional)
sf.write('test_signal.wav', test_signal, samplerate)