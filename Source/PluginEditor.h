#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ImageKnob final : public juce::Slider
{
public:
    ImageKnob (const juce::Image& sourceImage,
               juce::Rectangle<int> sourceArea,
               float defaultNormalisedPosition = 0.5f);

    void paint (juce::Graphics&) override;

private:
    juce::Image source;
    juce::Rectangle<int> sourceArea;
    float defaultPosition = 0.5f;
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
    void drawMeters (juce::Graphics&);
    void drawDynamicReadouts (juce::Graphics&);
    void drawStateLights (juce::Graphics&);
    void setControlBounds (juce::Component&, juce::Rectangle<int> designBounds);
    juce::Rectangle<float> scaleRect (juce::Rectangle<float>) const;
    juce::Point<float> scalePoint (juce::Point<float>) const;

    PERCULATORAudioProcessor& processor;
    juce::Image panelImage;

    ImageKnob harmonics;
    ImageKnob balance;
    ImageKnob circuit;
    ImageKnob input;
    ImageKnob bias;
    ImageKnob mix;
    ImageKnob output;
    ImageKnob irMix;
    ImageKnob irLevel;

    juce::TextButton loadIR;
    juce::TextButton previousIR;
    juce::TextButton nextIR;
    juce::ToggleButton irOn;
    juce::ToggleButton phase;
    juce::ToggleButton bypass;
    juce::ComboBox oversampling;

    std::unique_ptr<juce::FileChooser> chooser;

    std::unique_ptr<SliderAttachment> aHarmonics, aBalance, aCircuit;
    std::unique_ptr<SliderAttachment> aInput, aBias, aMix, aOutput;
    std::unique_ptr<SliderAttachment> aIrMix, aIrLevel;
    std::unique_ptr<ButtonAttachment> aIrOn, aPhase, aBypass;
    std::unique_ptr<ComboAttachment> aOversampling;

    static constexpr float designWidth  = 1536.0f;
    static constexpr float designHeight = 1024.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PERCULATORAudioProcessorEditor)
};
