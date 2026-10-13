#pragma once

#include <JuceHeader.h>

#include "../PluginProcessor.h"
#include "DarkLookAndFeel.h"

/** Two stacked indicators: the oscillator shape the morph knob is pointing at, and
    the lowpass curve it runs into. The response is drawn from the filter's own
    coefficients, so the peak moves with Res and the knee with Cutoff and Slope. */
class ScopeComponent : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    enum Mode { Combined, WaveOnly, FilterOnly };
    ScopeComponent (PluginProcessor& p, Mode mode = Combined) : processor (p), mode (mode)
    {
        if (mode == WaveOnly)
            setTooltip ("One cycle of the oscillator, sine through saw to square.");
        else if (mode == FilterOnly)
            setTooltip ("Resonant lowpass response, 20 Hz to 20 kHz.");
        else
            setTooltip ("Top: one cycle of the oscillator, sine through saw to square.\n"
                        "Bottom: the resonant lowpass response, 20 Hz to 20 kHz.");
    }

    void setScale (float s)
    {
        uiScale = s;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (Theme::panel);
        g.fillRoundedRectangle (area, Theme::corner);
        g.setColour (Theme::panelOutline);
        g.drawRoundedRectangle (area, Theme::corner, 1.0f);

        if (mode == WaveOnly)
            drawWave (g, area.reduced (2.0f));
        else if (mode == FilterOnly)
            drawResponse (g, area.reduced (2.0f));
        else
        {
            const float half = (area.getHeight() - 3.0f * uiScale) * 0.5f;
            drawWave (g, { area.getX() + 2.0f, area.getY() + 2.0f, area.getWidth() - 4.0f, half });
            drawResponse (g, { area.getX() + 2.0f, area.getY() + half + 4.0f * uiScale,
                                area.getWidth() - 4.0f, half });
        }
    }
private:
    void drawWave (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const float morph = processor.getWaveForScope();

        g.setColour (Theme::gridLine);
        g.drawLine (r.getX(), r.getCentreY(), r.getRight(), r.getCentreY(), 0.7f);

        juce::Path path;
        constexpr int samples = 180;
        for (int i = 0; i <= samples; ++i)
        {
            const float t = static_cast<float> (i) / samples;   // phase 0..1
            const float v = shapeAt (morph, t);
            const float x = r.getX() + t * r.getWidth();
            const float y = r.getCentreY() - v * r.getHeight() * 0.42f;
            i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
        }
        g.setColour (Theme::accent);
        g.strokePath (path, juce::PathStrokeType (1.6f));

        g.setColour (Theme::textDim);
        g.setFont (Theme::font (8.5f * uiScale, juce::Font::bold));
        g.drawText (waveName (morph), r.reduced (4.0f * uiScale, 2.0f),
                    juce::Justification::topRight, false);
    }

    void drawResponse (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (Theme::gridLine);
        for (int i = 1; i < 4; ++i)
        {
            const float x = r.getX() + r.getWidth() * static_cast<float> (i) / 4.0f;
            g.drawLine (x, r.getY(), x, r.getBottom(), 0.6f);
        }
        g.drawLine (r.getX(), r.getBottom() - r.getHeight() * 0.5f,
                    r.getRight(), r.getBottom() - r.getHeight() * 0.5f, 0.6f);

        const float cutoff = processor.getCutoffHz();
        const float res = processor.getResonance();
        const float slope = processor.getSlope();

        juce::Path path;
        constexpr int points = 140;
        for (int i = 0; i <= points; ++i)
        {
            const float t = static_cast<float> (i) / points;
            const float freq = 20.0f * std::pow (1000.0f, t);       // 20 Hz .. 20 kHz
            const float db = magnitudeDb (freq, cutoff, res, slope);

            const float x = r.getX() + t * r.getWidth();
            // map +18 dB .. -48 dB onto the box
            const float norm = jlimit (0.0f, 1.0f, (db + 48.0f) / 66.0f);
            const float y = r.getBottom() - norm * r.getHeight();
            i == 0 ? path.startNewSubPath (x, y) : path.lineTo (x, y);
        }
        g.setColour (Theme::accentAlt);
        g.strokePath (path, juce::PathStrokeType (1.5f));

        g.setColour (Theme::textDim);
        g.setFont (Theme::font (8.5f * uiScale, juce::Font::bold));
        g.drawText (juce::String::formatted ("%.0f Hz  Q%.1f  %.0f dB/oct",
                                             cutoff, 0.5 + res * res * 18.0f, 12.0 + 12.0 * slope),
                    r.reduced (4.0f * uiScale, 2.0f), juce::Justification::bottomRight, false);
    }

    /** Same blend the oscillator uses, evaluated over one cycle. */
    static float shapeAt (float morph, float t)
    {
        const float saw = 2.0f * t - 1.0f;
        const float square = t < 0.5f ? 1.0f : -1.0f;
        const float sine = std::sin (juce::MathConstants<float>::twoPi * t);

        if (morph <= 0.5f)
            return (1.0f - morph * 2.0f) * sine + morph * 2.0f * saw;
        const float k = (morph - 0.5f) * 2.0f;
        return (1.0f - k) * saw + k * square;
    }

    static juce::String waveName (float morph)
    {
        if (morph < 0.04f)  return "SINE";
        if (morph > 0.96f)  return "SQUARE";
        if (std::abs (morph - 0.5f) < 0.04f) return "SAW";
        return morph < 0.5f ? "SINE-SAW" : "SAW-SQUARE";
    }

    /** Second-order lowpass magnitude, blended between the 12 and 24 dB sections the
        engine crossfades, with the resonance bump added on top of the knee. */
    static float magnitudeDb (float freq, float cutoff, float res, float slope)
    {
        const float q = 0.5f + res * res * 18.0f;
        const float ratio = freq / jmax (1.0f, cutoff);
        const float r2 = ratio * ratio;

        const float one = 1.0f / std::sqrt ((1.0f - r2) * (1.0f - r2) + (r2 / (q * q)));
        const float two = one * one;                          // cascaded, half the Q
        const float db = 20.0f * std::log10 (jmax (1.0e-5f, one + slope * (two - one)));

        // the engine's second section runs cooler, so the peak never reaches the full Q
        const float bump = res * res * 14.0f
                         * std::exp (-std::log (ratio) * std::log (ratio) / 0.10f);
        return db + bump;
    }

    Mode mode = Combined;
    PluginProcessor& processor;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeComponent)
};
