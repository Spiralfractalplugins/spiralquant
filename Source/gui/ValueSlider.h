#pragma once
#include <JuceHeader.h>
#include <functional>
#include "SciFiLookAndFeel.h"

/** Rotary slider with custom value formatting. */
class ValueSlider : public juce::Slider
{
public:
    using FormatFn = std::function<juce::String (double)>;

    FormatFn format;
    juce::String labelText;

    void setLabelText (const juce::String& l) { labelText = l; repaint(); }

    void paint (juce::Graphics& g) override
    {
        juce::Slider::paint (g);
        if (labelText.isEmpty())
            return;
        g.setColour (SciFiTheme::textDim);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawFittedText (labelText, getLocalBounds().removeFromTop (12),
                          juce::Justification::centred, 1);
    }

    juce::String getTextFromValue (double value) override
    {
        if (format)
            return format (value);
        return juce::Slider::getTextFromValue (value);
    }

    double getValueFromText (const juce::String& text) override
    {
        auto s = text.trim();
        const bool percent = s.contains ("%");
        const bool kilo = s.containsIgnoreCase ("k");
        auto num = s.retainCharacters ("0123456789.+-eE").getDoubleValue();
        if (percent) num /= 100.0;
        if (kilo) num *= 1000.0;
        return num;
    }
};
