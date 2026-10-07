#include "PluginProcessor.h"
#include "PluginEditor.h"

PERCULATORAudioProcessor::PERCULATORAudioProcessor()
 : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
   apvts (*this, nullptr, "PARAMETERS", createLayout()), irMixer (4096)
{}

juce::AudioProcessorValueTreeState::ParameterLayout PERCULATORAudioProcessor::createLayout()
{
    using PF = juce::AudioParameterFloat;
    using PB = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add (std::make_unique<PF> ("input", "Input", juce::NormalisableRange<float> (-24.f, 24.f, 0.01f), 0.f));
    p.add (std::make_unique<PF> ("harmonics", "Harmonics", 0.f, 10.f, 5.f));
    p.add (std::make_unique<PF> ("bias", "Bias", -10.f, 10.f, 0.f));
    p.add (std::make_unique<PF> ("balance", "Balance", 0.f, 10.f, 7.f));
    p.add (std::make_unique<PF> ("mix", "Mix", 0.f, 100.f, 100.f));
    p.add (std::make_unique<PF> ("output", "Output", juce::NormalisableRange<float> (-24.f, 12.f, 0.01f), -6.f));
    p.add (std::make_unique<PB> ("studio", "Studio Mode", false));
    p.add (std::make_unique<PB> ("irOn", "IR On", false));
    p.add (std::make_unique<PF> ("irMix", "IR Mix", 0.f, 100.f, 100.f));
    p.add (std::make_unique<PF> ("irLevel", "IR Level", juce::NormalisableRange<float> (-24.f, 12.f, 0.01f), 0.f));
    p.add (std::make_unique<PB> ("phase", "IR Phase", false));
    p.add (std::make_unique<PB> ("bypass", "Bypass", false));
    return p;
}

void PERCULATORAudioProcessor::prepareToPlay (double sr, int block)
{
    sampleRateHz = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) block, (juce::uint32) getTotalNumOutputChannels() };
    convolution.prepare (spec);
    irMixer.prepare (spec);
    irMixer.setWetLatency (convolution.getLatency());
}

bool PERCULATORAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet()
        && (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
         || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void PERCULATORAudioProcessor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    if (apvts.getRawParameterValue ("bypass")->load() > 0.5f) return;
    const auto inputGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("input")->load());
    const auto outputGain = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load());
    const auto drive = 1.0f + 2.2f * apvts.getRawParameterValue ("harmonics")->load();
    const auto bias = 0.055f * apvts.getRawParameterValue ("bias")->load();
    const auto balance = 0.25f + 0.125f * apvts.getRawParameterValue ("balance")->load();
    const auto wet = apvts.getRawParameterValue ("mix")->load() * 0.01f;
    const bool studio = apvts.getRawParameterValue ("studio")->load() > 0.5f;

    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        auto* x = b.getWritePointer (ch);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float dry = x[i];
            float v = dry * inputGain;
            const float ge = std::tanh (v * drive + bias) - std::tanh (bias);
            const float si = ge >= 0.f ? std::tanh (ge * (studio ? 1.65f : 2.05f))
                                      : 0.82f * std::tanh (ge * (studio ? 1.35f : 1.75f));
            const float shaped = si * balance;
            x[i] = (dry * (1.0f - wet) + shaped * wet) * outputGain;
        }
    }

    const bool irOn = apvts.getRawParameterValue ("irOn")->load() > 0.5f && irFile.existsAsFile();
    if (irOn)
    {
        juce::dsp::AudioBlock<float> block (b);
        juce::dsp::ProcessContextReplacing<float> context (block);
        irMixer.setWetMixProportion (apvts.getRawParameterValue ("irMix")->load() * 0.01f);
        irMixer.pushDrySamples (block);
        convolution.process (context);
        block.multiplyBy (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("irLevel")->load()));
        if (apvts.getRawParameterValue ("phase")->load() > 0.5f) block.multiplyBy (-1.0f);
        irMixer.mixWetSamples (block);
    }
}

void PERCULATORAudioProcessor::loadImpulseResponse (const juce::File& f)
{
    if (! f.existsAsFile()) return;
    irFile = f; irName = f.getFileName();
    convolution.loadImpulseResponse (f, juce::dsp::Convolution::Stereo::yes,
                                     juce::dsp::Convolution::Trim::yes, 0,
                                     juce::dsp::Convolution::Normalise::yes);
    apvts.getParameter ("irOn")->setValueNotifyingHost (1.0f);
}

void PERCULATORAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("irPath", irFile.getFullPathName(), nullptr);
    state.setProperty ("panelColour", panelColour.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void PERCULATORAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            apvts.replaceState (state);
            setPanelColourIndex ((int) state.getProperty ("panelColour", 0));
            juce::File f (state.getProperty ("irPath").toString());
            if (f.existsAsFile()) loadImpulseResponse (f);
        }
    }
}

juce::AudioProcessorEditor* PERCULATORAudioProcessor::createEditor() { return new PERCULATORAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PERCULATORAudioProcessor(); }
