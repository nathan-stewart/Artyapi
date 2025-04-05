#include "Plotter.h"
#include <iostream>

using namespace std;
SDL_Color HSVtoRGB(float h, float s, float v)
{
    Uint8 r, g, b, a = 255;
    float hf = h / 60.0f;
    Uint8 i = static_cast<Uint8>(hf);
    float f = hf - static_cast<float>(i);
    Uint8 pv = static_cast<Uint8>(255.0f*(v * (1 - s / 255.0f)));
    Uint8 qv = static_cast<Uint8>(255.0f*(v * (1 - s / 255.0f * f)));
    Uint8 tv = static_cast<Uint8>(255.0f*(v * (1 - s / 255.0f * (1 - f))));

    switch (i) {
        case 0:
            r = static_cast<Uint8>(255.0f * v);
            g = tv;
            b = pv;
            break;
        case 1:
            r = qv;
            g = static_cast<Uint8>(255.0f * v);
            b = pv;
            break;
        case 2:
            r = pv;
            g = static_cast<Uint8>(255.0f * v);
            b = tv;
            break;
        case 3:
            r = pv;
            g = qv;
            b = static_cast<Uint8>(255.0f * v);
            break;
        case 4:
            r = tv;
            g = pv;
            b = static_cast<Uint8>(255.0f * v);
            break;
        case 5:
        default:
            r = static_cast<Uint8>(255.0f * v);
            g = pv;
            b = qv;
            break;
    }
    return {r, g, b, a};
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

void Plotter::plotSpectrum(const SpectralHistory& spectral_history)
{
    // enforce width and range constraints
    if (spectral_history.size() != width ||
        any_of(spectral_history.begin(), spectral_history.end(), [](const auto& p) { return p.first < 0.0f || p.first > 1.0f || p.second < 0.0f || p.second > 1.0f; }))
    {
        throw invalid_argument("Invalid spectrum");
    }

    // transform vector of pair<float,float> to vector of SDL_Color
    vector<SDL_Color> colorized;
    for (const auto& [bin, decay] : spectrum)
    {
        auto color = HSVtoRGB(0, decay, bin);
        colorized.push_back(color);
    }
    spectral.push_front(colorized);


    // Create a texture to hold the pixel data
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, static_cast<int>(width), static_cast<int>(height));
    if (!texture) {
        throw runtime_error("Failed to create texture");
    }

    // Lock the texture to get a pointer to the pixel data
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) {
        SDL_DestroyTexture(texture);
        throw runtime_error("Failed to lock texture");
    }

    // Update the pixel data
    Uint32* pixel_data = static_cast<Uint32*>(pixels);
    for (size_t y = 0; y < spectral.size(); ++y)
    {
        for (size_t x = 0; x < spectral[y].size(); ++x)
        {
            SDL_Color color = spectral[y][x];
            pixel_data[y * width + x] = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), color.r, color.g, color.b, color.a);
        }
    }

    SDL_UnlockTexture(texture);
    clear(); // Clear the renderer
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);
}

void Plotter::clear()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}
