#pragma once
#include <vector>
#include <cstddef>
#include "Signal.h"

struct FilterCoefficients {
    std::vector<float> b;
    std::vector<float> a;
    FilterCoefficients() = default;
    FilterCoefficients(const std::vector<float>& b, const std::vector<float>& a) : b(b), a(a) {}
    FilterCoefficients(std::vector<float>&& b, std::vector<float>&& a) : b(std::move(b)), a(std::move(a)) {}
};

FilterCoefficients butterworth(size_t order, float cutoff, float sample_rate, bool hpf);
Signal filter(const FilterCoefficients& coefficients, const Signal& input);


Signal hanning_window(size_t size);
void apply_window(const Signal& window, Signal& signal);
