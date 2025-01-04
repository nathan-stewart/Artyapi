#include "../src/Plotter.h"
#include "test_util.h"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace std;
// Plotter(size_t width, size_t height, PlotMode mode, bool rotate = false);
// ~Plotter();

// void plotVolume(const std::vector<float>& vrms, const std::vector<float>& vpk);
// void plotSpectrum(const std::vector<std::vector<float>>& log2fft);
// void clear();


TEST(Plotter, VolumePlot)
{
    Plotter plotter(1920, 480, Plotter::PlotMode::Volume);
    plotter.clear();

    Signal vrms = sine_wave(1.0f, 1920.0f, 1920) * 54.0f - 42.0f; // vrms should be a sine wave from -96 to +12 over 1920 samples
    Signal vpk = sine_wave(7.2f, 1920.0f, 1920)*5.0f + white_noise(1920) * 4.0f + vrms;
    plotter.plotVolume(vrms, vpk);

    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        SDL_Delay(100); // Add a small delay to avoid busy-waiting
    }
}

TEST(Plotter, Spectrum)
{
    Plotter plotter(1920, 480, Plotter::PlotMode::Spectrum);
    plotter.clear();

    std::vector<std::vector<float>> log2fft(480);

    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        SDL_Delay(100); // Add a small delay to avoid busy-waiting
    }
}
