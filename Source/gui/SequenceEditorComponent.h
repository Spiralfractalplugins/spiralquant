#pragma once

#include <JuceHeader.h>

#include "../PluginProcessor.h"
#include "DarkLookAndFeel.h"

/** The step grid: pitch rows follow the selected scale, flag rows edit slide and
    accent. Click paints a note, click on a note erases it, drag draws a line of
    notes, and the flag rows toggle their marker on the notes underneath. */
class SequenceEditorComponent : public juce::Component,
                                public juce::SettableTooltipClient
{
public:
    static constexpr int kNumPitchRows = 14;

    explicit SequenceEditorComponent (PluginProcessor& p) : processor (p)
    {
        setTooltip ("Click or drag to enter notes.\n"
                    "Click a note again to remove it.\n"
                    "TIE, SL and AC rows below the grid hold, slide and accent.\n"
                    "Right-click a step for the same options.");
    }

    void setScale (float s)
    {
        uiScale = s;
        repaint();
    }

    void refresh()
    {
        cached = processor.getSequence();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        if (cachedSequenceVersion != processor.getSequenceVersion())
        {
            cachedSequenceVersion = processor.getSequenceVersion();
            cached = processor.getSequence();
        }

        auto area = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (Theme::panel);
        g.fillRoundedRectangle (area, Theme::corner);
        g.setColour (Theme::panelOutline);
        g.drawRoundedRectangle (area, Theme::corner, 1.0f);

        const float gutter = 28.0f * uiScale;
        const float top = 18.0f * uiScale;
        const float bottom = 2.0f * uiScale;
        const juce::Rectangle<float> grid (area.getX() + gutter, area.getY() + top,
                                           area.getWidth() - gutter - 2.0f,
                                           area.getHeight() - top - bottom);
        const int steps = jmax (1, cached.numSteps());
        const float stepW = grid.getWidth() / static_cast<float> (steps);
        const float rowH = grid.getHeight() / static_cast<float> (kNumPitchRows + 3);
        const float pitchBottom = grid.getY() + rowH * static_cast<float> (kNumPitchRows);

        // step columns
        for (int s = 0; s < steps; ++s)
        {
            const float x = grid.getX() + static_cast<float> (s) * stepW;
            const bool beat = s % 4 == 0;
            g.setColour (beat ? Theme::gridBeat : Theme::gridLine);
            g.drawLine (x, grid.getY(), x, grid.getBottom(), beat ? 1.2f : 0.7f);

            if (s % 4 == 3)
            {
                g.setColour (Theme::gridBeat.withAlpha (0.5f));
                g.drawLine (x + stepW, grid.getY(), x + stepW, grid.getBottom(), 0.7f);
            }

            if (processor.getPlayingStep() == s)
            {
                g.setColour (Theme::playhead.withAlpha (0.10f));
                g.fillRect (x, grid.getY(), stepW, grid.getHeight());
                g.setColour (Theme::playhead);
                g.fillRect (x, grid.getY() - 5.0f * uiScale, stepW, 3.0f * uiScale);
            }
        }

        // pitch rows
        for (int r = 0; r < kNumPitchRows; ++r)
        {
            const float y = grid.getY() + static_cast<float> (kNumPitchRows - 1 - r) * rowH;
            const int degree = r;
            const bool base = degree == 0;

            if (base)
            {
                g.setColour (Theme::accent.withAlpha (0.14f));
                g.fillRect (grid.getX(), y, grid.getWidth(), rowH);
            }

            const int midi = midiForDegree (degree);
            g.setColour (base ? Theme::accent : Theme::textDim);
            g.setFont (Theme::font (9.0f * uiScale, base ? juce::Font::bold : juce::Font::plain));
            g.drawText (noteName (midi),
                        juce::Rectangle<float> (area.getX() + 2.0f * uiScale, y, gutter - 4.0f * uiScale, rowH),
                        juce::Justification::centred, false);

            g.setColour (Theme::gridLine.withAlpha (0.6f));
            g.drawLine (grid.getX(), y, grid.getRight(), y, 0.6f);
        }

        // notes
        for (int s = 0; s < steps; ++s)
        {
            const float x = grid.getX() + static_cast<float> (s) * stepW;

            if (cached.isNote (s))
            {
                const int degree = cached.degree (s);
                if (degree >= 0 && degree < kNumPitchRows)
                {
                    const float y = grid.getY() + static_cast<float> (kNumPitchRows - 1 - degree) * rowH;
                    g.setColour (Theme::accent.withAlpha (cached.hasAccent (s) ? 1.0f : 0.78f));
                    g.fillRoundedRectangle (x + 1.5f, y + 1.5f, stepW - 3.0f, rowH - 3.0f, 3.0f);
                    if (cached.hasAccent (s))
                    {
                        g.setColour (juce::Colours::white);
                        g.fillRect (x + stepW * 0.5f - 2.0f, y + rowH * 0.5f - 2.0f, 4.0f, 4.0f);
                    }
                    if (cached.hasSlide (s))
                    {
                        g.setColour (Theme::background);
                        g.drawRoundedRectangle (x + 3.0f, y + 3.0f, stepW - 6.0f, rowH - 6.0f, 2.0f, 1.2f);
                    }
                }
            }
            else if (cached.isTie (s))
            {
                // A held step reads as a short dash in the row under the pitch grid.
                g.setColour (Theme::accentAlt);
                g.fillRoundedRectangle (x + stepW * 0.18f, pitchBottom + rowH * 0.5f - 1.5f * uiScale,
                                        stepW * 0.64f, 3.0f * uiScale, 1.5f * uiScale);
            }

            // slide / accent marks in their own rows
            if (cached.isNote (s))
            {
                auto mark = [&] (bool on, const juce::String& text, int row, const juce::Colour& colour)
                {
                    if (! on)
                        return;
                    const float y = pitchBottom + rowH * static_cast<float> (row);
                    g.setColour (colour);
                    g.setFont (Theme::font (9.5f * uiScale, juce::Font::bold));
                    g.drawText (text, juce::Rectangle<float> (x, y, stepW, rowH),
                                juce::Justification::centred, false);
                };
                mark (cached.hasSlide (s), "SL", 1, Theme::pitch);
                mark (cached.hasAccent (s), "AC", 2, Theme::accentAlt);
            }
        }

        // row captions
        auto caption = [&] (const juce::String& text, float y)
        {
            g.setColour (Theme::textDim);
            g.setFont (Theme::font (9.0f * uiScale, juce::Font::bold));
            g.drawText (text, juce::Rectangle<float> (area.getX() + 2.0f * uiScale, y,
                                                      gutter - 4.0f * uiScale, rowH),
                        juce::Justification::centredLeft, false);
        };
        caption ("TIE", pitchBottom);
        caption ("SL", pitchBottom + rowH * 1.0f);
        caption ("AC", pitchBottom + rowH * 2.0f);

        g.setColour (Theme::textDim);
        g.setFont (Theme::font (9.5f * uiScale, juce::Font::bold));
        g.drawText ("BASE " + noteName (cached.baseMidi) + "  |  "
                       + Sequence::scaleNames()[cached.scaleIndex],
                    juce::Rectangle<float> (grid.getX(), area.getY(), grid.getWidth(), top),
                    juce::Justification::centredLeft, false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu ())
        {
            showStepMenu (e);
            return;
        }
        handleMouse (e, true);
    }
    void mouseDrag (const juce::MouseEvent& e) override { handleMouse (e, false); }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dirty)
        {
            dirty = false;
            processor.setSequence (cached);
            cachedSequenceVersion = processor.getSequenceVersion();
            repaint();
        }
    }

    void showStepMenu (const juce::MouseEvent& e)
    {
        const int step = stepAt (e.x);
        if (step < 0)
            return;
        cached = processor.getSequence();
        juce::PopupMenu menu;
        menu.addItem (1, "Tie (hold previous note)");
        menu.addItem (2, "Rest");
        menu.addItem (3, "Slide");
        menu.addItem (4, "Accent");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [this, step] (int result)
        {
            if (result == 0)
                return;
            auto seq = processor.getSequence();
            if (result == 1)
                seq.steps[static_cast<size_t> (step)] = Sequence::kTie;
            else if (result == 2)
                seq.steps[static_cast<size_t> (step)] = Sequence::kRest;
            else if (result == 3 && seq.isNote (step))
                seq.toggleSlide (step);
            else if (result == 4 && seq.isNote (step))
                seq.toggleAccent (step);
            processor.setSequence (seq);
            cached = seq;
            repaint();
        });
    }

