#include "AudioProcessor.h"
#include <cmath>

const float LOGMIN = 1e-10f;

std::vector<float> get_slice(const boost::circular_buffer<float>& buffer, size_t n) 
{
    if (n == 0)
        n = buffer.size();
    std::vector<float> slice(n);
    auto it = buffer.end() - n;
    std::copy(it, buffer.end(), slice.begin());
    return slice;
}

AudioProcessor::AudioProcessor(size_t display_w, size_t display_h, size_t window_size)
: disp_w(display_w)
, disp_h(display_h)
{
    raw.set_capacity(window_size);
    vpk.set_capacity(display_w);
    vrms.set_capacity(display_w);
}


AudioProcessor::~AudioProcessor()
{
}

void AudioProcessor::process_data(const std::vector<float>& data)
{
    // Append data to the circular buffer
    raw.insert(raw.end(), data.begin(), data.end());

    // Compute RMS and peak
    float rms = 0.0;
    float peak = 0.0;

    for (float v : data) {
        v = fabsf(v);
        rms += v * v;
        if (v > peak)
            peak = v;
    }
    rms = sqrtf(rms / float(data.size()));
    // Update circular buffers with the db values
    vpk.push_back(20 * log10f(peak + LOGMIN ));
    vrms.push_back(20 * log10f(rms + LOGMIN ));
}

const std::vector<float> AudioProcessor::Vrms() const
{
    return get_slice(vrms);
}

const std::vector<float> AudioProcessor::Vpeak() const
{
    return get_slice(vpk);
}

