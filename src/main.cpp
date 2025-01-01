#include "AudioProcessor.h"


Signal sine_wave(float frequency, float sample_rate, size_t samples)
{
    Signal sine_wave(samples);
    float amplitude = 1.0f;
    float phase = 0.0f;
    float increment = 2.0f * M_PIf * frequency / sample_rate;
    for (size_t i = 0; i < samples; ++i) {
        sine_wave[i] = amplitude * std::sin(phase);
        phase += increment;
    }
    return sine_wave;
}

int main() 
{
    Signal data = sine_wave(440.0f, 48e3f, 1 << 16);
    AudioProcessor ap;
    while (true)
    {
        ap.process(data);
        ap.update_plot();
    }        
    return 0;
}