#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
juce::String formatValue (float value, int decimals, const juce::String& suffix)
{
    return juce::String (value, decimals) + suffix;
}
}

PERCULATORAudioProcessorEditor::PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      panelImage (juce::ImageCache::getFromMemory (BinaryData::PerculatorPanel_png,
                                                   BinaryData::PerculatorPanel_pngSize))
{
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (763, 512, 1831, 1229);
    getConstrainer()->setFixedAspectRatio (designWidth / designHeight);
    setSize (1536, 1024);

    configureKnob (harmonics, "", 1);
    configureKnob (balance, "", 1);
    configureKnob (circuit, "", 0);
    configureKnob (input, " dB", 1);
    configureKnob (bias, " %", 2);
    configureKnob (mix, " %", 1);
    configureKnob (output, " dB", 1);
    configureKnob (irMix, " %", 1);
    configureKnob (irLevel, " dB", 1);

    circuit.textFromValueFunction = [] (double value)
    {
        const juce::StringArray names { "NPN OD", "D310 Diodes", "Albino" };
        return names[juce::jlimit (0, 2, juce::roundToInt (value))];
    };

    configureToggle (irOn);
    configureToggle (phase);
    configureToggle (bypass);

    loadIR.onClick = [this] { chooseIR(); };
    previousIR.onClick = [this] { chooseIR(); };
    nextIR.onClick = [this] { chooseIR(); };
    os2x.onClick = [this] { selectOversampling (0); };
    os4x.onClick = [this] { selectOversampling (1); };
    os8x.onClick = [this] { selectOversampling (2); };

    juce::Component* components[]
    {
        &harmonics, &balance, &circuit, &input, &bias, &mix, &output, &irMix, &irLevel,
        &previousIR, &nextIR, &loadIR, &irOn, &phase, &bypass, &os2x, &os4x, &os8x
    };
    for (auto* component : components)
        addAndMakeVisible (*component);

    aHarmonics = std::make_unique<SliderAttachment> (p.apvts, "harmonics", harmonics);
    aBalance = std::make_unique<SliderAttachment> (p.apvts, "balance", balance);
    aCircuit = std::make_unique<SliderAttachment> (p.apvts, "circuit", circuit);
    aInput = std::make_unique<SliderAttachment> (p.apvts, "input", input);
    aBias = std::make_unique<SliderAttachment> (p.apvts, "bias", bias);
    aMix = std::make_unique<SliderAttachment> (p.apvts, "mix", mix);
    aOutput = std::make_unique<SliderAttachment> (p.apvts, "output", output);
    aIrMix = std::make_unique<SliderAttachment> (p.apvts, "irmix", irMix);
    aIrLevel = std::make_unique<SliderAttachment> (p.apvts, "irlevel", irLevel);
    aIrOn = std::make_unique<ButtonAttachment> (p.apvts, "iron", irOn);
    aPhase = std::make_unique<ButtonAttachment> (p.apvts, "phase", phase);
    aBypass = std::make_unique<ButtonAttachment> (p.apvts, "bypass", bypass);

    refreshOversamplingButtons();
    startTimerHz (30);
}

PERCULATORAudioProcessorEditor::~PERCULATORAudioProcessorEditor() = default;

void PERCULATORAudioProcessorEditor::configureKnob (PerculatorKnob& knob,
                                                     const juce::String& suffix,
                                                     int decimals)
{
    knob.setNumDecimalPlacesToDisplay (decimals);
    knob.setTextValueSuffix (suffix);
}

void PERCULATORAudioProcessorEditor::configureToggle (juce::TextButton& button)
{
    button.setClickingTogglesState (true);
    button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff171918));
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff8e2c21));
    button.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffded8c6));
    button.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

void PERCULATORAudioProcessorEditor::selectOversampling (int index)
{
    if (auto* parameter = processor.apvts.getParameter ("oversampling"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (index)));
        parameter->endChangeGesture();
    }
    refreshOversamplingButtons();
}

void PERCULATORAudioProcessorEditor::refreshOversamplingButtons()
{
    const int selected = juce::roundToInt (
        processor.apvts.getRawParameterValue ("oversampling")->load());
    juce::TextButton* buttons[] { &os2x, &os4x, &os8x };
    for (int i = 0; i < 3; ++i)
    {
        buttons[i]->setColour (juce::TextButton::buttonColourId,
                               i == selected ? juce::Colour (0xff9b7931)
                                             : juce::Colour (0xff171918));
        buttons[i]->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    }
}

void PERCULATORAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff384239));
    if (panelImage.isValid())
        g.drawImage (panelImage, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    drawMeters (g);
    drawDynamicReadouts (g);
    drawStateLights (g);
}

juce::Rectangle<float> PERCULATORAudioProcessorEditor::scaleRect (juce::Rectangle<float> r) const
{
    const float sx = getWidth() / designWidth;
    const float sy = getHeight() / designHeight;
    return { r.getX() * sx, r.getY() * sy, r.getWidth() * sx, r.getHeight() * sy };
}

