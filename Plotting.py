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
        self.RMS_LENGTH = width
        self.RMS_HEIGHT = height
        self.fft_data = np.zeros((self.FFT_HISTORY_LENGTH, self.FFT_BINS, 3))  # RGB channels
        self.rms_data = np.zeros((self.RMS_LENGTH, self.RMS_HEIGHT))
            
        # Create figure and axes
        self.fig, self.ax_fft = plt.subplots(figsize=(width / self.dpi, height / self.dpi))
        self.fig_rms, self.ax_rms = plt.subplots(figsize=(width / self.dpi, height / self.dpi))

        # Initialize the image for FFT display
        self.im = self.ax_fft.imshow(self.fft_data, aspect='auto', interpolation='none', norm=mcolors.Normalize(vmin=0, vmax=1))
        
        # Set titles and labels
        self.ax_fft.set_title('FFT Display')
        self.ax_fft.set_xlabel('Frequency')
        
        # Set x-axis properties
        self.ax_fft.xaxis.set_label_position('top')
        ticks = [int(round(f0 * 2 ** (i/3),0)) for i in range(27)]
        if f1 not in ticks:
            ticks.append(int(f1))

        self.ax_fft.set_xscale('log', base=2)
        self.ax_fft.set_xlim(f0, f1)
        self.ax_fft.set_xticks(ticks)
        self.ax_fft.xaxis.tick_top()
        self.ax_fft.xaxis.set_major_formatter(plt.FuncFormatter(lambda x, _: f'{format_hz(x)}'))
        
        self.ax_fft.yaxis.set_visible(False)

        # Frame rate counter
        self.frame_count = 0
        self.start_time = time.time()

    def update(self, fft):
        # Update FFT data
        self.fft_data = np.roll(self.fft_data, 1, axis=0)
        self.fft_data[0] = fft

        # Update the image data
        self.im.set_data(self.fft_data)

        return [self.im]

    def update_rms(self, rms):
        # Update RMS data
        self.rms_data = np.roll(self.rms_data, 1, axis=0)
        self.rms_data[0] = rms

        # Update the image data
        self.im_rms.set_data(self.rms_data)

        return [self.im_rms]
    
    def start(self, datatype='fft'):
        if datatype == 'fft':
            self.ani = animation.FuncAnimation(self.fig, self.update, interval=33, blit=True)
        elif datatype == 'rms':
            self.ani = animation.FuncAnimation(self.fig_rms, self.update_rms, interval=33, blit=True)
        plt.tight_layout()
        plt.show()

# Example usage
if __name__ == "__main__":
    plotter = Plotting()

    test_ftt = np.random.rand(480, 1920, 3)
    test_rms = np.random.rand(1920, 480)

    # Profiler
    profiler = cProfile.Profile()
    profiler.enable()

    plotter.start('fft')
    
    profiler.disable()
    with open('profile_output.txt', 'w') as f:
        ps = pstats.Stats(self.profiler, stream=f)
        ps.strip_dirs().sort_stats('cumulative').print_stats(20)
    print("Profiling data saved to 'profile_output.txt'")
