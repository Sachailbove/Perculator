#include "PluginEditor.h"
#include <cmath>

namespace
{
const auto textColour = juce::Colour (0xfff0ead8);
const auto darkColour = juce::Colour (0xff121514);
const auto goldColour = juce::Colour (0xffedcf72);

juce::String valueText (juce::Slider& slider)
{
    const auto units = slider.getProperties().getWithDefault ("units", "").toString();
    const int decimals = static_cast<int> (slider.getProperties().getWithDefault ("decimals", 1));
    return juce::String (slider.getValue(), decimals) + units;
}
}

PerculatorLookAndFeel::PerculatorLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff191c1b));
    setColour (juce::ComboBox::textColourId, textColour);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff696b64));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff171918));
    setColour (juce::PopupMenu::textColourId, textColour);
    setColour (juce::Slider::textBoxTextColourId, textColour);
}

void PerculatorLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width,
                                               int height, float position, float startAngle,
                                               float endAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height));
    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight()) - 8.0f;
    bounds = bounds.withSizeKeepingCentre (diameter, diameter);
    const auto centre = bounds.getCentre();
    const float radius = diameter * 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (bounds.translated (3.0f, 5.0f));

    juce::ColourGradient outer (juce::Colour (0xff424443), bounds.getX(), bounds.getY(),
                                juce::Colour (0xff050606), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (outer);
    g.fillEllipse (bounds);
    g.setColour (juce::Colours::black);
    g.drawEllipse (bounds, 3.0f);

    for (int i = 0; i < 12; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * static_cast<float> (i) / 12.0f;
        const auto p1 = centre + juce::Point<float> (std::cos (a), std::sin (a)) * radius * 0.76f;
        const auto p2 = centre + juce::Point<float> (std::cos (a), std::sin (a)) * radius * 0.93f;
        g.setColour (juce::Colour (0xff686a68));
        g.drawLine ({ p1, p2 }, juce::jmax (1.0f, radius * 0.035f));
    }

    auto hub = bounds.reduced (radius * 0.50f);
    juce::ColourGradient metal (juce::Colour (0xfff0f0eb), hub.getX(), hub.getY(),
                                juce::Colour (0xff777873), hub.getRight(), hub.getBottom(), false);
    g.setGradientFill (metal);
    g.fillEllipse (hub);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (hub, 1.5f);

    const float angle = startAngle + position * (endAngle - startAngle);
    juce::Path pointer;
    pointer.addRoundedRectangle (-2.0f, -radius * 0.88f, 4.0f, radius * 0.46f, 2.0f);
    g.setColour (juce::Colours::white);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    auto readout = slider.getLocalBounds().toFloat().removeFromBottom (24.0f).reduced (10.0f, 1.0f);
    g.setColour (juce::Colour (0xff111412));
    g.fillRoundedRectangle (readout, 3.0f);
    g.setColour (juce::Colour (0xffb5aa78));
    g.drawRoundedRectangle (readout, 3.0f, 1.0f);
    g.setColour (textColour);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (valueText (slider), readout, juce::Justification::centred);
}

void PerculatorLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                   const juce::Colour&, bool over, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    auto c = down ? goldColour.darker (0.4f) : over ? juce::Colour (0xff3c403e) : darkColour;
    g.setColour (c);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (goldColour.withAlpha (button.getToggleState() ? 1.0f : 0.35f));
    g.drawRoundedRectangle (r, 5.0f, button.getToggleState() ? 2.5f : 1.0f);
}

void PerculatorLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                             bool, bool)
{
    g.setColour (textColour);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

void PerculatorLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                               bool over, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (2.0f);
    const bool on = button.getToggleState();

    if (button.getButtonText() == "PHASE")
    {
        g.setColour (juce::Colour (0xff171918));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (juce::Colour (0xff777a75));
        g.drawRoundedRectangle (r, 6.0f, 1.5f);

        auto track = r.withSizeKeepingCentre (r.getWidth() * 0.35f, r.getHeight() * 0.72f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (track, track.getWidth() * 0.45f);
        const float cy = on ? track.getY() + track.getHeight() * 0.28f
                            : track.getBottom() - track.getHeight() * 0.28f;
        g.setColour (juce::Colour (0xffd0cec4));
        g.fillEllipse (track.getCentreX() - track.getWidth() * 0.34f,
                       cy - track.getWidth() * 0.34f,
                       track.getWidth() * 0.68f, track.getWidth() * 0.68f);
        return;
    }

    auto led = r.removeFromLeft (r.getHeight()).reduced (4.0f);
    const auto active = button.getButtonText() == "BYPASS" ? juce::Colour (0xffff2b1c)
                                                            : juce::Colour (0xff35ff24);
    g.setColour (on ? active : juce::Colour (0xff243024));
    g.fillEllipse (led);
    g.setColour (on ? active.brighter() : juce::Colours::black);
    g.drawEllipse (led, 2.0f);
    g.setColour (over || down ? textColour : textColour.withAlpha (0.9f));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (button.getButtonText(), r, juce::Justification::centredLeft);
}

void PerculatorLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                           int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));
    g.setColour (juce::Colour (0xff161918));
    g.fillRoundedRectangle (r.reduced (1.0f), 4.0f);
    g.setColour (juce::Colour (0xff61635e));
    g.drawRoundedRectangle (r.reduced (1.0f), 4.0f, 1.0f);
    juce::Path arrow;
    arrow.addTriangle (width - 20.0f, height * 0.40f, width - 10.0f, height * 0.40f,
                       width - 15.0f, height * 0.62f);
    g.setColour (textColour);
    g.fillPath (arrow);
}

juce::Font PerculatorLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (14.0f));
}

ArcMeter::ArcMeter (PERCULATORAudioProcessor& p, Type t) : processor (p), type (t)
{
    setInterceptsMouseClicks (false, false);
}

void ArcMeter::paint (juce::Graphics& g)
{
    const float level = type == Type::input ? processor.getInputMeter() : processor.getOutputMeter();
    auto r = getLocalBounds().toFloat();
    const auto centre = juce::Point<float> (r.getCentreX(), r.getBottom() * 0.77f);
    const float radius = juce::jmin (r.getWidth() * 0.44f, r.getHeight() * 0.72f);
    constexpr int count = 20;

    for (int i = 0; i < count; ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (count - 1);
        const float angle = juce::MathConstants<float>::pi * (1.12f + 0.76f * t);
        const auto p = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * radius;
        const auto colour = t < 0.50f ? juce::Colour (0xff34e950)
                          : t < 0.70f ? juce::Colour (0xffffdf28)
                          : t < 0.86f ? juce::Colour (0xffff8918)
                                       : juce::Colour (0xffff251e);
        const float d = juce::jmax (5.0f, r.getWidth() * 0.048f);
        g.setColour (t <= level ? colour : juce::Colour (0xff22271f));
        g.fillEllipse (p.x - d * 0.5f, p.y - d * 0.5f, d, d);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (p.x - d * 0.5f, p.y - d * 0.5f, d, d, 1.0f);
    }
}

CircuitSelector::CircuitSelector()
{
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (0.0, 2.0, 1.0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
}

void CircuitSelector::paint (juce::Graphics& g)
{
    g.setColour (textColour);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("NPN OD", 0, getHeight() / 2 - 8, getWidth() / 3, 20, juce::Justification::centred);
    g.drawText ("D310 DIODES", getWidth() / 3, 2, getWidth() / 3, 20, juce::Justification::centred);
    g.drawText ("ALBINO", getWidth() * 2 / 3, getHeight() / 2 - 8,
                getWidth() / 3, 20, juce::Justification::centred);
}

void CircuitSelector::resized()
{
    slider.setBounds (getLocalBounds().reduced (getWidth() / 4, getHeight() / 5));
}

OversamplingSelector::OversamplingSelector()
{
    addAndMakeVisible (combo);
    combo.addItemList ({ "2x", "4x", "8x" }, 1);
    combo.setVisible (false);
}

void OversamplingSelector::paint (juce::Graphics& g)
{
    const int selected = juce::jlimit (0, 2, combo.getSelectedItemIndex());
    const char* labels[] { "2x", "4x", "8x" };
    const float gap = 8.0f;
    const float w = (getWidth() - gap * 2.0f) / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        auto r = juce::Rectangle<float> (i * (w + gap), 0.0f, w, static_cast<float> (getHeight())).reduced (1.0f);
        g.setColour (juce::Colour (0xff171a19));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (i == selected ? goldColour : juce::Colour (0xff5d605b));
        g.drawRoundedRectangle (r, 5.0f, i == selected ? 2.5f : 1.0f);
        g.setColour (textColour);
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText (labels[i], r, juce::Justification::centred);
    }
}

