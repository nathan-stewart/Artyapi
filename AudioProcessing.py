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


def colorize(intensity, autocorr):
        blue_point = 0.02
        r = np.clip(255*autocorr, 0, 255)
        g = np.clip(255*intensity, 0, 255)
        b = np.clip(255 * (1 - np.exp(-np.log(2) / blue_point * intensity)), 0, 255)
        b = np.clip(b-g, 0, 255)
        return np.array([r,g,b]).transpose(1,0).astype(np.uint8)


class AudioProcessor:
    def __init__(self, window_size=65536, samplerate = 48000, plot_duration = 30.0):
        self.plot_duration = plot_duration
        self.samplerate = samplerate

        # FFT parameters
        self.window_size = 2**(round(math.log2(window_size)))
        self.linear_freq_bins = np.fft.rfftfreq(self.window_size, 1 / self.samplerate)
        self.log_freq_bins = np.logspace(np.log2(self.x_major[0]), np.log2(self.x_major[-1]), self.plot_width, base=2)

        self.f0 = 40
        self.f1 = 20e3
        self.window = get_window('hann', self.window_size)
        self.target_fps = 20
        self.hpf = firwin(1023, 2 * self.f0 / self.samplerate, pass_zero=False)
        self.lpf = firwin(1023, 2 * self.f1 / self.samplerate, pass_zero=True)
        self.raw = np.zeros(self.window_size)
        self.volume = np.zeros(self.target_fps * self.plot_duration)
        self.fft = np.zeros((self.target_fps * self.plot_duration, self.window_size // 2 + 1))

    def update_history(self, data):
        roll_len = min(len(data), self.hist_len)
        self.history = np.roll(self.history[0], -roll_len)
        self.history[-roll_len:] = data[-roll_len:]

    def process_data(self, data):
        self.update_history(data)

        # Compute the RMS volume of the signal
        self.volume = np.roll(self.volume, -1)
        self.volume[-1] = np.sqrt(np.mean(data[i*spf:(i+1)*spf] ** 2))

        # Compute the FFT of the signal
        windowed_data = self.history[-self.window_size:] * self.window
        filtered = lfilter(self.hpf, 1, windowed_data)
        filtered = lfilter(self.lpf, 1, filtered)
        fft_data = np.abs(fftw_rfft(windowed_data))
        normalized_fft  = np.clip(fft_data / self.window_size, 0, 1)
        interpolated_fft = np.interp(self.log_freq_bins, self.linear_freq_bins, normalized_fft)
        log_fft_data = np.log2(1 + 100 * interpolated_fft) / np.log2(101)

        # autocorrelate and normalize
        autocorr = np.fft.ifft(np.abs(np.fft.fft(log_fft_data))**2).real
        autocorr = autocorr[:len(autocorr)//2] # keep only positive lags
        autocorr = np.clip(autocorr, 0, 1)
        autocorr = np.where(autocorr > 0.4, autocorr, 0) # suppress bins with low correlation
        autocorr = np.interp(self.log_freq_bins, np.linspace(0, len(autocorr), len(autocorr)), autocorr)

        self.fft = np.roll(self.fft, -1, axis=0)
        self.fft[:-1, :] = colorize(log_fft_data, autocorr)

