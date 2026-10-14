#include "AdsrCanvas.h"
#include "SciFiLookAndFeel.h"

void AdsrCanvas::setRaw (const char* id, float raw, float min, float max)
{
    const float norm = (raw - min) / (max - min);
    if (auto* p = apvts().getParameter (id))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
}

void AdsrCanvas::updateHandles (float w, float h)
{
    const float attackNorm = (getRaw ("attack") - 1.0f) / 1999.0f;
    const float decayNorm  = (getRaw ("decay")  - 1.0f) / 1999.0f;
    const float sustainLevel = getRaw ("sustain");
    const float releaseNorm = (getRaw ("release") - 1.0f) / 4999.0f;

    const float attackX = 0.16f * w + attackNorm * 0.16f * w;
    const float decayX  = attackX + 0.10f * w + decayNorm * 0.20f * w;
    const float sustainY = h - sustainLevel * (h * 0.72f) - h * 0.12f;
    const float releaseX = decayX + 0.12f * w + releaseNorm * 0.30f * w;

    attackH  = { attackX, h * 0.12f };
    decayH   = { decayX, sustainY };
    sustainH = { (decayX + releaseX) * 0.5f, sustainY };
    releaseH = { releaseX, h - h * 0.10f };
}

void AdsrCanvas::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    updateHandles (w, h);

    g.fillAll (SciFiTheme::panelDeep);
    g.setColour (SciFiTheme::strokeDim);
    for (int i = 1; i < 5; ++i)
    {
        const float x = w * (float) i / 5.0f;
        g.drawVerticalLine ((int) x, 0, (int) h);
    }
    g.drawHorizontalLine ((int) (h * 0.5f), 0, (int) w);

    juce::Path env;
    env.startNewSubPath (w * 0.05f, h - h * 0.08f);
    env.lineTo (attackH.x, h * 0.12f);
    env.lineTo (decayH.x, decayH.y);
    env.lineTo (releaseH.x, decayH.y);
    env.lineTo (releaseH.x + (w - releaseH.x) * 0.35f, h - h * 0.08f);

    g.setColour (SciFiTheme::neon.withAlpha (0.2f));
    g.strokePath (env, juce::PathStrokeType (5.0f));
    g.setColour (SciFiTheme::neon);
    g.strokePath (env, juce::PathStrokeType (1.8f));

    auto drawHandle = [&] (juce::Point<float> p, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.25f));
        g.fillEllipse (p.x - 9, p.y - 9, 18, 18);
        g.setColour (c);
        g.fillEllipse (p.x - 4, p.y - 4, 8, 8);
        g.setColour (SciFiTheme::text);
        g.drawEllipse (p.x - 4, p.y - 4, 8, 8, 1.0f);
    };
    drawHandle (attackH, SciFiTheme::neon);
    drawHandle (decayH, SciFiTheme::holo);
    drawHandle (sustainH, SciFiTheme::holo);
    drawHandle (releaseH, SciFiTheme::amber);

    g.setColour (SciFiTheme::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("ADSR: drag handles (attack / decay / sustain / release)", getLocalBounds().reduced (8, 4).removeFromTop (14),
                juce::Justification::centredLeft, false);
}

void AdsrCanvas::mouseMove (const juce::MouseEvent& e)
{
    updateHandles ((float) getWidth(), (float) getHeight());
    const auto p = e.position;
    const bool over = hit (p, attackH) || hit (p, decayH) || hit (p, sustainH) || hit (p, releaseH);
    setMouseCursor (over ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void AdsrCanvas::mouseDown (const juce::MouseEvent& e)
{
    updateHandles ((float) getWidth(), (float) getHeight());
    const auto p = e.position;
    if (hit (p, attackH)) dragging = Attack;
    else if (hit (p, decayH) || hit (p, sustainH)) dragging = Decay;
    else if (hit (p, releaseH)) dragging = Release;
    else dragging = None;
}

void AdsrCanvas::mouseDrag (const juce::MouseEvent& e)
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    updateHandles (w, h);
    switch (dragging)
    {
        case Attack:
        {
            const float aNorm = juce::jlimit (0.0f, 1.0f, (e.position.x - 0.16f * w) / (0.16f * w));
            setRaw ("attack", 1.0f + aNorm * 1999.0f, 1.0f, 2000.0f);
            break;
        }
        case Decay:
        {
            const float dNorm = juce::jlimit (0.0f, 1.0f, (e.position.x - attackH.x - 0.10f * w) / (0.20f * w));
            setRaw ("decay", 1.0f + dNorm * 1999.0f, 1.0f, 2000.0f);
            const float sust = juce::jlimit (0.0f, 1.0f, (h - e.position.y - h * 0.12f) / (h * 0.72f));
            setRaw ("sustain", sust, 0.0f, 1.0f);
            break;
        }
        case Release:
        {
            const float rNorm = juce::jlimit (0.0f, 1.0f, (e.position.x - decayH.x - 0.12f * w) / (0.30f * w));
            setRaw ("release", 1.0f + rNorm * 4999.0f, 1.0f, 5000.0f);
            break;
        }
        default: break;
    }
}

void AdsrCanvas::mouseUp (const juce::MouseEvent&)
{
    dragging = None;
}