void OversamplingSelector::resized()
{
    combo.setBounds (getLocalBounds());
}

void OversamplingSelector::mouseUp (const juce::MouseEvent& e)
{
    const int index = juce::jlimit (0, 2, static_cast<int> (e.position.x * 3.0f / getWidth()));
    combo.setSelectedItemIndex (index, juce::sendNotificationSync);
    repaint();
}

PERCULATORAudioProcessorEditor::PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      inputMeter (p, ArcMeter::Type::input),
      outputMeter (p, ArcMeter::Type::output)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (800, 500, 1920, 1200);
    getConstrainer()->setFixedAspectRatio (designWidth / designHeight);
    setSize (1200, 750);

    configureLabel (titleLabel, "PERCULATOR", 45.0f);
    configureLabel (subtitleLabel, "DUAL POLARITY HARMONIC GENERATOR", 14.0f);
    configureLabel (harmonicsLabel, "HARMONICS", 16.0f);
    configureLabel (balanceLabel, "BALANCE", 16.0f);
    configureLabel (inputLabel, "INPUT", 16.0f);
    configureLabel (biasLabel, "BIAS", 16.0f);
    configureLabel (mixLabel, "MIX", 16.0f);
    configureLabel (outputLabel, "OUTPUT", 16.0f);
    configureLabel (cabinetLabel, "CABINET IR", 18.0f);
    configureLabel (irMixLabel, "IR MIX", 14.0f);
    configureLabel (irLevelLabel, "IR LEVEL", 14.0f);
    configureLabel (phaseLabel, "PHASE", 12.0f);
    configureLabel (circuitLabel, "CIRCUIT", 15.0f);

    configureKnob (harmonics, "", 1);
    configureKnob (balance, "", 1);
    configureKnob (input, " dB", 1);
    configureKnob (bias, " %", 2);
    configureKnob (mix, " %", 1);
    configureKnob (output, " dB", 1);
    configureKnob (irMix, " %", 1);
    configureKnob (irLevel, " dB", 1);

    panelColourBox.addItem ("Dictatorship", 1);
    panelColourBox.addItem ("Surf Party", 2);
    panelColourBox.addItem ("Fetish Red", 3);
    panelColourBox.setSelectedId (processor.getPanelColourIndex() + 1, juce::dontSendNotification);
    panelColourBox.onChange = [this]
    {
        processor.setPanelColourIndex (panelColourBox.getSelectedId() - 1);
        repaint();
    };

    irFileLabel.setText (processor.getIRName(), juce::dontSendNotification);
    irFileLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff0d100f));
    irFileLabel.setColour (juce::Label::textColourId, textColour);
    irFileLabel.setJustificationType (juce::Justification::centredLeft);

    loadIR.onClick = [this] { chooseIR(); };
    previousIR.onClick = [this] { chooseIR(); };
    nextIR.onClick = [this] { chooseIR(); };

    juce::Component* components[]
    {
        &panelColourBox, &titleLabel, &subtitleLabel, &harmonicsLabel, &balanceLabel,
        &inputLabel, &biasLabel, &mixLabel, &outputLabel, &cabinetLabel,
        &irMixLabel, &irLevelLabel, &phaseLabel, &circuitLabel,
        &harmonics, &balance, &input, &bias, &mix, &output, &irMix, &irLevel,
        &inputMeter, &outputMeter, &circuitSelector, &oversamplingSelector,
        &irFileLabel, &previousIR, &nextIR, &loadIR, &irOn, &phase, &bypass
    };

    for (auto* component : components)
        addAndMakeVisible (*component);

    inputMeter.toBack();
    outputMeter.toBack();

    aHarmonics = std::make_unique<SliderAttachment> (p.apvts, "harmonics", harmonics);
    aBalance = std::make_unique<SliderAttachment> (p.apvts, "balance", balance);
    aInput = std::make_unique<SliderAttachment> (p.apvts, "input", input);
    aBias = std::make_unique<SliderAttachment> (p.apvts, "bias", bias);
    aMix = std::make_unique<SliderAttachment> (p.apvts, "mix", mix);
    aOutput = std::make_unique<SliderAttachment> (p.apvts, "output", output);
    aIrMix = std::make_unique<SliderAttachment> (p.apvts, "irmix", irMix);
    aIrLevel = std::make_unique<SliderAttachment> (p.apvts, "irlevel", irLevel);
    aCircuit = std::make_unique<SliderAttachment> (p.apvts, "circuit", circuitSelector.slider);
    aIrOn = std::make_unique<ButtonAttachment> (p.apvts, "iron", irOn);
    aPhase = std::make_unique<ButtonAttachment> (p.apvts, "phase", phase);
    aBypass = std::make_unique<ButtonAttachment> (p.apvts, "bypass", bypass);
    aOversampling = std::make_unique<ComboAttachment> (p.apvts, "oversampling", oversamplingSelector.combo);

    startTimerHz (30);
}

