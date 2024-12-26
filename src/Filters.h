#ifndef FILTERS_H
#define FILTERS_H

#include <vector>
#include <cstddef>


using FilterCoefficients = std::pair<std::vector<float>, std::vector<float> >;
FilterCoefficients butterworth(size_t order, float cutoff, float sample_rate, bool hpf);
void apply_filter(const FilterCoefficients& coefficients, std::vector<float>& signal);


std::vector<float> hanning_window(size_t size);
void apply_window(const std::vector<float>& window, std::vector<float>& signal);

#endif // FILTERS_H