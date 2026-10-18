#include "AdsrCanvas.h"
#include "Palette.h"

void AdsrCanvas::setRaw (const char* id, float raw, float min, float max)
{
    const float norm = (raw - min) / (max - min);
    if (auto* p = apvts().getParameter (id))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
}

void AdsrCanvas::timerCallback()
{
    const float sig = getRaw ("attack") + getRaw ("decay") * 2.0f
                    + getRaw ("sustain") * 4.0f + getRaw ("release") * 8.0f;
    if (sig == lastSig)
        return;
    lastSig = sig;
    repaint();
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
    const Palette& pal = Skins::current();
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    const float fs = juce::jlimit (7.0f, 12.0f, h * 0.07f);
    updateHandles (w, h);

    g.fillAll (pal.panelDeep);
    g.setColour (pal.gridLine);
    for (int i = 1; i < 5; ++i)
        g.drawVerticalLine ((int) (w * (float) i / 5.0f), 0, h);
    g.drawHorizontalLine ((int) (h * 0.5f), 0, h);

    juce::Path env;
    env.startNewSubPath (w * 0.05f, h - h * 0.08f);
    env.lineTo (attackH.x, h * 0.12f);
    env.lineTo (decayH.x, decayH.y);
    env.lineTo (releaseH.x, decayH.y);
    env.lineTo (releaseH.x + (w - releaseH.x) * 0.35f, h - h * 0.08f);

    juce::Path fill (env);
    fill.lineTo (releaseH.x + (w - releaseH.x) * 0.35f, h - h * 0.08f);
    fill.lineTo (w * 0.05f, h - h * 0.08f);
    fill.closeSubPath();
    g.setColour (pal.env.withAlpha (0.10f));
    g.fillPath (fill);

    if (pal.neonGlow)
    {
        g.setColour (pal.env.withAlpha (0.20f));
        g.strokePath (env, juce::PathStrokeType (5.0f));
    }
    g.setColour (pal.env);
    g.strokePath (env, juce::PathStrokeType (1.8f));

    auto drawHandle = [&] (juce::Point<float> p, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.25f));
        g.fillEllipse (p.x - 9, p.y - 9, 18, 18);
        g.setColour (c);
        g.fillEllipse (p.x - 4, p.y - 4, 8, 8);
        g.setColour (pal.text);
        g.drawEllipse (p.x - 4, p.y - 4, 8, 8, 1.0f);
    };
    drawHandle (attackH, pal.env);
    drawHandle (decayH, pal.env.brighter (0.35f));
    drawHandle (sustainH, pal.env.brighter (0.35f));
    drawHandle (releaseH, pal.potential);

    auto drawValue = [&] (juce::Point<float> p, const juce::String& text, float dx)
    {
        g.setColour (pal.textTick);
        g.setFont (Skins::font (fs));
        g.drawText (text, juce::Rectangle<float> (p.x + dx - 30.0f, p.y - 7.0f, 60.0f, 14.0f),
                    juce::Justification::centred, false);
    };
    drawValue (attackH, juce::String (juce::roundToInt (getRaw ("attack"))) + "ms", -2.0f);
    drawValue (decayH, juce::String (juce::roundToInt (getRaw ("decay"))) + "ms", 26.0f);
    drawValue (sustainH, juce::String (juce::roundToInt (getRaw ("sustain") * 100.0f)) + "%", 0.0f);
    drawValue (releaseH, juce::String (juce::roundToInt (getRaw ("release"))) + "ms", -26.0f);

    g.setColour (pal.textDim);
    g.setFont (Skins::font (fs * 1.1f, juce::Font::bold));
    g.drawText ("ADSR", getLocalBounds().reduced (8, 4).removeFromTop (16),
                juce::Justification::centredLeft, false);
    g.setColour (pal.textDim.withAlpha (0.6f));
    g.setFont (Skins::font (fs));
    g.drawText ("drag attack / decay / release", getLocalBounds().reduced (8, 4).removeFromTop (16).withX (w * 0.4f),
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