PERCULATORAudioProcessorEditor::~PERCULATORAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void PERCULATORAudioProcessorEditor::configureKnob (juce::Slider& slider,
                                                     const juce::String& units,
                                                     int decimals)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.getProperties().set ("units", units);
    slider.getProperties().set ("decimals", decimals);
    slider.setDoubleClickReturnValue (true, slider.getValue());
}

void PERCULATORAudioProcessorEditor::configureLabel (juce::Label& label,
                                                      const juce::String& text,
                                                      float size)
{
    label.setText (text, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, textColour);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (size, juce::Font::bold));
    label.setInterceptsMouseClicks (false, false);
}

juce::Colour PERCULATORAudioProcessorEditor::currentPanelColour() const
{
    switch (processor.getPanelColourIndex())
    {
        case 0: return juce::Colour (0xff657447);
        case 2: return juce::Colour (0xffbc181d);
        default: return juce::Colour (0xff8fd8b8);
    }
}

void PERCULATORAudioProcessorEditor::drawScrew (juce::Graphics& g, float x, float y, float radius)
{
    juce::ColourGradient metal (juce::Colour (0xffe0dfd7), x - radius, y - radius,
                                juce::Colour (0xff686965), x + radius, y + radius, false);
    g.setGradientFill (metal);
    g.fillEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colours::black);
    g.drawEllipse (x - radius, y - radius, radius * 2.0f, radius * 2.0f, 1.5f);
    g.drawLine (x - radius * 0.55f, y, x + radius * 0.55f, y, 1.5f);
}

void PERCULATORAudioProcessorEditor::drawPanel (juce::Graphics& g)
{
    auto panel = juce::Rectangle<float> (10.0f, 54.0f, designWidth - 20.0f, designHeight - 66.0f);
    const auto colour = currentPanelColour();
    juce::ColourGradient paint (colour.brighter (0.13f), panel.getX(), panel.getY(),
                                colour.darker (0.22f), panel.getRight(), panel.getBottom(), false);
    g.setGradientFill (paint);
    g.fillRoundedRectangle (panel, 17.0f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawRoundedRectangle (panel, 17.0f, 4.0f);

    auto cabinet = juce::Rectangle<float> (770.0f, 82.0f, 395.0f, 430.0f);
    g.setColour (juce::Colour (0xff151918));
    g.fillRoundedRectangle (cabinet, 10.0f);
    g.setColour (juce::Colour (0xff686a65));
    g.drawRoundedRectangle (cabinet, 10.0f, 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawLine (755.0f, 90.0f, 755.0f, 515.0f, 2.0f);
    g.drawLine (32.0f, 548.0f, 1168.0f, 548.0f, 2.0f);

    drawScrew (g, 32.0f, 78.0f, 10.0f);
    drawScrew (g, 1168.0f, 78.0f, 10.0f);
    drawScrew (g, 32.0f, 720.0f, 10.0f);
    drawScrew (g, 1168.0f, 720.0f, 10.0f);
}

void PERCULATORAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0c0e0d));
    const auto transform = juce::AffineTransform::scale (getWidth() / designWidth,
                                                         getHeight() / designHeight);
    g.addTransform (transform);

    g.setColour (juce::Colour (0xff121514));
    g.fillRect (0.0f, 0.0f, designWidth, 48.0f);
    drawPanel (g);

    g.setColour (juce::Colour (0xff131615));
    g.fillRect (0.0f, 724.0f, designWidth, 26.0f);
    g.setColour (juce::Colour (0xffd6d5cc));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("v0.5.0 vector UI", 18, 725, 160, 23, juce::Justification::centredLeft);
    g.drawText ("BOVINA AUDIO", 1025, 725, 155, 23, juce::Justification::centredRight);
}

