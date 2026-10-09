#include "PluginProcessor.h"
#include "PluginEditor.h"
#pragma once
#include <JuceHeader.h>

class PerculatorKnob : public juce::Slider
{
public:
    PerculatorKnob() : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox)
    {
        setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, 
                            juce::MathConstants<float>::pi * 2.75f, true);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        auto centre = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

        // 1. Ghiera esterna zigrinata / scura
        g.setColour(juce::Colour(0xff121413));
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        // Simulazione zigrinatura bordo esterno
        g.setColour(juce::Colour(0xff080908));
        for (float r = radius; r > radius * 0.82f; r -= 2.0f)
            g.drawEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 1.0f);

        // 2. Anello metallico intermedio a gradini
        float ringRadius = radius * 0.82f;
        juce::ColourGradient ringGrad(juce::Colour(0xff686d6a), centre.x, centre.y - ringRadius,
                                      juce::Colour(0xff1c1e1d), centre.x, centre.y + ringRadius, false);
        g.setGradientFill(ringGrad);
        g.fillEllipse(centre.x - ringRadius, centre.y - ringRadius, ringRadius * 2.0f, ringRadius * 2.0f);

        // 3. Cupola centrale in metallo spazzolato / lucido
        float capRadius = ringRadius * 0.80f;
        juce::ColourGradient capGrad(juce::Colour(0xffe2e6e3), centre.x, centre.y - capRadius,
                                     juce::Colour(0xff8c928f), centre.x, centre.y + capRadius, false);
        g.setGradientFill(capGrad);
        g.fillEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        // Riflesso speculare sulla cupola
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.fillEllipse(centre.x - capRadius * 0.7f, centre.y - capRadius * 0.85f, capRadius * 1.4f, capRadius * 0.7f);

        // 4. Tacca / Indicatore nero rotante
        const float rotVal = (getValue() - getMinimum()) / (getMaximum() - getMinimum());
        const float angle = getRotaryParameters().startAngle + 
                            rotVal * (getRotaryParameters().endAngle - getRotaryParameters().startAngle);

        g.setColour(juce::Colour(0xff111311));
        juce::Path pointer;
        float pointerWidth = 7.0f;
        float pointerLength = capRadius * 0.95f;
        pointer.addRoundedRectangle(-pointerWidth * 0.5f, -pointerLength, pointerWidth, pointerLength, 3.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
        g.fillPath(pointer);
    }
};
static juce::String formatValue (float value, int decimalPlaces, const juce::String& suffix)
{
    return juce::String (value, decimalPlaces) + suffix;
}
static constexpr float designWidth = 1536.0f;
static constexpr float designHeight = 1024.0f;

juce::Rectangle<float> PERCULATORAudioProcessorEditor::scaleRect (juce::Rectangle<float> designRect) const
{
    float scaleX = static_cast<float> (getWidth()) / designWidth;
    float scaleY = static_cast<float> (getHeight()) / designHeight;
    return { designRect.getX() * scaleX, designRect.getY() * scaleY, designRect.getWidth() * scaleX, designRect.getHeight() * scaleY };
}

juce::Point<float> PERCULATORAudioProcessorEditor::scalePoint (juce::Point<float> designPoint) const
{
    float scaleX = static_cast<float> (getWidth()) / designWidth;
    float scaleY = static_cast<float> (getHeight()) / designHeight;
    return { designPoint.x * scaleX, designPoint.y * scaleY };
}

