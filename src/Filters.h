#ifndef FILTERS_H
#define FILTERS_H

#include <vector>
#include <cstddef>


// Signal is just a typed vector<float>
struct Signal {
    std::vector<float> data;

    // Constructors for convenience
    Signal() = default;
    Signal(size_t size, float value = 0.0f) : data(size, value) {}
    Signal(const std::vector<float>& vec) : data(vec) {}
    Signal(std::vector<float>&& vec) : data(std::move(vec)) {}

   // Type aliases for iterators
    using iterator = std::vector<float>::iterator;
    using const_iterator = std::vector<float>::const_iterator;

    // Provide access to the underlying vector
    float& operator[](size_t index) { return data[index]; }
    const float& operator[](size_t index) const { return data[index]; }
    size_t size() const { return data.size(); }
    void resize(size_t newSize) { data.resize(newSize); }
    void fill(float value) { std::fill(data.begin(), data.end(), value); }
    // expose the underlying iterators
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
};

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

#endif // FILTERS_H