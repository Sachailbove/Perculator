#include "PluginEditor.h"
#include <cmath>

namespace
{
juce::String formatValue (float value, int decimals, const juce::String& suffix)
{
    return juce::String (value, decimals) + suffix;
}
}

PerculatorKnob::PerculatorKnob()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                         juce::MathConstants<float>::pi * 2.75f,
                         true);
}

void PerculatorKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    auto centre = bounds.getCentre();
    float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (bounds.translated (0.0f, 4.0f));

    g.setColour (juce::Colour (0xff1a1d1a));
    g.fillEllipse (bounds);

    juce::ColourGradient metalGrad (juce::Colour (0xffd8d8d8), centre.x, centre.y - radius,
                                    juce::Colour (0xff707470), centre.x, centre.y + radius, true);
    g.setGradientFill (metalGrad);
    g.fillEllipse (bounds.reduced (3.0f));

    juce::ColourGradient innerGrad (juce::Colour (0xffffffff), centre.x, centre.y - radius * 0.6f,
                                    juce::Colour (0xffaaaaaa), centre.x, centre.y + radius * 0.6f, true);
    g.setGradientFill (innerGrad);
    g.fillEllipse (bounds.reduced (radius * 0.25f));

    const double normalised = valueToProportionOfLength (getValue());
    const float rotAngle = juce::MathConstants<float>::pi * (1.25f + normalised * 1.50f);

    g.setColour (juce::Colour (0xff111111));
    juce::Path pointer;
    float pointerWidth = radius * 0.12f;
    float pointerLength = radius * 0.65f;
    pointer.addRectangle (-pointerWidth * 0.5f, -radius * 0.85f, pointerWidth, pointerLength);
    
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (rotAngle, centre.x, centre.y));
    g.fillPath (pointer);
    g.restoreState();
}

PERCULATORAudioProcessorEditor::PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      panelImage (juce::ImageCache::getFromMemory (BinaryData::PerculatorPanel_png,
                                                   BinaryData::PerculatorPanel_pngSize)),
      loadIR (""), previousIR (""), nextIR (""), irOn (""), phase (""), bypass (""),
      os2x ("2x"), os4x ("4x"), os8x ("8x")
{
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (960, 640, 1920, 1280);
    getConstrainer()->setFixedAspectRatio (designWidth / designHeight);
    setSize (1536, 1024);

    for (auto* knob : { &harmonics, &balance, &circuit, &input, &bias, &mix,
                        &output, &irMix, &irLevel })
        addAndMakeVisible (*knob);

    addAndMakeVisible (loadIR);
    addAndMakeVisible (previousIR);
    addAndMakeVisible (nextIR);
    addAndMakeVisible (irOn);
    addAndMakeVisible (phase);
    addAndMakeVisible (bypass);
    addAndMakeVisible (os2x);
    addAndMakeVisible (os4x);
    addAndMakeVisible (os8x);

    irOn.setClickingTogglesState (true);
    phase.setClickingTogglesState (true);
    bypass.setClickingTogglesState (true);

    loadIR.setAlpha (0.01f);
    previousIR.setAlpha (0.01f);
    nextIR.setAlpha (0.01f);
    irOn.setAlpha (0.01f);
    phase.setAlpha (0.01f);
    bypass.setAlpha (0.01f);
    
    os2x.setAlpha (0.01f);
    os4x.setAlpha (0.01f);
    os8x.setAlpha (0.01f);

    loadIR.onClick = [this] { chooseIR(); };

    auto setOversampling = [this] (float val)
    {
        if (auto* param = processor.apvts.getParameter ("oversampling"))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (val));
            param->endChangeGesture();
        }
    };

    os2x.onClick = [setOversampling] { setOversampling (0.0f); };
    os4x.onClick = [setOversampling] { setOversampling (1.0f); };
    os8x.onClick = [setOversampling] { setOversampling (2.0f); };

    aHarmonics = std::make_unique<SliderAttachment> (p.apvts, "harmonics", harmonics);
    aBalance   = std::make_unique<SliderAttachment> (p.apvts, "balance", balance);
    aCircuit   = std::make_unique<SliderAttachment> (p.apvts, "circuit", circuit);
    aInput     = std::make_unique<SliderAttachment> (p.apvts, "input", input);
    aBias      = std::make_unique<SliderAttachment> (p.apvts, "bias", bias);
    aMix       = std::make_unique<SliderAttachment> (p.apvts, "mix", mix);
    aOutput    = std::make_unique<SliderAttachment> (p.apvts, "output", output);
    aIrMix     = std::make_unique<SliderAttachment> (p.apvts, "irmix", irMix);
    aIrLevel   = std::make_unique<SliderAttachment> (p.apvts, "irlevel", irLevel);
    aIrOn      = std::make_unique<ButtonAttachment> (p.apvts, "iron", irOn);
    aPhase     = std::make_unique<ButtonAttachment> (p.apvts, "phase", phase);
    aBypass    = std::make_unique<ButtonAttachment> (p.apvts, "bypass", bypass);

    startTimerHz (30);
}

