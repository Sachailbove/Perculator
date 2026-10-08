#include "PluginEditor.h"
#include <cmath>

void PedalLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float size = juce::jmin (r.getWidth(), r.getHeight()); r = r.withSizeKeepingCentre (size, size);
    const auto c = r.getCentre(); const float radius = size * 0.46f;
    g.setColour (juce::Colours::black.withAlpha (0.28f)); g.fillEllipse (r.translated (3.0f, 5.0f));
    juce::ColourGradient body (juce::Colour (0xff4a4a4a), c.x - radius, c.y - radius, juce::Colours::black, c.x + radius, c.y + radius, false);
    g.setGradientFill (body); g.fillEllipse (r); g.setColour (juce::Colour (0xff050505)); g.drawEllipse (r, 3.0f);
    auto hub = r.reduced (radius * 0.48f); juce::ColourGradient metal (juce::Colours::white, hub.getX(), hub.getY(), juce::Colour (0xff777777), hub.getRight(), hub.getBottom(), false);
    g.setGradientFill (metal); g.fillEllipse (hub); g.setColour (juce::Colours::black.withAlpha (0.45f)); g.drawEllipse (hub, 1.5f);
    const float ang = start + pos * (end - start); juce::Path p; p.addRoundedRectangle (-2.0f, -radius * 0.86f, 4.0f, radius * 0.42f, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f)); g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}
void PedalLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f); g.setColour (down ? juce::Colour (0xffff9d22) : over ? juce::Colour (0xff555555) : juce::Colour (0xff202020));
    g.fillRoundedRectangle (r, 4.0f); g.setColour (juce::Colours::black); g.drawRoundedRectangle (r, 4.0f, 2.0f);
}

void MeterArc::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f); const auto c = r.getCentre(); const float radius = juce::jmin (r.getWidth(), r.getHeight()) * 0.46f;
    constexpr int leds = 18; const float start = juce::MathConstants<float>::pi * 1.15f, end = juce::MathConstants<float>::pi * 1.85f;
    for (int i = 0; i < leds; ++i)
    {
        const float t = (float) i / (float) (leds - 1); const float a = start + t * (end - start);
        juce::Colour col = t < 0.50f ? juce::Colour (0xff30e84f) : t < 0.70f ? juce::Colour (0xffffe22b) : t < 0.86f ? juce::Colour (0xffff8b19) : juce::Colour (0xffff2525);
        const bool on = t <= level; g.setColour (on ? col : juce::Colours::black.withAlpha (0.32f));
        const auto p = c + juce::Point<float> (std::cos (a), std::sin (a)) * radius; g.fillEllipse (p.x - 4.5f, p.y - 4.5f, 9.0f, 9.0f);
        if (on) { g.setColour (col.withAlpha (0.25f)); g.drawEllipse (p.x - 7.0f, p.y - 7.0f, 14.0f, 14.0f, 2.0f); }
    }
}

