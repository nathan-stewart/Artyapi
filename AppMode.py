#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import os
import math
from fftw3 import *
from scipy.signal import firwin, lfilter, get_window
from util import *
import time

if is_raspberry_pi():
    os.environ['SDL_VIDEODRIVER'] = 'kmsdrm'
    os.environ["SDL_FBDEV"] = "/dev/fb0"
    rotate = True
else:
    rotate = False

os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'
import pygame

LOGMIN = 10**(-96/20)
LOGMAX = 10**(12/20)

screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)
screen_width, screen_height = screen.get_size()

if rotate:
    screen_width, screen_height = screen_height, screen_width

class BaseMode:
    major_color = (255, 255, 255)
    minor_color = (127, 127, 127)
    major_tick_length = 8
    minor_tick_length = 4
    major_tick_width = 3
    minor_tick_width = 2

    def __init__(self):
        self.font = pygame.font.Font(None, 24)
        sample_label = self.calculate_label_size(['20k'])
        self.x_margin = sample_label[0] + 2 * BaseMode.major_tick_length
        self.y_margin = sample_label[1] + 2 * BaseMode.major_tick_length
        self.plot_width = screen_width - 2 * self.x_margin
        self.plot_height = screen_height - 2 * self.y_margin
        self.plot_color = (255, 0, 255)
        self.plot_surface = pygame.Surface((self.plot_width, self.plot_height))

        self.mx = self.my = 1
        self.bx = self.by = 0

    def setup_plot(self):
        screen.fill((0,0,0)) # blank doesn't clear the screen outside of plot_surface
        self.blank()
        self.draw_axes()
        # pygame.display.flip()

    def blank(self):
        screen.fill((0,0,0)) # blank doesn't clear the screen outside of plot_surface


    def update_plot(self):
        self.blank()
        pygame.draw.rect(self.plot_surface, self.plot_color, (0, 0, self.plot_width, self.plot_height), 1)  # Draw only the outline
        self.draw_axes()
        screen.blit(self.plot_surface, (self.x_margin, self.y_margin))
        # pygame.display.flip()

    def calculate_label_size(self, labels):
        width, height = 0, 0
        for label in labels:
            text = self.font.render(label, True, BaseMode.major_color)
            if text.get_height() > height + 10:
                height = text.get_height() + 10
            if text.get_width() > width:
                width = text.get_width()
        return width, height

    def scale_xpos(self, pos):
        return self.x_margin + int(pos * self.mx + self.bx)

    def scale_ypos(self, pos):
        return self.y_margin + int(pos * self.my + self.by)

    def draw_ticks(self
    , series=[], orientation='x', mode='major'):
        if mode == 'major':
            length = self.major_tick_length
            width = self.major_tick_width
            color = self.major_color
        else:
            length = self.minor_tick_length
            width = self.minor_tick_width
            color = self.minor_color

        if orientation == 'x':
            plot_min = 0
            plot_max = self.plot_width
        else:
            plot_min = 0
            plot_max = self.plot_width

        for tick in series:
            if orientation == 'x':
                x = self.x_margin + self.scale_xpos(tick)
                y = self.y_margin - self.major_tick_length
                start_pos = (x, y)
                end_pos = (x, y + length)
            else:
                x = self.x_margin
                y = self.scale_ypos(tick)
                start_pos = (x, y)
                end_pos = (x - length, y)
            pygame.draw.line(screen, color, start_pos, end_pos, width)

    def draw_labels(self, labels, series, orientation='x'):
        if len(labels) != len(series):
            raise ValueError('Length of labels must match length of major ticks')

        for label, value in zip(labels, series):
            text = self.font.render(label, True, BaseMode.major_color)
            if orientation == 'x':
                    x = self.scale_xpos(value) + self.x_margin - self.text_size[0]//3
                    y = self.y_margin - 1.5*self.major_tick_length - self.text_size[1]
            else:
                for i, label in enumerate(labels):
                    x = self.x_margin - self.text_size[0] - 1.5*self.major_tick_length
                    y = self.scale_ypos(value) - self.text_size[1]//3
            screen.blit(text, (x, y))

    def draw_axis(self, labels=None, major=None, minor=None, orientation='x'):
        # Draw major ticks
        if major:
            self.draw_ticks(major, orientation, 'major')

        # Draw minor ticks
        if minor:
            self.draw_ticks(minor, orientation, 'minor')

        if labels:
            self.draw_labels(labels, major, orientation)

