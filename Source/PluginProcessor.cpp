#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

PERCULATORAudioProcessor::PERCULATORAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
PERCULATORAudioProcessor::createParameterLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<F> (juce::ParameterID { "harmonics", 1 }, "Harmonics",
                                     juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.5f));
    layout.add (std::make_unique<F> (juce::ParameterID { "balance", 1 }, "Balance",
                                     juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f));
    layout.add (std::make_unique<C> (juce::ParameterID { "circuit", 1 }, "Circuit",
                                     juce::StringArray { "NPN OD", "D310 Diodes", "Albino" }, 1));
    layout.add (std::make_unique<F> (juce::ParameterID { "input", 1 }, "Input",
                                     juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (juce::ParameterID { "bias", 1 }, "Bias",
                                     juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<F> (juce::ParameterID { "mix", 1 }, "Mix",
                                     juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    juce::NormalisableRange<float> outputRange (-24.0f, 12.0f, 0.1f);
    outputRange.setSkewForCentre (0.0f);
    layout.add (std::make_unique<F> (juce::ParameterID { "output", 1 }, "Output",
                                     outputRange, 0.0f));

    layout.add (std::make_unique<B> (juce::ParameterID { "iron", 1 }, "IR On", false));
    layout.add (std::make_unique<F> (juce::ParameterID { "irmix", 1 }, "IR Mix",
                                     juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    juce::NormalisableRange<float> irLevelRange (-24.0f, 12.0f, 0.1f);
    irLevelRange.setSkewForCentre (0.0f);
    layout.add (std::make_unique<F> (juce::ParameterID { "irlevel", 1 }, "IR Level",
                                     irLevelRange, 0.0f));
    layout.add (std::make_unique<B> (juce::ParameterID { "phase", 1 }, "Phase 180", false));
    layout.add (std::make_unique<C> (juce::ParameterID { "oversampling", 1 }, "Oversampling",
                                     juce::StringArray { "2x", "4x", "8x" }, 1));
    layout.add (std::make_unique<B> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}

void PERCULATORAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    const juce::dsp::ProcessSpec spec { sampleRate,
                                        static_cast<juce::uint32> (samplesPerBlock),
                                        static_cast<juce::uint32> (juce::jmax (1, getTotalNumOutputChannels())) };
    convolution.prepare (spec);
    irMixer.prepare (spec);
    memory.fill (0.0f);
    albinoLowState.fill (0.0f);
    albinoHighState.fill (0.0f);
    dcX1.fill (0.0f);
    dcY1.fill (0.0f);
    inputMeter.store (0.0f);
    outputMeter.store (0.0f);
}

bool PERCULATORAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
            && output == layouts.getMainInputChannelSet();
}

float PERCULATORAudioProcessor::processCircuitSample (float x, int channel, int circuit,
                                                       float h, float b) noexcept
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

    const float calibration[] { juce::Decibels::decibelsToGain (27.0f),
                                juce::Decibels::decibelsToGain (29.0f),
                                juce::Decibels::decibelsToGain (28.0f) };
    const float userInputGain = juce::Decibels::decibelsToGain (inputDb);
    constexpr float internalInputBoostDb = 7.0f;
    const float circuitInputGain = calibration[juce::jlimit (0, 2, circuit)]
                                 * juce::Decibels::decibelsToGain (internalInputBoostDb);
    const float outputGain = juce::Decibels::decibelsToGain (outputDb);
    const float dcR = std::exp (-juce::MathConstants<float>::twoPi * 10.0f
                                / static_cast<float> (sampleRateHz));

    float inputPeak = 0.0f;
    float outputPeak = 0.0f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);
        const auto state = static_cast<size_t> (juce::jmin (channel, 1));

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float dry = samples[sample];
            const float meteredInput = dry * userInputGain;
            inputPeak = juce::jmax (inputPeak, std::abs (meteredInput));
            const float circuitInput = meteredInput * circuitInputGain;

            if (std::abs (meteredInput) < 1.0e-7f)
            {
                memory[state] *= 0.995f;
                albinoLowState[state] *= 0.995f;
                albinoHighState[state] *= 0.995f;
            }

            memory[state] = 0.9975f * memory[state] + 0.0025f * circuitInput;
            float processed = processCircuitSample (circuitInput - 0.12f * memory[state],
                                                     static_cast<int> (state), circuit,
                                                     harmonics, bias);
            processed *= juce::jmap (balance, 0.15f, 1.25f);

            const float dcBlocked = processed - dcX1[state] + dcR * dcY1[state];
            dcX1[state] = processed;
            dcY1[state] = dcBlocked;

            const float wetSignal = (std::abs (meteredInput) < 1.0e-7f
                                     && std::abs (dcBlocked) < 1.0e-6f) ? 0.0f : dcBlocked;
            samples[sample] = bypassed ? dry : juce::jmap (wetMix, dry, wetSignal);
        }
    }

    if (! bypassed && apvts.getRawParameterValue ("iron")->load() > 0.5f
        && irFile.existsAsFile())
    {
        irMixer.setWetMixProportion (apvts.getRawParameterValue ("irmix")->load() / 100.0f);
        juce::dsp::AudioBlock<float> block (buffer);
        irMixer.pushDrySamples (block);
        juce::dsp::ProcessContextReplacing<float> context (block);
        convolution.process (context);
        irMixer.mixWetSamples (block);
        buffer.applyGain (juce::Decibels::decibelsToGain (
            apvts.getRawParameterValue ("irlevel")->load()));
        if (apvts.getRawParameterValue ("phase")->load() > 0.5f)
            buffer.applyGain (-1.0f);
    }

    if (! bypassed)
        buffer.applyGain (outputGain);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        outputPeak = juce::jmax (outputPeak,
                                 buffer.getMagnitude (channel, 0, buffer.getNumSamples()));

    const auto updateMeter = [] (float currentLevel, float previousLevel)
    {
        const float released = juce::jmax (currentLevel, previousLevel * 0.82f);
        return released < 0.01f ? 0.0f : released;
    };

    inputMeter.store (updateMeter (peakToMeter (inputPeak), inputMeter.load()));
    outputMeter.store (updateMeter (peakToMeter (outputPeak), outputMeter.load()));
}

