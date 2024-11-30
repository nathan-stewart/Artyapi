#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import os
import math
from fftw3 import *
from scipy.signal import firwin, lfilter, get_window
from util import *
import time


LOGMIN = 10**(-96/20)
LOGMAX = 10**(12/20)

class AudioProcessor:
    def __init__(self, window_size=65536, samplerate = 48000, plot_duration = 30.0):
        self.plot_duration = plot_duration
        self.samplerate = samplerate

        # FFT parameters
        self.window_size = 2**(round(math.log2(window_size)))
        self.f0 = 40
        self.f1 = 20e3
        self.linear_freq_bins = np.fft.rfftfreq(self.window_size, 1 / self.samplerate)

        self.window = get_window('hann', self.window_size)
        self.target_fps = 20
        self.hpf = firwin(1023, 2 * self.f0 / self.samplerate, pass_zero=False)
        self.lpf = firwin(1023, 2 * self.f1 / self.samplerate, pass_zero=True)
        self.raw = np.zeros(self.window_size)
        self.history_len = int(self.target_fps * self.plot_duration)
        self.volume = np.zeros(self.history_len)
        self.bincount = self.window_size // 2 + 1
        self.fft = np.zeros((self.bincount, self.history_len, 2))

    def update_history(self, data):
        roll_len = min(len(data), self.history_len)
        if roll_len == 0:
            return
        self.raw = np.roll(self.raw, -roll_len)
        self.raw[-roll_len:] = data[-roll_len:]

    def process_data(self, data):
        self.update_history(data)

        # Compute the RMS volume of the signal
        self.volume = np.roll(self.volume, -1)
        self.volume[-1] = np.sqrt(np.mean(data ** 2))

        # Compute the FFT of the signal
        windowed_data = self.raw[-self.window_size:] * self.window
        filtered = lfilter(self.hpf, 1, windowed_data)
        filtered = lfilter(self.lpf, 1, filtered)
        fft_data = np.abs(fftw_rfft(windowed_data))
        normalized_fft  = np.clip(fft_data / self.window_size, 0, 1)
        log_fft_data = np.log2(1 + 100 * normalized_fft) / np.log2(101)

        # autocorrelate and normalize, keeping only positive lags
        autocorr_full = np.fft.ifft(np.abs(np.fft.fft(log_fft_data))**2).real
        autocorr = autocorr_full[:self.bincount]
        autocorr = np.clip(autocorr, 0, 1)
        autocorr = np.where(autocorr > 0.4, autocorr, 0) # suppress bins with low correlation
        
        self.fft = np.roll(self.fft, -1, axis=1)
        self.fft[:, -1, 0] = log_fft_data
        self.fft[:, -1, 1] = autocorr

