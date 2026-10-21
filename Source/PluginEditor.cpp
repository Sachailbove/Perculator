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

IrOnLedButton::IrOnLedButton()
    : juce::TextButton()
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Enable or disable the cabinet impulse response");
}

void IrOnLedButton::paintButton (juce::Graphics& g,
                                 bool isMouseOverButton,
                                 bool isButtonDown)
{
    juce::Graphics::ScopedSaveState saveState (g);
    g.addTransform (juce::AffineTransform::scale (
        static_cast<float> (getWidth()) / 120.0f,
        static_cast<float> (getHeight()) / 75.0f));

    const bool on = getToggleState();
    const auto c = juce::Point<float> (29.0f, 30.0f)
                       .translated (0.0f, isButtonDown ? 1.0f : 0.0f);

    if (on)
    {
        juce::ColourGradient glow (juce::Colour (0x703cff19), c.x, c.y,
                                   juce::Colours::transparentBlack,
                                   c.x + 27.0f, c.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (c.x - 27.0f, c.y - 27.0f, 54.0f, 54.0f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.78f));
    g.fillEllipse (c.x - 21.0f, c.y - 21.0f, 42.0f, 42.0f);

    juce::ColourGradient bezel (juce::Colour (0xff555b50),
                                c.x - 12.0f, c.y - 14.0f,
                                juce::Colour (0xff10130f),
                                c.x + 14.0f, c.y + 16.0f, false);
    g.setGradientFill (bezel);
    g.fillEllipse (c.x - 18.0f, c.y - 18.0f, 36.0f, 36.0f);
    g.setColour (juce::Colour (0xff050705));
    g.drawEllipse (c.x - 18.0f, c.y - 18.0f, 36.0f, 36.0f, 2.0f);

    juce::ColourGradient lens (
        on ? juce::Colour (0xffd9ff79) : juce::Colour (0xff526148),
        c.x - 6.0f, c.y - 8.0f,
        on ? juce::Colour (0xff08a900) : juce::Colour (0xff142016),
        c.x + 8.0f, c.y + 10.0f, true);
    lens.addColour (0.42, on ? juce::Colour (0xff39ff16)
                             : juce::Colour (0xff2b432b));
    g.setGradientFill (lens);
    g.fillEllipse (c.x - 13.0f, c.y - 13.0f, 26.0f, 26.0f);

    g.setColour (on ? juce::Colour (0xffbaff7a).withAlpha (0.85f)
                    : juce::Colour (0xff9eaa96).withAlpha (0.28f));
    g.fillEllipse (c.x - 6.5f, c.y - 8.5f, 7.0f, 5.0f);

    g.setColour (isMouseOverButton ? juce::Colour (0xfff4f0df)
                                   : juce::Colour (0xffded8c6));
    g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    g.drawText ("IR ON", juce::Rectangle<float> (54.0f, 9.0f, 66.0f, 42.0f),
                juce::Justification::centredLeft, false);
}

PhaseToggleSwitch::PhaseToggleSwitch()
    : juce::TextButton()
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Invert the cabinet IR phase");
}

