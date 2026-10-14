#pragma once
#include <JuceHeader.h>

namespace SciFiTheme
{
    inline const juce::Colour background { 0xff050a12 };
    inline const juce::Colour panel      { 0xff0a1624 };
    inline const juce::Colour panelDeep  { 0xff07101a };
    inline const juce::Colour stroke     { 0xff1a3a5a };
    inline const juce::Colour strokeDim  { 0xff10253a };
    inline const juce::Colour text       { 0xffbfe9ff };
    inline const juce::Colour textDim    { 0xff5a7a99 };
    inline const juce::Colour neon      { 0xff00e5ff };
    inline const juce::Colour holo       { 0xff4aa8ff };
    inline const juce::Colour amber      { 0xffffb03b };
    inline const juce::Colour grid       { 0xff0f2338 };
    inline constexpr float corner        = 6.0f;
}

class SciFiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SciFiLookAndFeel()
    {
        setColour (juce::Slider::backgroundColourId, SciFiTheme::panel);
        setColour (juce::Slider::trackColourId, SciFiTheme::strokeDim);
        setColour (juce::Slider::rotarySliderFillColourId, SciFiTheme::neon);
        setColour (juce::Slider::rotarySliderOutlineColourId, SciFiTheme::stroke);
        setColour (juce::Slider::thumbColourId, SciFiTheme::text);
        setColour (juce::Label::textColourId, SciFiTheme::text);
        setColour (juce::TextButton::buttonColourId, SciFiTheme::panel);
        setColour (juce::TextButton::textColourOffId, SciFiTheme::text);
        setColour (juce::TextButton::textColourOnId, SciFiTheme::neon);
        setColour (juce::ToggleButton::textColourId, SciFiTheme::text);
        setColour (juce::ComboBox::backgroundColourId, SciFiTheme::panel);
        setColour (juce::ComboBox::textColourId, SciFiTheme::text);
        setColour (juce::ComboBox::outlineColourId, SciFiTheme::stroke);
        setColour (juce::PopupMenu::backgroundColourId, SciFiTheme::panel);
        setColour (juce::PopupMenu::textColourId, SciFiTheme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, SciFiTheme::neon.withAlpha (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId, SciFiTheme::text);
        setColour (juce::Slider::textBoxTextColourId, SciFiTheme::text);
        setColour (juce::Slider::textBoxBackgroundColourId, SciFiTheme::panelDeep);
        setColour (juce::Slider::textBoxOutlineColourId, SciFiTheme::strokeDim);
        setColour (juce::TextButton::buttonOnColourId, SciFiTheme::neon.withAlpha (0.16f));
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
                g.setColour (SciFiTheme::neon.withAlpha (0.9f));
                g.drawRoundedRectangle (3.5f, 3.5f, (float) w - 7.0f, (float) h - 7.0f,
                                        SciFiTheme::corner + 3.0f, 2.0f);
            }
        };
        return std::make_unique<juce::FocusOutline> (std::make_unique<Props>());
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isMouseOver, bool isButtonDown) override
    {
        const auto r = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = button.getToggleState();
        g.setColour (isButtonDown ? SciFiTheme::panelDeep
                    : isMouseOver ? SciFiTheme::panel.brighter (0.25f)
                    : on ? SciFiTheme::neon.withAlpha (0.16f) : SciFiTheme::panelDeep);
        g.fillRoundedRectangle (r, SciFiTheme::corner);
        g.setColour (on || isMouseOver ? SciFiTheme::neon.withAlpha (on ? 1.0f : 0.45f) : SciFiTheme::strokeDim);
        g.drawRoundedRectangle (r, SciFiTheme::corner, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (juce::FontOptions (12.0f));
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        using namespace juce;
        const Rectangle<int> r (0, 0, width, height);
        g.setColour (box.findColour (ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r.toFloat(), SciFiTheme::corner);
        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), SciFiTheme::corner, 1.0f);
        if (box.hasKeyboardFocus (true))
        {
            g.setColour (SciFiTheme::neon.withAlpha (0.85f));
            g.drawRoundedRectangle (r.toFloat().reduced (0.5f).expanded (2.0f),
                                    SciFiTheme::corner + 2.0f, 2.0f);
        }

        const Rectangle<int> arrow (width - 26, 0, 20, height);
        juce::Path p;
        p.startNewSubPath ((float) arrow.getX() + 3.0f, (float) arrow.getCentreY() - 2.0f);
        p.lineTo ((float) arrow.getCentreX(), (float) arrow.getCentreY() + 3.0f);
        p.lineTo ((float) arrow.getRight() - 3.0f, (float) arrow.getCentreY() - 2.0f);
        g.setColour (SciFiTheme::neon.withAlpha (0.85f));
        g.strokePath (p, PathStrokeType (1.6f));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        using namespace juce;
        const auto r = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = button.getToggleState();
        g.setColour (on ? SciFiTheme::neon.withAlpha (0.16f) : SciFiTheme::panelDeep);
        g.fillRoundedRectangle (r, SciFiTheme::corner);
        g.setColour (on ? SciFiTheme::neon : SciFiTheme::stroke);
        g.drawRoundedRectangle (r, SciFiTheme::corner, 1.0f);
        if (button.hasKeyboardFocus (true))
        {
            g.setColour (SciFiTheme::neon.withAlpha (0.85f));
            g.drawRoundedRectangle (r.expanded (2.0f), SciFiTheme::corner + 2.0f, 2.0f);
        }
        g.setColour (on ? SciFiTheme::neon : SciFiTheme::text);
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (6, 0),
                          Justification::centred, 1);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        using namespace juce;
        if (slider.hasKeyboardFocus (true))
        {
            g.setColour (SciFiTheme::neon.withAlpha (0.85f));
            g.drawRoundedRectangle (Rectangle<float> (x, y, width, height).reduced (1.0f),
                                    SciFiTheme::corner + 4.0f, 2.0f);
        }
        const float cx = (float) x + (float) width * 0.5f;
        const float cy = (float) y + (float) height * 0.5f;
        const float outer = juce::jmin ((float) width, (float) height) * 0.5f - 2.0f;
        const float trackR = outer - 5.0f;
        const float bodyR  = trackR - 4.0f;
        const float norm   = juce::jlimit (0.0f, 1.0f, sliderPos);
        if (slider.isMouseOver())
        {
            g.setColour (SciFiTheme::neon.withAlpha (0.06f));
            g.fillEllipse (cx - outer - 2.0f, cy - outer - 2.0f, (outer + 2.0f) * 2.0f, (outer + 2.0f) * 2.0f);
        }
        const float angle  = rotaryStartAngle + norm * (rotaryEndAngle - rotaryStartAngle)
                             - MathConstants<float>::halfPi;
        const Colour fill  = SciFiTheme::neon;

        // ticks
        for (int i = 0; i <= 10; ++i)
        {
            const float t = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle)
                            * (float) i / 10.0f - MathConstants<float>::halfPi;
            const bool major = (i == 0 || i == 5 || i == 10);
            const float r0 = trackR + 1.0f;
            const float r1 = major ? outer : outer - 1.5f;
            g.setColour (( (float) i / 10.0f) <= norm + 0.001f
                             ? fill.withAlpha (0.85f) : SciFiTheme::textDim.withAlpha (0.45f));
            g.drawLine (cx + std::cos (t) * r0, cy + std::sin (t) * r0,
                        cx + std::cos (t) * r1, cy + std::sin (t) * r1, major ? 1.3f : 0.8f);
        }

        // glow + value arc
        const float sweep = rotaryEndAngle - rotaryStartAngle;
        const float valEnd = rotaryStartAngle + norm * sweep;
        for (int i = 3; i >= 1; --i)
        {
            Path glow;
            glow.addCentredArc (cx, cy, trackR, trackR, 0.0f, rotaryStartAngle, valEnd, true);
            g.setColour (fill.withAlpha (0.05f * (float) i));
            g.strokePath (glow, PathStrokeType (3.0f + (float) i * 2.0f,
                                                 PathStrokeType::curved, PathStrokeType::rounded));
        }

        // full track
        Path track;
        track.addCentredArc (cx, cy, trackR, trackR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (SciFiTheme::stroke);
        g.strokePath (track, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));

        // value arc
        if (norm > 0.002f)
        {
            Path arc;
            arc.addCentredArc (cx, cy, trackR, trackR, 0.0f, rotaryStartAngle, valEnd, true);
            g.setColour (fill);
            g.strokePath (arc, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // body
        g.setGradientFill (ColourGradient (Colour (0xff12263a), cx - bodyR, cy - bodyR,
                                          Colour (0xff06101a), cx + bodyR, cy + bodyR, false));
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (fill.withAlpha (0.35f));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        // indicator
        const float angleRad = angle;
        g.setColour (SciFiTheme::text);
        g.drawLine (cx + std::cos (angleRad) * bodyR * 0.55f,
                    cy + std::sin (angleRad) * bodyR * 0.55f,
                    cx + std::cos (angleRad) * bodyR * 0.92f,
                    cy + std::sin (angleRad) * bodyR * 0.92f, 2.0f);
        g.setColour (fill);
        g.fillEllipse (cx + std::cos (angleRad) * trackR - 2.2f,
                       cy + std::sin (angleRad) * trackR - 2.2f, 4.4f, 4.4f);

        (void) slider;
    }
};
