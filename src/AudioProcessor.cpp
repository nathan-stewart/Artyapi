#include "AudioProcessor.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <complex>
#include <iostream>

const float LOGMIN = 1e-10f;

void  process_volume(const std::vector<float>& data, boost::circular_buffer<float>& vrms, boost::circular_buffer<float>& vpk)
{
    float rms = 0.0f;
    float pk = 0.0f;

    for (auto& sample : data)
    {
        rms += sample * sample;
        pk = std::max(pk, std::abs(sample));
    }

    rms = sqrtf(rms / static_cast<float>(data.size()));
    vrms.push_back(rms);
    vpk.push_back(pk);
}


AudioProcessor::AudioProcessor(size_t display_w, size_t display_h, size_t window_size)
: disp_w(display_w)
, disp_h(display_h)
, process_spectrum(display_w, display_h, window_size)
{
    vpk.set_capacity(display_w);
    vrms.set_capacity(display_w);
}


AudioProcessor::~AudioProcessor()
{
}


void AudioProcessor::process(const std::vector<float>& data)
{
    if (data.size() == 0)
        return;

    process_volume(data, vrms, vpk);
    process_spectrum(data);
}
