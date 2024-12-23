#ifndef FILTERS_H
#define FILTERS_H

#include <vector>
#include <cstddef>


using FilterCoefficients = std::pair<std::vector<float>, std::vector<float> >;

void apply_filter(const FilterCoefficients& coefficients, std::vector<float>& signal);


std::vector<float> hanning_window(size_t size);
void apply_window(const std::vector<float>& window, std::vector<float>& signal);

#endif // FILTERS_H