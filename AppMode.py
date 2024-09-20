import matplotlib.pyplot as plt
import numpy as np

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
        fig, (ax_fft, ax_vol) = plt.subplots(1, 2, figsize=plotsize, gridspec_kw={'width_ratios': [SCREEN_WIDTH - 100, 100]})
        fig.subplots_adjust(left=0, right=1, top=1, bottom=0)

        # FFT plot
        img_fft = ax_fft.imshow(self.fft_data, aspect='auto', origin='lower', cmap='inferno', extent=[40, 20000, 0, N_TIME_BINS])
        ax_fft.set_xlabel('Hz')
        ax_fft.set_xscale('log', base=2)

        return fig, ax_fft, ax_vol, img_fft

    def update_plot(self, new_fft_data, new_volume_data):
        # FFT data
        self.fft_data = np.roll(self.fft_data, -1, axis=0)
        self.fft_data[-1, :] = new_fft_data

        # Volume data
        self.vol_data = np.roll(self.vol_data, -1)
        self.vol_data[-1] = new_volume_data

        # Update plot visuals
        self.img_fft.set_data(self.fft_data)
        self.img_vol.set_data(self.vol_data[:, np.newaxis])
        plt.draw()
        self.fig.canvas.draw_idle()

class SPLMode(BaseMode):
    def __init__(self, vol_data, plotsize):
        super().__init__()
        self.vol_data = vol_data
        self.fig, self.ax_spl, self.img_spl = self.setup_plot(vol_data, plotsize)

    def setup_plot(self, vol_data, plotsize):
        fig, ax_spl = plt.subplots(1, 1, figsize=plotsize)
        fig.subplots_adjust(left=0, right=1, top=1, bottom=0)

        # SPL plot
        self.img_spl, = ax_spl.plot(vol_data, color='b')  # Fix here by extracting the Line2D object
        ax_spl.set_xlabel('Time (s)')
        ax_spl.set_ylabel('Volume (dB)')
        ax_spl.set_ylim(-96, 12)
        ax_spl.set_yticks(np.arange(-96, 12, 3))
        return fig, ax_spl, self.img_spl

    def update_plot(self, spl):
         # Shift the data to the left and append the new SPL value
        self.vol_data = np.roll(self.vol_data, -1, axis=0)
        self.vol_data[-1] = spl

        self.img_spl.set_ydata(self.vol_data)
        self.img_spl.set_ydata(self.vol_data)
        plt.draw()
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
