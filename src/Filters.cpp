#include "Filters.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>

FilterCoefficients butterworth_filter(int order, float cutoff)
{
    float wc = tanf(M_PIf * cutoff);
    std::vector<std::complex<float>> poles(order);
    for (int k = 0; k < order; ++k) {
        float theta = M_PIf * (2.0f * static_cast<float>(k) + 1.0f) / (2.0f * static_cast<float>(order));
        poles[k] = std::complex<float>(-wc * sinf(theta), wc * cosf(theta));
    }

    std::vector<std::complex<float>> a_complex(order + 1, 0.0f);
    a_complex[0] = 1.0f;
    for (int i = 0; i < order; ++i) {
        for (int j = i + 1; j > 0; --j) {
            a_complex[j] = a_complex[j] - poles[i] * a_complex[j - 1];
        }
    }

    std::vector<float> a(order + 1);
    std::vector<float> b(order + 1, 0.0f);
    for (int i = 0; i <= order; ++i) {
        a[i] = std::real(a_complex[i]);
    }
    b[0] = 1.0f;
    return {a, b};
}


// Function to apply a filter to a signal
void apply_filter(const FilterCoefficients& coefficients,std::vector<float>& signal)
{
    const std::vector<float>& a = coefficients.first;
    const std::vector<float>& b = coefficients.second;
    
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
