#pragma once
#include "PluginProcessor.h"

class RackLook : public juce::LookAndFeel_V4 {
public:
    juce::Slider::SliderLayout getSliderLayout(juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class GuitarEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit GuitarEditor(GuitarProcessor&);
    ~GuitarEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Canvas : juce::Component {
        std::function<void(juce::Graphics&)> draw;
        void paint(juce::Graphics& g) override { if (draw) draw(g); }
    } canvas;
    GuitarProcessor& processor;
    RackLook look;
    std::array<juce::Slider, 8> sliders;
    std::array<juce::TextButton, 6> colors;
    juce::ToggleButton reamp;
    juce::Slider amp;
    juce::TextButton undo{"↶"}, redo{"↷"}, compare{"A → B"}, load{"Load"}, save{"Save"},
                     options{"▾"}, help{"?"};
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reampAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Image texture, cloth;
    std::array<juce::ValueTree, 2> comparison;
    std::vector<juce::ValueTree> history;
    int historyIndex = -1, activeSlot = 0;
    bool restoring = false;
    juce::ValueTree baseline;
    juce::String presetTitle = "Setup A";
    float meterIn = 0, meterOut = 0;
    std::array<float,2> meterInput{},meterOutput{};
    int clipIn = 0, clipOut = 0;
    juce::TooltipWindow tooltips{this, 450};
    void timerCallback() override;
    void drawPanel(juce::Graphics&);
    void presetMenu();
    void savePreset();
    void loadPreset();
    void moreOptions();
    void setParameter(const juce::String&, float);
    void recordChange();
    void restore(const juce::ValueTree&);
    void undoState(int);
    void switchComparison();
    void cycleColor(int, bool reverse);
};
