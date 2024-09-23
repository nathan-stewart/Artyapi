#!/usr/bin/env python3
import soundfile as sf
import pyaudio
import time
import numpy as np
import threading
import os


def RealTimeAudioSource(chunksize=16384, readbufsize=4096, samplerate=44100):
    def _capture_audio():
        nonlocal buffer, write_index

        while not stop_flag:
            # Capture new audio data
            data = stream.read(readbufsize)
            new_data = np.frombuffer(data, dtype=np.int16)

            # Safely update the buffer in a thread-safe manner (circular buffer)
            with lock:
                # Find the space available and wrap around if necessary
                end_index = (write_index + len(new_data)) % buffer.size

                # If no wrap-around, directly copy
                if write_index < end_index:
                    buffer[write_index:end_index] = new_data
                else:
                    # If wrapping, copy in two parts
                    buffer[write_index:] = new_data[:buffer.size - write_index]
                    buffer[:end_index] = new_data[buffer.size - write_index:]

                # Update write_index
                write_index = end_index

    # Initialize audio capture
    p = pyaudio.PyAudio()
    stream = p.open(format=pyaudio.paInt16, channels=1, rate=samplerate, input=True, frames_per_buffer=readbufsize)

    # Initialize circular buffer and threading
    buffer = np.zeros(chunksize, dtype=np.int16)
    write_index = 0
    lock = threading.Lock()
    stop_flag = False
    capture_thread = threading.Thread(target=_capture_audio)
    capture_thread.start()

    # Generator to yield the latest audio data
    while True:
        t0 = time.time()

        # Safely access the buffer and return the last `chunksize` samples
        with lock:
            # Return a copy of the circular buffer, starting from the correct point
            if write_index == chunksize:
                chunk = buffer.copy()  # No wrapping needed
            else:
                # Return the last `chunksize` samples in proper order (handle wrap-around)
                chunk = np.concatenate((buffer[write_index:], buffer[:write_index]))

        yield chunk

def FileAudioSource(testdir, chunksize):
    files = os.listdir(testdir)
    while True:
        for f in files:
            fullpath = os.path.join(testdir, f)
            if not (fullpath and fullpath.lower().endswith('.wav')):
                print(fullpath)
                raise ValueError('Only .wav files are supported')

            with sf.SoundFile(fullpath) as audio_file:
                samplerate = audio_file.samplerate
                n_samples = len(audio_file)
                t0 = time.time()
                position = 0

                while position < n_samples:
                    running_time = time.time() - t0
                    position = int(running_time * samplerate)
                    audio_file.seek(position)
                    chunk = audio_file.read(chunksize, dtype='float32')

                    # Convert to mono if necessary
                    if len(chunk.shape) == 2:
                        chunk = chunk.mean(axis=1)

                    chunk /= 32768.0  # Normalize to [-1, 1]

                    if len(chunk) < chunksize:
                        chunk = np.pad(chunk, (0, chunksize - len(chunk)), mode='constant')

                    yield chunk

        files = os.listdir(testdir)