class SPLMode(BaseMode):
    def __init__(self):
        super().__init__()
        self.mx = 1.0
        self.bx = self.x_margin
        self.my = -(self.plot_height)/(12 + 96)
        self.by = -12 * self.my
        self.plot_color = (0, 200, 200)
        self.y_major = [y for y in range(-96, 13, 12)]
        self.y_labels = [f'{y:+d}' for y in self.y_major]
        self.y_labels[-2] = " 0" # fix intentionally broken python behavior
        self.text_size = self.calculate_label_size(self.y_labels)
        self.y_minor = [y for y in range(-96, 12, 3) if y not in self.y_major]
        self.spl_plot = np.array([-96] * self.plot_width)
        self.plot_surface.set_colorkey((0, 0, 0))  # Use a transparent color

    def draw_axes(self):
        self.draw_axis(major = self.y_major, labels = self.y_labels, minor = self.y_minor, orientation='y')

    def process_data(self, data):
        # Compute RMS (root mean square) volume of the signal
        rms = np.sqrt(np.mean(data ** 2))
        if np.isnan(rms):
            rms = 0
        rms = max(LOGMIN, min(rms, LOGMAX))
        spl = round(20 * np.log10(rms), 1)  # Convert to dB

        # roll data and push new volume
        self.spl_plot = np.roll(self.spl_plot, -1)
        self.spl_plot[-1] = spl

        # draw the SPL plot to plot_surface
        self.plot_surface.fill((0,0,0))

        for x in range(len(self.spl_plot) -1):
            p0 = (self.scale_xpos(x),   self.scale_ypos(self.spl_plot[x  ]))
            p1 = (self.scale_xpos(x+1), self.scale_ypos(self.spl_plot[x+1]))
            pygame.draw.line(self.plot_surface, self.plot_color, p0, p1)

            # draw pixels instead of lines
            #self.plot_surface.set_at(p0, self.plot_color)

