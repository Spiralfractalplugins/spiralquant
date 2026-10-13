#pragma once

#include <JuceHeader.h>

/** Slider tuned for this editor: the mouse wheel nudges by one percent of the
    range per notch (five percent with Shift held), and double-click opens an
    exact-value entry box, so a target like 55 Hz can be typed rather than
    hunted by dragging. */
class KnobSlider : public juce::Slider
{
public:
    KnobSlider (Slider::SliderStyle style = RotaryHorizontalVerticalDrag,
                Slider::TextEntryBoxPosition boxStyle = NoTextBox)
        : juce::Slider (style, boxStyle)
    {
        setSliderSnapsToMousePosition (false);
    }

    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel) override
    {
        if (! isEnabled() || wheel.deltaY == 0.0f)
        {
            Slider::mouseWheelMove (e, wheel);
            return;
        }

        const double span = static_cast<double> (getMaximum()) - static_cast<double> (getMinimum());
        double step = getInterval() > 0.0 ? static_cast<double> (getInterval()) : span * 0.01;
        if (e.mods.isShiftDown())
            step *= 5.0;

        setValue (juce::jlimit (static_cast<double> (getMinimum()),
                                static_cast<double> (getMaximum()),
                                getValue() + static_cast<double> (wheel.deltaY) * step));
    }

    void mouseDoubleClick (const MouseEvent&) override
    {
        if (! isEnabled())
            return;

        auto alert = std::make_shared<juce::AlertWindow> (
            getName().isNotEmpty() ? getName() : "Value",
            "Enter an exact value:", juce::AlertWindow::NoIcon);
        alert->addTextEditor ("value", getTextFromValue (getValue()), "Value");
        alert->addButton ("OK", 1);
        alert->addButton ("Cancel", 0);

        juce::Component::SafePointer<KnobSlider> self (this);
        alert->enterModalState (true, juce::ModalCallbackFunction::create(
            [alert, self] (int result)
            {
                if (result == 1)
                    applyEnteredText (self.getComponent(), alert->getTextEditorContents ("value"));
                alert->setVisible (false);
            }));
    }

private:
    // setValueFromText-style conversion needs the slider's own range mapping,
    // which is protected; a static member can reach it for a same-class pointer.
    static void applyEnteredText (KnobSlider* s, const juce::String& text)
    {
        if (s != nullptr)
            s->setValue (s->getValueFromText (text));
    }
};
