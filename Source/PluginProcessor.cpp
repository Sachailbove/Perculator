#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

PERCULATORAudioProcessor::PERCULATORAudioProcessor()
 : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
   apvts (*this, nullptr, "STATE", createLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout PERCULATORAudioProcessor::createLayout()
{
    using F = juce::AudioParameterFloat; using B = juce::AudioParameterBool; using C = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<F> (juce::ParameterID { "harmonics", 1 }, "Harmonics", juce::NormalisableRange<float> (0, 10, 0.01f), 5.5f));
    l.add (std::make_unique<F> (juce::ParameterID { "balance", 1 }, "Balance", juce::NormalisableRange<float> (0, 10, 0.01f), 5.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "input", 1 }, "Input", juce::NormalisableRange<float> (-24, 24, 0.1f), 0.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "bias", 1 }, "Bias", juce::NormalisableRange<float> (-10, 10, 0.01f), 0.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> (0, 100, 0.1f), 100.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "output", 1 }, "Output", juce::NormalisableRange<float> (-24, 12, 0.1f), 0.0f));
    l.add (std::make_unique<C> (juce::ParameterID { "circuit", 1 }, "Circuit", juce::StringArray { "NPN OD", "D310 Diodes", "Albino" }, 1));
    l.add (std::make_unique<B> (juce::ParameterID { "iron", 1 }, "IR On", false));
    l.add (std::make_unique<F> (juce::ParameterID { "irmix", 1 }, "IR Mix", juce::NormalisableRange<float> (0, 100, 0.1f), 100.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "irlevel", 1 }, "IR Level", juce::NormalisableRange<float> (-24, 12, 0.1f), 0.0f));
    l.add (std::make_unique<B> (juce::ParameterID { "phase", 1 }, "Phase 180", false));
    l.add (std::make_unique<C> (juce::ParameterID { "oversampling", 1 }, "Oversampling", juce::StringArray { "2x", "4x", "8x" }, 1));
    l.add (std::make_unique<B> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return l;
}

void PERCULATORAudioProcessor::prepareToPlay (double sr, int block)
{
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) block, (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };
    sampleRateHz = sr;
    convolution.prepare (spec); irMixer.prepare (spec); irMixer.setWetMixProportion (1.0f);
    memory.fill (0.0f); albinoLowState.fill (0.0f); albinoHighState.fill (0.0f);
}

bool PERCULATORAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && out == l.getMainInputChannelSet();
}

float PERCULATORAudioProcessor::processCircuitSample (float x, int channel, int circuit, float h, float b) noexcept
{
    const float drive = 1.0f + 2.2f * h;
    if (circuit == 0) // Barge dual-NPN: firmer and more symmetric
        return 0.78f * std::tanh (drive * (x + 0.18f * b)) + 0.10f * std::tanh (4.0f * x);

    if (circuit == 1) // Giblet / D310 Diodes: asymmetric germanium-style knee
    {
        const float z = drive * x + 0.32f * b;
        return z >= 0.0f ? 0.88f * std::tanh (1.25f * z)
                         : 0.68f * std::tanh (1.85f * z);
    }

    // Sardonic Albinator / Alex Frias topology.
    // Q1 is low-gain 2N404A PNP germanium (hFE 45), Q2 is 2N3565 NPN
    // silicon (hFE 265), with 1N695 clipping, C3 2.2 uF and C7 1.5 nF.
    // This is a stable real-time circuit-inspired model, not a SPICE solver.
    const auto ch = (size_t) juce::jlimit (0, 1, channel);
    const float lowA = std::exp (-juce::MathConstants<float>::twoPi * 18.0f / (float) sampleRateHz);
    albinoLowState[ch] = lowA * albinoLowState[ch] + (1.0f - lowA) * x;
    const float inputAc = x - 0.34f * albinoLowState[ch];

    const float z = (1.15f + 2.65f * h) * inputAc + 0.42f * b;
    const float gePnp = z >= 0.0f ? 0.74f * std::tanh (1.10f * z)
                                 : 0.96f * std::tanh (1.62f * z);
    const float siliconNpn = 0.82f * std::tanh (1.52f * gePnp + 0.20f * z);
    const float diodePair = siliconNpn >= 0.0f ? 0.86f * std::tanh (1.38f * siliconNpn)
                                               : 0.70f * std::tanh (1.88f * siliconNpn);

    const float highA = std::exp (-juce::MathConstants<float>::twoPi * 6800.0f / (float) sampleRateHz);
    albinoHighState[ch] = highA * albinoHighState[ch] + (1.0f - highA) * diodePair;
    return 0.84f * albinoHighState[ch] + 0.16f * diodePair;
}

