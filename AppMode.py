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

LOGMIN = 10**(-96/20)
LOGMAX = 10**(12/20)

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
    def __init__(self, windowsize=8192, samplerate=48000, numfolds=4):
        super().__init__()
        self.samplerate = samplerate
        self.acf_plot = np.zeros((self.plot_width, self.plot_height,3), dtype=np.uint8)
        self.plot_color = (12, 200, 255)

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
        self.numfolds(numfolds)
        self.previous = None
        self.freq_bins = np.fft.rfftfreq(self.window_size, 1/self.samplerate)
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
        if roll_len > 0:
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
        # Something is definitely up with folding
        # sweep is disontinuous
        # I think the issue is related to half of linear vs half of log but I can't
        # articulate that right now
        # we need to come up with a way to test the fft folding with synthetic ffts
        self.update_history(data)

        # Initialize the combined FFT result
        combined_fft = np.zeros(self.window_size // 2 + 1)

        # Process each octave
        for fold in range(self.num_folds + 1):
            # Apply window function
            windowed_data = self.history[fold][-self.window_size:] * self.window

            # Compute FFT
            fft_data = np.fft.rfft(windowed_data, n=self.window_size)
            fft_data = np.abs(fft_data)

            # Normalize the FFT data
            normalized_fft = 20 * np.log10(np.clip(fft_data / self.window_size, LOGMIN, 1.0))

            # generate fake data
            normalized_fft = np.full(len(normalized_fft), -96)
            for f in  sorted([40 * 2**i for i in range(0,9)] + [43 * 2**i for i in range(0,9)]):
                index = int(f * self.window_size / self.samplerate)
                normalized_fft[index] = 1.0

            # Replace lower resolution values with higher resolution FFT data
            lower_half = slice(0, int(len(normalized_fft) * (2**-fold)))
            combined_fft[lower_half] = normalized_fft[lower_half]
            
        log_fft_data = np.interp(self.log_freq_bins, self.freq_bins, combined_fft)
        
        # scale data to input range
        log_fft_data = np.clip((log_fft_data + 96) * (255 / 108), 0, 255)
        self.acf_plot = np.roll(self.acf_plot, -1, axis=1)
        self.acf_plot[:, -1, :] = np.stack([log_fft_data]*3, axis=-1)

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

def test_acf():
    global start_time
    mode = ACFMode(windowsize=4096, samplerate=48000, numfolds = 4)
    mode.setup_plot()
    start_time = time.time()
    mode.acf_plot = np.zeros((mode.plot_width, mode.plot_height,3), dtype=np.uint8)
    elapsed = time.time() - start_time
    duration = 4.0
    sweep = sweep_generator(40, 20e3, duration, 12.0)
    while elapsed < duration:
        elapsed = time.time() - start_time
        data = next(sweep)
        mode.process_data(data)
        mode.update_plot()

    perfold = 4.0
    num_folds = 4
    start_time = time.time()    
    discriminator = resolution_generator()
    previous = None
    for fold in range(num_folds):
        elapsed = 0
        while elapsed < perfold:
            elapsed = time.time() - start_time
            folding = int(num_folds * elapsed / duration)
            if folding != previous:
                mode.numfolds(folding)
                previous = folding
            mode.process_data(next(discriminator))
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
