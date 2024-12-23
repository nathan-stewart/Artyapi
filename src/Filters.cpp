#include "Filters.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>


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