class ACFMode(BaseMode):
    def __init__(self, windowsize=16384, samplerate=48000):
        super().__init__()
        self.samplerate = samplerate
        self.acf_plot = np.zeros((self.plot_width, self.plot_height,3), dtype=np.uint8)
        self.plot_color = (0, 0, 255)

        # last tick is 16.3k but the plot goes to 20k to allow label space
        self.x_major = [(40*2**(f/2)) for f in range(0, 18)] + [20e3]
        self.x_labels = [format_hz(f) for f in self.x_major]
        self.x_minor= [(self.x_major[0]*2**(f/6)) for f in range(0, 54) if f % 3 != 0]
        self.text_size = self.calculate_label_size(self.x_labels)

        self.my = -1
        self.by = 0
        self.mx = self.plot_width / (math.log2(self.x_major[-1])-math.log2(self.x_major[0]))
        self.bx = -self.mx * math.log2(self.x_major[0])

        # FFT parameters
        self.window_size = 2**(int(math.log2(windowsize)))
        self.linear_freq_bins = np.fft.rfftfreq(self.window_size, 1 / self.samplerate)
        self.log_freq_bins = np.logspace(np.log2(self.x_major[0]), np.log2(self.x_major[-1]), self.plot_width, base=2)
        self.fake = False

        self.history = np.zeros(self.window_size)
        self.hpf = firwin(1023, 2*40/self.samplerate, pass_zero=False)
        self.lpf = firwin(1023, 2*20e3/self.samplerate, pass_zero=True)
        self.window = get_window('hann', self.window_size)
        acf_hpf_idx = np.argmax(self.linear_freq_bins > 200)
        f0 = acf_hpf_idx // 2
        self.acf_mask = np.array([
            0.0                       if f < f0 else 
            (f - f0) / (f0) if f < acf_hpf_idx  else 
            1.0
            for f in range(len(self.linear_freq_bins)//2)])

    def scale_xpos(self, pos):
        return int(math.log2(pos) * self.mx + self.bx)

    def draw_axes(self):
        self.draw_axis(major = self.x_major, labels = self.x_labels, minor = self.x_minor, orientation='x')

    def update_history(self, data):
        # Roll the history buffer and push new data
        roll_len = min(len(data), self.window_size)
        if roll_len > 0:
            self.history = np.roll(self.history, -roll_len)
            self.history[-roll_len:] = data[-roll_len:]

    def process_data(self, data):
        def fake_fft():
            # generate fake data
            normalized_fft = np.zeros(self.window_size // 2 + 1)
            for f in  sorted([40 * 2**i for i in range(0,9)] + [43 * 2**i for i in range(0,9)]):
                index = int(f * self.window_size / self.samplerate)
                normalized_fft[index] = self.window_size
            return normalized_fft        
        
        self.update_history(data)

        # Apply the window to the history buffer
        windowed_data = self.history[-self.window_size:] * self.window

        # Apply high-pass filter to the windowed data
        filtered = lfilter(self.hpf, 1, windowed_data)

        # Apply low-pass filters to the history buffer
        filtered = lfilter(self.lpf, 1, filtered)

        if self.fake:
            fft_data = fake_fft()
        else:
            fft_data = np.abs(fftw_rfft(windowed_data))

        normalized_fft  = np.clip(fft_data / self.window_size, 0, 1)

        # Interpolate the FFT data to the log frequency bins
        interpolated_fft = np.interp(self.log_freq_bins, self.linear_freq_bins, normalized_fft)

        # Convert to log scale
        log_fft_data = np.log2(1 + 100 * interpolated_fft)/6.64
        
        # autocorrelate and normalize
        autocorr = np.fft.ifft(np.abs(np.fft.fft(log_fft_data))**2).real
        autocorr = autocorr[:len(autocorr)//2] # keep only positive lags
        
        # roll off low frequency correlation
        #autocorr *= self.acf_mask
        autocorr = np.clip(autocorr, 0, 1)
        
        # suppress bins with low correlation
        #autocorr = np.where(autocorr > 0.5, autocorr, 0)

        
        # map autocorrelation to log_bins so we can combine it with fft
        autocorr = np.interp(self.log_freq_bins, np.linspace(0, len(autocorr), len(autocorr)), autocorr)

        # roll data and push new volume
        self.acf_plot = np.roll(self.acf_plot, -1, axis=1)
        self.acf_plot[:, -1, :] = colorize(log_fft_data, autocorr)

        # Draw the ACF plot to plot_surface
        self.blank()
        pygame.surfarray.blit_array(self.plot_surface, self.acf_plot)

def test_spl():
    global start_time, LOGMIN, LOGMAX

    # Test SPLMode
    mode = SPLMode()
    mode.setup_plot()
    duration = 10.0
    sine_1khz = sine_generator(frequency = 1e3)
    start_time = time.time()
    elapsed = 0
    while elapsed < duration:
        elapsed = time.time() - start_time
        # ramp sine volume from -96db to 12db linearly over duration
        scale_factor = 10 ** ((108 * (0.9 * elapsed/duration) -96)/20)
        scale_factor = min(LOGMAX, max(LOGMIN, scale_factor))
        mode.process_data(next(sine_1khz) * scale_factor)
        mode.update_plot()
        pygame.display.flip()

def test_acf():
    global start_time
    duration = 8.0
    plot_color = make_color_palette(1)
    mode = ACFMode(windowsize=32768, samplerate=48000)
    mode.setup_plot()
    
    # Sweep Test
    mode.history = np.zeros(mode.window_size)
    mode.plot_color = plot_color[0]
    mode.fake = False
    start_time = time.time()
    elapsed = time.time() - start_time
    sweep = sweep_generator(40, 20e3, duration, 12.0)
    while elapsed < duration:
        elapsed = time.time() - start_time
        data = next(sweep)
        mode.process_data(data)
        mode.update_plot()
        pygame.display.flip()

    start_time = time.time()
    elapsed = 0

    # Resolution test
    discriminator = resolution_generator()
    mode.history = np.zeros(mode.window_size)
    mode.plot_color = plot_color[0]
    mode.fake = True
    while elapsed < 2.0:
        elapsed = time.time() - start_time
        mode.process_data(next(discriminator))
        mode.update_plot()
        pygame.display.flip()

if __name__ == "__main__":
    import sys
    pygame.init()

    tests = [test_spl, test_acf]
    if len(sys.argv) > 1:
        if sys.argv[1] == 'spl':
            tests = [test_spl]
        elif sys.argv[1] == 'acf':
            tests = [test_acf]

    for test in tests:
        test()
        wait_for_keypress()
    pygame.quit()