void PhaseToggleSwitch::paintButton (juce::Graphics& g,
                                     bool isMouseOverButton,
                                     bool isButtonDown)
{
    juce::Graphics::ScopedSaveState saveState (g);
    g.addTransform (juce::AffineTransform::scale (
        static_cast<float> (getWidth()) / 95.0f,
        static_cast<float> (getHeight()) / 90.0f));

    const bool inverted = getToggleState();
    const float pressed = isButtonDown ? 1.0f : 0.0f;
    const juce::Point<float> centre { 47.5f, 35.0f + pressed };

    // Shadow and recessed oval mounting plate.
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (juce::Rectangle<float> (25.5f, 3.0f + pressed,
                                                     44.0f, 65.0f),
                            21.0f);

    juce::ColourGradient plate (juce::Colour (0xffd8d2bd),
                                31.0f, 7.0f,
                                juce::Colour (0xff574f3e),
                                65.0f, 65.0f, false);
    g.setGradientFill (plate);
    g.fillRoundedRectangle (juce::Rectangle<float> (28.5f, 4.5f + pressed,
                                                     38.0f, 61.0f),
                            18.0f);
    g.setColour (juce::Colour (0xff181713));
    g.drawRoundedRectangle (juce::Rectangle<float> (28.5f, 4.5f + pressed,
                                                     38.0f, 61.0f),
                            18.0f, 2.0f);

    // Dark slot through which the toggle lever moves.
    g.setColour (juce::Colour (0xff090a08));
    g.fillRoundedRectangle (juce::Rectangle<float> (37.0f, 13.0f + pressed,
                                                     21.0f, 45.0f),
                            10.0f);
    g.setColour (juce::Colour (0xff777467));
    g.drawRoundedRectangle (juce::Rectangle<float> (37.0f, 13.0f + pressed,
                                                     21.0f, 45.0f),
                            10.0f, 1.2f);

    // Lever pivots between the upper and lower positions.
    const juce::Point<float> pivot { 47.5f, 34.0f + pressed };
    const juce::Point<float> tip = inverted
        ? juce::Point<float> (47.5f, 51.0f + pressed)
        : juce::Point<float> (47.5f, 18.0f + pressed);

    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawLine (pivot.x + 2.0f, pivot.y + 2.0f,
                tip.x + 2.0f, tip.y + 2.0f, 10.0f);

    juce::ColourGradient lever (juce::Colour (0xfff3f0e7),
                                42.0f, tip.y,
                                juce::Colour (0xff615e56),
                                53.0f, pivot.y, false);
    g.setGradientFill (lever);
    g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 9.0f);

    g.setColour (juce::Colour (0xff24231f));
    g.drawEllipse (pivot.x - 7.0f, pivot.y - 7.0f, 14.0f, 14.0f, 1.5f);
    g.setColour (isMouseOverButton ? juce::Colour (0xfff8f5e9)
                                   : juce::Colour (0xffd8d4c8));
    g.fillEllipse (tip.x - 5.5f, tip.y - 5.5f, 11.0f, 11.0f);
    g.setColour (juce::Colour (0xff4b4943));
    g.drawEllipse (tip.x - 5.5f, tip.y - 5.5f, 11.0f, 11.0f, 1.2f);

    // Reference markings match the target preview.
    g.setColour (juce::Colour (0xffded8c6));
    g.setFont (juce::FontOptions (17.0f));
    g.drawText (juce::String::fromUTF8 ("Ø"),
                juce::Rectangle<float> (69.0f, 25.0f, 24.0f, 25.0f),
                juce::Justification::centred, false);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("PHASE", juce::Rectangle<float> (14.0f, 69.0f, 67.0f, 18.0f),
                juce::Justification::centred, false);
}

OversamplingButton::OversamplingButton (const juce::String& label)
    : juce::TextButton (label)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void OversamplingButton::setSelected (bool shouldBeSelected)
{
    if (selected != shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }
}

void OversamplingButton::paintButton (juce::Graphics& g,
                                      bool isMouseOverButton,
                                      bool isButtonDown)
{
    juce::Graphics::ScopedSaveState saveState (g);
    g.addTransform (juce::AffineTransform::scale (
        static_cast<float> (getWidth()) / 112.0f,
        static_cast<float> (getHeight()) / 58.0f));

    const float pressOffset = isButtonDown ? 1.5f : 0.0f;
    const juce::Rectangle<float> body (1.5f, 1.5f + pressOffset, 109.0f, 54.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (body.translated (0.0f, 2.0f), 5.5f);

    juce::ColourGradient face (isMouseOverButton ? juce::Colour (0xff303330)
                                                  : juce::Colour (0xff242724),
                               body.getX(), body.getY(),
                               juce::Colour (0xff111311),
                               body.getX(), body.getBottom(), false);
    g.setGradientFill (face);
    g.fillRoundedRectangle (body, 5.5f);

    if (selected)
    {
        g.setColour (juce::Colour (0x329f7d2b));
        g.fillRoundedRectangle (body.reduced (2.0f), 4.0f);
        g.setColour (juce::Colour (0xffc7a24b));
        g.drawRoundedRectangle (body.reduced (1.0f), 5.0f, 2.2f);
        g.setColour (juce::Colour (0x50d9b85c));
        g.drawRoundedRectangle (body.reduced (3.5f), 3.5f, 1.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff777b72));
        g.drawRoundedRectangle (body, 5.5f, 1.2f);
        g.setColour (juce::Colour (0xff373a35));
        g.drawRoundedRectangle (body.reduced (2.0f), 4.0f, 1.0f);
    }

    g.setColour (selected ? juce::Colour (0xfffff1ba)
                          : juce::Colour (0xffeeeadd));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText (getButtonText(), body,
                juce::Justification::centred, false);
}

IrNavigationButton::IrNavigationButton (bool pointsRight)
    : juce::TextButton(), right (pointsRight)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip (right ? "Next cabinet IR" : "Previous cabinet IR");
}