PERCULATORAudioProcessorEditor::~PERCULATORAudioProcessorEditor() = default;

juce::Rectangle<float> PERCULATORAudioProcessorEditor::scaleRect (juce::Rectangle<float> r) const
{
    const float sx = static_cast<float> (getWidth()) / designWidth;
    const float sy = static_cast<float> (getHeight()) / designHeight;
    return { r.getX() * sx, r.getY() * sy, r.getWidth() * sx, r.getHeight() * sy };
}

juce::Point<float> PERCULATORAudioProcessorEditor::scalePoint (juce::Point<float> p) const
{
    return { p.x * static_cast<float> (getWidth()) / designWidth,
             p.y * static_cast<float> (getHeight()) / designHeight };
}

void PERCULATORAudioProcessorEditor::setControlBounds (juce::Component& c,
                                                        juce::Rectangle<int> r)
{
    c.setBounds (scaleRect (r.toFloat()).toNearestInt());
}

void PERCULATORAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    
    if (panelImage.isValid())
        g.drawImage (panelImage, getLocalBounds().toFloat());

    drawMeters (g);
    drawDynamicReadouts (g);
    drawStateLights (g);
}

void PERCULATORAudioProcessorEditor::drawMeters (juce::Graphics& g)
{
    const auto drawArc = [&] (juce::Point<float> designCentre, float designRadius, float level)
    {
        const auto centre = scalePoint (designCentre);
        const float scale = static_cast<float> (getWidth()) / designWidth;
        const float radius = designRadius * scale;
        constexpr int ledCount = 20;

        for (int i = 0; i < ledCount; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (ledCount - 1);
            const float angle = juce::MathConstants<float>::pi * (1.13f + 0.74f * t);
            const auto point = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * radius;

            juce::Colour active = t < 0.50f ? juce::Colour (0xff31e94e)
                                : t < 0.70f ? juce::Colour (0xffffdf28)
                                : t < 0.86f ? juce::Colour (0xffff8a18)
                                             : juce::Colour (0xffff2525);

            const bool lit = t <= level;
            const float size = 10.0f * scale;
            g.setColour (lit ? active : juce::Colour (0xff22251d));
            g.fillEllipse (point.x - size * 0.5f, point.y - size * 0.5f, size, size);
            g.setColour (juce::Colours::black.withAlpha (0.65f));
            g.drawEllipse (point.x - size * 0.5f, point.y - size * 0.5f, size, size, 1.0f);
        }
    };

    drawArc ({ 171.0f, 565.0f }, 109.0f, processor.getInputMeter());
    drawArc ({ 815.0f, 565.0f }, 109.0f, processor.getOutputMeter());
}

