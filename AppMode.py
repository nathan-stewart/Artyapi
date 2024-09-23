import matplotlib.pyplot as plt
import numpy as np
import os
import pygame

# This is global so that imported modules can access it
pygame.init()
screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)

class BaseMode:
    major_color = (255, 255, 255)
    minor_color = (127, 127, 127)
    major_tick_length = 8
    minor_tick_length = 4
    major_tick_width = 3
    minor_tick_width = 2

    def __init__(self):
        self.mx = self.my = 1
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
            screen_max = screen.get_width()
        else:
            screen_min = 0
            screen_max = screen.get_height()

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
    def __init__(self, vol_data):
        super().__init__()
        self.vol_data = vol_data
        self.mx = 1.0
        self.bx = 0
        self.my = float(screen.get_height() / (12 + 96))
        self.by = screen.get_height() - 12 * self.my
        self.plot_color = (12, 200, 255)
        
    def setup_plot(self, vol_data):
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
        

    def update_plot(self, vol_data):
        self.draw_axes()
        for x in range(len(vol_data)-1):
            p0 = (self.scale_xpos(x),   self.scale_ypos(vol_data[x  ]))
            p1 = (self.scale_xpos(x+1), self.scale_ypos(vol_data[x+1]))
            pygame.draw.line(screen, self.plot_color, p0, p1)
            
        pygame.display.flip()