void IrNavigationButton::paintButton (juce::Graphics& g,
                                      bool isMouseOverButton,
                                      bool isButtonDown)
{
    juce::Graphics::ScopedSaveState saveState (g);
    g.addTransform (juce::AffineTransform::scale (
        static_cast<float> (getWidth()) / 70.0f,
        static_cast<float> (getHeight()) / 52.0f));

    const float offset = isButtonDown ? 1.5f : 0.0f;
    const juce::Rectangle<float> body (1.5f, 1.5f + offset, 67.0f, 47.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (body.translated (0.0f, 2.0f), 5.0f);

    juce::ColourGradient face (isMouseOverButton ? juce::Colour (0xff343733)
                                                  : juce::Colour (0xff272a27),
                               body.getX(), body.getY(),
                               juce::Colour (0xff101210),
                               body.getX(), body.getBottom(), false);
    g.setGradientFill (face);
    g.fillRoundedRectangle (body, 5.0f);
    g.setColour (juce::Colour (0xff777b72));
    g.drawRoundedRectangle (body, 5.0f, 1.2f);
    g.setColour (juce::Colour (0xff3b3e39));
    g.drawRoundedRectangle (body.reduced (2.0f), 3.8f, 1.0f);

    juce::Path arrow;
    if (right)
    {
        arrow.startNewSubPath (27.0f, 15.0f + offset);
        arrow.lineTo (42.0f, 26.0f + offset);
        arrow.lineTo (27.0f, 37.0f + offset);
    }
    else
    {
        arrow.startNewSubPath (43.0f, 15.0f + offset);
        arrow.lineTo (28.0f, 26.0f + offset);
        arrow.lineTo (43.0f, 37.0f + offset);
    }
    g.setColour (isMouseOverButton ? juce::Colour (0xfffff3c7)
                                   : juce::Colour (0xffe4dfcf));
    g.strokePath (arrow, juce::PathStrokeType (4.0f,
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

LoadIrButton::LoadIrButton()
    : juce::TextButton()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Load a cabinet impulse response");
}

void LoadIrButton::paintButton (juce::Graphics& g,
                                bool isMouseOverButton,
                                bool isButtonDown)
{
    juce::Graphics::ScopedSaveState saveState (g);
    g.addTransform (juce::AffineTransform::scale (
        static_cast<float> (getWidth()) / 232.0f,
        static_cast<float> (getHeight()) / 52.0f));

    const float offset = isButtonDown ? 1.5f : 0.0f;
    const juce::Rectangle<float> body (1.5f, 1.5f + offset, 229.0f, 47.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (body.translated (0.0f, 2.0f), 5.0f);

    juce::ColourGradient face (isMouseOverButton ? juce::Colour (0xff343733)
                                                  : juce::Colour (0xff272a27),
                               body.getX(), body.getY(),
                               juce::Colour (0xff101210),
                               body.getX(), body.getBottom(), false);
    g.setGradientFill (face);
    g.fillRoundedRectangle (body, 5.0f);
    g.setColour (juce::Colour (0xff777b72));
    g.drawRoundedRectangle (body, 5.0f, 1.2f);
    g.setColour (juce::Colour (0xff3b3e39));
    g.drawRoundedRectangle (body.reduced (2.0f), 3.8f, 1.0f);

    g.setColour (isMouseOverButton ? juce::Colour (0xfffff3c7)
                                   : juce::Colour (0xffeeeadd));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("LOAD IR...", body,
                juce::Justification::centred, false);
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
    // Paint in the original 200 x 178 design space, then scale the complete
    // footswitch to the bounds assigned by resized().
    juce::Graphics::ScopedSaveState saveState (g);
    g.addTransform (juce::AffineTransform::scale (
        static_cast<float> (getWidth()) / 200.0f,
        static_cast<float> (getHeight()) / 178.0f));

    const juce::Rectangle<float> area (0.0f, 0.0f, 200.0f, 178.0f);
    const auto centreX = area.getCentreX() - 0.5f;
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
    OversamplingButton* buttons[] { &os2x, &os4x, &os8x };
    for (int i = 0; i < 3; ++i)
        buttons[i]->setSelected (i == selected);
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
    // The complete three-button row exactly matches the 388 px IR display width.
    setControlBounds (previousIR, { 1052, 248, 70, 52 });
    setControlBounds (nextIR,     { 1130, 248, 70, 52 });
    setControlBounds (loadIR,     { 1208, 248, 232, 52 });
    setControlBounds (irOn,       { 1363, 330, 120, 75 });
    setControlBounds (phase,      { 1368, 397, 95, 90 });
    // Compact, centred oversampling group with clearance from the lower screws.
    setControlBounds (os2x,       { 1038, 620, 112, 58 });
    setControlBounds (os4x,       { 1183, 620, 112, 58 });
    setControlBounds (os8x,       { 1328, 620, 112, 58 });
    setControlBounds (bypass,     { 1242, 768, 200, 178 });
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

    auto irBox = scaleRect ({ 1052, 157, 388, 62 });
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

void PERCULATORAudioProcessorEditor::drawStateLights (juce::Graphics&)
{
    // IR ON is painted by IrOnLedButton.
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
