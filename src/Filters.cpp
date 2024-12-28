#include "Filters.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <iostream>

// Function to apply a filter to a signal
Signal filter(const FilterCoefficients& coeff, const Signal& input)
{
    Signal output(input.size());
    if (std::any_of(input.begin(), input.end(), [](float v) { return std::isnan(v); })) {
        std::cerr << "Input signal contains NaN" << std::endl;
        throw std::runtime_error("Input signal contains NaN");
    }

    if (coeff.a[0] != 1.0f)
        throw std::runtime_error("Filter is not normalized");

    for (size_t n = 0; n < input.size(); ++n) {
        float yn = coeff.b[0] * input[n];
        for (size_t i = 1; i < coeff.b.size(); ++i) {
            if (n >= i) {
                yn += coeff.b[i] * input[n - i];
            }
        }
        for (size_t i = 1; i < coeff.a.size(); ++i) {
            if (n >= i) {
                yn -= coeff.a[i] * output[n - i];
            }
        }
        output[n] = yn; // Assign the computed value to the output
    }
    if (std::any_of(output.begin(), output.end(), [](float v) { return std::isnan(v); })) {
        std::cerr << "Transformed signal contains NaN" << std::endl;
        throw std::runtime_error("Transformed signal contains NaN");
    }
    return output;
}

// Function to generate a Hanning window
Signal hanning_window(size_t size)
{
    std::vector<float> window(size);
    for (size_t i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PIf * static_cast<float>(i) / (static_cast<float>(size) - 1)));
    }
    return window;
}

// Function to apply a window to a signal
void apply_window(const Signal& window, Signal& signal)
{
    for (size_t i = 0; i < signal.size(); ++i) {
        signal[i] *= window[i];
    }
}
