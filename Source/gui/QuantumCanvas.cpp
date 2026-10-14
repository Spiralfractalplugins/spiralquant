#include "QuantumCanvas.h"
#include "SciFiLookAndFeel.h"
#include <cmath>

void QuantumCanvas::setNorm (const char* id, float norm)
{
    if (auto* p = apvts().getParameter (id))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
}

void QuantumCanvas::cycleWaveType()
{
    if (auto* p = apvts().getParameter ("waveType"))
    {
        const int idx = (p->getValue() * 5.0f + 1.0f);
        p->setValueNotifyingHost ((float) (idx % 6) / 5.0f);
    }
}

void QuantumCanvas::cyclePotentialType()
{
    if (auto* p = apvts().getParameter ("potentialType"))
    {
        const int idx = (int) (p->getValue() * 5.0f + 1.0f);
        p->setValueNotifyingHost ((float) (idx % 6) / 5.0f);
    }
}

void QuantumCanvas::updateHandles()
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    const float cx = w * 0.5f;
    const float waveY = h * 0.32f;
    const float potBase = h * 0.86f;
    const float potHeightPx = h * 0.42f * getRaw ("potentialHeight");
    const float potX = cx + unitToXNorm (getRaw ("potentialShift")) * (w * 0.42f);
    const float waveX = cx + unitToXNorm (getRaw ("waveShift")) * (w * 0.42f);
    waveCenterH = { waveX, waveY };
    waveScaleH = { waveX + 12.0f + getRaw ("waveScale") * (w * 0.16f), waveY };
    potCenterH = { potX, potBase };
    potHeightH = { potX, potBase - potHeightPx };
    potScaleH = { potX + (0.08f + getRaw ("potentialScale") * 0.28f) * w, potBase };
}

void QuantumCanvas::paint (juce::Graphics& g)
{
    updateHandles();
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    const float cx = w * 0.5f;

    g.fillAll (SciFiTheme::panelDeep);
    g.setColour (SciFiTheme::strokeDim);
    for (int i = 1; i < 6; ++i)
    {
        const float x = w * (float) i / 6.0f;
        g.drawVerticalLine ((int) x, 0, (int) h);
    }
    for (int i = 1; i < 4; ++i)
    {
        const float y = h * (float) i / 4.0f;
        g.drawHorizontalLine ((int) y, 0, (int) w);
    }

    const int waveType = juce::roundToInt (getRaw ("waveType"));
    const int potType = juce::roundToInt (getRaw ("potentialType"));

    // potential curve outline
    juce::Path potPath;
    bool first = true;
    const float waveY = h * 0.32f;
    const float potBase = h * 0.86f;
    for (int i = 0; i < 128; ++i)
    {
        const float xNorm = (float) i / 127.0f * 2.0f - 1.0f;
        const float px = cx + xNorm * (w * 0.42f);
        const float depth = getRaw ("potentialScale");
        const float center = unitToXNorm (getRaw ("potentialShift")) * 1.0f;
        const float heightScale = getRaw ("potentialHeight");
        float v = 0.0f;
        const float dx = xNorm - center;
        if (potType == 1) v = 0.5f * (0.2f + depth) * dx * dx * 8.0f;
        else if (potType == 2) v = (std::fabs (xNorm) > 0.9f) ? 1.0f : 0.0f;
        else if (potType == 3) v = (std::fabs (dx) < (0.05f + 0.2f * depth)) ? 1.0f : 0.0f;
        else if (potType == 4) v = (dx * dx - 0.25f) * (dx * dx - 0.25f) * 5.0f;
        else if (potType == 5) v = 0.6f * dx + 0.3f * getRaw ("potentialShift");
        v = juce::jlimit (0.0f, 1.0f, v * (0.25f + 0.75f * heightScale));
        const float y = potBase - v * (h * 0.34f);
        if (first) { potPath.startNewSubPath (px, y); first = false; }
        else potPath.lineTo (px, y);
    }
    g.setColour (SciFiTheme::amber.withAlpha (0.22f));
    g.fillPath (potPath);
    g.setColour (SciFiTheme::amber);
    g.strokePath (potPath, juce::PathStrokeType (2.0f));

    // wave guide (analytic)
    juce::Path wavePath;
    first = true;
    const float waveShiftRaw = getRaw ("waveShift");
    const float waveScaleRaw = getRaw ("waveScale");
    const float cc = juce::jlimit (-1.0f, 1.0f, waveShiftRaw * 2.0f - 1.0f);
    const float width = 0.08f + waveScaleRaw * 0.55f;
    for (int i = 0; i < 128; ++i)
    {
        const float xNorm = (float) i / 127.0f * 2.0f - 1.0f;
        float v = 0.0f;
        const float d = xNorm - cc;
        if (waveType == 0) v = std::exp (-d * d / (2.0f * width * width));
        else if (waveType == 1) v = std::sin (juce::MathConstants<float>::pi * (1.0f + 4.0f * waveScaleRaw) * (xNorm + 1.0f));
        else if (waveType == 2) v = std::cos (juce::MathConstants<float>::pi * (1.0f + 4.0f * waveScaleRaw) * (xNorm + 1.0f));
        else if (waveType == 3) v = (xNorm - (waveShiftRaw * 2.0f - 1.0f)) * (xNorm - (waveShiftRaw * 2.0f - 1.0f));
        else if (waveType == 4) v = xNorm;
        else v = (xNorm < (waveShiftRaw * 2.0f - 1.0f)) ? 1.0f : -1.0f;
        const float y = waveY - v * (h * 0.16f);
        const float px = cx + xNorm * (w * 0.42f);
        if (first) { wavePath.startNewSubPath (px, y); first = false; }
        else wavePath.lineTo (px, y);
    }
    g.setColour (SciFiTheme::neon.withAlpha (0.35f));
    g.strokePath (wavePath, juce::PathStrokeType (4.0f));
    g.setColour (SciFiTheme::neon);
    g.strokePath (wavePath, juce::PathStrokeType (1.6f));

    // handles
    auto drawHandle = [&] (juce::Point<float> pt, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.25f));
        g.fillEllipse (pt.x - 10.0f, pt.y - 10.0f, 20.0f, 20.0f);
        g.setColour (c);
        g.fillEllipse (pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f);
        g.setColour (SciFiTheme::text);
        g.drawEllipse (pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f, 1.0f);
    };
    drawHandle (waveCenterH, SciFiTheme::neon);
    drawHandle (waveScaleH, SciFiTheme::holo);
    drawHandle (potCenterH, SciFiTheme::amber);
    drawHandle (potHeightH, SciFiTheme::amber);
    drawHandle (potScaleH, SciFiTheme::holo);

    // title
    g.setColour (SciFiTheme::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("WAVE (top) / POTENTIAL (bottom) - drag handles; click areas to change type",
                getLocalBounds().reduced (8, 4).removeFromTop (14), juce::Justification::centredLeft, false);
}

