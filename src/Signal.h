#pragma once

#include <vector>
#include <cstddef>
#include <boost/circular_buffer.hpp>

// Signal is just a typed vector<float>
struct Signal {
    std::vector<float> data;

    // Constructors for convenience
    Signal() = default;
    Signal(size_t size, float value = 0.0f) : data(size, value) {}
    Signal(const std::vector<float>& vec) : data(vec) {}
    Signal(std::vector<float>&& vec) : data(std::move(vec)) {}
    Signal(const boost::circular_buffer<float>& buf) : data(buf.begin(), buf.end()) {}

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
    operator std::vector<float>& () { return data; }
    operator const std::vector<float>& () const { return data; }

    Signal operator+ (const Signal& other) const {
        Signal result(size());
        for (size_t i = 0; i < size(); ++i) {
            result[i] = data[i] + other[i];
        }
        return result;
    }
    Signal operator* (float scalar) const {
        Signal result(size());
        for (size_t i = 0; i < size(); ++i) {
            result[i] = data[i] * scalar;
        }
        return result;
    }

    Signal operator- (float scalar) const {
        Signal result(size());
        for (size_t i = 0; i < size(); ++i) {
            result[i] = data[i] - scalar;
        }
        return result;
    }

    // expose the underlying iterators
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
};
