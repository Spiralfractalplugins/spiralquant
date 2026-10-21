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
        every other rectangle in the window. Chips are pills; the chevron gets a
        hover ring so the dropdown reads as interactive. */
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        using namespace juce;
        const Palette& pal = Skins::current();
        const Rectangle<int> r (0, 0, width, height);
        const float rad = jmin ((float) height, (float) width) * 0.5f;
        const bool over = box.isMouseOver();

        if (! box.isMouseButtonDown())
        {
            g.setColour (Colour (0x2e000000));
            g.fillRoundedRectangle (r.toFloat().translated (0, 1), rad);
        }
        g.setColour (box.isMouseButtonDown() ? pal.panelDeep.darker (0.2f)
                    : over ? pal.panel.brighter (0.10f) : pal.panelDeep);
        g.fillRoundedRectangle (r.toFloat(), rad);
        g.setColour (over ? pal.accent.withAlpha (0.55f) : box.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), rad, 1.0f);
        if (box.hasKeyboardFocus (true))
        {
            g.setColour (pal.accent.withAlpha (0.85f));
            g.drawRoundedRectangle (r.toFloat().reduced (0.5f).expanded (2.0f), rad + 2.0f, 2.0f);
        }

        const Rectangle<int> arrow (width - 24, 0, 18, height);
        Path p;
        p.startNewSubPath ((float) arrow.getX() + 3.0f, (float) arrow.getCentreY() - 2.0f);
        p.lineTo ((float) arrow.getCentreX(), (float) arrow.getCentreY() + 3.0f);
        p.lineTo ((float) arrow.getRight() - 3.0f, (float) arrow.getCentreY() - 2.0f);
        g.setColour (box.findColour (ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 0.9f : 0.2f));
        g.strokePath (p, PathStrokeType (1.8f));
    }

    /** Shared pill renderer: fully rounded chip, soft drop shadow, subtle
        vertical gradient, LED dot on the left that lights with the accent
        colour when the state is on, and a hover/press outline language. */
    void drawPill (juce::Graphics& g, juce::Rectangle<float> r, const Palette& pal,
                   bool on, bool over, bool down, juce::Colour tint, bool showLed,
                   float fontHeight, const juce::String& text, bool bold)
    {
        using namespace juce;
        const float hgt = r.getHeight();
        // Guard against chips narrower than they are tall: the cap keeps the
        // pill from turning into an egg on compact layouts.
        const float rad = jmin (hgt, r.getWidth()) * 0.5f;

        if (! down)
        {
            g.setColour (Colour (0x2e000000));
            g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
        }

        // Body: vertical gradient — tinted when on, deep panel when off.
        ColourGradient body (on ? tint.withAlpha (0.34f)
                                    .overlaidWith (pal.panelDeep)
                              : (down ? pal.panelDeep.darker (0.25f) : pal.panelDeep),
                             r.getX(), r.getY(),
                             down ? pal.panelDeep : pal.panel,
                             r.getX(), r.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (r, rad);

        // Outline: accent when on, brighter on hover, quiet otherwise.
        g.setColour (on ? tint.withAlpha (over ? 1.0f : 0.8f)
                        : over ? pal.accent.withAlpha (0.55f) : pal.panelOutline);
        g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.1f);
        if (on && over)
        {
            g.setColour (tint.withAlpha (0.25f));
            g.drawRoundedRectangle (r.expanded (1.5f).reduced (0.5f), rad + 1.5f, 1.5f);
        }

        auto textR = r;
        if (showLed)
        {
            const float ledR = jlimit (2.2f, 3.4f, hgt * 0.13f);
            const float ledX = r.getX() + hgt * 0.48f;
            const float ledY = r.getCentreY();
            if (on)
            {
                g.setColour (tint.withAlpha (0.30f));
                g.fillEllipse (ledX - ledR * 2.4f, ledY - ledR * 2.4f, ledR * 4.8f, ledR * 4.8f);
            }
            g.setColour (on ? tint.brighter (0.35f) : pal.textDim.withAlpha (0.35f));
            g.fillEllipse (ledX - ledR, ledY - ledR, ledR * 2.0f, ledR * 2.0f);
            if (on)
            {
                g.setColour (Colours::white.withAlpha (0.75f));
                g.fillEllipse (ledX - ledR * 0.4f, ledY - ledR * 0.4f, ledR * 0.8f, ledR * 0.8f);
            }
            textR = textR.withTrimmedLeft (hgt * 0.95f);
        }

        if (text.isNotEmpty())
        {
            textR = textR.withTrimmedRight (hgt * 0.28f);
            g.setColour (on ? Colours::white.overlaidWith (pal.text.withAlpha (0.35f)).brighter (0.15f)
                            : pal.textDim);
            g.setFont (Skins::font (fontHeight, bold ? Font::bold : Font::plain));
            g.drawFittedText (text, textR.toNearestInt(), Justification::centred, 1);
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isMouseOver, bool isButtonDown) override
    {
        const Palette& pal = Skins::current();
        const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();
        const juce::Colour tint = button.findColour (juce::TextButton::buttonOnColourId);
        const bool showLed = on; // cycle chips and preset buttons light up while active
        drawPill (g, r, pal, on, isMouseOver, isButtonDown, tint.withAlpha (1.0f), showLed,
                  Skins::fontButton * uiScale, button.getButtonText(), on);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        using namespace juce;
        const Palette& pal = Skins::current();
        const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = button.getToggleState();
        const Colour tint = button.findColour (ToggleButton::tickColourId);
        const bool over = button.isMouseOver();
        const bool down = button.isMouseButtonDown();

        drawPill (g, r, pal, on, over, down, tint, true,
                  Skins::fontButton * uiScale, button.getButtonText(), on);

        if (button.hasKeyboardFocus (true))
        {
            g.setColour (pal.accent.withAlpha (0.85f));
            g.drawRoundedRectangle (r.expanded (2.0f), r.getHeight() * 0.5f + 2.0f, 2.0f);
        }
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
        const float outer  = jmin ((float) width, (float) height) * 0.5f - 1.0f;
        const float trackR = outer - 7.5f;
        const float bodyR  = trackR - 4.5f;
        const float norm   = jlimit (0.0f, 1.0f, sliderPos);
        const float sweep  = rotaryEndAngle - rotaryStartAngle;
        const float angle  = rotaryStartAngle + norm * sweep - MathConstants<float>::halfPi;
        const Colour fill  = slider.findColour (Slider::rotarySliderFillColourId);

        if (slider.hasKeyboardFocus (true) || slider.isMouseOverOrDragging())
        {
            g.setColour (pal.accent.withAlpha (slider.isMouseButtonDown() ? 0.28f : 0.16f));
            g.fillEllipse (cx - outer - 3.0f, cy - outer - 3.0f, (outer + 3.0f) * 2.0f, (outer + 3.0f) * 2.0f);
        }

        // All sliders share the same start/sweep angles, so the tick unit vectors
        // are memoised instead of 42 trig calls per knob per frame.
        constexpr int numTicks = 21;
        struct TickTable { float a0 = 0.0f, sw = 0.0f; float ux[numTicks], uy[numTicks]; bool valid = false; };
        static thread_local TickTable ticks;
        if (! ticks.valid || ticks.a0 != rotaryStartAngle || ticks.sw != sweep)
        {
            ticks.a0 = rotaryStartAngle; ticks.sw = sweep; ticks.valid = true;
            for (int i = 0; i < numTicks; ++i)
            {
                const float t = rotaryStartAngle + sweep * (float) i / (float) (numTicks - 1)
                                - MathConstants<float>::halfPi;
                ticks.ux[i] = std::cos (t);
                ticks.uy[i] = std::sin (t);
            }
        }

        // Tick ring: major marks at 0/25/50/75/100%, lit ticks follow the value.
        for (int i = 0; i < numTicks; ++i)
        {
            const bool major = i % 5 == 0;
            const float r0 = trackR + 2.5f;
            const float r1 = major ? outer : outer - 2.5f;
            g.setColour ((float) i / (float) (numTicks - 1) <= norm + 0.001f
                             ? fill.withAlpha (major ? 0.95f : 0.6f)
                             : pal.textDim.withAlpha (major ? 0.55f : 0.3f));
            g.drawLine (cx + ticks.ux[i] * r0, cy + ticks.uy[i] * r0,
                        cx + ticks.ux[i] * r1, cy + ticks.uy[i] * r1, major ? 1.5f : 1.0f);
        }

        // Track: a dark groove under the plain track arc reads as depth.
        // Path arcs measure angles from 12 o'clock, so they take the raw slider
        // angles, unlike the cos/sin maths below.
        Path groove;
        groove.addCentredArc (cx, cy, trackR, trackR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0x59000000));
        g.strokePath (groove, PathStrokeType (5.5f, PathStrokeType::curved, PathStrokeType::rounded));

        g.setColour (pal.knobTrack);
        g.strokePath (groove, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        if (norm > 0.002f)
        {
            Path arc;
            arc.addCentredArc (cx, cy, trackR, trackR, 0.0f, rotaryStartAngle, rotaryStartAngle + norm * sweep, true);
            g.setColour (fill.withAlpha (0.28f));
            g.strokePath (arc, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (fill);
            g.strokePath (arc, PathStrokeType (2.8f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Knob body: drop shadow, diagonal gradient with a lit mid-band, double rim.
        g.setColour (juce::Colour (0x70000000));
        g.fillEllipse (cx - bodyR + 1.5f, cy - bodyR + 3.0f, bodyR * 2.0f, bodyR * 2.0f);

        ColourGradient body (pal.knobHi, cx - bodyR, cy - bodyR, pal.knobLo, cx + bodyR, cy + bodyR, false);
        g.setGradientFill (body);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        // Lit upper-left rim + inner ring.
        Path rimLight;
        rimLight.addCentredArc (cx, cy, bodyR - 1.0f, bodyR - 1.0f, 0.0f,
                                MathConstants<float>::pi * 1.05f, MathConstants<float>::pi * 1.75f, true);
        g.setColour (juce::Colour (0x66ffffff));
        g.strokePath (rimLight, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (fill.withAlpha (0.5f));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.2f);

        const float c = std::cos (angle), s = std::sin (angle);

        // Grip marks opposite the pointer, so the knob reads as a turned object.
        for (float da : { MathConstants<float>::twoPi / 3.0f, -MathConstants<float>::twoPi / 3.0f })
        {
            const float gc = std::cos (angle + da), gs = std::sin (angle + da);
            g.setColour (juce::Colour (0x60000000));
            g.fillEllipse (cx + gc * bodyR * 0.72f - 1.7f, cy + gs * bodyR * 0.72f - 1.7f, 3.4f, 3.4f);
        }

        // Inner well + pointer
        g.setColour (juce::Colour (0x30000000));
        g.fillEllipse (cx - bodyR * 0.42f, cy - bodyR * 0.42f, bodyR * 0.84f, bodyR * 0.84f);
        g.setColour (pal.text);
        g.drawLine (cx + c * bodyR * 0.30f, cy + s * bodyR * 0.30f,
                    cx + c * bodyR * 0.92f, cy + s * bodyR * 0.92f, 2.4f);
        g.setColour (pal.text.brighter (0.25f));
        g.drawLine (cx + c * bodyR * 0.30f, cy + s * bodyR * 0.30f,
                    cx + c * bodyR * 0.92f, cy + s * bodyR * 0.92f, 1.0f);

        // Value dot riding on the arc + accent hub
        g.setColour (fill);
        g.fillEllipse (cx + c * trackR - 2.6f, cy + s * trackR - 2.6f, 5.2f, 5.2f);
        g.setColour (pal.text);
        g.fillEllipse (cx - 1.6f, cy - 1.6f, 3.2f, 3.2f);
    }
};
