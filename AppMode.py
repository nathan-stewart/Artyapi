import matplotlib.pyplot as plt
import numpy as np
import os
import math
from scipy.signal import firwin, lfilter, freqz

os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'
import pygame
LOGMIN = 1.584e-5

def is_raspberry_pi():
    try:
        with open('/proc/device-tree/model', 'r') as f:
            return 'Raspberry Pi' in f.read()
    except FileNotFoundError:
        return False
    return False

if is_raspberry_pi():
    os.environ['SDL_VIDEODRIVER'] = 'kmsdrm'
    os.environ["SDL_FBDEV"] = "/dev/fb0"
    rotate = True
else:
    rotate = False

pygame.init()
screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)
screen_width, screen_height = screen.get_size()

class BaseMode:
    major_color = (255, 255, 255)
    minor_color = (127, 127, 127)
    major_tick_length = 8
    minor_tick_length = 4
    major_tick_width = 3
    minor_tick_width = 2

    def __init__(self):
        mx = self.my = 1
        self.bx = self.by = 0
    
    def setup_plot(self):
        raise NotImplementedError

    def update_plot(self, data):
        raise NotImplementedError

    def blank(self):
        screen.fill((0, 0, 0))
            
    def calculate_label_size(self, labels, font):
        width, height = 0, 0
        for label in labels:
            text = font.render(label, True, BaseMode.major_color)
            if text.get_height() > height + 10:
                height = text.get_height() + 10
            if text.get_width() > width:
                width = text.get_width()
        return width, height

    def scale_xpos(self, pos):
        return int(pos * self.mx + self.bx)

    def scale_ypos(self, pos):
        return int(pos * self.my + self.by)
     
    def logscale_xpos(self, pos):
        return int(math.log2(pos) * self.mx + self.bx)
    
    def draw_ticks(self, series=[], orientation='x', mode='major'):
        if mode == 'major':
            length = BaseMode.major_tick_length
            width = BaseMode.major_tick_width
            color = BaseMode.major_color
        else:
            length = BaseMode.minor_tick_length
            width = BaseMode.minor_tick_width
            color = BaseMode.minor_color
        
        if orientation == 'x':
            screen_min = 0
            screen_max = screen_width
        else:
            screen_min = 0
            screen_max = screen_height

        data_min = min(series)
        data_range = max(series) - data_min
        screen_range = screen_max - screen_min
        
        for tick in series:
            if orientation == 'x':
                x = self.scale_xpos(tick)
                y = 0
                start_pos = (x, y)
                end_pos = (x, y + length)
            else:
                x = 0
                y = self.scale_ypos(tick)
                start_pos = (x, y)
                end_pos = (x - length, y)
            pygame.draw.line(screen, color, start_pos, end_pos, width)

    def draw_axis(self, labels=None, major=None, minor=None, orientation='x'):
        if labels:
            if len(labels) != len(major):
                raise ValueError('Length of labels must match length of major ticks')
        
        # Draw major ticks
        if major:
            self.draw_ticks(major, orientation, 'major')
        
        # Draw minor ticks
        if minor:
            self.draw_ticks(minor, orientation, 'minor')

class SPLMode(BaseMode):
    def __init__(self):
        global rotate
        super().__init__()
        if rotate:
            self.spl_plot = np.zeros(screen_height)
            self.mx = -1
            self.bx = screen_height
            self.my = float(screen_width / (12 + 96))
            self.by = screen_width - 12 * self.my
        else:
            self.spl_plot = np.zeros(screen_width)
            self.mx = 1.0
            self.bx = 0
            self.my = float(screen_height / (12 + 96))
            self.by = screen_height - 12 * self.my
        self.plot_color = (12, 200, 255)
        
    def setup_plot(self):
        self.blank()
        self.draw_axes()
        pygame.display.flip()

    def draw_axes(self):
        global rotate
        font = pygame.font.Font(None, 36)
        y_major = [y for y in range(-96, 13, 12)]
        y_labels = [str(y) for y in y_major]
        y_minor = [y for y in range(-96, 12, 3) if y not in y_major]
        self.text_size = self.calculate_label_size(y_labels, font)
        self.draw_axis(major = y_major, labels = y_labels, minor = y_minor, orientation='y' if rotate else 'x')

    def process_data(self, data):
        global rotate
        # Compute RMS (root mean square) volume of the signal
        rms = np.sqrt(np.mean(data ** 2))
        if np.isnan(rms):
            rms = 0
        spl = round(20 * np.log10(np.where(rms < LOGMIN, LOGMIN, rms)),1)  # Convert to dB

        # roll data and push new volume
        self.spl_plot = np.roll(self.spl_plot, -1)            
        self.spl_plot[-1] = spl

    def update_plot(self):
        global rotate
        self.blank()
        self.draw_axes()
        for x in range(len(self.spl_plot)-1):
            if rotate:
                p0 = (self.scale_ypos(self.spl_plot[x  ]), self.scale_xpos(x))
                p1 = (self.scale_ypos(self.spl_plot[x+1]), self.scale_xpos(x+1))
            else:
                p0 = (self.scale_xpos(x),   self.scale_ypos(self.spl_plot[x  ]))
                p1 = (self.scale_xpos(x+1), self.scale_ypos(self.spl_plot[x+1]))
            pygame.draw.line(screen, self.plot_color, p0, p1)
        pygame.display.flip()

def get_filter_freq(filter, samplerate):
    w,h = freqz(filter)
    gain_db = 20 * np.log10(np.abs(h))

    # Find the frequency where the gain drops to -3 dB
    corner_freq_index = np.where(gain_db <= -3)[0][0]
    corner_freq = w[corner_freq_index] * (0.5 * samplerate) / np.pi  # Assuming a sample rate of 48000 Hz
    return corner_freq

