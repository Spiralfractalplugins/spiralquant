#pragma once

#include <JuceHeader.h>

namespace Theme
{
    inline const juce::Colour background   { 0xff181b20 };
    inline const juce::Colour panel        { 0xff22262d };
    inline const juce::Colour panelOutline { 0xff333a44 };
    inline const juce::Colour text         { 0xffc8d0dc };
    inline const juce::Colour textDim      { 0xff7a8492 };
    inline const juce::Colour accent       { 0xffe8a03c }; // 303 amber
    inline const juce::Colour accentAlt    { 0xff4ac8e8 }; // cyan
    inline const juce::Colour pitch        { 0xffb06ae8 }; // violet
    inline const juce::Colour gate         { 0xffe8564a }; // gate red
    inline const juce::Colour gridLine     { 0xff2c313a };
    inline const juce::Colour gridBeat     { 0xff3d4451 };
    inline const juce::Colour knobTrack    { 0xff3a4150 };
    inline const juce::Colour playhead     { 0xfff2f5f9 };

    /** One radius for every rectangle in the UI: panels, grids, buttons, combo boxes. */
    inline constexpr float corner = 6.0f;

    /** JUCE 8 deprecated Font(height, styleFlags), so all UI text goes through here. */
    inline juce::Font font (float height, int style = juce::Font::plain)
    {
        return juce::Font (juce::FontOptions (height, style));
    }
}

class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    float uiScale = 1.0f;

    DarkLookAndFeel()
    {
        setColour (juce::Slider::backgroundColourId, Theme::panel);
        setColour (juce::Slider::trackColourId, Theme::knobTrack);
        setColour (juce::Slider::rotarySliderFillColourId, Theme::accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, Theme::knobTrack);
        setColour (juce::Slider::thumbColourId, Theme::text);
        setColour (juce::Label::textColourId, Theme::text);
        setColour (juce::TextButton::buttonColourId, Theme::panel);
        setColour (juce::TextButton::textColourOffId, Theme::text);
        setColour (juce::TextButton::textColourOnId, Theme::accent);
        setColour (juce::ToggleButton::textColourId, Theme::text);
        setColour (juce::PopupMenu::backgroundColourId, Theme::panel);
        setColour (juce::PopupMenu::textColourId, Theme::text);
        setColour (juce::PopupMenu::highlightedTextColourId, Theme::background);
    }

    juce::Font getTextButtonFont (juce::TextButton& button, int buttonHeight) override
    {
        return juce::LookAndFeel_V4::getTextButtonFont (button, buttonHeight).withHeight (13.0f * uiScale);
    }

    /** V4 hard-codes a 3 px corner here, which reads as a different shape next to every
        other rectangle in the window. */
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        using namespace juce;
        const Rectangle<int> boxBounds (0, 0, width, height);

        g.setColour (box.findColour (ComboBox::backgroundColourId));
        g.fillRoundedRectangle (boxBounds.toFloat(), Theme::corner);

        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), Theme::corner, 1.0f);

        const Rectangle<int> arrowZone (width - 30, 0, 20, height);
        Path path;
        path.startNewSubPath (static_cast<float> (arrowZone.getX()) + 3.0f,
                              static_cast<float> (arrowZone.getCentreY()) - 2.0f);
        path.lineTo (static_cast<float> (arrowZone.getCentreX()),
                     static_cast<float> (arrowZone.getCentreY()) + 3.0f);
        path.lineTo (static_cast<float> (arrowZone.getRight()) - 3.0f,
                     static_cast<float> (arrowZone.getCentreY()) - 2.0f);

        g.setColour (box.findColour (ComboBox::arrowColourId)
                        .withAlpha (box.isEnabled() ? 0.9f : 0.2f));
        g.strokePath (path, PathStrokeType (2.0f));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        using namespace juce;
        const float cx = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
        const float cy = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
        const float outer = jmin (static_cast<float> (width), static_cast<float> (height)) * 0.5f - 1.0f;
        const float trackR = outer - 6.0f;
        const float bodyR  = trackR - 3.5f;
        const float norm   = static_cast<float> (juce::jlimit (0.0, 1.0, static_cast<double> (sliderPos)));
        const float angle  = rotaryStartAngle + norm * (rotaryEndAngle - rotaryStartAngle) - MathConstants<float>::halfPi;
        const Colour fill = slider.findColour (Slider::rotarySliderFillColourId);

        // tick marks
        for (int i = 0; i <= 10; ++i)
        {
            const float t = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * static_cast<float> (i) / 10.0f
                            - MathConstants<float>::halfPi;
            const bool major = i == 0 || i == 5 || i == 10;
            const float r0 = trackR + 2.0f;
            const float r1 = major ? outer : outer - 2.0f;
            g.setColour (static_cast<float> (i) / 10.0f <= norm + 0.001f
                             ? fill.withAlpha (0.9f) : Theme::textDim.withAlpha (0.45f));
            g.drawLine (cx + std::cos (t) * r0, cy + std::sin (t) * r0,
                        cx + std::cos (t) * r1, cy + std::sin (t) * r1, major ? 1.4f : 1.0f);
        }

        // Track and value arcs. Path arcs measure angles from 12 o'clock,
        // so they take the raw slider angles, unlike the cos/sin maths below.
        const float sweep = rotaryEndAngle - rotaryStartAngle;
        Path track;
        track.addCentredArc (cx, cy, trackR, trackR, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (Theme::knobTrack);
        g.strokePath (track, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        if (norm > 0.002f)
        {
            Path arc;
            arc.addCentredArc (cx, cy, trackR, trackR, 0.0f,
                               rotaryStartAngle, rotaryStartAngle + norm * sweep, true);
            g.setColour (fill);
            g.strokePath (arc, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // knob body
        g.setGradientFill (ColourGradient (Colour (0xff464e5c), cx - bodyR, cy - bodyR,
                                           Colour (0xff1b1e24), cx + bodyR, cy + bodyR, false));
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (fill.withAlpha (0.5f));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        // short indicator near the rim
        g.setColour (Theme::text);
        g.drawLine (cx + std::cos (angle) * bodyR * 0.55f, cy + std::sin (angle) * bodyR * 0.55f,
                    cx + std::cos (angle) * bodyR * 0.90f, cy + std::sin (angle) * bodyR * 0.90f, 2.5f);

        // dot at the arc end
        g.setColour (fill);
        g.fillEllipse (cx + std::cos (angle) * trackR - 2.4f,
                       cy + std::sin (angle) * trackR - 2.4f, 4.8f, 4.8f);
    }
};
