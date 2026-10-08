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
    l.add (std::make_unique<F> (juce::ParameterID { "harmonics", 1 }, "Harmonics", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.5f));
    l.add (std::make_unique<F> (juce::ParameterID { "balance", 1 }, "Balance", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "input", 1 }, "Input", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "bias", 1 }, "Bias", juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f), 0.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    juce::NormalisableRange<float> outputRange (-24.0f, 12.0f, 0.1f);
    outputRange.setSkewForCentre (0.0f);
    l.add (std::make_unique<F> (juce::ParameterID { "output", 1 }, "Output", outputRange, 0.0f));
    l.add (std::make_unique<C> (juce::ParameterID { "circuit", 1 }, "Circuit", juce::StringArray { "NPN OD", "D310 Diodes", "Albino" }, 1));
    l.add (std::make_unique<B> (juce::ParameterID { "iron", 1 }, "IR On", false));
    l.add (std::make_unique<F> (juce::ParameterID { "irmix", 1 }, "IR Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    l.add (std::make_unique<F> (juce::ParameterID { "irlevel", 1 }, "IR Level", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
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
    dcX1.fill (0.0f); dcY1.fill (0.0f);
    inputMeter.store (0.0f); outputMeter.store (0.0f);
}

bool PERCULATORAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && out == l.getMainInputChannelSet();
}

float PERCULATORAudioProcessor::processCircuitSample (float x, int channel, int circuit, float h, float b) noexcept
{
    const auto ch = static_cast<size_t> (juce::jlimit (0, 1, channel));
    const float drive = 1.0f + 2.7f * h;
    const float asymmetry = juce::jlimit (0.55f, 1.45f, 1.0f + 0.35f * b);

    if (circuit == 0)
    {
        const float z = drive * x;
        const float shaped = z >= 0.0f ? std::tanh (z * asymmetry)
                                       : std::tanh (z / asymmetry);
        return 0.86f * shaped + 0.08f * std::tanh (4.0f * x);
    }

    if (circuit == 1)
    {
        const float z = drive * x;
        return z >= 0.0f ? 0.88f * std::tanh (1.30f * z * asymmetry)
                         : 0.69f * std::tanh (1.85f * z / asymmetry);
    }

    const float lowA = std::exp (-juce::MathConstants<float>::twoPi * 18.0f
                                 / static_cast<float> (sampleRateHz));
    albinoLowState[ch] = lowA * albinoLowState[ch] + (1.0f - lowA) * x;
    const float z = (1.3f + 3.0f * h) * (x - 0.34f * albinoLowState[ch]);
    const float ge = z >= 0.0f ? 0.74f * std::tanh (1.10f * z * asymmetry)
                               : 0.96f * std::tanh (1.62f * z / asymmetry);
    const float si = 0.82f * std::tanh (1.52f * ge + 0.20f * z);
    const float di = si >= 0.0f ? 0.86f * std::tanh (1.38f * si * asymmetry)
                                : 0.70f * std::tanh (1.88f * si / asymmetry);
    const float highA = std::exp (-juce::MathConstants<float>::twoPi * 6800.0f
                                  / static_cast<float> (sampleRateHz));
    albinoHighState[ch] = highA * albinoHighState[ch] + (1.0f - highA) * di;
    return 0.84f * albinoHighState[ch] + 0.16f * di;
}

float PERCULATORAudioProcessor::peakToMeter (float peak) noexcept
{
    if (peak < 0.00002f)
        return 0.0f;

    const float dB = juce::Decibels::gainToDecibels (peak, -60.0f);
    return juce::jlimit (0.0f, 1.0f, (dB + 18.0f) / 18.0f);
}

void PERCULATORAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float inputDb = apvts.getRawParameterValue ("input")->load();
    const float outputDb = apvts.getRawParameterValue ("output")->load();
    const float harmonics = apvts.getRawParameterValue ("harmonics")->load() / 10.0f;
    const float balance = apvts.getRawParameterValue ("balance")->load() / 10.0f;
    const float bias = apvts.getRawParameterValue ("bias")->load() / 10.0f;
    const float wetMix = apvts.getRawParameterValue ("mix")->load() / 100.0f;
    const int circuit = juce::roundToInt (apvts.getRawParameterValue ("circuit")->load());
    const bool bypassed = apvts.getRawParameterValue ("bypass")->load() > 0.5f;

    const float circuitCalibration[]
    {
        juce::Decibels::decibelsToGain (27.0f),
        juce::Decibels::decibelsToGain (29.0f),
        juce::Decibels::decibelsToGain (28.0f)
    };

    const float userInputGain = juce::Decibels::decibelsToGain (inputDb);
    const float circuitInputGain = circuitCalibration[juce::jlimit (0, 2, circuit)];
    const float outputGain = juce::Decibels::decibelsToGain (outputDb);
    const float dcR = std::exp (-juce::MathConstants<float>::twoPi * 10.0f
                                / static_cast<float> (sampleRateHz));

    float inputPeak = 0.0f;
    float outputPeak = 0.0f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);
        const int stateChannel = juce::jmin (channel, 1);
        const auto stateIndex = static_cast<size_t> (stateChannel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float dry = samples[sample];
            const float meteredInput = dry * userInputGain;
            inputPeak = juce::jmax (inputPeak, std::abs (meteredInput));
            const float circuitInput = meteredInput * circuitInputGain;

            if (std::abs (meteredInput) < 1.0e-7f)
            {
                memory[stateIndex] *= 0.995f;
                albinoLowState[stateIndex] *= 0.995f;
                albinoHighState[stateIndex] *= 0.995f;
            }

            memory[stateIndex] = 0.9975f * memory[stateIndex] + 0.0025f * circuitInput;
            float processed = processCircuitSample (circuitInput - 0.12f * memory[stateIndex],
                                                    stateChannel, circuit, harmonics, bias);
            processed *= juce::jmap (balance, 0.15f, 1.25f);

            const float dcBlocked = processed - dcX1[stateIndex] + dcR * dcY1[stateIndex];
            dcX1[stateIndex] = processed;
            dcY1[stateIndex] = dcBlocked;

            float wetSample = dcBlocked;
            if (std::abs (meteredInput) < 1.0e-7f && std::abs (wetSample) < 1.0e-6f)
                wetSample = 0.0f;

<<<<<<< HEAD
            samples[sample] = bypassed ? dry : juce::jmap (wetMix, dry, wetSample);
        }
    }

    const bool irEnabled = apvts.getRawParameterValue ("iron")->load() > 0.5f;
    if (! bypassed && irEnabled && irFile.existsAsFile())
    {
        irMixer.setWetMixProportion (apvts.getRawParameterValue ("irmix")->load() / 100.0f);
        juce::dsp::AudioBlock<float> block (buffer);
        irMixer.pushDrySamples (block);
        juce::dsp::ProcessContextReplacing<float> context (block);
        convolution.process (context);
        irMixer.mixWetSamples (block);
        buffer.applyGain (juce::Decibels::decibelsToGain (
            apvts.getRawParameterValue ("irlevel")->load()));
=======
            const auto index = static_cast<size_t> (stateChannel);

            const float dcBlockedSample =
                processedSample
                - dcX1[index]
                + dcBlockCoefficient * dcY1[index];

            dcX1[index] = processedSample;
            dcY1[index] = dcBlockedSample;

            float finalProcessedSample = dcBlockedSample;

            if (std::abs (meteredInput) < 1.0e-7f
                && std::abs (finalProcessedSample) < 1.0e-6f)
            {
                finalProcessedSample = 0.0f;
            }

            samples[sample] = bypassed
    ? drySample
    : juce::jmap (
        wetMix,
        drySample,
        finalProcessedSample);
        }
    }

    const bool irEnabled =
        apvts.getRawParameterValue ("iron")->load() > 0.5f;

      if (! bypassed && irEnabled && irFile.existsAsFile())
    {
        const float irWetMix =
            apvts.getRawParameterValue ("irmix")->load()
            / 100.0f;

        irMixer.setWetMixProportion (irWetMix);

        juce::dsp::AudioBlock<float> audioBlock (buffer);
        irMixer.pushDrySamples (audioBlock);

        juce::dsp::ProcessContextReplacing<float> context (
            audioBlock);

        convolution.process (context);
        irMixer.mixWetSamples (audioBlock);

        const float irLevel =
            juce::Decibels::decibelsToGain (
                apvts.getRawParameterValue ("irlevel")->load());

        buffer.applyGain (irLevel);
>>>>>>> 594760541938d596cfa0122b428573341b34098e

        if (apvts.getRawParameterValue ("phase")->load() > 0.5f)
            buffer.applyGain (-1.0f);
    }

<<<<<<< HEAD
    // Output is applied exactly once, after the complete active chain.
    if (! bypassed)
        buffer.applyGain (outputGain);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        outputPeak = juce::jmax (outputPeak,
                                 buffer.getMagnitude (channel, 0, buffer.getNumSamples()));

    inputMeter.store (juce::jmax (peakToMeter (inputPeak), inputMeter.load() * 0.82f));
    outputMeter.store (juce::jmax (peakToMeter (outputPeak), outputMeter.load() * 0.82f));
}

void PERCULATORAudioProcessor::loadImpulseResponse (const juce::File& f)
=======
    /*
        Output è l'ultimo stadio della catena.
        In bypass il segnale resta a guadagno unitario.
    */
    if (! bypassed)
        buffer.applyGain (outputGain);

    for (int channel = 0;
         channel < buffer.getNumChannels();
         ++channel)
    {
        outputPeak = juce::jmax (
            outputPeak,
            buffer.getMagnitude (
                channel,
                0,
                buffer.getNumSamples()));
    }

void PERCULATORAudioProcessor::loadImpulseResponse (const juce::File& file)
>>>>>>> 594760541938d596cfa0122b428573341b34098e
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