class ACFMode(BaseMode):
    def __init__(self, windowsize, samplerate):
        global rotate
        super().__init__()
        self.windowsize = windowsize
        self.samplerate = samplerate
        if rotate:
            self.acf_plot = np.zeros((screen_width, screen_height - self.major_tick_length,3), dtype=np.uint8)
            self.plot_surface = pygame.Surface((screen_width, screen_height - self.major_tick_length))
        else:
            self.acf_plot = np.zeros((screen_height, screen_width - self.major_tick_length,3), dtype=np.uint8)
            self.plot_surface = pygame.Surface((screen_height, screen_width - self.major_tick_length))

        self.plot_color = (12, 200, 255)
        self.num_folds = 5
        self.lpf = [ firwin(101, 0.83*2**-(n)) for n in range(0,self.num_folds)]
        
        def format_hz(hz):
            if hz < 1000:
                return f'{hz:.0f}'
            else:
                k = int(hz) // 1000
                c = (int(hz) % 1000) // 100
                if c == 0:
                    return f'{k}k'
                elif k < 10:
                    return f'{k}k{c:02d}'
                else:
                    return f'{k}k{c:01d}'

        
        # last tick is 16.3k but the plot goes to 20k to allow label space
        self.x_major = [(40*2**(f/2)) for f in range(0, 18)]
        self.x_labels = [format_hz(f) for f in self.x_major]
        self.x_minor= [(self.x_major[0]*2**(f/6)) for f in range(0, 54) if f % 3 != 0]
        self.my = 1
        if rotate:
            self.by = screen_width - self.major_tick_length
            self.mx = screen_height / (math.log2(self.x_minor[-1])-math.log2(self.x_major[0]))
        else:
            self.by = screen_height - self.major_tick_length
            self.mx = screen_width / (math.log2(self.x_minor[-1])-math.log2(self.x_major[0]))
        self.bx = -self.mx * math.log2(self.x_major[0])
        
    def scale_xpos(self, pos):
        return int(math.log2(pos) * self.mx + self.bx)
    
    def setup_plot(self):
        self.blank()
        self.draw_axes()
        pygame.display.flip()

    def draw_axes(self):
        global rotate
        font = pygame.font.Font(None, 36)

        self.text_size = self.calculate_label_size(self.x_labels, font)
        self.draw_axis(major = self.x_major, labels = self.x_labels, minor = self.x_minor, orientation='y' if rotate else 'x')

    # progressive FFT
    def process_data(self, data):
        global rotate

       # Initial window size
        initial_window_size = 256
        # Initialize the combined FFT result
        combined_fft = None

        # Process each octave
        for fold in range(self.num_folds):
            # Apply anti-aliasing filter before downsampling (if necessary)
            filtered_data =  lfilter(self.lpf[fold], 1, data)

            # Downsample the signal
            downsampled_data = filtered_data[::2**fold]
                        
            # Compute FFT
            fft_data = np.fft.rfft(downsampled_data, n=initial_window_size)
            fft_data = np.abs(fft_data)
            fft_data = np.maximum(fft_data, LOGMIN)
            fft_data = 20 * np.log10(fft_data / initial_window_size)
            
            # Interpolate FFT data to log-spaced bins
            freq_bins = np.fft.rfftfreq(initial_window_size, 1/self.samplerate)
            if rotate:
                log_freq_bins = np.logspace(np.log2(self.x_major[0]), np.log2(self.x_minor[-1]), screen_height, base=2)
            else:
                log_freq_bins = np.logspace(np.log2(self.x_major[0]), np.log2(self.x_minor[-1]), screen_width, base=2)
            log_fft_data = np.interp(log_freq_bins, freq_bins, fft_data)
            if combined_fft is None:
                combined_fft = np.zeros_like(log_fft_data)
            # Add the FFT data to the combined FFT result
            combined_fft = np.average([combined_fft, log_fft_data], axis=0)
                  
        # scale data to input range
        combined_fft = np.clip(combined_fft, -96, 12)
        combined_fft = (combined_fft + 96) * (255 / 108)
        
        if rotate:
            combined_fft = combined_fft.reshape((1,screen_height))
            self.acf_plot = np.roll(self.acf_plot, -1, axis=0)
            self.acf_plot[-1, :, :] = np.stack([combined_fft]*3, axis=0).T
        else:
            self.acf_plot = np.roll(self.acf_plot, -1, axis=1)
            self.acf_plot[:, -1, :] = np.stack([combined_fft]*3, axis=-1)
    
    def update_plot(self):
        self.blank()
        self.draw_axes()
        pygame.surfarray.blit_array(self.plot_surface, self.acf_plot)
        screen.blit(self.plot_surface, (0, self.major_tick_length))
        pygame.display.flip()

if __name__ == "__main__":
    # Test SPLMode
    rotate = True
    mode = SPLMode()
    mode.setup_plot()

    numsamples = 1920
    min_db = -96
    max_db = 12
    sawtooth = np.linspace(min_db, max_db, numsamples)
    sawtooth = np.tile(sawtooth, 10)

    for i in range(len(sawtooth) // numsamples):
        fake_data = sawtooth[i*numsamples:(i+1)*numsamples]
        mode.process_data(fake_data)
        mode.update_plot()

    # mode = ACFMode(1024, 48000)
    # for i in range(480):
    #     mode.process_data(np.random.rand(1920))
    #     mode.update_plot()
    #     pygame.time.wait(100)
    pygame.time.wait(5000)
    pygame.quit()
