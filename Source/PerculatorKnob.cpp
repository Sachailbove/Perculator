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

        // Simulazione zigrinatura bordo esterno (cerchietti concentrici / dentini)
        g.setColour(juce::Colour(0xff080908));
        for (float r = radius; r > radius * 0.82f; r -= 2.0f)
            g.drawEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 1.0f);

        // 2. Anello metallico intermedio a gradini
        float ringRadius = radius * 0.82f;
        juduce_metal_ring:
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