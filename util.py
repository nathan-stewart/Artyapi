#!/usr/bin/env python3
# utility functions for the project
from scipy.signal import find_peaks, freqz
import numpy as np

def is_raspberry_pi():
    try:
        with open('/proc/device-tree/model', 'r') as f:
            return 'Raspberry Pi' in f.read()
    except FileNotFoundError:
        return False
    return False

last_print = None

def print_fft_summary(label, fft_data, freq_bins):
    """
    Print the significant peaks found in the FFT data for debugging.

    Parameters:
    - label: A label for the FFT data (e.g., "FFT Data").
    - fft_data: The FFT data in decibels.
    - freq_bins: The corresponding frequency bins for the FFT data.
    - height_threshold: The minimum height (in dB) to consider a peak.
    - prominence: The prominence of the peaks to consider.
    - num_peaks: The number of top peaks to display.
    """
    global last_print
    height_threshold = 0
    prominence = 1

    # ANSI escape code for bold text
    bold = "\033[1m"
    reset = "\033[0m"

    # Find peaks in the FFT data
    peaks, _ = find_peaks(fft_data, height=height_threshold, prominence=prominence)

    # Sort peaks by magnitude
    sorted_peaks = sorted(peaks, key=lambda x: fft_data[x], reverse=True)

    # Select the top N peaks
    num_peaks = min(2,len(sorted_peaks))
    top_peaks = sorted_peaks[:num_peaks]

    # Print the peaks
    if num_peaks > 0:
        peak_string = 'Peaks: ' + ', '.join([f'{fft_data[f]:+.2f} db @ {freq_bins[f]:.1f} Hz' for f in top_peaks])
        this_print = f'{label} - Mean: {np.mean(fft_data):.1f} db, Min: {np.min(fft_data):.1f} db, Max: {np.max(fft_data):.1f} db' + peak_string
    else:
        this_print = f"{label} - Mean: {np.mean(fft_data):.1f} db, Min: {np.min(fft_data):.1f} db, Max: {np.max(fft_data):.1f} db"
    if last_print != this_print:
        print(this_print)
        last_print = this_print


def format_hz(hz):
    hz = round(hz)
    if hz < 1000:
        return f'{hz:.0f}'
    else:
        k = int(hz) // 1000
        c = (int(hz) % 1000) // 100
        if c == 0:
            return f'{k}k'
        elif k < 10:
            return f'{k}k{c:02d}'
        else:
            return f'{k}k{c:01d}'


def get_filter_freq(filter, samplerate):
    w,h = freqz(filter)
    gain_db = 20 * np.log10(np.abs(h))

    # Find the frequency where the gain drops to -3 dB
    if np.any(gain_db <= -3):
        corner_freq_index = np.where(gain_db <= -3)[0][0]
        corner_freq = w[corner_freq_index] * (0.5 * samplerate) / np.pi  # Assuming a sample rate of 48000 Hz
    else:
        corner_freq = samplerate / 2
    return corner_freq
