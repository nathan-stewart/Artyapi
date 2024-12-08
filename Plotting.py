import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import time
import cProfile, pstats
from matplotlib.ticker import MultipleLocator, FormatStrFormatter, FuncFormatter
from math import ceil
from matplotlib.font_manager import FontProperties

LOGMIN = 10**(-96/20)
LOGMAX = 10**(12/20)

def format_hz(hz):
    hz = round(hz) 
    if hz < 100:
        return f'{hz:d}'
    elif hz < 1000:
        return f'{round(hz / 10) * 10:d}'
    elif hz < 10000:
        k = hz // 1000
        c = round((hz % 1000) / 100)
        return f'{k}k{c}'
    else:
        k = hz // 1000
        return f'{k}k'


def generate_ticks(f0, f1, per_octave):
    octaves = ceil(np.log2(f1 / f0))
    ticks = [int(round(f0 * 2 ** (i/3), 0)) for i in range(3*octaves)]
    if f1 not in ticks:
        ticks.append(int(f1))
    return ticks


class Plotting:

    def colorize(self, log_fft_data, autocorr):
        # Normalize log_fft_data to range [0, 1]
        maxval = max(np.max(log_fft_data), LOGMIN)
        log_fft_data = log_fft_data / maxval
        cutoff = 0.05

        # Create HSV array
        hsv = np.zeros((log_fft_data.shape[0], 3))
        # Hue is Red (0)
        # Saturation is the autocorrelation
        # Value is the log_fft_data
        hsv[:, 0] = 0
        hsv[:, 1] = autocorr
        hsv[:, 2] = log_fft_data  # Value: intensity

        # Convert HSV to RGB
        rgb = hsv_to_rgb(hsv)
        return rgb
        
    def __init__(self, width=1920, height=480, f0=40, f1=20e3):
        self.dpi = 100
        self.FFT_BINS = width
        self.FFT_HISTORY_LENGTH = height
        self.RMS_LENGTH = width
        self.RMS_HEIGHT = height
        self.fft_data = np.zeros((self.FFT_HISTORY_LENGTH, self.FFT_BINS, 3))  # RGB channels
        self.rms_data = np.full((self.RMS_LENGTH), -96)
        self.line_rms = None
        self.im_fft = None
        self.lin_freq_bins = np.fft.rfftfreq(65536, 48000)
        self.log_freq_bins = np.logspace(np.log2(f0), np.log2(f1), self.FFT_BINS, base=2)
        
        # Create figure and axes for RMS display
        # self.create_rms_plot(width, height)
        self.create_fft_plot(width=width, height=height, f0=f0, f1=f1)

        # Frame rate counter
        self.frame_count = 0
        self.start_time = time.time()

    def create_rms_plot(self, width=1920, height=480):
        def custom_y_formatter(x, pos):
            if x > 10:
                return f'+{int(x)}'
            elif x > 0:
                return f'+ {int(x)}'
            elif x == 0:
                return f'  {int(x)}'
            elif x > -10:
                return f'- {abs(int(x))}'
            else:
                return f'{int(x)}'
            
        # Create figure and axes for RMS display
        self.fig_rms, self.ax_rms = plt.subplots(figsize=(width / self.dpi, height / self.dpi))
        self.fig_rms.patch.set_facecolor('black')
        self.ax_rms.patch.set_facecolor('black')
        self.line_rms = self.ax_rms.plot(self.rms_data, color='white')
        self.ax_rms.spines['right'].set_color('white')
        self.ax_rms.spines['bottom'].set_color('white')
        self.ax_rms.spines['left'].set_color('white')
        self.ax_rms.set_ylim(-96, 12)
        self.ax_rms.set_yticks(range(-96, 13, 6))
        self.ax_rms.tick_params(axis='y', colors='white', left=True, right=True, labelleft=False, labelright=True)
        self.ax_rms.yaxis.set_major_locator(MultipleLocator(6))
        self.ax_rms.yaxis.set_major_formatter(FuncFormatter(custom_y_formatter))
        self.ax_rms.yaxis.ymin = -96
        self.ax_rms.yaxis.ymax = 12
        self.ax_rms.set_xlim(0, self.RMS_LENGTH)
        self.ax_rms.xaxis.set_visible(False)
        self.fig_rms.subplots_adjust(left=0.005, right=0.97, top=0.97, bottom=0.02)

        # Set monospace font for y-axis labels
        monospace_font = FontProperties(family='monospace')
        for label in self.ax_rms.get_yticklabels():
            label.set_fontproperties(monospace_font)

    def create_fft_plot(self, width=1920, height=480, f0=40, f1=20e3):
        # Create figure and axes for FFT display
        self.fig_fft, self.ax_fft = plt.subplots(figsize=(width / self.dpi, height / self.dpi))
        self.fig_fft.patch.set_facecolor('black')
        self.ax_fft.patch.set_facecolor('black')
        self.im_fft = self.ax_fft.imshow(self.fft_data, aspect='auto', interpolation='nearest', norm=mcolors.Normalize(vmin=0, vmax=1))
        self.ax_fft.xaxis.set_label_position('top')
        self.ax_fft.set_xscale('log', base=2)
        self.ax_fft.set_xlim(f0, f1)
        self.ax_fft.set_xticks(generate_ticks(40,20e3,3))
        self.ax_fft.xaxis.tick_top()
        self.ax_fft.xaxis.set_major_formatter(plt.FuncFormatter(lambda x, _: f'{format_hz(x)}'))
        self.ax_fft.yaxis.set_visible(False)
        self.ax_fft.spines['top'].set_color('white')
        self.ax_fft.spines['bottom'].set_color('white')
        self.ax_fft.spines['left'].set_color('white')
        self.ax_fft.spines['right'].set_color('white')
        self.ax_fft.tick_params(axis='x', colors='white')
        self.fig_fft.subplots_adjust(left=0.008, right=0.985, top=0.92, bottom=0.01)


    def update_data(self, rms, fft, acf):
        # Update RMS data
        if self.line_rms and not np.isnan(rms):
            self.rms_data = np.roll(self.rms_data, -1)
            self.rms_data[-1] = rms
            self.line_rms[0].set_ydata(self.rms_data)
            self.fig_rms.canvas.draw()
            self.fig_rms.canvas.flush_events()

       # Update FFT data
        if self.fig_fft:
            mf = np.max(fft)
            if abs(mf) > 0:
                fft = fft / mf

            ls_fft = np.interp(self.log_freq_bins, self.lin_freq_bins, fft)
            ls_acf = np.interp(self.log_freq_bins, self.lin_freq_bins, acf)
            # colored_fft = self.colorize(ls_fft, ls_acf)

            self.fft_data = np.roll(self.fft_data, 1, axis=0)
            # self.fft_data[0] = colored_fft

            # generate fake fft data at 1khz
            #self.fft_data[0] = np.zeros_like(self.fft_data[0])
            #idx_1khz = np.searchsorted(self.log_freq_bins, 1000)
            # self.fft_data[0][idx_1khz] = [1, 0, 0]
            #self.fft_data[0] = [1, 0, 0]
            
            self.fft_data = np.zeros_like(self.fft_data)
            for i in range(self.FFT_BINS - 1):
                j = int(np.floor(self.FFT_HISTORY_LENGTH * (i/self.FFT_BINS)))
                self.fft_data[j][i] = [1, 0, 0]
            

            # Plot fft data
            self.im_fft.set_data(self.fft_data)
            self.fig_fft.canvas.draw()
            self.fig_fft.canvas.flush_events()


        plt.pause(0.001)

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