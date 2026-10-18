#pragma once
#include <JuceHeader.h>
#include <functional>
#include "Palette.h"

/** Rotary knob: wheel steps 1% (5% with Shift), double-click opens exact-value
    entry, captions are drawn by the editor as separate name/value labels. */
class ValueSlider : public juce::Slider
{
public:
    using FormatFn = std::function<juce::String (double)>;

    FormatFn format;
    juce::String caption { "value" };

    ValueSlider()
    {
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    }

    juce::String valueText() { return getTextFromValue (getValue()); }

    void paint (juce::Graphics& g) override
    {
        juce::Slider::paint (g);
        if (isMouseOver() && isEnabled())
        {
            const Palette& pal = Skins::current();
            g.setColour (pal.accent.withAlpha (0.12f));
            const float r = (float) juce::jmin (getWidth(), getHeight()) * 0.5f;
            g.fillEllipse ((float) getWidth() * 0.5f - r, (float) getHeight() * 0.5f - r, r * 2.0f, r * 2.0f);
        }
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (! isEnabled())
            return;
        auto alert = std::make_shared<juce::AlertWindow> (
            "Set " + caption, "Enter an exact value:", juce::AlertWindow::NoIcon);
        alert->addTextEditor ("value", getTextFromValue (getValue()), "Value");
        alert->addButton ("OK", 1);
        alert->addButton ("Cancel", 0);

        juce::Component::SafePointer<ValueSlider> self (this);
        alert->enterModalState (true, juce::ModalCallbackFunction::create (
            [alert, self] (int result)
            {
                if (result == 1 && self != nullptr)
                    self->setValue (self->getValueFromText (alert->getTextEditorContents ("value")));
                alert->setVisible (false);
            }));
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (! isEnabled() || wheel.deltaY == 0.0f)
        {
            juce::Slider::mouseWheelMove (e, wheel);
            return;
        }
        const double range = getRange().getLength();
        const double step = range * (e.mods.isShiftDown() ? 0.05 : 0.01);
        setValue (juce::jlimit (getRange().getStart(), getRange().getEnd(),
                                getValue() + (wheel.deltaY > 0.0f ? step : -step)));
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
        const bool hz = s.containsIgnoreCase ("hz");
        auto num = s.retainCharacters ("0123456789.+-eE").getDoubleValue();
        if (percent) num /= 100.0;
        if (kilo || hz) num *= 1000.0;
        return num;
    }
};
