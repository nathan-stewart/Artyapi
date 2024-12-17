#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import math
from scipy.signal import iirfilter, lfilter, get_window, freqz
import matplotlib.pyplot as plt
from src.fftw3 import *
from src.util import *

LOGMIN = 10 ** (-96 / 20)
LOGMAX = 10 ** (12 / 20)


class AudioProcessor:
    def __init__(self, window_size=65536, samplerate=48000, resolution=1876):
        self.samplerate = samplerate

        # FFT parameters
        self.window_size = 2 ** (round(math.log2(window_size)))
        self.f0 = 40
        self.f1 = 20e3
        order = 4
        self.window = get_window("hann", self.window_size)
        self.b, self.a = iirfilter(
            order,
            Wn=[self.f0, self.f1],
            fs=self.samplerate,
            btype="band",
            ftype="butter",
        )
        self.raw = np.zeros(self.window_size)
        self.c_idx = 0
        self.bincount = self.window_size // 2 + 1
        self.linspace = np.linspace(0, self.samplerate / 2, self.bincount)
        self.logspace = np.logspace(
            np.log2(self.f0), np.log2(self.f1), resolution, base=2
        )
        self.bin_indices = np.digitize(self.linspace, self.logspace)

    def update_history(self, data):
        roll_len = min(len(data), self.window_size)
        if roll_len == 0:
            return

        e_idx = (self.c_idx + roll_len) % self.window_size
        if (
            e_idx == self.c_idx
        ):  # if roll_len is exactly windowsize - replace the whole buffer
            self.raw[:] = data[: self.window_size]
        elif e_idx < self.c_idx:  # wrap around case
            self.raw[self.c_idx :] = data[: self.window_size - self.c_idx]
            self.raw[:e_idx] = data[self.window_size - self.c_idx :]
        else:  # normal case
            self.raw[self.c_idx : e_idx] = data[:roll_len]
        self.c_idx = e_idx

    def process_data(self, data):
        if data is None or np.any(np.isnan(data)):
            return
        self.update_history(data)

        # Compute the RMS volume of the current batch of data
        rms = 20 * np.log10(np.sqrt(np.mean(data**2)) + LOGMIN)
        peak = 20 * np.log10(np.max(np.abs(data)) + LOGMIN)

        # Compute the FFT of the signal on the windowed and filtered data
        windowed = self.window * self.raw
        filtered = lfilter(self.b, self.a, windowed)
        fft_data = np.abs(fftw_rfft(filtered))
        normalized_fft = 2 * fft_data / self.window_size

        # autocorrelate and normalize, keeping only positive lags
        autocorr_full = np.fft.ifft(np.abs(np.fft.fft(normalized_fft)) ** 2).real
        autocorr = autocorr_full[: self.bincount]
        autocorr = np.clip(autocorr, 0, 1)

        # interpolate loses bin count - implement a summing interpolation to preserve

        # Sum the FFT magnitudes within each logspace bin
        fft = np.array(
            [
                normalized_fft[self.bin_indices == i].sum()
                for i in range(1, len(self.logspace))
            ]
        )
        autocorr = np.array(
            [
                autocorr[self.bin_indices == i].sum()
                for i in range(1, len(self.logspace))
            ]
        )
        return rms, peak, fft, autocorr
