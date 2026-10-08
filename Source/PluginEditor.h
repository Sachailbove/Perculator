#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PedalLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
};

class MeterArc final : public juce::Component
{
public:
    MeterArc() { setInterceptsMouseClicks (false, false); }
    void setLevel (float v) { level = juce::jlimit (0.0f, 1.0f, v); repaint(); }
    void paint (juce::Graphics&) override;
private:
    float level = 0.0f;
};

class PERCULATORAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer,
                                              public juce::FileDragAndDropTarget
{
public:
    explicit PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor&);
    ~PERCULATORAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    void timerCallback() override;
    void chooseIR();
    void styleSlider (juce::Slider&, const juce::String&, const juce::String&);
    void addLabel (juce::Label&, const juce::String&);
    juce::Colour panelColour() const;

    PERCULATORAudioProcessor& processor;
    PedalLookAndFeel look;
    juce::Slider harmonics, balance, input, bias, mix, output, irMix, irLevel, circuit;
    MeterArc inputMeter, outputMeter;
    juce::TextButton loadIR { "LOAD IR..." }, prevIR { "<" }, nextIR { ">" };
    juce::ToggleButton irOn { "IR ON" }, phase { "180 deg" }, bypass { "BYPASS" };
    juce::Label irLabel, harmonicsLabel, balanceLabel, inputLabel, biasLabel, mixLabel, outputLabel, irMixLabel, irLevelLabel, circuitLabel;
    juce::ComboBox colourBox, oversamplingBox;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<SliderAttachment> aHarmonics, aBalance, aInput, aBias, aMix, aOutput, aIrMix, aIrLevel, aCircuit;
    std::unique_ptr<ButtonAttachment> aIrOn, aPhase, aBypass;
    std::unique_ptr<ComboAttachment> aOversampling;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PERCULATORAudioProcessorEditor)
};
