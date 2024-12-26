#include "Filters.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/embed.h>

namespace py = pybind11;

FilterCoefficients butterworth(size_t order, float cutoff, float sample_rate, bool hpf)
{
    py::scoped_interpreter guard{}; // Start Python interpreter
    py::module_ scipy = py::module_::import("scipy.signal");
    py::module_ np = py::module_::import("numpy");
    py::tuple result = scipy.attr("butter")(order, 2.0f * cutoff / sample_rate, hpf?"high":"low");

    std::vector<float> b = result[0].cast<std::vector<float>>();
    std::vector<float> a = result[1].cast<std::vector<float>>();
    return std::make_pair(b, a);
}

// Function to apply a filter to a signal
void apply_filter(const FilterCoefficients& coefficients, std::vector<float>& signal)
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
        for (size_t i = 1; i < a.size(); ++i) {
            if (n >= i) {
                filtered[n] -= a[i] * filtered[n - i];
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
