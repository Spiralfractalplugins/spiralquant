#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

/** One complete colour scheme. Skins::all() holds every skin; widgets read the
    active one at paint time, so a skin switch only needs a repaint. */
struct Palette
{
    juce::Colour background, panel, panelDeep, panelOutline;
    juce::Colour gridLine, gridLineStrong;
    juce::Colour text, textDim, textTick;
    juce::Colour accent;                        // focus ring / global highlight
    juce::Colour wave, potential, env;          // per-group accents
    juce::Colour filter, fx, harmony;
    juce::Colour knobTrack, knobHi, knobLo;
    float corner = 6.0f;
    bool neonGlow = false;                      // thick glow strokes on graphs
    juce::String name;
};

namespace Skins
{
    inline constexpr float fontTick   = 8.0f;
    inline constexpr float fontValue  = 9.0f;
    inline constexpr float fontLabel  = 10.0f;
    inline constexpr float fontBody   = 11.0f;
    inline constexpr float fontButton = 13.0f;
    inline constexpr float fontTitle  = 14.0f;
    inline constexpr int   gap1 = 4, gap2 = 8, gap3 = 14;

    /** JUCE 8 deprecated Font(height, styleFlags), so all UI text goes through here. */
    inline juce::Font font (float height, int style = juce::Font::plain)
    {
        return juce::Font (juce::FontOptions (height, style));
    }

    inline const std::array<Palette, 4>& all()
    {
        static const std::array<Palette, 4> skins = []
        {
            Palette dark;
            dark.name = "Dark";
            dark.background = juce::Colour (0xff181b20);
            dark.panel = juce::Colour (0xff22262d);
            dark.panelDeep = juce::Colour (0xff12151a);
            dark.panelOutline = juce::Colour (0xff333a44);
            dark.gridLine = juce::Colour (0xff2c313a);
            dark.gridLineStrong = juce::Colour (0xff3d4451);
            dark.text = juce::Colour (0xffc8d0dc);
            dark.textDim = juce::Colour (0xff7a8492);
            dark.textTick = juce::Colour (0xff8f99a8);
            dark.accent = juce::Colour (0xff4ac8e8);
            dark.wave = juce::Colour (0xff4ac8e8);
            dark.potential = juce::Colour (0xffe8a03c);
            dark.env = juce::Colour (0xffb06ae8);
            dark.filter = juce::Colour (0xff56d384);
            dark.fx = juce::Colour (0xffe8564a);
            dark.harmony = juce::Colour (0xffb06ae8);
            dark.knobTrack = juce::Colour (0xff3a4150);
            dark.knobHi = juce::Colour (0xff464e5c);
            dark.knobLo = juce::Colour (0xff1b1e24);

            Palette neon;
            neon.name = "SciFi Neon";
            neon.background = juce::Colour (0xff050a12);
            neon.panel = juce::Colour (0xff0a1624);
            neon.panelDeep = juce::Colour (0xff07101a);
            neon.panelOutline = juce::Colour (0xff1a3a5a);
            neon.gridLine = juce::Colour (0xff0f2338);
            neon.gridLineStrong = juce::Colour (0xff1a3a5a);
            neon.text = juce::Colour (0xffbfe9ff);
            neon.textDim = juce::Colour (0xff5a7a99);
            neon.textTick = juce::Colour (0xff5a7a99);
            neon.accent = juce::Colour (0xff00e5ff);
            neon.wave = juce::Colour (0xff00e5ff);
            neon.potential = juce::Colour (0xffffb03b);
            neon.env = juce::Colour (0xff4aa8ff);
            neon.filter = juce::Colour (0xff00e5ff);
            neon.fx = juce::Colour (0xff4aa8ff);
            neon.harmony = juce::Colour (0xff4aa8ff);
            neon.knobTrack = juce::Colour (0xff10253a);
            neon.knobHi = juce::Colour (0xff12263a);
            neon.knobLo = juce::Colour (0xff06101a);
            neon.neonGlow     = true;

            Palette console;
            console.name = "Console";
            console.background = juce::Colour (0xff14120e);
            console.panel = juce::Colour (0xff1e1b14);
            console.panelDeep = juce::Colour (0xff0e0c08);
            console.panelOutline = juce::Colour (0xff3a3424);
            console.gridLine = juce::Colour (0xff26221a);
            console.gridLineStrong = juce::Colour (0xff3a3424);
            console.text = juce::Colour (0xffe8dcc0);
            console.textDim = juce::Colour (0xff9a8f74);
            console.textTick = juce::Colour (0xff9a8f74);
            console.accent = juce::Colour (0xffffb03b);
            console.wave = juce::Colour (0xffe8a03c);
            console.potential = juce::Colour (0xffffb03b);
            console.env = juce::Colour (0xffd08830);
            console.filter = juce::Colour (0xffc8b060);
            console.fx = juce::Colour (0xffe8783c);
            console.harmony = juce::Colour (0xffd0a048);
            console.knobTrack = juce::Colour (0xff3a3424);
            console.knobHi = juce::Colour (0xff332f24);
            console.knobLo = juce::Colour (0xff14120e);

            // "Golden": warm gold and bronze on graphite. Accents sit one
            // golden-section turn apart in hue so every group still reads
            // distinct while the whole skin stays inside one metal family.
            Palette gold;
            gold.name = "Golden";
            gold.background = juce::Colour (0xff1a1712);
            gold.panel = juce::Colour (0xff252118);
            gold.panelDeep = juce::Colour (0xff14110c);
            gold.panelOutline = juce::Colour (0xff463d2b);
            gold.gridLine = juce::Colour (0xff2f2a1e);
            gold.gridLineStrong = juce::Colour (0xff463d2b);
            gold.text = juce::Colour (0xffe8dfc8);
            gold.textDim = juce::Colour (0xff96896c);
            gold.textTick = juce::Colour (0xffa89b7c);
            gold.accent = juce::Colour (0xffe8b54a);
            gold.wave = juce::Colour (0xffe8b54a);
            gold.potential = juce::Colour (0xffc98a3e);
            gold.env = juce::Colour (0xff9fc39a);
            gold.filter = juce::Colour (0xffd8b968);
            gold.fx = juce::Colour (0xffcf7f4a);
            gold.harmony = juce::Colour (0xffe6d28e);
            gold.knobTrack = juce::Colour (0xff4a4128);
            gold.knobHi = juce::Colour (0xff5c5133);
            gold.knobLo = juce::Colour (0xff191510);
            gold.corner = 7.0f;

            std::array<Palette, 4> result { dark, neon, console, gold };
        return result;
        }();
        return skins;
    }

