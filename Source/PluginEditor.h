#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PerculatorKnob : public juce::Slider
{
public:
    PerculatorKnob();
    void paint (juce::Graphics& g) override;
};

class PERCULATORAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      public juce::Timer,
                                      public juce::FileDragAndDropTarget
{
public:
    PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor&);
    ~PERCULATORAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    PERCULATORAudioProcessor& processor;

    juce::Image panelImage;

    PerculatorKnob harmonics, balance, circuit, input, bias, mix, output, irMix, irLevel;
    juce::TextButton loadIR, previousIR, nextIR, irOn, phase, bypass;
    juce::TextButton os2x, os4x, os8x;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> aHarmonics, aBalance, aCircuit, aInput, aBias, aMix, aOutput, aIrMix, aIrLevel;
    std::unique_ptr<ButtonAttachment> aIrOn, aPhase, aBypass;

    std::unique_ptr<juce::FileChooser> chooser;

    void chooseIR();
    void drawMeters (juce::Graphics& g);
    void drawDynamicReadouts (juce::Graphics& g);
    void drawStateLights (juce::Graphics& g);

    juce::Rectangle<float> scaleRect (juce::Rectangle<float> r) const;
    juce::Point<float> scalePoint (juce::Point<float> p) const;
    void setControlBounds (juce::Component& c, juce::Rectangle<int> r);

    static constexpr float designWidth = 1536.0f;
    static constexpr float designHeight = 1024.0f;
};
