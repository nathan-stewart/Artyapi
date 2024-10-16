#!/usr/bin/env python3
# utility functions for the project
from scipy.signal import find_peaks
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
        peak_string = 'Peaks: ' + ', '.join([f'{freq_bins[f]:.1f} Hz' for f in top_peaks])
        this_print = f'{label} - Mean: {np.mean(fft_data):.1f} db, Min: {np.min(fft_data):.1f} db, Max: {np.max(fft_data):.1f} db' + peak_string
    else:
        this_print = f"{label} - Mean: {np.mean(fft_data):.1f} db, Min: {np.min(fft_data):.1f} db, Max: {np.max(fft_data):.1f} db"
    if last_print != this_print:
        print(this_print)
        last_print = this_print