private:
    int stepAt (int x) const
    {
        auto area = getLocalBounds().toFloat().reduced (1.0f);
        const float gutter = 28.0f * uiScale;
        const float top = 18.0f * uiScale;
        const juce::Rectangle<float> grid (area.getX() + gutter, area.getY() + top,
                                           area.getWidth() - gutter - 2.0f,
                                           area.getHeight() - top - 2.0f * uiScale);
        const int steps = jmax (1, cached.numSteps());
        const float stepW = grid.getWidth() / static_cast<float> (steps);
        if (x < grid.getX() || stepW <= 0.0f)
            return -1;
        const int s = static_cast<int> ((static_cast<float> (x) - grid.getX()) / stepW);
        return s >= steps ? -1 : s;
    }

    int rowAt (int y, bool& flagRow, int& flagKind) const
    {
        flagRow = false;
        flagKind = 0;
        auto area = getLocalBounds().toFloat().reduced (1.0f);
        const float top = 18.0f * uiScale;
        const float rowH = (area.getHeight() - top - 2.0f * uiScale) / static_cast<float> (kNumPitchRows + 3);
        const float pitchBottom = area.getY() + top + rowH * static_cast<float> (kNumPitchRows);

        if (y >= pitchBottom)
        {
            flagRow = true;
            const int index = static_cast<int> ((static_cast<float> (y) - pitchBottom) / rowH);
            flagKind = index == 0 ? 3 : (index == 1 ? 1 : (index == 2 ? 2 : 0)); // 3 tie, 1 slide, 2 accent
            return -1;
        }
        const int r = static_cast<int> ((pitchBottom - static_cast<float> (y)) / rowH);
        return jlimit (0, kNumPitchRows - 1, r);
    }

    void handleMouse (const juce::MouseEvent& e, bool isDown)
    {
        const int step = stepAt (e.x);
        if (step < 0)
            return;

        if (! isDown && ! dirty)
            cached = processor.getSequence();

        bool flagRow = false;
        int flagKind = 0;
        const int row = rowAt (e.y, flagRow, flagKind);

        if (flagRow)
        {
            if (isDown)
            {
                if (flagKind == 3)
                {
                    cached.steps[static_cast<size_t> (step)]
                        = cached.isTie (step) ? Sequence::kRest : Sequence::kTie;
                    dirty = true;
                }
                else if (flagKind != 0 && cached.isNote (step))
                {
                    flagKind == 1 ? cached.toggleSlide (step) : cached.toggleAccent (step);
                    dirty = true;
                }
            }
            repaint();
            return;
        }

        const int degree = row;
        if (cached.isNote (step) && cached.degree (step) == degree && isDown)
        {
            cached.steps[static_cast<size_t> (step)] = Sequence::kRest;
        }
        else
        {
            cached.steps[static_cast<size_t> (step)] = static_cast<juce::uint16> (
                Sequence::kNoteBase + degree
                + (cached.hasSlide (step) ? Sequence::kSlideFlag : 0)
                + (cached.hasAccent (step) ? Sequence::kAccentFlag : 0));
        }
        dirty = true;
        repaint();
    }

    int midiForDegree (int degree) const
    {
        const auto& offsets = Sequence::scaleOffsets (cached.scaleIndex);
        return cached.baseMidi + offsets[static_cast<size_t> (std::min (degree, static_cast<int> (offsets.size()) - 1))];
    }

    static juce::String noteName (int midi)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return names[midi % 12] + juce::String (midi / 12 - 1);
    }

    PluginProcessor& processor;
    Sequence cached;
    int cachedSequenceVersion = -1;
    float uiScale = 1.0f;
    bool dirty = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceEditorComponent)
};
