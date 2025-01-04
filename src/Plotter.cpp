#include "Plotter.h"
#include <iostream>

Plotter::Plotter(size_t width, size_t height, PlotMode mode, bool rotate)
: width(width), height(height), mode(mode), rotate(rotate), window(nullptr), renderer(nullptr)
{
    initSDL();
}

Plotter::~Plotter()
{
    destroySDL();
}

void Plotter::initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    window = SDL_CreateWindow("Plotter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    if (rotate) {
        SDL_RenderSetLogicalSize(renderer, static_cast<int>(height), static_cast<int>(width));
        SDL_RenderSetViewport(renderer, nullptr);
    }
}

void Plotter::destroySDL()
{
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

void Plotter::plotVolume(const std::vector<float>& vrms, const std::vector<float>& vpk)
{
    clear();
    for (size_t i = 0; i < vrms.size(); ++i) {
        int y = static_cast<int>((vrms[i] + 96) / 108 * static_cast<float>(height));
        drawPixel(static_cast<int>(i), static_cast<int>(height) - y, 0, 255, 0); // Green for vrms

        y = static_cast<int>((vpk[i] + 96) / 108 * static_cast<float>(height));
        drawPixel(static_cast<int>(i), static_cast<int>(height) - y, 255, 0, 0); // Red for vpk
    }
    SDL_RenderPresent(renderer);
}

void Plotter::plotSpectrum(const std::vector<std::vector<float>>& log2fft)
{
    clear();
    for (size_t y = 0; y < log2fft.size(); ++y) {
        for (size_t x = 0; x < log2fft[y].size(); ++x) {
            float intensity = log2fft[y][x];
            Uint8 value = static_cast<Uint8>(intensity * 255);
            drawPixel(static_cast<int>(x), static_cast<int>(y), value, value, value); // Grayscale for intensity
        }
    }
    SDL_RenderPresent(renderer);
}

void Plotter::clear()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void Plotter::drawPixel(int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderDrawPoint(renderer, x, y);
}