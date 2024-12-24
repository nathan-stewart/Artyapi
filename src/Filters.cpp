#include "Filters.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <iostream>

FilterCoefficients butterworth_hpf(size_t order, float cutoff, float sample_rate)
{
    // Hardcoded coefficients for a 4th order 1kHz HPF
    if (!(order== 4 && cutoff == 1000.0f && sample_rate == 48000.0f))
        throw std::runtime_error("Unimplemented filter parameters");

    return { {0.9931822f, -3.9727288f, 5.9590931f, -3.9727288f, 0.9931822f},
             {1.0000000f, -3.9863177f, 5.9590467f, -3.9591398f, 0.9864109f} };

}

FilterCoefficients butterworth_lpf(size_t order, float cutoff, float sample_rate)
{
    // Hardcoded coefficients for a 4th order 1kHz LPF
    if (!(order == 4 && cutoff == 1000.0f && sample_rate == 48000.0f))
        throw std::runtime_error("Unimplemented filter parameters");

    return { {0.4998150f, 1.9992600f, 2.9988900f, 1.9992600f, 0.4998150f},
             {1.0000000f, 2.6386277f, 2.7693098f, 1.3392808f, 0.2498217f} };

}

// Function to apply a filter to a signal
void apply_filter(const FilterCoefficients& coefficients,std::vector<float>& signal)
{
    const std::vector<float>& b = coefficients.first;
    const std::vector<float>& a = coefficients.second;

    std::vector<float> filtered(signal.size(), 0.0f);
    for (size_t n = 0; n < signal.size(); ++n) {
        filtered[n] = b[0] * signal[n];
        // std::cout << "b[";
        for (size_t i = 1; i < b.size(); ++i) {
            if (n >= i) {
                filtered[n] += b[i] * signal[n - i];
                // std::cout <<  b[i];
            }
        }
        // std::cout << "]" << std::endl;

        // std::cout << "a[";
        for (size_t j = 1; j < a.size(); ++j) {
            if (n >= j) {
                filtered[n] -= a[j] * filtered[n - j];
                // std::cout <<  a[j];
            }
        }
        // std::cout << "]" << std::endl;

    }
    signal = filtered;
}

// Function to generate a Hanning window
std::vector<float> hanning_window(size_t size)
{
    std::vector<float> window(size);
    for (size_t i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PIf * static_cast<float>(i) / (static_cast<float>(size) - 1)));
    }
    return window;
}

// Function to apply a window to a signal
void apply_window(const std::vector<float>& window, std::vector<float>& signal)
{
    for (size_t i = 0; i < signal.size(); ++i) {
        signal[i] *= window[i];
    }
    }
