#pragma once

#include <JuceHeader.h>

#include "DarkLookAndFeel.h"

/** A block of dim text that flows across as many lines as the box allows.
    JUCE 8 took word wrap out of Label, and the preset credits are exactly the
    sort of line that needs it. */
class WrappedLabel : public juce::Component
{
public:
    explicit WrappedLabel (juce::Colour colour = Theme::textDim) : textColour (colour) {}

    void setText (const juce::String& t) { text = t; repaint(); }
    const juce::String& getText() const { return text; }

    void setScale (float s)
    {
        uiScale = s;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (textColour);
        g.setFont (Theme::font (9.0f * uiScale));
        g.drawMultiLineText (text, 1, static_cast<int> (11.0f * uiScale),
                             jmax (10, getWidth() - 2));
    }

private:
    juce::String text;
    juce::Colour textColour;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WrappedLabel)
};
