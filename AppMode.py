import matplotlib.pyplot as plt
import numpy as np
import os
import math

os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'
import pygame

def is_raspberry_pi():
    try:
        with open('/proc/device-tree/model', 'r') as f:
            return 'Raspberry Pi' in f.read()
    except FileNotFoundError:
        return False
    return False

if is_raspberry_pi():
    # use framebuffer for display
    os.environ['SDL_VIDEODRIVER'] = 'kmsdrm'
    os.environ["SDL_FBDEV"] = "/dev/fb1"
    # os.environ["SDL_MOUSEDEV"] = "/dev/input/touchscreen"
    os.environ["SDL_MOUSEDEV"] = "/dev/input/event0"
    
    pygame.init()
    screen_width=1920
    screen_height=480
    screen = pygame.display.set_mode((screen_width, screen_height), pygame.FULLSCREEN)
else:
    pygame.init()
    # This is global so that imported modules can access it
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
        super().__init__()
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
        font = pygame.font.Font(None, 36)
        y_major = [y for y in range(-96, 13, 12)]
        y_labels = [str(y) for y in y_major]
        y_minor = [y for y in range(-96, 12, 3) if y not in y_major]
        self.text_size = self.calculate_label_size(y_labels, font)
        self.draw_axis(major = y_major, labels = y_labels, minor = y_minor)

        # time axis isn't super useful for SPL histogram - leave it out
        

    def process_data(self, data):
        # Compute RMS (root mean square) volume of the signal
        rms = np.sqrt(np.mean(data ** 2))
        if np.isnan(rms):
            rms = 0
            print('NaN')
        spl = round(20 * np.log10(np.where(rms < 1.584e-5, 1.584e-5, rms)),1)  # Convert to dB
        self.spl_plot = np.roll(self.spl_plot, -1)
        self.spl_plot[-1] = spl

    def update_plot(self):
        self.blank()
        self.draw_axes()
        for x in range(len(self.spl_plot)-1):
            p0 = (self.scale_xpos(x),   self.scale_ypos(self.spl_plot[x  ]))
            p1 = (self.scale_xpos(x+1), self.scale_ypos(self.spl_plot[x+1]))
            pygame.draw.line(screen, self.plot_color, p0, p1)
        pygame.display.flip()

class ACFMode(BaseMode):
    def __init__(self, windowsize, samplerate):
        super().__init__()
        self.windowsize = windowsize
        self.samplerate = samplerate
        self.acf_plot = np.zeros((screen_width, screen_height - self.major_tick_length,3), dtype=np.uint8)
        self.mx = 1.0
        self.bx = 0
        self.my = float(screen_height / 100)
        self.by = screen_height
        self.plot_color = (12, 200, 255)

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
                
        self.x_major = [(30*2**(f/3)) for f in range(1, 29)]
        self.x_labels = [format_hz(30*2**(f/3)) for f in range(1, 29)]

    def setup_plot(self):
        self.blank()
        self.draw_axes()
        pygame.display.flip()

    def draw_axes(self):
        font = pygame.font.Font(None, 36)

        self.text_size = self.calculate_label_size(self.x_labels, font)
        self.draw_axis(major = self.x_major, labels = self.x_labels, minor = None, orientation='x')

    # progressive FFT
    def process_data(self, data):
       # Initial window size
        initial_window_size = 1024
        num_octaves = 9
        downsample_factor = 2

        # Initialize the combined FFT result
        combined_fft = np.zeros(1920)

        # Process each octave
        for octave in range(num_octaves):
            # Downsample the signal
            downsampled_data = data[::downsample_factor**octave]
            
            # Apply anti-aliasing filter before downsampling (if necessary)
            # downsampled_data = apply_anti_aliasing_filter(downsampled_data)
            
            # Compute FFT
            fft_data = np.fft.rfft(downsampled_data, n=initial_window_size)
            fft_data = np.abs(fft_data)
            fft_data = 20 * np.log10(fft_data / initial_window_size)
            
            # Interpolate FFT data to log-spaced bins
            freq_bins = np.fft.rfftfreq(initial_window_size, 1/self.samplerate)
            log_freq_bins = np.logspace(np.log10(40), np.log10(20480), 1920)
            log_fft_data = np.interp(log_freq_bins, freq_bins, fft_data)
            
            # Combine the FFT results
            combined_fft += log_fft_data

        # Normalize the combined FFT result
        combined_fft = np.interp(combined_fft, (combined_fft.min(), combined_fft.max()), (0, 255)).astype(np.uint8)

        # Compute the autocorrelation on the original signal
        acf = np.correlate(data, data, mode='full')
        acf = acf[len(acf)//2:]
        acf = acf / acf.max()

        # Initialize the acf_plot array
        self.acf_plot = np.zeros((screen_width, screen_height - self.major_tick_length, 3), dtype=np.uint8)

        # Combine FFT and ACF data - FFT is gray, ACF is red
        for x in range(screen_width):
            gray_value = combined_fft[x % len(combined_fft)]
            self.acf_plot[x, :, 0] = gray_value  # Set the red channel for FFT data

        for y in range(screen_height - self.major_tick_length):
            similarity_value = int(np.interp(acf[y % len(acf)], (acf.min(), acf.max()), (0, 255)))
            self.acf_plot[:, y, 2] = similarity_value  # Set the blue channel for autocorrelation data



    def map_db_to_color(self, db):
        pass
        # blue to green to red gradient

    def update_plot(self):
        self.blank()
        self.draw_axes()
        
        # iterate across frequencies in the plot
        for x in range(len(self.freq_data[0:])-1):
            pass    
            # x is self.x_major[0] to self.x_major[-1]
            # y is self.freq_data[x] - color is intensity
        pygame.display.flip()

if __name__ == "__main__":
    # Test SPLMode
    mode = SPLMode()
    mode.setup_plot()
    mode.process_data(np.random.rand(1920))
    mode.update_plot()
    pygame.time.wait(5000)

    mode = ACFMode(1024, 48000)
    mode.setup_plot()
    mode.update_plot()
    pygame.time.wait(5000)
    pygame.quit()
