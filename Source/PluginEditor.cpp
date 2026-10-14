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

BypassFootswitchButton::BypassFootswitchButton()
    : juce::TextButton()
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void BypassFootswitchButton::paintButton (juce::Graphics& g,
                                           bool isMouseOverButton,
                                           bool isButtonDown)
{
    const auto area = getLocalBounds().toFloat();
    const auto centreX = area.getCentreX();
    const float pressOffset = isButtonDown ? 2.0f : 0.0f;

    // Status LED: red while the effect is active, grey while bypassed.
    const bool effectActive = ! getToggleState();
    const juce::Point<float> ledCentre { centreX, 23.0f };
    const float ledRadius = 12.0f;

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (ledCentre.x - 15.0f, ledCentre.y - 15.0f, 30.0f, 30.0f);
    g.setColour (juce::Colour (0xff50504b));
    g.fillEllipse (ledCentre.x - 12.5f, ledCentre.y - 12.5f, 25.0f, 25.0f);

    if (effectActive)
    {
        g.setColour (juce::Colour (0xffff2018).withAlpha (0.25f));
        g.fillEllipse (ledCentre.x - 20.0f, ledCentre.y - 20.0f, 40.0f, 40.0f);
        juce::ColourGradient led (juce::Colour (0xfffff0d0),
                                  ledCentre.x - 4.0f, ledCentre.y - 5.0f,
                                  juce::Colour (0xffc40000),
                                  ledCentre.x + 7.0f, ledCentre.y + 8.0f, true);
        g.setGradientFill (led);
    }
    else
    {
        juce::ColourGradient led (juce::Colour (0xff9a9a94),
                                  ledCentre.x - 4.0f, ledCentre.y - 5.0f,
                                  juce::Colour (0xff30302d),
                                  ledCentre.x + 7.0f, ledCentre.y + 8.0f, true);
        g.setGradientFill (led);
    }
    g.fillEllipse (ledCentre.x - ledRadius, ledCentre.y - ledRadius,
                   ledRadius * 2.0f, ledRadius * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawEllipse (ledCentre.x - ledRadius, ledCentre.y - ledRadius,
                   ledRadius * 2.0f, ledRadius * 2.0f, 2.0f);

    // Hexagonal retaining nut.
    const juce::Point<float> switchCentre { centreX, 103.0f + pressOffset };
    juce::Path nut;
    constexpr int sides = 8;
    constexpr float nutRadius = 57.0f;
    for (int i = 0; i < sides; ++i)
    {
        const float angle = juce::MathConstants<float>::twoPi
                            * static_cast<float> (i) / static_cast<float> (sides)
                            + juce::MathConstants<float>::pi / 8.0f;
        const auto point = switchCentre
                         + juce::Point<float> (std::cos (angle), std::sin (angle)) * nutRadius;
        if (i == 0) nut.startNewSubPath (point); else nut.lineTo (point);
    }
    nut.closeSubPath();

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillPath (nut, juce::AffineTransform::translation (3.0f, 5.0f));
    juce::ColourGradient metal (juce::Colour (0xfff0ede4),
                                switchCentre.x - 35.0f, switchCentre.y - 42.0f,
                                juce::Colour (0xff55524d),
                                switchCentre.x + 40.0f, switchCentre.y + 45.0f, false);
    g.setGradientFill (metal);
    g.fillPath (nut);
    g.setColour (juce::Colour (0xff272622));
    g.strokePath (nut, juce::PathStrokeType (3.0f));

    // Circular footswitch cap.
    constexpr float outerRadius = 44.0f;
    juce::ColourGradient outer (juce::Colour (0xffebe7dc),
                                switchCentre.x - 24.0f, switchCentre.y - 30.0f,
                                juce::Colour (0xff4a4843),
                                switchCentre.x + 28.0f, switchCentre.y + 34.0f, false);
    g.setGradientFill (outer);
    g.fillEllipse (switchCentre.x - outerRadius, switchCentre.y - outerRadius,
                   outerRadius * 2.0f, outerRadius * 2.0f);
    g.setColour (juce::Colour (0xff24231f));
    g.drawEllipse (switchCentre.x - outerRadius, switchCentre.y - outerRadius,
                   outerRadius * 2.0f, outerRadius * 2.0f, 3.0f);

    constexpr float capRadius = 33.0f;
    juce::ColourGradient cap (isMouseOverButton ? juce::Colour (0xff5f5f5b)
                                                : juce::Colour (0xff4b4b48),
                               switchCentre.x - 15.0f, switchCentre.y - 18.0f,
                               juce::Colour (0xff20201e),
                               switchCentre.x + 18.0f, switchCentre.y + 20.0f, false);
    g.setGradientFill (cap);
    g.fillEllipse (switchCentre.x - capRadius, switchCentre.y - capRadius,
                   capRadius * 2.0f, capRadius * 2.0f);
    g.setColour (juce::Colour (0xff777770));
    g.drawEllipse (switchCentre.x - capRadius, switchCentre.y - capRadius,
                   capRadius * 2.0f, capRadius * 2.0f, 1.5f);
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
    setControlBounds (bypass,     { 1275, 728, 200, 178 });
}

void PERCULATORAudioProcessorEditor::drawDynamicReadouts (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xffded8c6));
    g.setFont (juce::FontOptions (17.0f * getWidth() / designWidth,
                                  juce::Font::bold));
    g.drawText ("IR MIX", scaleRect ({ 1010, 326, 155, 30 }),
                juce::Justification::centred);
    g.drawText ("IR LEVEL", scaleRect ({ 1193, 326, 155, 30 }),
                juce::Justification::centred);

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
    drawArc ({ 171.0f, 571.0f }, 98.0f, processor.getInputMeter());
    drawArc ({ 815.0f, 571.0f }, 98.0f, processor.getOutputMeter());
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
