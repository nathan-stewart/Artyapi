import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import time
from math import ceil

LOGMIN = 10**(-96/20)
LOGMAX = 10**(12/20)

def format_hz(x):
    if x >= 1000:
        return f'{x/1000:.1f}kHz'
    else:
        return f'{x}Hz'

class Plotting:
    def __init__(self, width=1920, height=480, f0=40, f1=20e3):
        self.dpi = 100
        self.FFT_BINS = width
        self.FFT_HISTORY_LENGTH = height
        self.RMS_LENGTH = width
        self.RMS_HEIGHT = height
        self.fft_data = np.zeros((self.FFT_HISTORY_LENGTH, self.FFT_BINS, 3))  # RGB channels
        self.rms_data = np.zeros((self.RMS_LENGTH, self.RMS_HEIGHT))
        self.latest_rms = np.zeros(self.RMS_HEIGHT)
        self.latest_fft = np.zeros((self.FFT_BINS, 3))
        
        # Create figure and axes for FFT display
        self.fig_fft, self.ax_fft = plt.subplots(figsize=(width / self.dpi, height / self.dpi))
        self.im_fft = self.ax_fft.imshow(self.fft_data, aspect='auto', interpolation='none', norm=mcolors.Normalize(vmin=0, vmax=1))
        self.ax_fft.set_title('FFT Display')
        self.ax_fft.xaxis.set_label_position('top')
        octaves = ceil(np.log2(f1 / f0))
        ticks = [int(round(f0 * 2 ** (i/3), 0)) for i in range(3*octaves)]
        if f1 not in ticks:
            ticks.append(int(f1))
        self.ax_fft.set_xscale('log', base=2)
        self.ax_fft.set_xlim(f0, f1)
        self.ax_fft.set_xticks(ticks)
        self.ax_fft.xaxis.tick_top()
        self.ax_fft.xaxis.set_major_formatter(plt.FuncFormatter(lambda x, _: f'{format_hz(x)}'))
        self.ax_fft.yaxis.set_visible(False)

        # Create figure and axes for RMS display
        self.fig_rms, self.ax_rms = plt.subplots(figsize=(width / self.dpi, height / self.dpi))
        self.im_rms = self.ax_rms.imshow(self.rms_data, aspect='auto', interpolation='none', norm=mcolors.Normalize(vmin=0, vmax=1))
        self.ax_rms.set_title('SPL')
        self.ax_rms.set_ylim(-96, 12)

        # Frame rate counter
        self.frame_count = 0
        self.start_time = time.time()

    def update_data(self, rms, fft):
        # Update RMS data
        self.rms_data = np.roll(self.rms_data, 1, axis=0)
        self.rms_data[0] = rms
        self.im_rms.set_data(self.rms_data)

        # Update FFT data
        self.fft_data = np.roll(self.fft_data, 1, axis=0)
        self.fft_data[0] = fft
        self.im_fft.set_data(self.fft_data)
        plt.pause(0.001)
        return [self.im_fft]

    def show(self):
        plt.show(block=False)

# Example usage
if __name__ == "__main__":
    plotter = Plotting()

    # Profiler
    profiler = cProfile.Profile()
    profiler.enable()

    plotter.start_both()
    
    profiler.disable()
    with open('profile_output.txt', 'w') as f:
        ps = pstats.Stats(profiler, stream=f)
        ps.strip_dirs().sort_stats('cumulative').print_stats(20)
    print("Profiling data saved to 'profile_output.txt'")