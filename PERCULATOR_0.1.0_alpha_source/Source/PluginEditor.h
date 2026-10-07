#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PERCULATORAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              public juce::FileDragAndDropTarget
{
public:
    explicit PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor&);
    ~PERCULATORAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    PERCULATORAudioProcessor& processor;
    juce::Slider harmonics, balance, input, bias, mix, output, irMix, irLevel;
    juce::ToggleButton studio { "ORIGINAL / STUDIO" }, irOn { "IR ON" }, phase { "PHASE 180" }, bypass { "BYPASS" };
    juce::TextButton loadIR { "LOAD IR..." };
    juce::ComboBox colourBox;
    juce::Label irLabel;
    std::vector<std::unique_ptr<SA>> sliderAttachments;
    std::vector<std::unique_ptr<BA>> buttonAttachments;
    std::unique_ptr<juce::FileChooser> chooser;
    void configureKnob (juce::Slider&, const juce::String& suffix);
    void chooseIR();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PERCULATORAudioProcessorEditor)
};
