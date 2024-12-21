#include "AudioProcessor.h"
#include <cmath>

std::vector<float> get_slice(const boost::circular_buffer<float>& cb) {
    std::vector<float> slice(cb.size());
    std::copy(cb.begin(), cb.end(), slice.begin());
    return slice;
}

AudioProcessor::AudioProcessor(size_t display_w, size_t display_h)
: disp_w(display_w)
, disp_h(display_h)
{
    size_t disp_w;
    size_t disp_h;

    vpk.set_capacity(display_w);
    vrms.set_capacity(display_w);
}


AudioProcessor::~AudioProcessor()
{
}

void AudioProcessor::process_data(const std::vector<float>& data)
{
    // Compute RMS and peak
    float rms = 0.0;
    float peak = 0.0;

    for (float v : data) {
        v = fabsf(v);
        rms += v * v;
        if (v > peak)
            peak = v;
    }
    rms = sqrtf(rms / data.size());

    // Update circular buffers with the db values
    vpk.push_back(20 * log10f(peak));
    vrms.push_back(20 * log10f(rms));
}

const std::vector<float> AudioProcessor::Vrms() const
{
    return get_slice(this->vrms);
}

const std::vector<float> AudioProcessor::Vpeak() const
{
    return get_slice(this->vrms);
}