void PERCULATORAudioProcessor::loadImpulseResponse (const juce::File& file)
{
    if (! file.existsAsFile())
        return;
    irFile = file;
    irName = file.getFileName();
    convolution.loadImpulseResponse (file, juce::dsp::Convolution::Stereo::yes,
                                     juce::dsp::Convolution::Trim::yes, 0,
                                     juce::dsp::Convolution::Normalise::yes);
}

bool PERCULATORAudioProcessor::loadAdjacentImpulseResponse (int direction)
{
    if (! irFile.existsAsFile() || direction == 0)
        return false;

    const auto folder = irFile.getParentDirectory();
    juce::Array<juce::File> files;
    folder.findChildFiles (files, juce::File::findFiles, false,
                           "*.wav;*.aif;*.aiff;*.flac");

    if (files.isEmpty())
        return false;

    struct FileNameComparator
    {
        static int compareElements (const juce::File& a, const juce::File& b)
        {
            return a.getFileName().compareNatural (b.getFileName(), true);
        }
    } comparator;

    files.sort (comparator, true);

    int currentIndex = files.indexOf (irFile);
    if (currentIndex < 0)
    {
        for (int i = 0; i < files.size(); ++i)
        {
            if (files.getReference (i).getFullPathName()
                    .equalsIgnoreCase (irFile.getFullPathName()))
            {
                currentIndex = i;
                break;
            }
        }
    }

    if (currentIndex < 0)
        return false;

    const int step = direction < 0 ? -1 : 1;
    const int nextIndex = (currentIndex + step + files.size()) % files.size();
    loadImpulseResponse (files.getReference (nextIndex));
    return true;
}

void PERCULATORAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto state = apvts.copyState();
    state.setProperty ("irPath", irFile.getFullPathName(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destination);
}

void PERCULATORAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            apvts.replaceState (state);
            const juce::File file (state.getProperty ("irPath").toString());
            if (file.existsAsFile())
                loadImpulseResponse (file);
        }
    }
}

juce::AudioProcessorEditor* PERCULATORAudioProcessor::createEditor()
{
    return new PERCULATORAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PERCULATORAudioProcessor();
}
