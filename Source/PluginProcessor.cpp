#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

PERCULATORAudioProcessor::PERCULATORAudioProcessor()
    : AudioProcessor (
          BusesProperties()
              .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
PERCULATORAudioProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<F> (
        juce::ParameterID { "harmonics", 1 },
        "Harmonics",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.5f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "balance", 1 },
        "Balance",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
        5.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "input", 1 },
        "Input",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "bias", 1 },
        "Bias",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f),
        0.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        100.0f));

    // 0 dB is the centre of the rotary travel, so the indicator is vertical.
    juce::NormalisableRange<float> outputRange (-24.0f, 12.0f, 0.1f);
    outputRange.setSkewForCentre (0.0f);

    layout.add (std::make_unique<F> (
        juce::ParameterID { "output", 1 },
        "Output",
        outputRange,
        0.0f));

    layout.add (std::make_unique<C> (
        juce::ParameterID { "circuit", 1 },
        "Circuit",
        juce::StringArray { "NPN OD", "D310 Diodes", "Albino" },
        1));

    layout.add (std::make_unique<B> (
        juce::ParameterID { "iron", 1 },
        "IR On",
        false));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "irmix", 1 },
        "IR Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        100.0f));

    layout.add (std::make_unique<F> (
        juce::ParameterID { "irlevel", 1 },
        "IR Level",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f),
        0.0f));

    layout.add (std::make_unique<B> (
        juce::ParameterID { "phase", 1 },
        "Phase 180",
        false));

    layout.add (std::make_unique<C> (
        juce::ParameterID { "oversampling", 1 },
        "Oversampling",
        juce::StringArray { "2x", "4x", "8x" },
        1));

    layout.add (std::make_unique<B> (
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false));

    return layout;
}

