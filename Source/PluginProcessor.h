#pragma once

#include <JuceHeader.h>

class PERCULATORAudioProcessor  : public juce::AudioProcessor
{
public:
    PERCULATORAudioProcessor();
    ~PERCULATORAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // =========================================================================
    // APVTS pubblico (necessario per la lettura dei parametri nell'Editor)
    // =========================================================================
    juce::AudioProcessorValueTreeState apvts;

    // =========================================================================
    // Metodi inline richiesti dall'Editor (NON reinserirli nel .cpp)
    // =========================================================================
    juce::String getIRName() const { return irName; }
    float getInputMeter() const { return inputMeter.load(); }
    float getOutputMeter() const { return outputMeter.load(); }

private:
    // Variabili di stato interne per IR e Meter
    juce::String irName { "No IR loaded" };
    std::atomic<float> inputMeter { 0.0f };
    std::atomic<float> outputMeter { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PERCULATORAudioProcessor)
};
