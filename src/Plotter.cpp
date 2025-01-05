#include "Plotter.h"
#include <iostream>

using namespace std;
tuple<int, int, int> HSVtoRGB(float h, float s, float v)
{
    float r, g, b;
    float hf = h / 60.0f;
    int i = static_cast<int>(hf);
    float f = hf - static_cast<float>(i);
    float pv = v * (1 - s / 255.0f);
    float qv = v * (1 - s / 255.0f * f);
    float tv = v * (1 - s / 255.0f * (1 - f));

    switch (i) {
        case 0:
            r = v;
            g = tv;
            b = pv;
            break;
        case 1:
            r = qv;
            g = v;
            b = pv;
            break;
        case 2:
            r = pv;
            g = v;
            b = tv;
            break;
        case 3:
            r = pv;
            g = qv;
            b = v;
            break;
        case 4:
            r = tv;
            g = pv;
            b = v;
            break;
        case 5:
        default:
            r = v;
            g = pv;
            b = qv;
            break;
    }
    return tuple<int, int, int>(static_cast<int>(r * 255.0f), static_cast<int>(g * 255.0f), static_cast<int>(b * 255.0f));
}

Plotter::Plotter(size_t width, size_t height, PlotMode mode, bool rotate)
: width(width), height(height), mode(mode), rotate(rotate), window(nullptr), renderer(nullptr)
{
    initSDL();

    vrms.resize(width, -96.0f);
    vpk.resize(width, -96.0f);
    spectral.resize(height, std::vector<SDL_Color>(width, {0, 0, 0, 255}));
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

void Plotter::plotVolume(float rms, float pk)
{
    const float ymin = -96.0f;
    const float ymax = 12.0f;
    vrms.push_back(rms);
    vpk.push_back(pk);
    clear();
    for (int x = 0; x < static_cast<int>(vrms.size()); ++x)
    {
        int y = static_cast<int>((vrms[x] - ymin) / (ymax - ymin) * static_cast<float>(height));
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // white for vrms
        SDL_RenderDrawLine(renderer, x, static_cast<int>(height) - y, x, static_cast<int>(height));

        y = static_cast<int>((vpk[x] + 96) / 108 * static_cast<float>(height));
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for vpk
        SDL_RenderDrawPoint(renderer, x, y);
    }
    SDL_RenderPresent(renderer);
}

void Plotter::plotSpectrum(const vector<pair<float,float>>& spectrum)
{
    if (spectrum.size() != width)
    {
        cerr << "Spectrum size does not match width" << endl;
        return;
    }
    // transform float,float pair vector to vector of SDL_Color
    vector<SDL_Color> colors;
    colors.reserve(spectrum.size());
    for (const auto& [bin, decay] : spectrum)
    {
        float h = 0;
        float s = 255.0f * decay;
        float v = 255.0f * bin;
        auto [r,g,b] = HSVtoRGB(h, s, v);
        colors.push_back({static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), 255});
    }
    spectral.push_front(vector<SDL_Color>(width, {0, 0, 0, 255}));
    clear();

    SDL_RenderPresent(renderer);
}

void Plotter::clear()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}