    inline std::atomic<int>& indexRef()
    {
        static std::atomic<int> index { 0 };
        return index;
    }

    inline int currentIndex() { return indexRef().load (std::memory_order_relaxed); }
    inline void select (int i) { indexRef().store (juce::jlimit (0, (int) all().size() - 1, i), std::memory_order_relaxed); }
    inline const Palette& current() { return all()[(std::size_t) currentIndex()]; }
    inline const juce::String& currentName() { return current().name; }
}

/** Rounded panel with title, used by every container in the editor. */
inline void drawSkinnedPanel (juce::Graphics& g, juce::Rectangle<float> r,
                              const Palette& pal, const juce::String& title,
                              juce::Colour accent, float titleFontHeight)
{
    g.setColour (pal.panel);
    g.fillRoundedRectangle (r, pal.corner);
    g.setColour (pal.panelOutline);
    g.drawRoundedRectangle (r.reduced (0.5f), pal.corner, 1.0f);

    if (title.isNotEmpty())
    {
        auto tr = r.reduced (Skins::gap2).removeFromTop (titleFontHeight + 8.0f);
        g.setColour (accent.withAlpha (0.9f));
        g.fillRoundedRectangle (tr.removeFromLeft (3.0f).withHeight (titleFontHeight + 2.0f), 1.5f);
        tr.removeFromLeft (5.0f);
        g.setFont (Skins::font (titleFontHeight, juce::Font::bold));
        g.setColour (accent.brighter (0.55f));
        g.drawText (title.toUpperCase(), tr, juce::Justification::centredLeft, false);
    }
}
