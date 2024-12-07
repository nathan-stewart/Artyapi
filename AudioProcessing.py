#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import os
from matplotlib.colors import hsv_to_rgb
import math
from fftw3 import *
from scipy.signal import firwin, lfilter, get_window
from util import *

LOGMIN = 10**(-96/20)
LOGMAX = 10**(12/20)
FFTBINS = 1920

class AudioProcessor:
    def __init__(self, window_size=65536, samplerate = 48000):
        self.samplerate = samplerate

        kernel_size = 401
        # FFT parameters
        self.window_size = 2**(round(math.log2(window_size)))
        self.f0 = 40
        self.f1 = 20e3
        self.linear_freq_bins = np.fft.rfftfreq(self.window_size, 1 / self.samplerate)
        self.log_freq_bins = np.logspace(np.log2(self.f0), np.log2(self.f1), FFTBINS)
        self.maxrms = -100

        window = get_window('hann', self.window_size)
        bpf = firwin(kernel_size, [self.f0, self.f1], fs=self.samplerate, pass_zero=False)
        self.windowed_bpf = bpf * window[:kernel_size]
        self.raw = np.zeros(self.window_size)
        self.c_idx = 0
        self.bincount = self.window_size // 2 + 1

        self.mic_gain = 15.0 # make full scale +12db
        
        
    def update_history(self, data):
        
        roll_len = min(len(data), self.window_size)
        if roll_len == 0:
            return
        
        e_idx = (self.c_idx + roll_len) % self.window_size
        if e_idx == self.c_idx: # if roll_len is exactly windowsize - replace the whole buffer
            self.raw[:] = data[:self.window_size]
        elif e_idx < self.c_idx: # wrap around case
            self.raw[self.c_idx:] = data[:self.window_size - self.c_idx]
            self.raw[:e_idx] = data[self.window_size - self.c_idx:]
        else: # normal case
            self.raw[self.c_idx:e_idx] = data[:roll_len]
        self.c_idx = e_idx


    def colorize(self, log_fft_data, autocorr):
        # Normalize log_fft_data to range [0, 1]
        log_fft_data = (log_fft_data - log_fft_data.min()) / (log_fft_data.max() - log_fft_data.min())

        # Create HSV array
        hsv = np.zeros((log_fft_data.shape[0], 3))
        hsv[:, 0] = 0.5 * (1 - log_fft_data)  # Hue: blue to green transition
        hsv[:, 1] = 1  # Saturation: full
        hsv[:, 2] = log_fft_data  # Value: intensity

        # Convert HSV to RGB
        rgb = hsv_to_rgb(hsv)

        # Add red channel for correlation
        rgb[:, 0] = autocorr  # Red channel

        return rgb
    
    def process_data(self, data):
        if data is None:
            return
        self.update_history(data)

        # Compute the RMS volume of the current batch of data
        rms = 20 * np.log10(np.sqrt(np.mean(data ** 2)) + LOGMIN) + self.mic_gain
        self.maxrms = max(rms, self.maxrms)
        # print(self.maxrms)
        
        # Compute the FFT of the signal on the windowed data
        normalized = np.clip(self.raw / self.window_size, 0, 1)
        filtered = lfilter(self.windowed_bpf, 1, normalized)
        fft_data = np.abs(fftw_rfft(filtered))
        normalized_fft  = np.clip(fft_data / self.window_size, 0, 1)
        log_fft_data = np.log2(1 + 100 * normalized_fft) / np.log2(101)

        # autocorrelate and normalize, keeping only positive lags
        autocorr_full = np.fft.ifft(np.abs(np.fft.fft(log_fft_data))**2).real
        autocorr = autocorr_full[:self.bincount]
        autocorr = np.clip(autocorr, 0, 1)
        autocorr = np.where(autocorr > 0.4, autocorr, 0) # suppress bins with low correlation

        ls_fft = np.interp(self.log_freq_bins, self.linear_freq_bins, log_fft_data)
        ls_autocorr = np.interp(self.log_freq_bins, self.linear_freq_bins, autocorr)
        colored_fft = self.colorize(ls_fft, ls_autocorr)
        return rms, colored_fft        

