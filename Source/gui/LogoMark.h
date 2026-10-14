#pragma once
#include <JuceHeader.h>

/** The SpiralQuant mark: a glowing wavefunction curling into a logarithmic
    spiral over an amber potential curve. Drawn at any size; reused by the
    editor header and the icon renderer. */
namespace SpiralQuantLogo
{
    inline void draw (juce::Graphics& g, juce::Rectangle<float> area, bool withBackground)
    {
        using namespace juce;
        const float s = jmin (area.getWidth(), area.getHeight());
        const float cx = area.getCentreX();
        const float cy = area.getCentreY();

        if (withBackground)
        {
            g.setColour (Colour (0xff050a12));
            g.fillRoundedRectangle (area, s * 0.18f);
            g.setColour (Colour (0xff00e5ff).withAlpha (0.25f));
            g.drawRoundedRectangle (area.reduced (s * 0.012f), s * 0.18f, s * 0.012f);
        }

        auto disc = area.reduced (s * 0.10f);
        const float halfW = disc.getWidth() * 0.5f;
        const float halfH = disc.getHeight() * 0.5f;
        Path clip;
        clip.addEllipse (-halfW, -halfH, disc.getWidth(), disc.getHeight());
        g.setOrigin (cx, cy);
        g.reduceClipRegion (clip);

        // log spiral r = a * e^(b*theta), one full turn + tail
        Path spiral;
        const float b = 0.155f;
        const float a = s * 0.018f;
        const float rMax = halfW * 1.04f;
        const int steps = 220;
        const float thetaMax = std::log (rMax / a) / b;
        for (int i = 0; i <= steps; ++i)
        {
            const float th = thetaMax * (float) i / (float) steps;
            const float r = a * std::exp (b * th);
            // ripple riding on the spiral: the wavefunction
            const float wob = 1.0f + 0.10f * std::sin (th * 7.0f);
            const float x = std::cos (th) * r * wob;
            const float y = std::sin (th) * r * wob;
            if (i == 0) spiral.startNewSubPath (x, y);
            else        spiral.lineTo (x, y);
        }

        for (int pass = 3; pass >= 1; --pass)
        {
            g.setColour (Colour (0xff00e5ff).withAlpha (0.10f * (4 - pass)));
            g.strokePath (spiral, PathStrokeType (s * 0.020f * (float) pass,
                                                  PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (Colour (0xffd6faff));
        g.strokePath (spiral, PathStrokeType (s * 0.022f, PathStrokeType::curved, PathStrokeType::rounded));

        // amber potential curve crossing the lower third
        Path pot;
        const float py = s * 0.24f;
        for (int i = 0; i <= 60; ++i)
        {
            const float t = (float) i / 60.0f;
            const float x = -disc.getWidth() * 0.55f + t * disc.getWidth() * 1.1f;
            const float y = py + std::sin (t * MathConstants<float>::pi * 2.0f) * s * 0.05f
                               + (t > 0.35f && t < 0.65f ? -s * 0.06f : 0.0f);
            if (i == 0) pot.startNewSubPath (x, y);
            else        pot.lineTo (x, y);
        }
        g.setColour (Colour (0xffffb03b).withAlpha (0.85f));
        g.strokePath (pot, PathStrokeType (s * 0.016f, PathStrokeType::curved, PathStrokeType::rounded));

        // core particle
        g.setColour (Colour (0xff00e5ff));
        g.fillEllipse (-s * 0.035f, -s * 0.035f, s * 0.07f, s * 0.07f);
        g.setColour (Colour (0xffd6faff));
        g.fillEllipse (-s * 0.016f, -s * 0.016f, s * 0.032f, s * 0.032f);
    }
}

class LogoMark : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        SpiralQuantLogo::draw (g, getLocalBounds().toFloat().reduced (1.0f), false);
    }
};
