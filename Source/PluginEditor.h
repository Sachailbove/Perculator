#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PerculatorKnob final : public juce::Slider
{
public:
    PerculatorKnob();
    void paint (juce::Graphics&) override;
};


class IrOnLedButton final : public juce::TextButton
{
public:
    IrOnLedButton();
    void paintButton (juce::Graphics&, bool isMouseOverButton,
                      bool isButtonDown) override;
};

class PhaseToggleSwitch final : public juce::TextButton
{
public:
    PhaseToggleSwitch();
    void paintButton (juce::Graphics&, bool isMouseOverButton,
                      bool isButtonDown) override;
};

class BypassFootswitchButton final : public juce::TextButton
{
public:
    BypassFootswitchButton();
    void paintButton (juce::Graphics&, bool isMouseOverButton,
                      bool isButtonDown) override;
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

    void timerCallback() override;
    void chooseIR();
    void configureKnob (PerculatorKnob&, const juce::String& suffix, int decimals);
    void configureToggle (juce::TextButton&);
    void selectOversampling (int index);
    void refreshOversamplingButtons();
    void drawMeters (juce::Graphics&);
    void drawDynamicReadouts (juce::Graphics&);
    void drawStateLights (juce::Graphics&);
    juce::Rectangle<float> scaleRect (juce::Rectangle<float>) const;
    juce::Point<float> scalePoint (juce::Point<float>) const;
    void setControlBounds (juce::Component&, juce::Rectangle<int>);

    PERCULATORAudioProcessor& processor;
    juce::Image panelImage;

    PerculatorKnob harmonics, balance, circuit, input, bias, mix, output, irMix, irLevel;
    juce::TextButton loadIR { "LOAD IR..." };
    juce::TextButton previousIR { "<" };
    juce::TextButton nextIR { ">" };
    IrOnLedButton irOn;
    PhaseToggleSwitch phase;
    BypassFootswitchButton bypass;
    juce::TextButton os2x { "2x" }, os4x { "4x" }, os8x { "8x" };

    std::unique_ptr<SliderAttachment> aHarmonics, aBalance, aCircuit, aInput;
    std::unique_ptr<SliderAttachment> aBias, aMix, aOutput, aIrMix, aIrLevel;
    std::unique_ptr<ButtonAttachment> aIrOn, aPhase, aBypass;
    std::unique_ptr<juce::FileChooser> chooser;

    static constexpr float designWidth = 1536.0f;
    static constexpr float designHeight = 1024.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PERCULATORAudioProcessorEditor)
};
