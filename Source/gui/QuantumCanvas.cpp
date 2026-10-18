#include "QuantumCanvas.h"
#include "Palette.h"
#include <cmath>

QuantumCanvas::QuantumCanvas (PluginProcessor& p) : processor (p)
{
    startTimerHz (30);
}

void QuantumCanvas::setNorm (const char* id, float norm)
{
    if (auto* p = apvts().getParameter (id))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm));
}

void QuantumCanvas::cycleWaveType()
{
    if (auto* p = apvts().getParameter ("waveType"))
    {
        const int idx = juce::roundToInt (p->getValue() * 5.0f);
        p->setValueNotifyingHost ((float) ((idx + 1) % 6) / 5.0f);
    }
}

void QuantumCanvas::cyclePotentialType()
{
    if (auto* p = apvts().getParameter ("potentialType"))
    {
        const int idx = juce::roundToInt (p->getValue() * 5.0f);
        p->setValueNotifyingHost ((float) ((idx + 1) % 6) / 5.0f);
    }
}

juce::Rectangle<float> QuantumCanvas::wavePanel() const
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);
    const float gap = 6.0f;
    const float split = b.getY() + b.getHeight() * 0.60f;
    return { b.getX(), b.getY(), b.getWidth(), split - b.getY() };
}

juce::Rectangle<float> QuantumCanvas::spectrumPanel() const
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);
    const float gap = 6.0f;
    const float split = b.getY() + b.getHeight() * 0.60f;
    return { b.getX(), split + gap, b.getWidth(), b.getBottom() - split - gap };
}

void QuantumCanvas::updateHandles()
{
    plot = wavePanel().reduced (14.0f, 24.0f).withBottom (wavePanel().getBottom() - 16.0f);
    const float cx = plot.getCentreX();
    const float halfW = plot.getWidth() * 0.5f;
    const float waveY = plot.getY() + plot.getHeight() * 0.30f;
    const float potBase = plot.getBottom();

    const float potX = cx + unitToXNorm (raw ("potentialShift")->load()) * halfW;
    const float waveX = cx + unitToXNorm (raw ("waveShift")->load()) * halfW;
    waveCenterH = { waveX, waveY };
    waveScaleH = { waveX + 12.0f + raw ("waveScale")->load() * (halfW * 0.35f), waveY };
    potCeiling = plot.getY() + plot.getHeight() * 0.55f;
    const float potSpan = potBase - potCeiling;
    potCenterH = { potX, potBase };
    potHeightH = { potX, potBase - potSpan * raw ("potentialHeight")->load() };
    potScaleH = { potX + (0.08f + raw ("potentialScale")->load() * 0.28f) * plot.getWidth(), potBase };
}

void QuantumCanvas::timerCallback()
{
    const unsigned seq = processor.getScopeSeq();
    const float sig = raw ("waveShift")->load() + raw ("waveScale")->load() * 2.0f
                    + raw ("potentialShift")->load() * 4.0f + raw ("potentialScale")->load() * 8.0f
                    + raw ("potentialHeight")->load() * 16.0f + raw ("showFFT")->load() * 32.0f;
    if (seq == lastSeq && sig == lastHandleSig)
        return;
    lastSeq = seq;
    lastHandleSig = sig;
    processor.snapshotScope (scopePos, scopeMom, scopePot);
    repaint();
}

