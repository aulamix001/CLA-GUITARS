#pragma once
#include <signalsmith-stretch/signalsmith-stretch.h>
#include <array>
#include <cmath>
#include <vector>

namespace guitar {
// Spectral peak/phase-linked stereo shifter. Equal input/output lengths preserve time.
// Storage and FFT plans are prepared off the audio thread; process uses stack IO only.
class PitchShifter {
    signalsmith::stretch::SignalsmithStretch<double> stretch{42};
    std::array<std::vector<double>, 2> dry;
    size_t head = 0;
    double semitones = 0, smoothing = 0;
    int latency = 0;
public:
    void prepare(double rate) {
        // Dense overlap reduces the phase-prediction tuning bias at octave shifts.
        stretch.configure(2, static_cast<int>(rate*0.12), static_cast<int>(rate*0.005));
        latency = stretch.inputLatency() + stretch.outputLatency();
        for (auto& channel : dry) channel.assign(static_cast<size_t>(latency), 0.0);
        head = 0; semitones = 0;
        smoothing = 1.0 - std::exp(-1.0 / (rate * 0.03));
        stretch.setTransposeFactor(1.0);
    }
    int latencySamples() const { return latency; }
    // Returns wet and time-aligned dry separately, allowing a transparent dry-only path.
    void process(const std::array<float, 2>& input, double targetSemitones,
                 std::array<float, 2>& wet, std::array<float, 2>& delayed) {
        semitones += smoothing * (targetSemitones - semitones);
        stretch.setTransposeFactor(std::pow(2.0, semitones / 12.0));
        double in[2] = {input[0], input[1]}, out[2]{};
        const double* inputs[2] = {&in[0], &in[1]};
        double* outputs[2] = {&out[0], &out[1]};
        stretch.process(inputs, 1, outputs, 1);
        for (size_t c = 0; c < 2; ++c) {
            delayed[c] = static_cast<float>(dry[c][head]);
            dry[c][head] = input[c];
            wet[c] = static_cast<float>(out[c]);
        }
        head = (head + 1) % dry[0].size();
    }
};
}
