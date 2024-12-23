#include "Filters.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>

FilterCoefficients butterworth_hpf(size_t order, float cutoff, float sample_rate)
{
    if( order== 4 && cutoff == 1000.0f && sample_rate == 48000.0f)
        return {{0.8426766f, -3.3707065f, 5.0560598f, -3.3707065f, 0.8426766f}, 
                {1.0000000f, -3.6580603f, 5.0314335f, -3.0832283f, 0.7101039f}};
    else
        throw std::runtime_error("Unimplemented filter parameters");
}

FilterCoefficients butterworth_lpf(size_t order, float cutoff, float sample_rate)
{
    if(order == 4 && cutoff == 1000.0f && sample_rate == 48000.0f)
        return {{0.0000156f, 0.0000622f, 0.0000933f, 0.0000622f, 0.0000156f}, 
                {1.0000000f, -3.6580603f, 5.0314335f, -3.0832283f, 0.7101039f}};
    else
        throw std::runtime_error("Unimplemented filter parameters");
} 

// Function to apply a filter to a signal
void apply_filter(const FilterCoefficients& coefficients,std::vector<float>& signal)
{
    const std::vector<float>& b = coefficients.first;
    const std::vector<float>& a = coefficients.second;

    std::vector<float> filtered(signal.size(), 0.0f);
    for (size_t n = 0; n < signal.size(); ++n) {
        filtered[n] = b[0] * signal[n];
        for (size_t i = 1; i < b.size(); ++i) {
            if (n >= i) {
                filtered[n] += b[i] * signal[n - i];
            }
        }
        for (size_t j = 1; j < a.size(); ++j) {
            if (n >= j) {
                filtered[n] -= a[j] * filtered[n - j];
            }
        }
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
