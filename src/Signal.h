#pragma once

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
    auto insert(iterator pos, iterator first, iterator last) { return data.insert(pos, first, last); } 
    
    operator float* () { return data.data(); }
    operator const float*() const  { return data.data(); }
    
    // expose the underlying iterators
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
};