void PERCULATORAudioProcessorEditor::setControlBounds (juce::Component& component, juce::Rectangle<int> designRect)
{
    auto scaled = scaleRect (designRect.toFloat());
    component.setBounds (scaled.toNearestInt());
}
void PERCULATORAudioProcessorEditor::resized()
{
    // Fila superiore (Harmonics e Balance grandi, Circuit medio con etichette dedicate)
    // Harmonics e Balance posizionati alla distanza corretta dalla finestrella del valore
    setControlBounds (harmonics, { 106, 140, 212, 212 });
    setControlBounds (balance,   { 420, 140, 212, 212 });
    setControlBounds (circuit,   { 730, 185, 175, 175 }); // Dimensionato coerentemente con la reference

    // Fila inferiore (Input, Bias, Mix, Output con archetti LED)
    setControlBounds (input,     {  76, 480, 190, 190 });
    setControlBounds (bias,      { 300, 480, 190, 190 });
    setControlBounds (mix,       { 498, 480, 190, 190 });
    setControlBounds (output,    { 720, 480, 190, 190 });
    
    // Sezione Cabinet IR (destra)
    setControlBounds (irMix,     { 1020, 365, 135, 135 });
    setControlBounds (irLevel,   { 1203, 365, 135, 135 });

    setControlBounds (previousIR, { 1020, 248, 70, 52 });
    setControlBounds (nextIR,     { 1098, 248, 70, 52 });
    setControlBounds (loadIR,     { 1186, 248, 294, 52 });
    setControlBounds (irOn,       { 1363, 330, 120, 75 });
    setControlBounds (phase,      { 1368, 397, 95, 115 });
    setControlBounds (bypass,     { 1270, 782, 145, 135 });

    // Pulsanti Oversampling in basso a destra
    setControlBounds (os2x, { 1046, 655, 115, 62 });
    setControlBounds (os4x, { 1198, 655, 115, 62 });
    setControlBounds (os8x, { 1350, 655, 115, 62 });
}

void PERCULATORAudioProcessorEditor::drawDynamicReadouts (juce::Graphics& g)
{
    const auto value = [this] (const char* id) { return processor.apvts.getRawParameterValue (id)->load(); };

    const auto label = [&] (juce::Rectangle<float> design, const juce::String& text, juce::Justification justification)
    {
        auto r = scaleRect (design);
        g.setColour (juce::Colour (0xffb7aa73));
        g.setFont (juce::FontOptions (14.0f * getWidth() / designWidth));
        g.drawText (text, r, justification);
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

    // Etichette statiche 0 - 10 per Harmonics e Balance
    label ({ 110, 355, 40, 20 }, "0", juce::Justification::left);
    label ({ 278, 355, 40, 20 }, "10", juce::Justification::right);
    label ({ 424, 355, 40, 20 }, "0", juce::Justification::left);
    label ({ 592, 355, 40, 20 }, "10", juce::Justification::right);

    // Box valori digitali superiori
    box ({ 156, 352, 112, 33 }, formatValue (value ("harmonics"), 1, ""));
    box ({ 470, 352, 112, 33 }, formatValue (value ("balance"), 1, ""));

    // Box valori digitali inferiori
    box ({ 116, 678, 112, 33 }, formatValue (value ("input"), 1, " dB"));
    box ({ 343, 678, 112, 33 }, formatValue (value ("bias"), 2, " %"));
    box ({ 565, 678, 112, 33 }, formatValue (value ("mix"), 1, " %"));
    box ({ 784, 678, 112, 33 }, formatValue (value ("output"), 1, " dB"));
    box ({ 1042, 515, 112, 31 }, formatValue (value ("irmix"), 1, " %"));
    box ({ 1218, 515, 112, 31 }, formatValue (value ("irlevel"), 1, " dB"));

    // Nome IR caricato
    auto irBox = scaleRect ({ 1024, 177, 383, 57 });
    g.setColour (juce::Colour (0xff0c0e0d));
    g.fillRoundedRectangle (irBox, 5.0f);
    g.setColour (juce::Colour (0xffdddddd));
    g.setFont (juce::FontOptions (18.0f * getWidth() / designWidth));
    g.drawText (processor.getIRName(), irBox.reduced (12.0f, 0.0f), juce::Justification::centredLeft);
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
            const float angle = juce::MathConstants<float>::pi * (1.15f + 0.70f * t);
            const auto point = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * radius;

            juce::Colour active = t < 0.50f ? juce::Colour (0xff31e94e)
                                : t < 0.70f ? juce::Colour (0xffffdf28)
                                : t < 0.86f ? juce::Colour (0xffff8a18)
                                             : juce::Colour (0xffff2525);

            const bool lit = t <= level;
            const float size = 9.0f * scale;
            g.setColour (lit ? active : juce::Colour (0xff22251d));
            g.fillEllipse (point.x - size * 0.5f, point.y - size * 0.5f, size, size);
            g.setColour (juce::Colours::black.withAlpha (0.65f));
            g.drawEllipse (point.x - size * 0.5f, point.y - size * 0.5f, size, size, 1.0f);
        }
    };

    // Archetti LED Input e Output posizionati sopra i rispettivi pomelli inferiori
    drawArc ({ 171.0f, 545.0f }, 98.0f, processor.getInputMeter());
    drawArc ({ 815.0f, 545.0f }, 98.0f, processor.getOutputMeter());
}
