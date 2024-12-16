import unittest
import numpy as np
from scipy.signal import lfilter
from src.AudioProcessing import AudioProcessor
import matplotlib.pyplot as plt

class TestAudioProcessor(unittest.TestCase):
    def setUp(self):
        self.window_size = 65536
        self.samplerate = 48000
        self.bincount = 1985
        self.logspace = np.logspace(0, 1, self.bincount)
        self.linspace = np.linspace(0, 1, self.bincount)
        self.windowed_bpf = np.ones(self.window_size)  # Replace with actual filter if needed
        self.raw = np.zeros(self.window_size)
        self.c_idx = 0

        self.processor = AudioProcessor(window_size=self.window_size, samplerate=self.samplerate)

    def test_process_data(self):
        def sine_wave(frequency, samplerate, samples):
            t = np.linspace(0, samples/samplerate, samples, endpoint=False)
            return np.sin(2 * np.pi * frequency * t)
        
        samples = 65536
        sample_rate = 48000
        
        # Generate test data - white noise
        white_noise = np.random.normal(0, 1, samples)
        white_noise *= 1 / np.sqrt(2)
        rms, peak, _, _ = self.processor.process_data(white_noise)
        self.assertAlmostEqual(rms, -3.01, delta=0.5, msg="RMS should be -3.0 dB")
        self.assertGreaterEqual(peak, 9.0,  msg="Peak should be great than 9dB")
        self.assertLessEqual(peak, 13.0, msg="Peak should be less than 13db")

        from scipy.signal import spectrogram

        # Generate test data: a sine wave
        f =440
        sine = sine_wave(f, sample_rate, samples)
        self.assertTrue(np.max(sine) > 0.0, "Sine wave should not be 0")
        self.assertTrue(np.max(sine) <= 1.0, "Sine wave should be normalized")
        
        # Process the test data
        rms, peak, fft, _ = self.processor.process_data(sine)
        self.assertAlmostEqual(rms, -3.0, delta=0.2, msg="RMS should be -3.0 dB")
        self.assertAlmostEqual(peak, 0.0, delta=0.2, msg="Peak should be 0.0 dB")

        # Check that the FFT data is not all zeros
        self.assertGreater(np.max(fft), 0.4, msg="FFT should have a peak")
        self.assertLess(np.max(fft), 1.0, msg="FFT should be normalized")        

        peak_frequency = np.argmax(fft) * sample_rate / self.window_size
        self.assertEqual(peak_frequency, 440, delta=5.0,  msg="Peak should be at 440 Hz")        
        
        # Create a white noise signal with itself delayed by 2ms for autocorrelation
        # delayed_samples = int(0.002 * sample_rate)
        # delayed_white_noise = np.concatenate([white_noise[delayed_samples:], white_noise[:delayed_samples]])
        # _, _, _, autocorr = self.processor.process_data(delayed_white_noise)
        # self.assertGreater(np.max(autocorr), 0.9, "autocorr should have a peak")
        # self.assertEqual(np.argmax(autocorr), delayed_samples, "Peak should be at 2ms")
        

if __name__ == '__main__':
    unittest.main()