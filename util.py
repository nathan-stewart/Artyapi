#!/usr/bin/env python3
# utility functions for the project
from scipy.signal import find_peaks, freqz
import numpy as np
import time
import colorsys
import os
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'
import pygame

def sine_generator(frequency):
    '''
    Sine wave generator which prentends to be an audio device filling up a buffer
    with samples since the last time called
    '''
    sample_rate = 48000
    amplitude = np.sqrt(2) * 10**(12/20) # +12 db
    start = time.time()
    phase = 0
    while True:
        now = time.time()
        elapsed = now - start
        num_samples = int(sample_rate * elapsed)
        t = np.linspace(0, elapsed, num_samples, endpoint=False)
        buffer = amplitude* np.sin(2 * np.pi * frequency * t + phase)

        phase += 2 * np.pi * frequency * elapsed
        phase = phase % (2 * np.pi)

        start = now
        yield buffer

def sweep_generator(f0, f1, duration, db):
    '''
    Sine wave generator which pretends to be an audio device filling up a buffer
    with samples since the last time called
    '''
    sample_rate = 48000
    amplitude = np.sqrt(2) * 10**(db/20)
    start = time.time()
    phase = 0
    sweep = 0
    while True:
        now = time.time()
        elapsed = now - start
        sweep += elapsed
        num_samples = int(sample_rate * elapsed)
        t = np.linspace(0, elapsed, num_samples, endpoint=False)

        # Linear frequency sweep
        frequency = f0 + (f1 - f0) * (sweep/duration)

        # Generate the buffer with the frequency sweep
        buffer = amplitude * np.sin(2 * np.pi * frequency * t + phase)

        # Update the phase
        phase += 2 * np.pi * (f0 * elapsed + 0.5 * (f1 - f0) * (elapsed ** 2) / duration) % (2 * np.pi)

        start = now
        yield buffer

def resolution_generator():
    '''
    Generate f0, f1 sine wave pairs at 40, 46, 80, 92, 160, 184 Hz...
    '''
    sample_rate = 48000
    amplitude = np.sqrt(2) * 10**(12/20)  # Example amplitude for 12 dB
    frequencies = [(40, 46), (80, 92), (160, 184)]
    phase = 0
    start = time.time()

    while True:
        now = time.time()
        elapsed = now - start
        num_samples = int(sample_rate * elapsed)
        t = np.linspace(0, elapsed, num_samples, endpoint=False)

        buffer = np.zeros(num_samples)

        for f0, f1 in frequencies:
            buffer += amplitude * np.sin(2 * np.pi * f0 * t + phase)
            buffer += amplitude * np.sin(2 * np.pi * f1 * t + phase)

        phase += 2 * np.pi * sample_rate * elapsed
        phase = phase % (2 * np.pi)

        start = now
        yield buffer

def wait_for_keypress():
    keypress = None
    while True:
        for event in pygame.event.get():
            if event.type == pygame.KEYDOWN:
                if (event.key == pygame.K_SPACE or event.key == pygame.K_q):
                    keypress = True
                break
        if keypress:
            break

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
    epsilon = 1e-10
    gain_db = 20 * np.log10(np.abs(h) + epsilon)

    # Find the frequency where the gain drops to -3 dB
    if np.any(gain_db <= -3):
        corner_freq_index = np.where(gain_db <= -3)[0][0]
        corner_freq = w[corner_freq_index] * (0.5 * samplerate) / np.pi
    else:
        corner_freq = samplerate / 2
    return corner_freq

def make_color_palette(n):
    grc = 0.61803398875
    colors = []
    hue = 0
    for c in range(n):
        hue = (hue + grc) % 1
        lightness = 0.5
        saturation = 0.9
        rgs = colorsys.hls_to_rgb(hue, lightness, saturation)
        rgb = tuple([int(255 * x) for x in rgs])
        colors.append(rgb)
    return colors