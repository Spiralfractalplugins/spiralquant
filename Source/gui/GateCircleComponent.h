#pragma once

#include <JuceHeader.h>

#include "../PluginProcessor.h"
#include "DarkLookAndFeel.h"

/** Fixed 4x4 gate grid: 16 squares, one per 1/16 step. Click or drag a square to
    mute (red) or unmute that 1/16; the knobs beside it shape the gate ramp.
    The sector count is no longer adjustable from the UI. */
class GateCircleComponent : public juce::Component,
                            public juce::SettableTooltipClient
{
public:
    explicit GateCircleComponent (PluginProcessor& p) : processor (p)
    {
        setTooltip ("Each square is one 1/16 step of the gate.\n"
                    "Red squares close the gate and mute the sound.\n"
                    "Click or drag a square to flip it.");
    }

    void setScale (float s)
    {
        uiScale = s;
        repaint();
        resized();
    }

    void refresh() { repaint(); }

    void paint (juce::Graphics& g) override
    {
        if (cachedGateVersion != processor.getGateVersion())
        {
            cachedGateVersion = processor.getGateVersion();
            cached = processor.getGate();
        }

        const auto area = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (Theme::panel);
        g.fillRoundedRectangle (area, Theme::corner);
        g.setColour (Theme::panelOutline);
        g.drawRoundedRectangle (area, Theme::corner, 1.0f);

        const int active = processor.getPlayingGateSector();
        const bool running = active >= 0;

        const float gap = 3.0f * uiScale;
        for (int i = 0; i < kCells; ++i)
        {
            auto r = cellBounds (i, area).reduced (gap);

            const bool closed = cached.isClosed (i);
            if (closed)
                g.setColour (Theme::gate.withAlpha (running && i == active ? 1.0f : 0.72f));
            else
                g.setColour (running && i == active ? Theme::accent.withAlpha (0.45f)
                                                    : Theme::background.withAlpha (0.55f));
            g.fillRoundedRectangle (r, Theme::corner * 0.5f);

            if (running && i == active)
            {
                g.setColour (Theme::playhead);
                g.drawRoundedRectangle (r, Theme::corner * 0.5f, 2.0f);
            }
            else
            {
                g.setColour (Theme::panelOutline);
                g.drawRoundedRectangle (r, Theme::corner * 0.5f, 1.0f);
            }
        }
    }

    void resized() override { repaint(); }

    void mouseDown (const juce::MouseEvent& e) override { toggleAt (e); }
    void mouseDrag (const juce::MouseEvent& e) override { toggleAt (e); }
    void mouseUp (const juce::MouseEvent&) override { lastToggled = -1; }

private:
    static constexpr int kColumns = 4;
    static constexpr int kRows = 4;
    static constexpr int kCells = kColumns * kRows;

    juce::Rectangle<float> cellBounds (int index, const juce::Rectangle<float>& area) const
    {
        const int col = index % kColumns;
        const int row = index / kColumns;
        const float cellW = area.getWidth() / static_cast<float> (kColumns);
        const float cellH = area.getHeight() / static_cast<float> (kRows);
        return juce::Rectangle<float> (area.getX() + static_cast<float> (col) * cellW,
                                       area.getY() + static_cast<float> (row) * cellH,
                                       cellW, cellH);
    }

    int sectorAt (juce::Point<int> pos) const
    {
        const auto area = getLocalBounds().toFloat().reduced (1.0f);
        const float cellW = area.getWidth() / static_cast<float> (kColumns);
        const float cellH = area.getHeight() / static_cast<float> (kRows);
        const int col = static_cast<int> ((static_cast<float> (pos.x) - area.getX()) / cellW);
        const int row = static_cast<int> ((static_cast<float> (pos.y) - area.getY()) / cellH);
        if (col < 0 || col >= kColumns || row < 0 || row >= kRows)
            return -1;
        return row * kColumns + col;
    }

    void toggleAt (const juce::MouseEvent& e)
    {
        const int s = sectorAt (e.getPosition());
        if (s < 0)
            return;

        if (s == lastToggled)
            return;

        auto gate = processor.getGate();
        gate.setSectorCount (kCells);
        gate.toggle (s);
        processor.setGate (gate);
        cached = gate;
        cachedGateVersion = processor.getGateVersion();
        lastToggled = s;
        repaint();
    }

    PluginProcessor& processor;
    GatePattern cached;
    int cachedGateVersion = -1;
    int lastToggled = -1;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateCircleComponent)
};

using GateGridComponent = GateCircleComponent;
using GatePadComponent = GateCircleComponent;
using GateSquareComponent = GateCircleComponent;