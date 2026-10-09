#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PerculatorLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    PerculatorLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float,
                           float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int,
                       juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

class ArcMeter final : public juce::Component
{
public:
    enum class Type { input, output };
    ArcMeter (PERCULATORAudioProcessor&, Type);
    void paint (juce::Graphics&) override;
private:
    PERCULATORAudioProcessor& processor;
    Type type;
};

class CircuitSelector final : public juce::Component
{
public:
    CircuitSelector();
    juce::Slider slider;
    void paint (juce::Graphics&) override;
    void resized() override;
};

class OversamplingSelector final : public juce::Component
{
public:
    OversamplingSelector();
    juce::ComboBox combo;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
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
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;
    void chooseIR();
    void configureKnob (juce::Slider&, const juce::String&, int decimals);
    void configureLabel (juce::Label&, const juce::String&, float size);
    void setScaledBounds (juce::Component&, int, int, int, int);
    juce::Colour currentPanelColour() const;
    void drawPanel (juce::Graphics&);
    void drawScrew (juce::Graphics&, float, float, float);

    PERCULATORAudioProcessor& processor;
    PerculatorLookAndFeel lookAndFeel;

    juce::ComboBox panelColourBox;
    juce::Label titleLabel, subtitleLabel;
    juce::Label harmonicsLabel, balanceLabel, inputLabel, biasLabel;
    juce::Label mixLabel, outputLabel, cabinetLabel;
    juce::Label irMixLabel, irLevelLabel, phaseLabel, circuitLabel;

    juce::Slider harmonics, balance, input, bias, mix, output, irMix, irLevel;
    ArcMeter inputMeter, outputMeter;
    CircuitSelector circuitSelector;
    OversamplingSelector oversamplingSelector;

    juce::Label irFileLabel;
    juce::TextButton previousIR { "<" }, nextIR { ">" }, loadIR { "LOAD IR..." };
    juce::ToggleButton irOn { "IR ON" }, phase { "PHASE" }, bypass { "BYPASS" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    std::unique_ptr<SliderAttachment> aHarmonics, aBalance, aInput, aBias;
    std::unique_ptr<SliderAttachment> aMix, aOutput, aIrMix, aIrLevel, aCircuit;
    std::unique_ptr<ButtonAttachment> aIrOn, aPhase, aBypass;
    std::unique_ptr<ComboAttachment> aOversampling;

    static constexpr float designWidth = 1200.0f;
    static constexpr float designHeight = 750.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PERCULATORAudioProcessorEditor)
};
