#pragma once
#include <JuceHeader.h>

class PERCULATORAudioProcessor final : public juce::AudioProcessor
{
public:
    PERCULATORAudioProcessor();
    ~PERCULATORAudioProcessor() override = default;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    void loadImpulseResponse (const juce::File&);
    juce::String getIRName() const { return irName; }
    int getPanelColourIndex() const { return panelColour.load(); }
    void setPanelColourIndex (int i) { panelColour.store (juce::jlimit (0, 2, i)); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::dsp::Convolution convolution;
    juce::dsp::DryWetMixer<float> irMixer;
    juce::String irName { "No IR Loaded" };
    juce::File irFile;
    std::atomic<int> panelColour { 0 };
    double sampleRateHz = 44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PERCULATORAudioProcessor)
};