juce::Point<float> PERCULATORAudioProcessorEditor::scalePoint (juce::Point<float> p) const
{
    return { p.x * getWidth() / designWidth, p.y * getHeight() / designHeight };
}

void PERCULATORAudioProcessorEditor::setControlBounds (juce::Component& component,
                                                        juce::Rectangle<int> r)
{
    component.setBounds (scaleRect (r.toFloat()).toNearestInt());
}

void PERCULATORAudioProcessorEditor::resized()
{
    setControlBounds (harmonics, { 106, 140, 212, 230 });
    setControlBounds (balance,   { 420, 140, 212, 230 });
    setControlBounds (circuit,   { 730, 185, 175, 195 });
    setControlBounds (input,     {  76, 480, 190, 215 });
    setControlBounds (bias,      { 300, 480, 190, 215 });
    setControlBounds (mix,       { 498, 480, 190, 215 });
    setControlBounds (output,    { 720, 480, 190, 215 });
    setControlBounds (irMix,     { 1020, 365, 135, 160 });
    setControlBounds (irLevel,   { 1203, 365, 135, 160 });
    setControlBounds (previousIR, { 1020, 248, 70, 52 });
    setControlBounds (nextIR,     { 1098, 248, 70, 52 });
    setControlBounds (loadIR,     { 1186, 248, 294, 52 });
    setControlBounds (irOn,       { 1363, 330, 120, 75 });
    setControlBounds (phase,      { 1368, 397, 95, 90 });
    setControlBounds (os2x,       { 1046, 655, 115, 62 });
    setControlBounds (os4x,       { 1198, 655, 115, 62 });
    setControlBounds (os8x,       { 1350, 655, 115, 62 });
    setControlBounds (bypass,     { 1270, 782, 145, 100 });
}

void PERCULATORAudioProcessorEditor::drawDynamicReadouts (juce::Graphics& g)
{
    // PerculatorKnob already draws one value display for every rotary control.
    // This method now draws only the IR filename, avoiding duplicate value boxes.
    auto irBox = scaleRect ({ 1024, 177, 383, 57 });
    g.setColour (juce::Colour (0xff0c0e0d));
    g.fillRoundedRectangle (irBox, 5.0f);
    g.setColour (juce::Colour (0xffdddddd));
    g.setFont (juce::FontOptions (18.0f * getWidth() / designWidth));
    g.drawText (processor.getIRName(), irBox.reduced (12.0f, 0.0f),
                juce::Justification::centredLeft);
}

void PERCULATORAudioProcessorEditor::drawMeters (juce::Graphics& g)
{
    const auto drawArc = [&] (juce::Point<float> designCentre, float designRadius, float level)
    {
        const auto centre = scalePoint (designCentre);
        const float scale = getWidth() / designWidth;
        const float radius = designRadius * scale;
        constexpr int count = 20;
        for (int i = 0; i < count; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (count - 1);
            const float angle = juce::MathConstants<float>::pi * (1.15f + 0.70f * t);
            const auto p = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * radius;
            const auto colour = t < 0.50f ? juce::Colour (0xff31e94e)
                              : t < 0.70f ? juce::Colour (0xffffdf28)
                              : t < 0.86f ? juce::Colour (0xffff8a18)
                                           : juce::Colour (0xffff2525);
            const float size = 9.0f * scale;
            g.setColour (t <= level ? colour : juce::Colour (0xff22251d));
            g.fillEllipse (p.x - size * 0.5f, p.y - size * 0.5f, size, size);
        }
    };
    drawArc ({ 171.0f, 591.0f }, 98.0f, processor.getInputMeter());
    drawArc ({ 815.0f, 591.0f }, 98.0f, processor.getOutputMeter());
}

void PERCULATORAudioProcessorEditor::drawStateLights (juce::Graphics& g)
{
    const auto dot = [&] (juce::Point<float> location, bool on, juce::Colour active)
    {
        const auto p = scalePoint (location);
        const float d = 17.0f * getWidth() / designWidth;
        g.setColour (on ? active : juce::Colour (0xff243024));
        g.fillEllipse (p.x - d * 0.5f, p.y - d * 0.5f, d, d);
    };
    dot ({ 1392.0f, 360.0f }, irOn.getToggleState(), juce::Colour (0xff35ff24));
    dot ({ 1340.0f, 770.0f }, bypass.getToggleState(), juce::Colour (0xffff2b1c));
}

void PERCULATORAudioProcessorEditor::timerCallback()
{
    refreshOversamplingButtons();
    repaint();
}

bool PERCULATORAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    return files.size() == 1 && juce::File (files[0]).hasFileExtension ("wav;aif;aiff;flac");
}

void PERCULATORAudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    if (! files.isEmpty())
        processor.loadImpulseResponse (juce::File (files[0]));
}

void PERCULATORAudioProcessorEditor::chooseIR()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Load cabinet IR", juce::File {}, "*.wav;*.aif;*.aiff;*.flac");
    chooser->launchAsync (juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file.existsAsFile())
                                  processor.loadImpulseResponse (file);
                          });
}
