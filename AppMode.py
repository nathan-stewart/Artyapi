import matplotlib.pyplot as plt
import numpy as np
import os

def is_framebuffer():
    display = os.environ.get('DISPLAY'):
    if not display:
        return True
    elif display != ':0':
        return True
    return False

if is_framebuffer():
    plt.use('Agg')

class BaseMode:
    def __init__(self):
        self.fig, self.ax = None, None

    def setup_plot(self):
        raise NotImplementedError

    def update_plot(self, data):
        raise NotImplementedError

class RTAMode(BaseMode):
    def __init__(self, fft_data):
        super().__init__()
        self.fft_data = fft_data
        self.fig, self.ax_fft, self.ax_vol, self.img_fft = self.setup_plot()

    def setup_plot(self, plotsize):
        fig, (ax_fft, ax_vol) = plt.subplots(1, 1, figsize=plotsize, gridspec_kw={'width_ratios': [SCREEN_WIDTH - 100, 100]})
        fig.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.05)

        # FFT plot
        img_fft = ax_fft.imshow(self.fft_data, aspect='auto', origin='lower', cmap='inferno', extent=[40, 20000, 0, N_TIME_BINS])
        ax_fft.set_xlabel('Hz')
        ax_fft.set_xscale('log', base=2)

        return fig, ax_fft, ax_vol, img_fft

    def update_plot(self, new_fft_data, new_volume_data):
        # FFT data
        self.fft_data = np.roll(self.fft_data, -1, axis=0)
        self.fft_data[-1, :] = new_fft_data

        # Update plot visuals
        self.img_fft.set_data(self.fft_data)
        plt.draw()
        self.fig.canvas.draw_idle()

class SPLMode(BaseMode):
    def __init__(self, vol_data, plotsize):
        super().__init__()
        self.vol_data = vol_data
        self.fig, self.ax_spl, self.img_spl = self.setup_plot(vol_data, plotsize)

    def setup_plot(self, vol_data, plotsize):
        plt.ion()  # Enable interactive mode
        fig, ax_spl = plt.subplots(1, 1, figsize=plotsize)
        fig.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.05)

        # SPL plot
        fig.patch.set_facecolor('black')
        ax_spl.set_facecolor('black')

        # Configure major and minor ticks
        ax_spl.yaxis.set_major_locator(plt.MultipleLocator(1))  # Major ticks every 1 second
        ax_spl.yaxis.set_major_locator(plt.MultipleLocator(12))  # Major ticks every 12db
        ax_spl.tick_params(which='minor', length=4, color='gray')  # Minor ticks customization
        ax_spl.tick_params(which='major', length=8, color='white')  # Major ticks customization
        ax_spl.xaxis.set_tick_params(labelcolor='white')  # Major tick labels color
        ax_spl.yaxis.set_tick_params(labelcolor='white')  # Y-axis tick labels color
        ax_spl.set_xlabel('seconds', color='white')
        ax_spl.set_ylabel('dB', color='white')


        self.img_spl, = ax_spl.plot(vol_data, color='b')  # Fix here by extracting the Line2D object
        ax_spl.set_xlabel('Time (s)')
        ax_spl.set_ylabel('Volume (dB)')
        ax_spl.set_ylim(-96, 12)
        ax_spl.set_yticks(np.arange(-96, 12, 3))
        ax_spl.set_xlim(-2000,2000)
        ax_spl.invert_xaxis()
        ax_spl.axhline(y=-10, color='r')
        self.t = 0.0
        return fig, ax_spl, self.img_spl

    def update_plot(self, vol_data):
        self.img_spl.set_ydata(vol_data)
        self.fig.canvas.flush_events()  # Force an update in interactive mode
        self.fig.canvas.draw_idle()

class AutocorrelationFeedbackMode(BaseMode):
    def __init__(self, autocorr_data):
        super().__init__()
        self.autocorr_data = autocorr_data
        self.fig, self.ax_acorr = self.setup_plot()

    def setup_plot(self, plotsize):
        # Placeholder setup for autocorrelation
        fig, ax_acorr = plt.subplots(1, 1, figsize=plotsize)
        fig.subplots_adjust(left=0.1, right=0.9, top=0.9, bottom=0.1)

        img_acorr, = ax_acorr.plot(self.autocorr_data, color='g')  # Plot autocorrelation
        ax_acorr.set_xlabel('Lag (samples)')
        ax_acorr.set_ylabel('Autocorrelation')
        return fig, ax_acorr, img_acorr

    def update_plot(self, new_autocorr_data):
        self.autocorr_data = new_autocorr_data
        self.img_acorr.set_ydata(self.autocorr_data)
        plt.draw()
        self.fig.canvas.draw_idle()