void QuantumCanvas::paint (juce::Graphics& g)
{
    const Palette& pal = Skins::current();
    updateHandles();

    const auto wp = wavePanel();
    const auto sp = spectrumPanel();
    const float fs = juce::jlimit (7.0f, 13.0f, (float) getHeight() * 0.022f);

    g.fillAll (pal.background);

    // ---- |psi(x)| panel -------------------------------------------------
    drawSkinnedPanel (g, wp, pal, "", pal.wave, fs);
    g.setColour (pal.textDim);
    g.setFont (Skins::font (fs, juce::Font::bold));
    g.drawText ("|PSI(X)|  REAL-SPACE WAVE", wp.reduced (12).removeFromTop (20).removeFromLeft (10),
                juce::Justification::centredLeft, false);
    const int wt = juce::roundToInt (raw ("waveType")->load());
    if (auto* p = apvts().getParameter ("waveType"))
    {
        g.setColour (pal.wave);
        g.setFont (Skins::font (fs));
        g.drawText (p->getText (wt, 0).toUpperCase(), wp.reduced (12).removeFromTop (20).withWidth (200),
                    juce::Justification::centredRight, false);
    }

    g.setColour (pal.gridLine);
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine ((int) (wp.getX() + wp.getWidth() * (float) i / 8.0f), wp.getY() + 20.0f, wp.getBottom() - 1.0f);
    g.setColour (pal.gridLine.withAlpha (0.6f));
    g.drawHorizontalLine ((int) plot.getCentreY(), wp.getX() + 1.0f, wp.getRight() - 1.0f);

    // potential (log scale: the wells go up to ~1000, linear would clip everything else)
    {
        const float logRef = std::log1p (1000.0f);
        juce::Path potPath;
        bool first = true;
        for (int i = 0; i < N; ++i)
        {
            const float px = plot.getX() + plot.getWidth() * (float) i / (float) (N - 1);
            const float v = juce::jlimit (0.0f, 1.0f, std::log1p (juce::jmax (0.0f, scopePot[i])) / logRef);
            const float y = plot.getBottom() - v * (plot.getBottom() - potCeiling);
            if (first) { potPath.startNewSubPath (px, y); first = false; }
            else potPath.lineTo (px, y);
        }
        potPath.lineTo (plot.getRight(), plot.getBottom());
        potPath.lineTo (plot.getX(), plot.getBottom());
        potPath.closeSubPath();
        g.setColour (pal.potential.withAlpha (0.13f));
        g.fillPath (potPath);
        g.setColour (pal.potential.withAlpha (0.85f));
        g.strokePath (potPath, juce::PathStrokeType (1.2f));
    }

    // |psi(x)| from the audio thread
    {
        float peak = 1.0e-9f;
        for (int i = 0; i < N; ++i) peak = juce::jmax (peak, scopePos[i]);
        const float baseline = plot.getY() + plot.getHeight() * 0.30f;
        const float amp = plot.getHeight() * 0.24f;
        juce::Path wave;
        for (int i = 0; i < N; ++i)
        {
            const float px = plot.getX() + plot.getWidth() * (float) i / (float) (N - 1);
            const float v = juce::jlimit (0.0f, 1.0f, scopePos[i] / peak);
            const float y = baseline - v * amp;
            if (i == 0) wave.startNewSubPath (px, y);
            else wave.lineTo (px, y);
        }
        juce::Path fill (wave);
        fill.lineTo (plot.getRight(), baseline);
        fill.lineTo (plot.getX(), baseline);
        fill.closeSubPath();
        g.setColour (pal.wave.withAlpha (0.24f));
        g.fillPath (fill);

        if (pal.neonGlow)
        {
            g.setColour (pal.wave.withAlpha (0.35f));
            g.strokePath (wave, juce::PathStrokeType (4.0f));
        }
        g.setColour (pal.wave);
        g.strokePath (wave, juce::PathStrokeType (1.6f));

        g.setColour (pal.textDim);
        g.setFont (Skins::font (fs * 0.85f));
        g.drawText ("-1", plot.withWidth (24).removeFromLeft (28).withY (baseline - 6).withHeight (12),
                    juce::Justification::centredRight, false);
        g.drawText ("0", juce::Rectangle<float> (plot.getCentreX() - 12.0f, plot.getBottom() - 14.0f, 24.0f, 12.0f),
                    juce::Justification::centred, false);
        g.drawText ("+1", plot.withWidth (24).withX (plot.getRight() + 4.0f).withY (baseline - 6.0f),
                    juce::Justification::centredLeft, false);
    }

    // drag handles
    auto drawHandle = [&] (juce::Point<float> pt, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.25f));
        g.fillEllipse (pt.x - 10.0f, pt.y - 10.0f, 20.0f, 20.0f);
        g.setColour (c);
        g.fillEllipse (pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f);
        g.setColour (pal.text);
        g.drawEllipse (pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f, 1.0f);
    };
    drawHandle (waveCenterH, pal.wave);
    drawHandle (waveScaleH, pal.wave.brighter (0.4f));
    drawHandle (potCenterH, pal.potential);
    drawHandle (potHeightH, pal.potential);
    drawHandle (potScaleH, pal.potential.brighter (0.4f));

    // ---- |psi(k)| spectrum panel -----------------------------------------
    const bool fftOn = raw ("showFFT")->load() > 0.5f;
    drawSkinnedPanel (g, sp, pal, "", pal.harmony, fs);
    g.setColour (fftOn ? pal.accent : pal.textDim);
    g.setFont (Skins::font (fs, juce::Font::bold));
    g.drawText ("|PSI(K)|  MOMENTUM SPECTRUM", sp.reduced (12).removeFromTop (20).removeFromLeft (10),
                juce::Justification::centredLeft, false);
    if (fftOn)
    {
        g.setColour (pal.accent.withAlpha (0.7f));
        g.drawRoundedRectangle (sp.reduced (0.5f), pal.corner, 1.5f);
    }

    const float f0 = processor.getUiNoteFreqHz();
    const juce::String f0Text = f0 > 0.0f ? juce::String (f0, 0) + " Hz" : juce::String ("-");
    g.setColour (pal.textDim);
    g.setFont (Skins::font (fs * 0.9f));
    g.drawText ("f0 " + f0Text, sp.reduced (12).removeFromTop (20).withWidth (160),
                juce::Justification::centredRight, false);

    const juce::Rectangle<float> sPlot = sp.reduced (14.0f, 24.0f).withTop (sp.getY() + 22.0f).withBottom (sp.getBottom() - 10.0f);
    constexpr int bins = N / 2;              // 64 non-redundant momentum bins
    constexpr float dbFloor = -48.0f;

    // horizontal dB grid
    g.setFont (Skins::font (fs * 0.8f));
    for (float db : { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f })
    {
        const float y = sPlot.getBottom() + db / dbFloor * sPlot.getHeight();
        g.setColour (db == 0.0f ? pal.gridLineStrong : pal.gridLine);
        g.drawHorizontalLine ((int) y, sPlot.getX(), sPlot.getRight());
        if (db != 0.0f)
        {
            g.setColour (pal.textDim);
            g.drawText (juce::String ((int) db) + "dB", juce::Rectangle<float> (sPlot.getX(), y - 6.0f, 30.0f, 12.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    // vertical harmonic grid with bin + Hz labels
    for (int b : { 1, 2, 4, 8, 16, 32, 63 })
    {
        const float x = sPlot.getX() + sPlot.getWidth() * (float) (b - 1) / (float) (bins - 2);
        g.setColour (pal.gridLine);
        g.drawVerticalLine ((int) x, sPlot.getY(), sPlot.getBottom());
        g.setColour (pal.textTick);
        const juce::String label = f0 > 0.0f
            ? juce::String (b) + "\u00b7f\u2080  " + juce::String (b * f0, 0) + "Hz"
            : juce::String (b) + "\u00b7f\u2080";
        g.drawText (label, juce::Rectangle<float> (x - 22.0f, sPlot.getBottom() + 1.0f, 60.0f, 10.0f),
                    juce::Justification::centredLeft, false);
    }

    // spectrum curve
    {
        float peak = 1.0e-9f;
        for (int i = 1; i < bins; ++i) peak = juce::jmax (peak, scopeMom[i]);
        // dbFloor is negative; map db==0 to top, db==dbFloor to bottom.
        auto yFor = [&] (float v)
        {
            const float db = 20.0f * std::log10 (juce::jmax (1.0e-9f, v / peak));
            const float norm = juce::jlimit (0.0f, 1.0f, db / dbFloor);
            return sPlot.getBottom() - (1.0f - norm) * sPlot.getHeight();
        };

        juce::Path curve;
        for (int i = 1; i < bins; ++i)
        {
            const float x = sPlot.getX() + sPlot.getWidth() * (float) (i - 1) / (float) (bins - 2);
            const float y = yFor (scopeMom[i]);
            if (i == 1) curve.startNewSubPath (x, y);
            else curve.lineTo (x, y);
        }
        juce::Path fill (curve);
        fill.lineTo (sPlot.getRight(), sPlot.getBottom());
        fill.lineTo (sPlot.getX(), sPlot.getBottom());
        fill.closeSubPath();
        g.setColour (pal.wave.withAlpha (0.20f));
        g.fillPath (fill);
        if (pal.neonGlow)
        {
            g.setColour (pal.wave.withAlpha (0.30f));
            g.strokePath (curve, juce::PathStrokeType (3.5f));
        }
        g.setColour (pal.wave.brighter (0.15f));
        g.strokePath (curve, juce::PathStrokeType (1.4f));
    }

    if (getWidth() > 0)
    {
        g.setColour (pal.textDim.withAlpha (0.7f));
        g.setFont (Skins::font (fs * 0.85f));
        g.drawText ("drag handles to shape wave/potential", wp.reduced (12).withY (wp.getBottom() - 16.0f).withHeight (14),
                    juce::Justification::centredLeft, false);
    }
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
        if (wavePanel().contains (p) && p.y < wavePanel().getCentreY()) cycleWaveType();
        else if (wavePanel().contains (p)) cyclePotentialType();
        dragging = None;
    }
}

void QuantumCanvas::mouseDrag (const juce::MouseEvent& e)
{
    updateHandles();
    const float halfW = plot.getWidth() * 0.5f;
    switch (dragging)
    {
        case WaveCenter:
            setNorm ("waveShift", xNormToUnit ((e.position.x - plot.getCentreX()) / halfW));
            break;
        case WaveScale:
        {
            const float dx = (e.position.x - waveCenterH.x) / halfW;
            setNorm ("waveScale", juce::jlimit (0.0f, 1.0f, dx * 1.8f));
            break;
        }
        case PotCenter:
            setNorm ("potentialShift", xNormToUnit ((e.position.x - plot.getCentreX()) / halfW));
            break;
        case PotHeight:
        {
            const float heightNorm = juce::jlimit (0.0f, 1.0f, (plot.getBottom() - e.position.y) / (plot.getBottom() - potCeiling));
            setNorm ("potentialHeight", heightNorm);
            break;
        }
        case PotScale:
        {
            const float dx = std::fabs (e.position.x - potCenterH.x) / (plot.getWidth() * 0.35f);
            setNorm ("potentialScale", juce::jlimit (0.0f, 1.0f, dx * 1.4f));
            break;
        }
        default: break;
    }
}

void QuantumCanvas::mouseUp (const juce::MouseEvent&)
{
    dragging = None;
}