void QuantumCanvas::mouseMove (const juce::MouseEvent& e)
{
    updateHandles();
    const auto p = e.position;
    const bool over = hit (p, waveCenterH) || hit (p, waveScaleH) || hit (p, potCenterH) ||
                     hit (p, potHeightH) || hit (p, potScaleH);
    setMouseCursor (over ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void QuantumCanvas::mouseDown (const juce::MouseEvent& e)
{
    updateHandles();
    const auto p = e.position;
    if (hit (p, waveCenterH)) dragging = WaveCenter;
    else if (hit (p, waveScaleH)) dragging = WaveScale;
    else if (hit (p, potHeightH)) dragging = PotHeight;
    else if (hit (p, potCenterH)) dragging = PotCenter;
    else if (hit (p, potScaleH)) dragging = PotScale;
    else
    {
        // click on empty area cycles type based on vertical region
        if (e.position.y < getHeight() * 0.5f) cycleWaveType();
        else cyclePotentialType();
        dragging = None;
    }
    lastMouse = p;
}

void QuantumCanvas::mouseDrag (const juce::MouseEvent& e)
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    const float cx = w * 0.5f;
    switch (dragging)
    {
        case WaveCenter:
        {
            const float u = xNormToUnit ((e.position.x - cx) / (w * 0.42f));
            setNorm ("waveShift", u);
            break;
        }
        case WaveScale:
        {
            const float dx = (e.position.x - waveCenterH.x) / (w * 0.42f);
            const float scale = juce::jlimit (0.0f, 1.0f, dx * 1.8f);
            setNorm ("waveScale", scale);
            break;
        }
        case PotCenter:
        {
            const float u = xNormToUnit ((e.position.x - cx) / (w * 0.42f));
            setNorm ("potentialShift", u);
            // also allow vertical drag to change height when moving handle at base
            break;
        }
        case PotHeight:
        {
            const float base = h * 0.86f;
            const float heightNorm = juce::jlimit (0.0f, 1.0f, (base - e.position.y) / (h * 0.42f));
            setNorm ("potentialHeight", heightNorm);
            break;
        }
        case PotScale:
        {
            const float dx = std::fabs (e.position.x - potCenterH.x) / (w * 0.35f);
            setNorm ("potentialScale", juce::jlimit (0.0f, 1.0f, dx * 1.4f));
            break;
        }
        default: break;
    }
    lastMouse = e.position;
}

void QuantumCanvas::mouseUp (const juce::MouseEvent&)
{
    dragging = None;
}