void PERCULATORAudioProcessorEditor::drawDynamicReadouts (juce::Graphics& g)
{
    const auto value = [this] (const char* id)
    {
        return processor.apvts.getRawParameterValue (id)->load();
    };

    const auto box = [&] (juce::Rectangle<float> design, const juce::String& text)
    {
        auto r = scaleRect (design);
        g.setColour (juce::Colour (0xff111311));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colour (0xffb7aa73));
        g.drawRoundedRectangle (r, 4.0f, 1.2f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (16.0f * getWidth() / designWidth));
        g.drawText (text, r, juce::Justification::centred);
    };

    box ({ 116, 678, 112, 33 }, formatValue (value ("input"), 1, " dB"));
    box ({ 343, 678, 112, 33 }, formatValue (value ("bias"), 2, " %"));
    box ({ 565, 678, 112, 33 }, formatValue (value ("mix"), 1, " %"));
    box ({ 784, 678, 112, 33 }, formatValue (value ("output"), 1, " dB"));
    box ({ 1042, 515, 112, 31 }, formatValue (value ("irmix"), 1, " %"));
    box ({ 1218, 515, 112, 31 }, formatValue (value ("irlevel"), 1, " dB"));

    auto irBox = scaleRect ({ 1024, 177, 383, 57 });
    g.setColour (juce::Colour (0xff0c0e0d));
    g.fillRoundedRectangle (irBox, 5.0f);
    g.setColour (juce::Colour (0xffdddddd));
    g.setFont (juce::FontOptions (18.0f * getWidth() / designWidth));
    g.drawText (processor.getIRName(), irBox.reduced (12.0f, 0.0f), juce::Justification::centredLeft);
}

void PERCULATORAudioProcessorEditor::drawStateLights (juce::Graphics& g)
{
    const auto light = [&] (juce::Point<float> p, bool on, juce::Colour colour)
    {
        const auto point = scalePoint (p);
        const float radius = 11.0f * getWidth() / designWidth;
        g.setColour (on ? colour : juce::Colour (0xff21311f));
        g.fillEllipse (point.x - radius, point.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (on ? colour.brighter (0.7f) : juce::Colours::black);
        g.drawEllipse (point.x - radius, point.y - radius, radius * 2.0f, radius * 2.0f, 2.0f);
    };

    light ({ 1394, 364 }, irOn.getToggleState(), juce::Colour (0xff33ff22));
    light ({ 1343, 774 }, ! bypass.getToggleState(), juce::Colour (0xffff2417));

    const int os = juce::roundToInt (processor.apvts.getRawParameterValue ("oversampling")->load());
    auto selectedOsRect = scaleRect (os == 0 ? juce::Rectangle<float> (1046, 603, 115, 62)
                                   : os == 1 ? juce::Rectangle<float> (1198, 603, 115, 62)
                                             : juce::Rectangle<float> (1350, 603, 115, 62));
    g.setColour (juce::Colour (0xffffdf76));
    g.drawRoundedRectangle (selectedOsRect, 6.0f, 3.0f);
}

void PERCULATORAudioProcessorEditor::resized()
{
    setControlBounds (harmonics, { 106, 165, 212, 212 });
    setControlBounds (balance,   { 420, 165, 212, 212 });
    setControlBounds (circuit,   { 730, 220, 175, 175 });
    setControlBounds (input,     {  76, 480, 190, 190 });
    setControlBounds (bias,      { 300, 480, 190, 190 });
    setControlBounds (mix,       { 498, 480, 190, 190 });
    setControlBounds (output,    { 720, 480, 190, 190 });
    setControlBounds (irMix,     { 1020, 365, 135, 135 });
    setControlBounds (irLevel,   { 1203, 365, 135, 135 });

    setControlBounds (previousIR, { 1020, 248, 70, 52 });
    setControlBounds (nextIR,     { 1098, 248, 70, 52 });
    setControlBounds (loadIR,     { 1186, 248, 294, 52 });
    setControlBounds (irOn,       { 1363, 330, 120, 75 });
    setControlBounds (phase,      { 1368, 397, 95, 115 });
    setControlBounds (bypass,     { 1270, 782, 145, 135 });

    setControlBounds (os2x, { 1046, 603, 115, 62 });
    setControlBounds (os4x, { 1198, 603, 115, 62 });
    setControlBounds (os8x, { 1350, 603, 115, 62 });
}

void PERCULATORAudioProcessorEditor::timerCallback()
{
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
        "Load cabinet impulse response", juce::File {}, "*.wav;*.aif;*.aiff;*.flac");

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fileChooser)
                          {
                              auto file = fileChooser.getResult();
                              if (file.existsAsFile())
                                  processor.loadImpulseResponse (file);
                          });
}
