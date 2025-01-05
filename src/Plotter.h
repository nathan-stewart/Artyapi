#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <tuple>
#include <boost/circular_buffer.hpp>

std::tuple<int, int, int> HSVtoRGB(float h, float s, float v);

class Plotter {
public:
    enum class PlotMode {
        Volume,
        Spectrum
    };

    Plotter(size_t width, size_t height, PlotMode mode, bool rotate = false);
    ~Plotter();

    void plotVolume(float rms, float pk);
    void plotSpectrum(const std::vector<std::pair<float,float>>& spectrum);
    void clear();

private:
    void initSDL();
    void destroySDL();

    boost::circular_buffer<float> vrms;
    boost::circular_buffer<float> vpk;
    boost::circular_buffer<std::vector<SDL_Color>> spectral;

    size_t width;
    size_t height;
    PlotMode mode;
    bool rotate;
    SDL_Window* window;
    SDL_Renderer* renderer;
};