#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.colors as mcolors
import time
import cProfile
import pstats
from util import * 

LOGMIN = 10**(-96/20)
LOGMAX = 10**(12/20)

class Plotting:
    def __init__(self, width=1920, height=480, f0 = 40, f1 = 20e3):
        self.dpi = 100
        self.FFT_BINS = width
        self.FFT_HISTORY_LENGTH = height
        self.fft_data = np.zeros((self.FFT_HISTORY_LENGTH, self.FFT_BINS, 3))  # RGB channels

        # Create figure and axes
        self.fig, self.ax_fft = plt.subplots(figsize=(width / self.dpi, height / self.dpi))

        # Initialize the image for FFT display
        self.im = self.ax_fft.imshow(self.fft_data, aspect='auto', origin='upper', interpolation='none')

        # Set titles and labels
        self.ax_fft.set_title('FFT Display')
        self.ax_fft.set_xlabel('Frequency')
        
        # Set x-axis properties
        self.ax_fft.xaxis.set_label_position('top')
        ticks = [int(round(f0 * 2 ** (i/3),0)) for i in range(27)]
        self.ax_fft.set_xscale('log', base=2)
        self.ax_fft.set_xlim(f0, f1)
        self.ax_fft.set_xticks(ticks)
        self.ax_fft.xaxis.tick_top()
        self.ax_fft.xaxis.set_major_formatter(plt.FuncFormatter(lambda x, _: f'{format_hz(x)}'))
        
        self.ax_fft.yaxis.set_visible(False)

        # FFT bins need interpolation
        self.fft_logspace = np.logspace(np.log2(f0), np.log2(f1), self.FFT_BINS)
        self.fft_bins = np.fft.rfftfreq(self.FFT_BINS, 1 / 48000)
        
        # Frame rate counter
        self.frame_count = 0
        self.start_time = time.time()

        # Profiler
        self.profiler = cProfile.Profile()

    def colorize(self, log_fft_data, autocorr):
        # Normalize log_fft_data to range [0, 1]
        log_fft_data = (log_fft_data - log_fft_data.min()) / (log_fft_data.max() - log_fft_data.min())

        # Create HSV array
        hsv = np.zeros((log_fft_data.shape[0], 3))
        hsv[:, 0] = 0.5 * (1 - log_fft_data)  # Hue: blue to green transition
        hsv[:, 1] = 1  # Saturation: full
        hsv[:, 2] = log_fft_data  # Value: intensity

        # Convert HSV to RGB
        rgb = mcolors.hsv_to_rgb(hsv)

        # Add red channel for correlation
        rgb[:, 0] = autocorr  # Red channel

        return rgb

    def update(self, frame):
        # Simulate FFT data (replace with actual data processing)
        new_fft = np.random.rand(self.FFT_BINS)
        autocorr = np.random.rand(self.FFT_BINS)

        # Colorize FFT data
        colored_fft = self.colorize(new_fft, autocorr)
    
        # Update FFT data
        self.fft_data = np.roll(self.fft_data, 1, axis=0)
        self.fft_data[0] = colored_fft

        # Update the image data
        self.im.set_data(self.fft_data)

        # Update frame count
        self.frame_count += 1
        if self.frame_count % 30 == 0:  # Print FPS every 30 frames
            elapsed_time = time.time() - self.start_time
            fps = self.frame_count / elapsed_time
            print(f"FPS: {fps:.2f}")

        return [self.im]

    def start(self):
        self.profiler.enable()
        ani = animation.FuncAnimation(self.fig, self.update, interval=33, blit=True)
        plt.tight_layout()
        plt.show()
        self.profiler.disable()
        with open('profile_output.txt', 'w') as f:
            ps = pstats.Stats(self.profiler, stream=f)
            ps.strip_dirs().sort_stats('cumulative').print_stats(20)
        print("Profiling data saved to 'profile_output.txt'")

# Example usage
if __name__ == "__main__":
    plotter = Plotting()
    plotter.start()