float PERCULATORAudioProcessor::peakToMeter (float p) noexcept
{
    const float db = juce::Decibels::gainToDecibels (juce::jmax (p, 0.000001f), -60.0f);
    return juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
}

void PERCULATORAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, buffer.getNumSamples());
    const auto inDb = apvts.getRawParameterValue ("input")->load();
    const auto outDb = apvts.getRawParameterValue ("output")->load();
    const auto h = apvts.getRawParameterValue ("harmonics")->load() / 10.0f;
    const auto bal = apvts.getRawParameterValue ("balance")->load() / 10.0f;
    const auto bias = apvts.getRawParameterValue ("bias")->load() / 10.0f;
    const auto wet = apvts.getRawParameterValue ("mix")->load() / 100.0f;
    const int circuit = juce::roundToInt (apvts.getRawParameterValue ("circuit")->load());
    const bool bypass = apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    const float inGain = juce::Decibels::decibelsToGain (inDb), outGain = juce::Decibels::decibelsToGain (outDb);
    float inPeak = 0.0f, outPeak = 0.0f;
    for (int c = 0; c < buffer.getNumChannels(); ++c)
    {
        auto* d = buffer.getWritePointer (c); const int mi = juce::jmin (c, 1);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            const float dry = d[n]; const float x = dry * inGain; inPeak = juce::jmax (inPeak, std::abs (x));
            memory[(size_t) mi] = 0.9975f * memory[(size_t) mi] + 0.0025f * x;
            float y = processCircuitSample (x - 0.12f * memory[(size_t) mi], mi, circuit, h, bias);
            y *= juce::jmap (bal, 0.15f, 1.35f);
            d[n] = bypass ? dry : juce::jmap (wet, dry, y) * outGain;
        }
    }
    if (! bypass && apvts.getRawParameterValue ("iron")->load() > 0.5f && irFile.existsAsFile())
    {
        irMixer.setWetMixProportion (apvts.getRawParameterValue ("irmix")->load() / 100.0f);
        juce::dsp::AudioBlock<float> block (buffer); irMixer.pushDrySamples (block);
        juce::dsp::ProcessContextReplacing<float> ctx (block); convolution.process (ctx); irMixer.mixWetSamples (block);
        buffer.applyGain (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("irlevel")->load()));
        if (apvts.getRawParameterValue ("phase")->load() > 0.5f) buffer.applyGain (-1.0f);
    }
    for (int c = 0; c < buffer.getNumChannels(); ++c)
        outPeak = juce::jmax (outPeak, buffer.getMagnitude (c, 0, buffer.getNumSamples()));
    inputMeter.store (juce::jmax (peakToMeter (inPeak), inputMeter.load() * 0.88f));
    outputMeter.store (juce::jmax (peakToMeter (outPeak), outputMeter.load() * 0.88f));
}

void PERCULATORAudioProcessor::loadImpulseResponse (const juce::File& f)
{
    if (! f.existsAsFile()) return;
    irFile = f; irName = f.getFileName();
    convolution.loadImpulseResponse (f, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes, 0, juce::dsp::Convolution::Normalise::yes);
}

void PERCULATORAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState(); state.setProperty ("irPath", irFile.getFullPathName(), nullptr); state.setProperty ("panelColour", panelColour.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}
void PERCULATORAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size)) { auto state = juce::ValueTree::fromXml (*xml); if (state.isValid()) { apvts.replaceState (state); panelColour.store ((int) state.getProperty ("panelColour", 1)); auto f = juce::File (state.getProperty ("irPath").toString()); if (f.existsAsFile()) loadImpulseResponse (f); } }
}
juce::AudioProcessorEditor* PERCULATORAudioProcessor::createEditor() { return new PERCULATORAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PERCULATORAudioProcessor(); }
