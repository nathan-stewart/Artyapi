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

// Singleton class to manage the Python interpreter
class __attribute__((visibility("default"))) PythonInterpreter {
public:
    static PythonInterpreter& getInstance() {
        static PythonInterpreter instance;
        return instance;
    }

private:
    py::scoped_interpreter guard;

    PythonInterpreter() : guard{} {}
    ~PythonInterpreter() = default;

    // Delete copy constructor and assignment operator
    PythonInterpreter(const PythonInterpreter&) = delete;
    PythonInterpreter& operator=(const PythonInterpreter&) = delete;
};

FilterCoefficients butterworth(size_t order, float cutoff, float sample_rate, bool hpf)
{
    PythonInterpreter::getInstance();

    py::module_ scipy = py::module_::import("scipy.signal");
    py::tuple result = scipy.attr("butter")(order, 2.0f * cutoff / sample_rate, hpf?"high":"low");

    py::array_t<float> b = result[0].cast<py::array_t<float>>();
    py::array_t<float> a = result[1].cast<py::array_t<float>>();

    // Access the data directly
    auto b_unchecked = b.unchecked<1>();
    auto a_unchecked = a.unchecked<1>();

    std::vector<float> b_vec(b_unchecked.size());
    std::vector<float> a_vec(a_unchecked.size());

    for (ssize_t i = 0; i < b_unchecked.size(); ++i) {
        b_vec[i] = b_unchecked(i);
    }

    for (ssize_t i = 0; i < a_unchecked.size(); ++i) {
        a_vec[i] = a_unchecked(i);
    }

    return std::make_pair(std::move(b_vec), std::move(a_vec));
}

// Function to apply a filter to a signal
void apply_filter(const FilterCoefficients& coefficients, std::vector<float>& signal)
{
    const std::vector<float>& b = coefficients.first;
    const std::vector<float>& a = coefficients.second;

    std::vector<float> filtered(signal.size(), 0.0f);
    if (std::any_of(signal.begin(), signal.end(), [](float v) { return std::isnan(v); })) {
        std::cerr << "Input signal contains NaN" << std::endl;
        throw std::runtime_error("Input signal contains NaN");
    }

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
    if (std::any_of(signal.begin(), signal.end(), [](float v) { return std::isnan(v); })) {
        std::cerr << "Transformed signal contains NaN" << std::endl;
        throw std::runtime_error("Transformed signal contains NaN");
    }
    // check for nan in output
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
