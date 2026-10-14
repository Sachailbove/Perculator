#include "PluginEditor.h"
#include <cmath>

PerculatorKnob::PerculatorKnob()
    : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox)
{
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                         juce::MathConstants<float>::pi * 2.75f, true);
}

void PerculatorKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (6.0f);
    auto readout = bounds.removeFromBottom (28.0f).reduced (8.0f, 2.0f)
                         .translated (0.0f, 5.0f);
    const auto centre = bounds.getCentre();
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.48f;

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (centre.x - radius + 3.0f, centre.y - radius + 5.0f,
                   radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colour (0xff121413));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    for (int i = 0; i < 20; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * static_cast<float> (i) / 20.0f;
        const auto p1 = centre + juce::Point<float> (std::cos (a), std::sin (a)) * radius * 0.83f;
        const auto p2 = centre + juce::Point<float> (std::cos (a), std::sin (a)) * radius * 0.98f;
        g.setColour (juce::Colour (0xff4f5350));
        g.drawLine ({ p1, p2 }, juce::jmax (1.0f, radius * 0.035f));
    }

    const float ringRadius = radius * 0.79f;
    juce::ColourGradient ring (juce::Colour (0xff777c79), centre.x, centre.y - ringRadius,
                               juce::Colour (0xff1b1e1c), centre.x, centre.y + ringRadius, false);
    g.setGradientFill (ring);
    g.fillEllipse (centre.x - ringRadius, centre.y - ringRadius,
                   ringRadius * 2.0f, ringRadius * 2.0f);

    const float capRadius = ringRadius * 0.78f;
    juce::ColourGradient cap (juce::Colour (0xffe2e6e3), centre.x, centre.y - capRadius,
                              juce::Colour (0xff858b88), centre.x, centre.y + capRadius, false);
    g.setGradientFill (cap);
    g.fillEllipse (centre.x - capRadius, centre.y - capRadius,
                   capRadius * 2.0f, capRadius * 2.0f);

    const float proportion = static_cast<float> (valueToProportionOfLength (getValue()));
    const auto rotary = getRotaryParameters();
    const float angle = rotary.startAngleRadians + proportion * (rotary.endAngleRadians - rotary.startAngleRadians);
    juce::Path pointer;
    pointer.addRoundedRectangle (-3.0f, -capRadius * 0.95f, 6.0f, capRadius * 0.72f, 3.0f);
    g.setColour (juce::Colour (0xff111311));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    g.setColour (juce::Colour (0xff111311));
    g.fillRoundedRectangle (readout, 4.0f);
    g.setColour (juce::Colour (0xffb7aa73));
    g.drawRoundedRectangle (readout, 4.0f, 1.0f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (getTextFromValue (getValue()), readout, juce::Justification::centred);
}
