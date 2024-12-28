% Function to format coefficients for C++ initializer list
pkg load signal;

% Generate Butterworth high-pass filter
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

% Format and print coefficients for C++ initializer list
samplerate = 48e3;
[b_hpf, a_hpf] = butter(4, 40 / (samplerate / 2), 'high');
formatted_b_hpf = format_coefficients(b_hpf);
formatted_a_hpf = format_coefficients(a_hpf);
printf('40Hz 4td order HPF Coefficients {b,a}: { %s, %s }\n', formatted_b_hpf, formatted_a_hpf);

[b_lpf, a_lpf] = butter(4, 20e3 / (samplerate / 2), 'low');
formatted_a_lpf = format_coefficients(a_lpf);
formatted_b_lpf = format_coefficients(b_lpf);
printf('20khz Hz 4th order HPF Coefficients: {b,a}: { %s, %s }\n', formatted_b_lpf, formatted_a_lpf);

[b_hpf, a_hpf] = butter(4, 1e3 / (samplerate / 2), 'high');
formatted_b_hpf = format_coefficients(b_hpf);
formatted_a_hpf = format_coefficients(a_hpf);
printf('1khz HPF 4th order  Coefficients {b,a}: { %s, %s }\n', formatted_b_hpf, formatted_a_hpf);

[b_lpf, a_lpf] = butter(4, 1e3 / (samplerate / 2), 'low');
formatted_b_lpf = format_coefficients(b_lpf);
formatted_a_lpf = format_coefficients(a_lpf);
printf('1khz LPF 4th order Coefficients: {b,a}: { %s, %s }\n', formatted_b_lpf, formatted_a_lpf);
