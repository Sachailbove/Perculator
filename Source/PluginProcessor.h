#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class PERCULATORAudioProcessor final : public juce::AudioProcessor
{
public:
    PERCULATORAudioProcessor();
    ~PERCULATORAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
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
    float getInputMeter() const { return inputMeter.load(); }
    float getOutputMeter() const { return outputMeter.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float processCircuitSample (float x, int channel, int circuit,
                                float harmonics, float bias) noexcept;
    static float peakToMeter (float peak) noexcept;

    juce::dsp::Convolution convolution;
    juce::dsp::DryWetMixer<float> irMixer;
    juce::String irName { "No IR loaded" };
    juce::File irFile;

    std::atomic<float> inputMeter { 0.0f };
    std::atomic<float> outputMeter { 0.0f };

    std::array<float, 2> memory {};
    std::array<float, 2> albinoLowState {};
    std::array<float, 2> albinoHighState {};
    std::array<float, 2> dcX1 {};
    std::array<float, 2> dcY1 {};
    double sampleRateHz = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PERCULATORAudioProcessor)
};
