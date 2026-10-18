#pragma once

#include <JuceHeader.h>
#include "Palette.h"

/** Skin-aware look and feel: every draw method reads Skins::current() at paint
    time, so switching skins only needs a repaint, not reconstruction. */
class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    float uiScale = 1.0f;

    DarkLookAndFeel() { refreshColours(); }

    void refreshColours()
    {
        const Palette& pal = Skins::current();
        setColour (juce::Slider::backgroundColourId, pal.panel);
        setColour (juce::Slider::trackColourId, pal.knobTrack);
        setColour (juce::Slider::rotarySliderOutlineColourId, pal.knobTrack);
        setColour (juce::Slider::thumbColourId, pal.text);
        setColour (juce::Label::textColourId, pal.text);
        setColour (juce::TextButton::buttonColourId, pal.panelDeep);
        setColour (juce::TextButton::textColourOffId, pal.text);
        setColour (juce::TextButton::textColourOnId, pal.accent);
        setColour (juce::ToggleButton::textColourId, pal.text);
        setColour (juce::ComboBox::backgroundColourId, pal.panelDeep);
        setColour (juce::ComboBox::textColourId, pal.text);
        setColour (juce::ComboBox::outlineColourId, pal.panelOutline);
        setColour (juce::ComboBox::arrowColourId, pal.accent);
        setColour (juce::PopupMenu::backgroundColourId, pal.panel);
        setColour (juce::PopupMenu::textColourId, pal.text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, pal.accent.withAlpha (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId, pal.text);
        setColour (juce::TextEditor::backgroundColourId, pal.panelDeep);
        setColour (juce::TextEditor::textColourId, pal.text);
        setColour (juce::TextEditor::highlightColourId, pal.accent.withAlpha (0.25f));
    }

    juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override
    {
        return juce::LookAndFeel_V4::getTextButtonFont (button, buttonHeight).withHeight (Skins::fontButton * uiScale);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return Skins::font (Skins::fontBody * uiScale); }

    /** V4 hard-codes a 3 px corner here, which reads as a different shape next to
        every other rectangle in the window. */
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        using namespace juce;
        const Palette& pal = Skins::current();
        const Rectangle<int> r (0, 0, width, height);

        g.setColour (box.findColour (ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r.toFloat(), pal.corner);
        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), pal.corner, 1.0f);
        if (box.hasKeyboardFocus (true))
        {
            g.setColour (pal.accent.withAlpha (0.85f));
            g.drawRoundedRectangle (r.toFloat().reduced (0.5f).expanded (2.0f), pal.corner + 2.0f, 2.0f);
        }

        const Rectangle<int> arrow (width - 24, 0, 18, height);
        Path p;
        p.startNewSubPath ((float) arrow.getX() + 3.0f, (float) arrow.getCentreY() - 2.0f);
        p.lineTo ((float) arrow.getCentreX(), (float) arrow.getCentreY() + 3.0f);
        p.lineTo ((float) arrow.getRight() - 3.0f, (float) arrow.getCentreY() - 2.0f);
        g.setColour (box.findColour (ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 0.9f : 0.2f));
        g.strokePath (p, PathStrokeType (1.8f));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isMouseOver, bool isButtonDown) override
    {
        const Palette& pal = Skins::current();
        const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();
        const juce::Colour tint = button.findColour (juce::TextButton::buttonOnColourId);
        g.setColour (isButtonDown ? pal.panelDeep.darker (0.2f)
                    : on ? tint
                    : isMouseOver ? pal.panel.brighter (0.18f) : pal.panelDeep);
        g.fillRoundedRectangle (r, pal.corner);
        g.setColour (on || isMouseOver ? pal.accent.withAlpha (on ? 0.9f : 0.45f) : pal.panelOutline);
        g.drawRoundedRectangle (r, pal.corner, 1.0f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        using namespace juce;
        const Palette& pal = Skins::current();
        const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();
        const Colour tint = button.findColour (ToggleButton::tickColourId);
        g.setColour (on ? tint.withAlpha (0.28f) : pal.panelDeep);
        g.fillRoundedRectangle (r, pal.corner);
        g.setColour (on ? tint : pal.panelOutline);
        g.drawRoundedRectangle (r, pal.corner, 1.0f);
        if (button.hasKeyboardFocus (true))
        {
            g.setColour (pal.accent.withAlpha (0.85f));
            g.drawRoundedRectangle (r.expanded (2.0f), pal.corner + 2.0f, 2.0f);
        }
        g.setColour (on ? pal.text : pal.textDim);
        g.setFont (Skins::font (Skins::fontButton * uiScale, on ? Font::bold : Font::plain));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (6, 0),
                          Justification::centred, 1);
    }

    std::unique_ptr<juce::FocusOutline> createFocusOutlineForComponent (juce::Component&) override
    {
        struct Props : juce::FocusOutline::OutlineWindowProperties
        {
            juce::Rectangle<int> getOutlineBounds (juce::Component& focused) override
            {
                return focused.getScreenBounds().expanded (3);
            }
            void drawOutline (juce::Graphics& g, int w, int h) override
            {
                const Palette& pal = Skins::current();
                g.setColour (pal.accent.withAlpha (0.9f));
                g.drawRoundedRectangle (3.5f, 3.5f, (float) w - 7.0f, (float) h - 7.0f,
                                        pal.corner + 3.0f, 2.0f);
            }
        };
        return std::make_unique<juce::FocusOutline> (std::make_unique<Props> ());
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        using namespace juce;
        const Palette& pal = Skins::current();
        const float cx = (float) x + (float) width * 0.5f;
        const float cy = (float) y + (float) height * 0.5f;
        const float outer = jmin ((float) width, (float) height) * 0.5f - 1.0f;
        const float trackR = outer - 6.0f;
        const float bodyR  = trackR - 3.5f;
        const float norm   = jlimit (0.0f, 1.0f, sliderPos);
        const float sweep  = rotaryEndAngle - rotaryStartAngle;
        const float angle  = rotaryStartAngle + norm * sweep - MathConstants<float>::halfPi;
        const Colour fill  = slider.findColour (Slider::rotarySliderFillColourId);

        if (slider.hasKeyboardFocus (true))
        {
            g.setColour (pal.accent.withAlpha (0.22f));
            g.fillEllipse (cx - outer - 3.0f, cy - outer - 3.0f, (outer + 3.0f) * 2.0f, (outer + 3.0f) * 2.0f);
        }

        // All sliders share the same start/sweep angles, so the 11 tick unit
        // vectors are memoised instead of 22 trig calls per knob per frame.
        struct TickTable { float a0 = 0.0f, sw = 0.0f; float ux[11], uy[11]; bool valid = false; };
        static thread_local TickTable ticks;
        if (! ticks.valid || ticks.a0 != rotaryStartAngle || ticks.sw != sweep)
        {
            ticks.a0 = rotaryStartAngle; ticks.sw = sweep; ticks.valid = true;
            for (int i = 0; i <= 10; ++i)
            {
                const float t = rotaryStartAngle + sweep * (float) i / 10.0f - MathConstants<float>::halfPi;
                ticks.ux[i] = std::cos (t);
                ticks.uy[i] = std::sin (t);
            }
        }

        for (int i = 0; i <= 10; ++i)
        {
            const bool major = i == 0 || i == 5 || i == 10;
            const float r0 = trackR + 2.0f;
            const float r1 = major ? outer : outer - 2.0f;
            g.setColour ((float) i / 10.0f <= norm + 0.001f
                             ? fill.withAlpha (0.9f) : pal.textDim.withAlpha (0.45f));
            g.drawLine (cx + ticks.ux[i] * r0, cy + ticks.uy[i] * r0,
                        cx + ticks.ux[i] * r1, cy + ticks.uy[i] * r1, major ? 1.4f : 1.0f);
        }

        // Track and value arcs. Path arcs measure angles from 12 o'clock,
        // so they take the raw slider angles, unlike the cos/sin maths below.
        Path track;
        track.addCentredArc (cx, cy, trackR, trackR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (pal.knobTrack);
        g.strokePath (track, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        if (norm > 0.002f)
        {
            Path arc;
            arc.addCentredArc (cx, cy, trackR, trackR, 0.0f, rotaryStartAngle, rotaryStartAngle + norm * sweep, true);
            g.setColour (fill);
            g.strokePath (arc, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Knob body: fake drop shadow, then lit-corner to shadowed-corner gradient.
        g.setColour (juce::Colour (0x60000000));
        g.fillEllipse (cx - bodyR + 1.5f, cy - bodyR + 2.5f, bodyR * 2.0f, bodyR * 2.0f);
        g.setGradientFill (ColourGradient (pal.knobHi, cx - bodyR, cy - bodyR,
                                          pal.knobLo, cx + bodyR, cy + bodyR, false));
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (fill.withAlpha (0.45f));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        const float c = std::cos (angle), s = std::sin (angle);
        g.setColour (pal.text);
        g.drawLine (cx + c * bodyR * 0.55f, cy + s * bodyR * 0.55f,
                    cx + c * bodyR * 0.90f, cy + s * bodyR * 0.90f, 2.5f);

        g.setColour (fill);
        g.fillEllipse (cx + c * trackR - 2.4f, cy + s * trackR - 2.4f, 4.8f, 4.8f);
    }
};
