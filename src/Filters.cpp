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

    return FilterCoefficients(b_vec,a_vec);
}

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
        output[n] = coeff.b[0] * input[n];
        for (size_t i = 1; i < coeff.b.size(); ++i) {
            if (n >= i) {
                output[n] += coeff.b[i] * input[n - i];
            }
        }
        for (size_t i = 1; i < coeff.a.size(); ++i) {
            if (n >= i) {
                output[n] -= coeff.a[i] * output[n - i];
            }
        }
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
