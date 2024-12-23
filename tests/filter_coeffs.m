% Function to format coefficients for C++ initializer list
pkg load signal;

function formatted_str = format_coefficients(coeffs)
    formatted_str = '{';
    for i = 1:length(coeffs)
        formatted_str = [formatted_str, sprintf('%.7ff', coeffs(i))];
        if i < length(coeffs)
            formatted_str = [formatted_str, ', '];
        end
    end
    formatted_str = [formatted_str, '}'];
end


% Define parameters
order = 4;
cutoff = 1000; % 1 kHz
samplerate = 48000;
samples = 2^16;
t = (0:samples-1) / samplerate;

% Frequencies to test
f0 = cutoff * 2^(-4); % 62.5 Hz
f1 = cutoff;         % 1 kHz
f2 = cutoff * 2^(4); % 16 kHz

% Generate sine waves
sine_f0 = sin(2 * pi * f0 * t);
sine_f1 = sin(2 * pi * f1 * t);
sine_f2 = sin(2 * pi * f2 * t);

% Generate Butterworth high-pass filter
[b_hpf, a_hpf] = butter(order, cutoff / (samplerate / 2), 'high');
[b_lpf, a_lpf] = butter(order, cutoff / (samplerate / 2), 'low');

% Format and print coefficients for C++ initializer list
formatted_b_hpf = format_coefficients(b_hpf);
formatted_a_hpf = format_coefficients(a_hpf);
printf('HPF Coefficients {b,a}: { %s, %s }\n', formatted_b_hpf, formatted_a_hpf);

formatted_b_lpf = format_coefficients(b_lpf);
formatted_a_lpf = format_coefficients(a_lpf);
printf('LPF Coefficients: {b,a}: { %s, %s }\n', formatted_b_lpf, formatted_a_lpf);

% Apply high-pass filter
filtered_f0_hpf = filter(b_hpf, a_hpf, sine_f0);
filtered_f1_hpf = filter(b_hpf, a_hpf, sine_f1);
filtered_f2_hpf = filter(b_hpf, a_hpf, sine_f2);

% Generate log2 spaced frequency points
frequencies = logspace(log2(20), log2(samplerate/2), 1024);

% Compute frequency response for HPF
[h_hpf, w_hpf] = freqz(b_hpf, a_hpf, frequencies, samplerate);

% Plot results
figure;
subplot(3, 2, 1);
plot(t(1:5000), sine_f0(1:5000), 'b', t(1:5000), filtered_f0_hpf(1:5000), 'r');
title('HPF 62.5 Hz');
legend('Original', 'Filtered');

subplot(3, 2, 3);
plot(t(1:5000), sine_f1(1:5000), 'b', t(1:5000), filtered_f1_hpf(1:5000), 'r');
title('HPF 1 kHz');
legend('Original', 'Filtered');

subplot(3, 2, 5);
plot(t(1:5000), sine_f2(1:5000), 'b', t(1:5000), filtered_f2_hpf(1:5000), 'r');
title('HPF 16 kHz');
legend('Original', 'Filtered');

figure;
semilogx(w_hpf, 20*log10(abs(h_hpf)));
title('HPF Frequency Response');
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');

% Wait for a keypress before exiting
disp('Press any key to exit');
pause;