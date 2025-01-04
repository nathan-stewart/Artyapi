#pragma once

#include <SDL2/SDL.h>
#include <vector>

class Plotter {
public:
    enum class PlotMode {
        Volume,
        Spectrum
    };

    Plotter(size_t width, size_t height, PlotMode mode, bool rotate = false);
    ~Plotter();

    void plotVolume(const std::vector<float>& vrms, const std::vector<float>& vpk);
    void plotSpectrum(const std::vector<std::vector<float>>& log2fft);
    void clear();

private:
    void initSDL();
    void destroySDL();
    void drawPixel(int x, int y, Uint8 r, Uint8 g, Uint8 b);

    size_t width;
    size_t height;
    PlotMode mode;
    bool rotate;
    SDL_Window* window;
    SDL_Renderer* renderer;
};