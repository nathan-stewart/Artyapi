#include "../src/Plotter.h"
#include "test_util.h"
#include <gtest/gtest.h>
#include <vector>
#include <tuple>
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
    size_t history = 480;
    size_t samples = 1920;
    Plotter plotter(samples, history, Plotter::PlotMode::Volume);
    plotter.clear();

    // These aren't actually audio Signals but plot-space test data. They're signals so the
    // plot has a recognizable shape for the test
    Signal vrms = sine_wave(1.0f, 1920.0f, samples) * 54.0f - 42.0f; // vrms should be a sine wave from -96 to +12 over 1920 samples
    Signal vpk = sine_wave(7.2f, 1920.0f, samples) * 5.0f + white_noise(1920) * 4.0f + vrms;
    for (size_t i = 0; i < samples; ++i)
    {
        plotter.plotVolume(vrms[i], vpk[i]);
    }

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
    float f0 = 40.0f;
    float f1 = 20e3f;
    const size_t bins = 1920;
    const size_t history = 480;
    Plotter plotter(1920, 480, Plotter::PlotMode::Spectrum);
    plotter.clear();

    size_t octaves = static_cast<size_t>(ceil(log2(f1/f0)));
    size_t bins_per_tick = static_cast<size_t>(bins / octaves) / 3;
    cout << "Octaves: " << octaves << " bins per tick: " << bins_per_tick << endl;
    for (size_t b = 0; b <= bins; b += bins_per_tick)
    {
        float n = static_cast<float>(b) / static_cast<float>(bins_per_tick);
        cout << b << " " << f0 * pow(2.0f, n/3) << " Hz" << endl;
    }
    vector<pair<float,float>> spectral(history);
    for (size_t t = 0; t < history;  ++t)
    {
        for (size_t b = 0; b < bins; ++b)
        {
            spectral[b] = make_pair((b % bins_per_tick) ? 1.0f : 0.0f, 0.0f);
        }
        plotter.plotSpectrum(spectral);
    }

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
