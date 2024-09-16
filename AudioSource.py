#!/usr/bin/env python3
import soundfile as sf
import alsaaudio
import time
import numpy as np
import threading
import os 

class AudioSource:
    def __init__(self):
        raise NotImplementedError("This method should be implemented by subclasses.")

    def read(self):
        raise NotImplementedError("This method should be implemented by subclasses.")

class RealTimeAudioSource(AudioSource):
    def __init__(self, device='default', sample_rate=48000, chunksize=16384, channels=1):
        self.device = alsaaudio.PCM(alsaaudio.PCM_CAPTURE, alsaaudio.PCM_NORMAL, device)
        self.device.setrate(sample_rate)
        self.device.setchannels(channels)
        self.device.setformat(alsaaudio.PCM_FORMAT_S16_LE)
        self.device.setperiodsize(chunksize)
        self.window_size = chunksize
        self.buffer = np.zeros(chunksize, dtype=np.int16)
        self.lock = threading.Lock()
        self.stop_flag = False
        self.capture_thread = threading.Thread(target=self._capture_audio)
        self.capture_thread.start()
    
    def _capture_audio(self):
        while not self.stop_flag:
            # Capture new audio data
            length, data = self.device.read()
            if length:
                new_data = np.frombuffer(data, dtype=np.int16)
                
                # Safely update the buffer in a thread-safe manner
                with self.lock:
                    # Append new data to the buffer, keeping only the last `chunksize` samples
                    self.buffer = np.concatenate((self.buffer, new_data))[-self.chunksize:]

    def read(self):
        while True:
            # Safely access the buffer and return the last `chunksize` samples
            t0 = time.time()
            with self.lock:
                running_time = time.time() - t0
                if running_time >= self.chunksize / self.sample_rate:
                    break
                # Safely access the buffer and return the last `chunksize` samples
                with self.lock:
                    chunk = self.buffer.copy()
                yield chunk
                t0 = time.time()
        
    def __del__(self):
        # Stop the capture thread when done
        self.stop_flag = True
        self.capture_thread.join()

class FileAudioSource(AudioSource):

    def __init__(self, filepath, chunksize):
        super().__init__()
        self.filepath = filepath
        self.chunksize = chunksize
    
    def process_file(filepath):
        if not (filepath and filepath.endswith('.wav')):
            raise ValueError('Only .wav files are supported')

        audio_data, sample_rate = sf.read(filepath)

        # Enforce fixed sample rate
        if sample_rate != self.sample_rate:
            raise ValueError(f'Invalid sample rate: {sample_rate} Hz. Only {SAMPLE_RATE} Hz supported')

        # Convert to mono if necessary
        if len(audio_data.shape) == 2:
            audio_data = audio_data.mean(axis=1)

        audio_data = audio_data.astype(np.float32)  # Convert to float
        audio_data /= np.max(np.abs(audio_data))  # Normalize to range [-1, 1]
        return sf.read(filepath)

    def read(self):
        files = os.listdir(self.filepath)
        while True:
            for f in files:
                fullpath = self.os.path.join(self.filepath, f)
                self.audio_data, self.sample_rate = FileAudioSource.process_file(fullpath)
                n_samples = len(self.audio_data)
                self.position = 0
                t0 = time.time()
                while True:
                    running_time = time.time() - t0
                    position = int(running_time * self.sample_rate)
                    if position + self.chunksize > self.chunksize:
                        chunk = self.audio_data[position:position + self.chunksize]
                        if len(chunk) < self.chunksize:
                            chunk = np.pad(chunk, (0, self.chunksize - len(chunk)), mode='constant')
                        yield chunk
                        break
                    else:
                        chunk = self.audio_data[position:position + self.chunksize]
                        yield chunk
            files = os.listdir(self.filepath)