void PERCULATORAudioProcessorEditor::setScaledBounds (juce::Component& component,
                                                       int x, int y, int width, int height)
{
    const float sx = getWidth() / designWidth;
    const float sy = getHeight() / designHeight;
    component.setBounds (juce::roundToInt (x * sx), juce::roundToInt (y * sy),
                         juce::roundToInt (width * sx), juce::roundToInt (height * sy));
}

void PERCULATORAudioProcessorEditor::resized()
{
    setScaledBounds (panelColourBox, 815, 10, 250, 30);
    setScaledBounds (titleLabel, 38, 570, 520, 62);
    setScaledBounds (subtitleLabel, 48, 628, 485, 24);

    setScaledBounds (harmonicsLabel, 82, 94, 190, 25);
    setScaledBounds (balanceLabel, 362, 94, 190, 25);
    setScaledBounds (circuitLabel, 600, 92, 135, 24);
    setScaledBounds (harmonics, 82, 118, 190, 205);
    setScaledBounds (balance, 362, 118, 190, 205);
    setScaledBounds (circuitSelector, 570, 115, 180, 205);

    setScaledBounds (inputLabel, 76, 346, 150, 25);
    setScaledBounds (biasLabel, 270, 346, 140, 25);
    setScaledBounds (mixLabel, 432, 346, 140, 25);
    setScaledBounds (outputLabel, 590, 346, 150, 25);

    setScaledBounds (inputMeter, 50, 365, 200, 95);
    setScaledBounds (outputMeter, 565, 365, 200, 95);
    setScaledBounds (input, 76, 380, 150, 165);
    setScaledBounds (bias, 270, 380, 140, 165);
    setScaledBounds (mix, 432, 380, 140, 165);
    setScaledBounds (output, 590, 380, 150, 165);

    setScaledBounds (cabinetLabel, 835, 93, 260, 28);
    setScaledBounds (irFileLabel, 800, 135, 280, 42);
    setScaledBounds (previousIR, 800, 187, 55, 38);
    setScaledBounds (nextIR, 862, 187, 55, 38);
    setScaledBounds (loadIR, 925, 187, 205, 38);
    setScaledBounds (irMixLabel, 808, 246, 130, 23);
    setScaledBounds (irLevelLabel, 957, 246, 130, 23);
    setScaledBounds (irMix, 800, 270, 145, 155);
    setScaledBounds (irLevel, 950, 270, 145, 155);
    setScaledBounds (irOn, 1080, 265, 70, 38);
    setScaledBounds (phase, 1083, 326, 62, 92);
    setScaledBounds (phaseLabel, 1075, 420, 78, 20);
    setScaledBounds (oversamplingSelector, 815, 455, 315, 48);
    setScaledBounds (bypass, 1015, 575, 115, 90);
}

void PERCULATORAudioProcessorEditor::timerCallback()
{
    inputMeter.repaint();
    outputMeter.repaint();
    oversamplingSelector.repaint();

    if (irFileLabel.getText() != processor.getIRName())
        irFileLabel.setText (processor.getIRName(), juce::dontSendNotification);
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
    fileChooser = std::make_unique<juce::FileChooser> (
        "Load cabinet impulse response", juce::File {}, "*.wav;*.aif;*.aiff;*.flac");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  auto file = chooser.getResult();
                                  if (file.existsAsFile())
                                      processor.loadImpulseResponse (file);
                              });
}
