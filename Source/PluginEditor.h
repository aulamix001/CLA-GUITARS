#pragma once
#include "PluginProcessor.h"
class RackLook : public juce::LookAndFeel_V4 {
public:
 void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
};
class GuitarEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit GuitarEditor(GuitarProcessor&);
 ~GuitarEditor() override {setLookAndFeel(nullptr);}
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 GuitarProcessor& processor; RackLook look;
 std::array<juce::Slider,8> sliders;
 std::array<juce::TextButton,6> colors;
 std::array<juce::ToggleButton,6> enabled;
 juce::ToggleButton reamp{"Re-Amplify"};
 juce::ComboBox amp;
 juce::TextButton load{"Load"},save{"Save"};
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttons;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ampAttachment;
 std::unique_ptr<juce::FileChooser> chooser;
 juce::Image texture;
 float meterIn=0,meterOut=0;
 void timerCallback() override;
 void presetMenu();
 void savePreset();
};