void PERCULATORAudioProcessor::prepareToPlay (double sampleRate,
                                               int maximumBlockSize)
{
    sampleRateHz = sampleRate;

    const juce::dsp::ProcessSpec spec
    {
        sampleRate,
        static_cast<juce::uint32> (maximumBlockSize),
        static_cast<juce::uint32> (juce::jmax (1, getTotalNumOutputChannels()))
    };

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

bool PERCULATORAudioProcessor::isBusesLayoutSupported (
    const BusesLayout& layouts) const
{
    const auto outputLayout = layouts.getMainOutputChannelSet();

    return (outputLayout == juce::AudioChannelSet::mono()
            || outputLayout == juce::AudioChannelSet::stereo())
        && outputLayout == layouts.getMainInputChannelSet();
}

float PERCULATORAudioProcessor::processCircuitSample (
    float input,
    int channel,
    int circuit,
    float harmonics,
    float bias) noexcept
{
    const auto stateChannel = static_cast<size_t> (juce::jlimit (0, 1, channel));
    const float drive = 1.0f + 2.7f * harmonics;

    // Bias changes asymmetry but never adds a DC source by itself.
    const float asymmetry = juce::jlimit (0.55f, 1.45f, 1.0f + 0.35f * bias);

    if (circuit == 0)
    {
        const float z = drive * input;
        const float shaped = z >= 0.0f
            ? std::tanh (z * asymmetry)
            : std::tanh (z / asymmetry);

        return 0.86f * shaped + 0.08f * std::tanh (4.0f * input);
    }

    if (circuit == 1)
    {
        const float z = drive * input;

        return z >= 0.0f
            ? 0.88f * std::tanh (1.30f * z * asymmetry)
            : 0.69f * std::tanh (1.85f * z / asymmetry);
    }

    // Albino: circuit-inspired PNP germanium + NPN silicon response.
    const float lowCoefficient = std::exp (
        -juce::MathConstants<float>::twoPi
        * 18.0f
        / static_cast<float> (sampleRateHz));

    albinoLowState[stateChannel] =
        lowCoefficient * albinoLowState[stateChannel]
        + (1.0f - lowCoefficient) * input;

    const float z = (1.3f + 3.0f * harmonics)
        * (input - 0.34f * albinoLowState[stateChannel]);

    const float germaniumStage = z >= 0.0f
        ? 0.74f * std::tanh (1.10f * z * asymmetry)
        : 0.96f * std::tanh (1.62f * z / asymmetry);

    const float siliconStage =
        0.82f * std::tanh (1.52f * germaniumStage + 0.20f * z);

    const float diodeStage = siliconStage >= 0.0f
        ? 0.86f * std::tanh (1.38f * siliconStage * asymmetry)
        : 0.70f * std::tanh (1.88f * siliconStage / asymmetry);

    const float highCoefficient = std::exp (
        -juce::MathConstants<float>::twoPi
        * 6800.0f
        / static_cast<float> (sampleRateHz));

    albinoHighState[stateChannel] =
        highCoefficient * albinoHighState[stateChannel]
        + (1.0f - highCoefficient) * diodeStage;

    return 0.84f * albinoHighState[stateChannel]
        + 0.16f * diodeStage;
}

float PERCULATORAudioProcessor::peakToMeter (float peak) noexcept
{
    if (peak < 0.00002f)
        return 0.0f;

    const float dB = juce::Decibels::gainToDecibels (peak, -60.0f);

    // Visual scale: -18 dBFS = first LED, 0 dBFS = final red LED.
    // A normal guitar chord around -12 dBFS remains in the green region.
    return juce::jlimit (0.0f, 1.0f, (dB + 18.0f) / 18.0f);
}

void PERCULATORAudioProcessor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float inputDb = apvts.getRawParameterValue ("input")->load();
    const float outputDb = apvts.getRawParameterValue ("output")->load();
    const float harmonics = apvts.getRawParameterValue ("harmonics")->load() / 10.0f;
    const float balance = apvts.getRawParameterValue ("balance")->load() / 10.0f;
    const float bias = apvts.getRawParameterValue ("bias")->load() / 10.0f;
    const float wetMix = apvts.getRawParameterValue ("mix")->load() / 100.0f;

    const int circuit = juce::roundToInt (
        apvts.getRawParameterValue ("circuit")->load());

    const bool bypassed =
        apvts.getRawParameterValue ("bypass")->load() > 0.5f;

    // Previous calibration plus the requested additional 12 dB.
    const float circuitCalibration[]
    {
        juce::Decibels::decibelsToGain (27.0f), // NPN OD
        juce::Decibels::decibelsToGain (29.0f), // D310 Diodes
        juce::Decibels::decibelsToGain (28.0f)  // Albino
    };

    const int safeCircuitIndex = juce::jlimit (0, 2, circuit);
    const float userInputGain = juce::Decibels::decibelsToGain (inputDb);
    const float circuitInputGain = circuitCalibration[safeCircuitIndex];
    const float outputGain = juce::Decibels::decibelsToGain (outputDb);

    float inputPeak = 0.0f;
    float outputPeak = 0.0f;

    const float dcBlockCoefficient = std::exp (
        -juce::MathConstants<float>::twoPi
        * 10.0f
        / static_cast<float> (sampleRateHz));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);
        const int stateChannel = juce::jmin (channel, 1);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float drySample = samples[sample];

            // Meter excludes the hidden circuit calibration gain.
            const float meteredInput = drySample * userInputGain;
            inputPeak = juce::jmax (inputPeak, std::abs (meteredInput));

            // DSP receives the requested additional 12 dB calibration.
            const float circuitInput = meteredInput * circuitInputGain;

            if (std::abs (meteredInput) < 1.0e-7f)
            {
                memory[static_cast<size_t> (stateChannel)] *= 0.995f;
                albinoLowState[static_cast<size_t> (stateChannel)] *= 0.995f;
                albinoHighState[static_cast<size_t> (stateChannel)] *= 0.995f;
            }

            memory[static_cast<size_t> (stateChannel)] =
                0.9975f * memory[static_cast<size_t> (stateChannel)]
                + 0.0025f * circuitInput;

            float processedSample = processCircuitSample (
                circuitInput
                    - 0.12f * memory[static_cast<size_t> (stateChannel)],
                stateChannel,
                circuit,
                harmonics,
                bias);

            processedSample *= juce::jmap (balance, 0.15f, 1.25f);

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

        if (apvts.getRawParameterValue ("phase")->load() > 0.5f)
            buffer.applyGain (-1.0f);
    }

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
{
    if (! file.existsAsFile())
        return;

    irFile = file;
    irName = file.getFileName();

    convolution.loadImpulseResponse (
        file,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes,
        0,
        juce::dsp::Convolution::Normalise::yes);
}

void PERCULATORAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto state = apvts.copyState();
    state.setProperty ("irPath", irFile.getFullPathName(), nullptr);
    state.setProperty ("panelColour", panelColour.load(), nullptr);

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
            panelColour.store (static_cast<int> (
                state.getProperty ("panelColour", 1)));

            const juce::File file (
                state.getProperty ("irPath").toString());

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
