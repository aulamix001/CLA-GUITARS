#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "GuitarDSP.h"
class GuitarProcessor : public juce::AudioProcessor {
public:
    GuitarProcessor();
    juce::AudioProcessorValueTreeState state;
    guitar::Engine engine;
    std::atomic<float> inMeter{0},outMeter{0};
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void prepareToPlay(double sr,int) override { engine.prepare(sr); }
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Guitar Rack"; }
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    double getTailLengthSeconds() const override {return 5;}
    int getNumPrograms() override;
    int getCurrentProgram() override {return program;}
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    struct Preset {const char* folder;const char* name;int amp; bool reamp;std::array<float,6> faders;std::array<int,6> colors;};
    static const std::vector<Preset>& presets();
private:
    int program=0;
};
