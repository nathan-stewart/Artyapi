#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import os
import math
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

LOGMIN = 1.584e-5
LOGMAX = 10**5.25

pygame.init()
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
        self.plot_color = (12,200,255)
        self.plot_surface = pygame.Surface((self.plot_width, self.plot_height))

        self.mx = self.my = 1
        self.bx = self.by = 0

    def setup_plot(self):
        screen.fill((0,0,0)) # blank doesn't clear the screen outside of plot_surface
        self.blank()
        self.draw_axes()
        pygame.display.flip()

    def blank(self):
        screen.fill((0,0,0)) # blank doesn't clear the screen outside of plot_surface


    def update_plot(self):
        self.blank()
        pygame.draw.rect(self.plot_surface, (255, 255, 255), (0, 0, self.plot_width, self.plot_height), 1)  # Draw only the outline
        self.draw_axes()
        screen.blit(self.plot_surface, (self.x_margin, self.y_margin))
        pygame.display.flip()

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

    def draw_ticks(self, series=[], orientation='x', mode='major'):
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
        self.plot_color = (12, 200, 255)
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
        spl = round(20 * np.log10(np.where(rms < LOGMIN, LOGMIN, rms)),1)  # Convert to dB

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
    def __init__(self, windowsize=4096, samplerate=48000, numfolds=4):
        super().__init__()
        self.samplerate = samplerate
        self.acf_plot = np.zeros((self.plot_width, self.plot_height,3), dtype=np.uint8)
        self.plot_color = (12, 200, 255)

        # FFT parameters
        self.window_size = 2**(int(math.log2(windowsize)))
        self.numfolds(numfolds)
        self.previous = None

        # last tick is 16.3k but the plot goes to 20k to allow label space
        self.x_major = [(40*2**(f/2)) for f in range(0, 18)] + [20e3]
        self.x_labels = [format_hz(f) for f in self.x_major]
        self.x_minor= [(self.x_major[0]*2**(f/6)) for f in range(0, 54) if f % 3 != 0]
        self.text_size = self.calculate_label_size(self.x_labels)

        self.my = -1
        self.by = 0
        self.mx = self.plot_width / (math.log2(self.x_major[-1])-math.log2(self.x_major[0]))
        self.bx = -self.mx * math.log2(self.x_major[0])
        self.log_freq_bins = np.logspace(np.log2(self.x_major[0]), np.log2(self.x_major[-1]), self.plot_width, base=2)

    def numfolds(self, f):
        f = int(f)
        self.num_folds = min(max(f,0), 8)
        self.lpf = [ firwin(1024, 0.999*2**-(n)) for n in range(0,self.num_folds+1)]
        self.history = [np.zeros(self.window_size * 2) for _ in range(self.num_folds + 1)]
        self.window = get_window('hann', self.window_size)

    def scale_xpos(self, pos):
        return int(math.log2(pos) * self.mx + self.bx)

    def draw_axes(self):
        self.draw_axis(major = self.x_major, labels = self.x_labels, minor = self.x_minor, orientation='x')

    def update_history(self, data):
        # Roll the history buffer and push new data
        roll_len = min(len(data), self.window_size * 2)
        self.history[0] = np.roll(self.history[0], -roll_len)        
        self.history[0][-roll_len:] = data[-roll_len:]

        # Apply low-pass filters to the history buffer and downsample
        for h in range(1,len(self.history)):
            filtered = lfilter(self.lpf[h], 1, self.history[h-1])
            downsampled = np.mean(filtered.reshape(-1, 2), axis=1)
            self.history[h] = np.roll( self.history[h], -self.window_size)
            self.history[h][-self.window_size:] = downsampled
        
    # progressive FFT
    def process_data(self, data):
        global LOGMIN, LOGMAX

        self.update_history(data)

        # Initialize the combined FFT result
        combined_fft = np.zeros(self.window_size // 2 + 1)

        # Interpolate FFT data to log-spaced bins
        freq_bins = np.fft.rfftfreq(self.window_size, 1/self.samplerate)

        # Process each octave
        for fold in range(self.num_folds + 1):
            # Apply window function
            windowed_data = self.history[fold][-self.window_size:] * self.window

            # Compute FFT
            fft_data = np.fft.rfft(windowed_data, n=self.window_size)
            fft_data = np.abs(fft_data)
            fft_data = np.clip(fft_data, LOGMIN, LOGMAX)

            # Normalize the FFT data
            normalized_fft = 20 * np.log10(np.clip(combined_fft / self.window_size, LOGMIN, LOGMAX))
            log_fft_data = np.interp(self.log_freq_bins, freq_bins, normalized_fft)

            if fold == 0:
                combined_fft = fft_data
            else:
                # Replace lower resolution values with higher resolution FFT data
                lower_half = len(fft_data) // 2
                combined_fft[0:lower_half] = fft_data[0:lower_half]
        if np.max(fft_data) > 12.0:
            print_fft_summary("fft", fft_data, freq_bins)

        # Average the combined FFT result
        combined_fft /= (self.num_folds + 1)

        # scale data to input range
        combined_fft = np.clip((log_fft_data + 96) * (255 / 108), 0, 255)

        self.acf_plot = np.roll(self.acf_plot, -1, axis=1)
        self.acf_plot[:, -1, :] = np.stack([combined_fft]*3, axis=-1)

        # Draw the ACF plot to plot_surface
        self.blank()
        self.acf_plot[:, -1, :] = np.stack([log_fft_data]*3, axis=-1)
        pygame.surfarray.blit_array(self.plot_surface, self.acf_plot)

def test_spl():
    global start_time

    # Test SPLMode
    mode = SPLMode()
    mode.setup_plot()
    start_time = time.time()
    elapsed = time.time() - start_time
    duration = 6.0
    sine_1khz = sine_generator(1e3, 12)

    while elapsed < duration:
        elapsed = time.time() - start_time
        # ramp sine from -96db to 12db linearly over duration
        data = next(sine_1khz) * 10**(elapsed/duration * 12/20)
        mode.process_data(data)
        mode.update_plot()

def test_acf():
    def generate_acf_data():
        global start_time
        samplerate = 48000
        duration = 2**16 / samplerate
        t = np.linspace(0, duration, int(samplerate * duration), endpoint=False)
        data = np.zeros(t.shape)

        def sine_wave(frequency, db):
            return 10**(db/20) * np.sin(2 * np.pi * frequency * t)

        now = time.time()
        elapsed = now - start_time
        sweep_time = 4.0
        f0 = 40
        f1 = 20e3
        if elapsed <= sweep_time:
            # sweep sine wave
            f = f0 + (f1 - f0) * ((sweep_time - elapsed)/sweep_time)
            data += sine_wave(f, 12)
        else:
            bin_centers = [f for f in range(1, 8)]
            # what is the frequency discrimination of each fold?
            for f in bin_centers:
                f0 = 40*2**f
                f1 = f0 + 1*2**f
                data += sine_wave(f0, 12) / 2
                data += sine_wave(f1, 12) / 2
        return data

    global start_time
    mode = ACFMode(windowsize=4096, samplerate=48000)
    mode.setup_plot()
    start_time = time.time()
    mode.acf_plot = np.zeros((mode.plot_width, mode.plot_height,3), dtype=np.uint8)
    elapsed = time.time() - start_time
    previous = None
    while elapsed < 24.0:
        elapsed = time.time() - start_time
        if elapsed > 6.0:
            f = int((elapsed - 6.0) / 2.0)
            if f != previous:
                mode.numfolds(f)
                previous = f
        mode.process_data(generate_acf_data())
        mode.update_plot()

if __name__ == "__main__":
    import sys

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