PERCULATORAudioProcessorEditor::PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&look); setResizable (true, true); setResizeLimits (900, 560, 1700, 1060); setSize (1400, 860);
    styleSlider (harmonics, "HARMONICS", ""); styleSlider (balance, "BALANCE", ""); styleSlider (input, "INPUT", " dB"); styleSlider (bias, "BIAS", " %");
    styleSlider (mix, "MIX", " %"); styleSlider (output, "OUTPUT", " dB"); styleSlider (irMix, "IR MIX", " %"); styleSlider (irLevel, "IR LEVEL", " dB");
    styleSlider (circuit, "CIRCUIT", ""); circuit.setNumDecimalPlacesToDisplay (0); circuit.setTextValueSuffix ("");
    circuit.textFromValueFunction = [] (double v) { const juce::StringArray names { "NPN OD", "D310 Diodes", "Albino" }; return names[juce::jlimit (0, 2, juce::roundToInt (v))]; };
    addLabel (harmonicsLabel, "HARMONICS"); addLabel (balanceLabel, "BALANCE"); addLabel (inputLabel, "INPUT"); addLabel (biasLabel, "BIAS"); addLabel (mixLabel, "MIX"); addLabel (outputLabel, "OUTPUT");
    addLabel (irMixLabel, "IR MIX"); addLabel (irLevelLabel, "IR LEVEL"); addLabel (circuitLabel, "CIRCUIT");
    for (auto* c : { (juce::Component*) &loadIR, &prevIR, &nextIR, &irOn, &phase, &bypass, &irLabel, &colourBox, &oversamplingBox, &inputMeter, &outputMeter }) addAndMakeVisible (c);
    irLabel.setText (processor.getIRName(), juce::dontSendNotification); irLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff151515)); irLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey); irLabel.setJustificationType (juce::Justification::centredLeft);
    colourBox.addItem ("Dictatorship", 1); colourBox.addItem ("Surf Party", 2); colourBox.addItem ("Fetish Red", 3); colourBox.setSelectedId (processor.getPanelColourIndex() + 1, juce::dontSendNotification);
    colourBox.onChange = [this] { processor.setPanelColourIndex (colourBox.getSelectedId() - 1); repaint(); };
    oversamplingBox.addItemList ({ "2x", "4x", "8x" }, 1);
    loadIR.onClick = [this] { chooseIR(); };
    aHarmonics = std::make_unique<SliderAttachment> (p.apvts, "harmonics", harmonics); aBalance = std::make_unique<SliderAttachment> (p.apvts, "balance", balance);
    aInput = std::make_unique<SliderAttachment> (p.apvts, "input", input); aBias = std::make_unique<SliderAttachment> (p.apvts, "bias", bias); aMix = std::make_unique<SliderAttachment> (p.apvts, "mix", mix); aOutput = std::make_unique<SliderAttachment> (p.apvts, "output", output);
    aIrMix = std::make_unique<SliderAttachment> (p.apvts, "irmix", irMix); aIrLevel = std::make_unique<SliderAttachment> (p.apvts, "irlevel", irLevel); aCircuit = std::make_unique<SliderAttachment> (p.apvts, "circuit", circuit);
    aIrOn = std::make_unique<ButtonAttachment> (p.apvts, "iron", irOn); aPhase = std::make_unique<ButtonAttachment> (p.apvts, "phase", phase); aBypass = std::make_unique<ButtonAttachment> (p.apvts, "bypass", bypass); aOversampling = std::make_unique<ComboAttachment> (p.apvts, "oversampling", oversamplingBox);
    startTimerHz (30);
}
PERCULATORAudioProcessorEditor::~PERCULATORAudioProcessorEditor() { setLookAndFeel (nullptr); }
void PERCULATORAudioProcessorEditor::styleSlider (juce::Slider& s, const juce::String&, const juce::String& suffix) { addAndMakeVisible (s); s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 82, 20); s.setTextValueSuffix (suffix); }
void PERCULATORAudioProcessorEditor::addLabel (juce::Label& l, const juce::String& t) { addAndMakeVisible (l); l.setText (t, juce::dontSendNotification); l.setJustificationType (juce::Justification::centred); l.setFont (juce::FontOptions (18.0f, juce::Font::bold)); }
juce::Colour PERCULATORAudioProcessorEditor::panelColour() const { const juce::Colour colours[] { juce::Colour (0xff647347), juce::Colour (0xff91dfbb), juce::Colour (0xffc8171d) }; return colours[juce::jlimit (0, 2, processor.getPanelColourIndex())]; }

void PERCULATORAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101010)); auto panel = getLocalBounds().reduced (14).withTrimmedTop (48).withTrimmedBottom (42).toFloat();
    juce::ColourGradient grad (panelColour().brighter (0.18f), panel.getX(), panel.getY(), panelColour().darker (0.20f), panel.getRight(), panel.getBottom(), false); g.setGradientFill (grad); g.fillRoundedRectangle (panel, 22.0f);
    g.setColour (juce::Colours::black.withAlpha (0.72f)); g.drawRoundedRectangle (panel, 22.0f, 4.0f);
    g.setColour (juce::Colours::black); g.setFont (juce::FontOptions (56.0f, juce::Font::bold)); g.drawText ("PERCULATOR", 55, getHeight() - 175, 500, 70, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (17.0f, juce::Font::bold)); g.drawText ("DUAL POLARITY HARMONIC GENERATOR", 60, getHeight() - 112, 510, 28, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold)); g.drawText ("CABINET IR", getWidth() - 420, 78, 300, 30, juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.9f)); g.setFont (juce::FontOptions (14.0f)); g.drawText ("PANEL COLOR", 30, 12, 110, 28, juce::Justification::centredLeft);
    g.drawText ("v0.2.1 alpha", 32, getHeight() - 34, 120, 20, juce::Justification::centredLeft);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.drawLine ((float) getWidth() * 0.68f, 105.0f, (float) getWidth() * 0.68f, (float) getHeight() - 80.0f, 2.0f);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold)); g.setColour (juce::Colours::black); g.drawText ("NPN OD        D310 DIODES        ALBINO", getWidth() - 520, getHeight() - 180, 450, 24, juce::Justification::centred);
}

void PERCULATORAudioProcessorEditor::resized()
{
    const float sx = getWidth() / 1400.0f, sy = getHeight() / 860.0f; auto S = [sx,sy] (int x,int y,int w,int h) { return juce::Rectangle<int> ((int)(x*sx),(int)(y*sy),(int)(w*sx),(int)(h*sy)); };
    colourBox.setBounds (S (145, 10, 160, 30)); oversamplingBox.setBounds (S (1105, 520, 110, 34));
    harmonicsLabel.setBounds (S (120, 100, 220, 30)); harmonics.setBounds (S (115, 130, 230, 235)); balanceLabel.setBounds (S (520, 100, 220, 30)); balance.setBounds (S (515, 130, 230, 235));
    inputLabel.setBounds (S (90, 390, 170, 28)); input.setBounds (S (85, 420, 180, 190)); inputMeter.setBounds (S (74, 402, 202, 188));
    biasLabel.setBounds (S (285, 390, 160, 28)); bias.setBounds (S (280, 420, 170, 190)); mixLabel.setBounds (S (475, 390, 160, 28)); mix.setBounds (S (470, 420, 170, 190));
    outputLabel.setBounds (S (655, 390, 180, 28)); output.setBounds (S (650, 420, 190, 190)); outputMeter.setBounds (S (642, 402, 206, 188));
    irLabel.setBounds (S (960, 130, 330, 55)); irOn.setBounds (S (900, 132, 60, 50)); prevIR.setBounds (S (965, 192, 60, 34)); nextIR.setBounds (S (1030, 192, 60, 34)); loadIR.setBounds (S (1100, 192, 190, 34));
    irMixLabel.setBounds (S (900, 245, 145, 26)); irMix.setBounds (S (895, 272, 150, 160)); irLevelLabel.setBounds (S (1050, 245, 145, 26)); irLevel.setBounds (S (1045, 272, 150, 160)); phase.setBounds (S (1210, 300, 100, 42)); bypass.setBounds (S (1210, 390, 100, 48));
    circuitLabel.setBounds (S (970, 650, 250, 28)); circuit.setBounds (S (1000, 680, 190, 160));
}
void PERCULATORAudioProcessorEditor::timerCallback() { inputMeter.setLevel (processor.getInputMeter()); outputMeter.setLevel (processor.getOutputMeter()); if (irLabel.getText() != processor.getIRName()) irLabel.setText (processor.getIRName(), juce::dontSendNotification); }
bool PERCULATORAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& f) { return f.size() == 1 && juce::File (f[0]).hasFileExtension ("wav;aif;aiff;flac"); }
void PERCULATORAudioProcessorEditor::filesDropped (const juce::StringArray& f, int, int) { if (! f.isEmpty()) processor.loadImpulseResponse (juce::File (f[0])); }
void PERCULATORAudioProcessorEditor::chooseIR() { chooser = std::make_unique<juce::FileChooser> ("Load cabinet impulse response", juce::File {}, "*.wav;*.aif;*.aiff;*.flac"); chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f.existsAsFile()) processor.loadImpulseResponse (f); }); }
