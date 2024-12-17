#!/usr/bin/env python3
import numpy as np
import argparse
import time
import os
from src.AudioProcessing import AudioProcessor
from src.Plotting import Plotting

argparse = argparse.ArgumentParser(description="Audio Visualizer")
argparse.add_argument(
    "--mode", choices=["spl", "acf"], default="", help="Mode to run the visualizer in"
)
argparse.add_argument(
    "--source", type=str, help="Use test data instead of real-time audio"
)
argparse.add_argument(
    "--windowsize", type=int, default=65536, help="Window size for FFT"
)
argparse.add_argument(
    "--rotate", choices=["true", "false", "True", "False"], default=None
)
argparse.add_argument("--profile", action="store_true", help="Profile the code")

args = argparse.parse_args()
if args.rotate:
    args.rotate = args.rotate.lower()

windowsize = int(args.windowsize)

# delay import of audio source to avoid loading pyaudio if not needed
if os.path.exists(args.source):
    from src.AudioSource import FileAudioSource

    audio_source = FileAudioSource(args.source)
else:
    from src.AudioSource import RealTimeAudioSource, list_audio_devices

    if args.source == "-l":
        list_audio_devices()
        exit()
    audio_source = RealTimeAudioSource(source=args.source)
if isinstance(audio_source, tuple):
    audio_source, samplerate = audio_source
else:
    samplerate = 48000
next(audio_source)


def main():
    global args
    global audio_source

    run = True
    ap = AudioProcessor(window_size=args.windowsize, samplerate=samplerate)
    plt = Plotting(width=1920, height=480, f0=40, f1=20e3)
    plt.show()

    t0 = time.time()
    while run:
        rms, peak, fft, acf = ap.process_data(next(audio_source))
        plt.update_data(rms, peak, fft, acf)
        t1 = time.time()
        # print(f'{1/(t1-t0):.1f} fps\r')
        t0 = t1
        if args.profile:
            loop_count -= 1
            if loop_count <= 0:
                run = False


if __name__ == "__main__":
    if args.profile:
        import cProfile, pstats

        profiler = cProfile.Profile()
        profiler.enable()

    main()

    if args.profile:
        profiler.disable()
        with open("profile_output.txt", "w") as f:
            ps = pstats.Stats(profiler, stream=f)
            ps.strip_dirs().sort_stats("cumulative").print_stats(20)
        print("Profiling data saved to 'profile_output.txt'")
