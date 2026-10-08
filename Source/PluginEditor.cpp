#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;

juce::String formatValue (float value, int decimals, const juce::String& suffix)
{
    return juce::String (value, decimals) + suffix;
}
}

ImageKnob::ImageKnob (const juce::Image& sourceImage,
                      juce::Rectangle<int> sourceRectangle,
                      float defaultNormalisedPosition)
    : source (sourceImage),
      sourceArea (sourceRectangle),
      defaultPosition (defaultNormalisedPosition)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                         juce::MathConstants<float>::pi * 2.75f,
                         true);
}

void ImageKnob::paint (juce::Graphics& g)
{
    if (! source.isValid())
        return;

    auto local = getLocalBounds().toFloat();
    auto centre = local.getCentre();

    const float normalised = static_cast<float> (valueToProportionOfLength (getValue()));
    
    // Calcolo dell'angolo basato sul range classico dei potenziometri
    const float minAngle = juce::MathConstants<float>::pi * 1.25f;
    const float maxAngle = juce::MathConstants<float>::pi * 2.75f;
    const float currentAngle = minAngle + normalised * (maxAngle - minAngle);
    
    // Angolo relativo rispetto alla posizione neutrale (ore 12 = 2.0 * PI)
    const float delta = currentAngle - (juce::MathConstants<float>::pi * 2.0f);

    g.saveState();
    
    // Ruota attorno al centro del componente
    g.addTransform (juce::AffineTransform::rotation (delta, centre.x, centre.y));

    // Disegna la porzione di manopola definita in sourceArea
    g.drawImage (source,
                 0, 0, getWidth(), getHeight(),
                 sourceArea.getX(), sourceArea.getY(), 
                 sourceArea.getWidth(), sourceArea.getHeight(),
                 false);

    g.restoreState();
}

PERCULATORAudioProcessorEditor::PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      panelImage (juce::ImageCache::getFromMemory (BinaryData::PerculatorPanel_png,
                                                   BinaryData::PerculatorPanel_pngSize)),
      harmonics (panelImage, { 108, 168, 208, 208 }),
      balance   (panelImage, { 422, 168, 208, 208 }),
      circuit   (panelImage, { 734, 222, 168, 168 }),
      input     (panelImage, {  78, 482, 186, 186 }),
      bias      (panelImage, { 302, 482, 186, 186 }),
      mix       (panelImage, { 522, 482, 186, 186 }), // Correggi X: da 498 a 522!
      output    (panelImage, { 722, 482, 186, 186 }),
      irMix     (panelImage, { 1022, 366, 132, 132 }),
      irLevel   (panelImage, { 1202, 366, 132, 132 }),
      loadIR (""), previousIR (""), nextIR (""), irOn (""), phase (""), bypass ("")
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
    addAndMakeVisible (oversampling);

    loadIR.setAlpha (0.01f);
    previousIR.setAlpha (0.01f);
    nextIR.setAlpha (0.01f);
    irOn.setAlpha (0.01f);
    phase.setAlpha (0.01f);
    bypass.setAlpha (0.01f);
    oversampling.setAlpha (0.01f);

    oversampling.addItemList ({ "2x", "4x", "8x" }, 1);

    loadIR.onClick = [this] { chooseIR(); };

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
    aOversampling = std::make_unique<ComboAttachment> (p.apvts, "oversampling", oversampling);

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
    auto selected = scaleRect ({ os == 0 ? 1066.0f : os == 1 ? 1193.0f : 1330.0f, 625, 112, 57 });
    g.setColour (juce::Colour (0xffffdf76));
    g.drawRoundedRectangle (selected, 6.0f, 3.0f);
}

void PERCULATORAudioProcessorEditor::resized()
{
    setControlBounds (harmonics, { 108, 168, 208, 208 });
    setControlBounds (balance,   { 422, 168, 208, 208 });
    setControlBounds (circuit,   { 734, 222, 168, 168 });
    setControlBounds (input,     {  78, 482, 186, 186 });
    setControlBounds (bias,      { 302, 482, 186, 186 });
    setControlBounds (mix,       { 522, 482, 186, 186 }); // Allineato al centro reale!
    setControlBounds (output,    { 722, 482, 186, 186 });
    setControlBounds (irMix,     { 1022, 366, 132, 132 });
    setControlBounds (irLevel,   { 1202, 366, 132, 132 });

    // Mantiieni il resto del layout per gli altri controlli...
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
