#pragma once
#include <JuceHeader.h>
#include <functional>

/** Rotary slider with custom value formatting. */
class ValueSlider : public juce::Slider
{
public:
    using FormatFn = std::function<juce::String (double)>;

    FormatFn format;

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
