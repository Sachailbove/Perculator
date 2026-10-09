#include "PluginProcessor.h"
#include "PluginEditor.h"

PERCULATORAudioProcessor::PERCULATORAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                 #if ! JucePlugin_IsMidiEffect
                  #if ! JucePlugin_IsSynth
                   .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                  #endif
                   .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                 #endif
                   ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

PERCULATORAudioProcessor::~PERCULATORAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PERCULATORAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("harmonics", "Harmonics", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("balance", "Balance", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("circuit", "Circuit", juce::StringArray { "NPN OD", "D310", "Albino" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("input", "Input", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("bias", "Bias", juce::NormalisableRange<float> (-10.0f, 10.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("output", "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("irmix", "IR Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("irlevel", "IR Level", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}

const juce::String PERCULATORAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PERCULATORAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PERCULATORAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PERCULATORAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PERCULATORAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PERCULATORAudioProcessor::getNumPrograms()
{
    return 1;
}

int PERCULATORAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PERCULATORAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PERCULATORAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PERCULATORAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void PERCULATORAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void PERCULATORAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PERCULATORAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PERCULATORAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Aggiornamento dei meter in tempo reale basato sul buffer audio
    float inRms = buffer.getNumChannels() > 0 ? buffer.getRMSLevel (0, 0, buffer.getNumSamples()) : 0.0f;
    inputMeter.store (inRms * 2.0f);
    outputMeter.store (inRms * 2.0f);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);
    }
}

bool PERCULATORAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PERCULATORAudioProcessor::createEditor()
{
    return new PERCULATORAudioProcessorEditor (*this);
}

void PERCULATORAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PERCULATORAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}
