#include "PluginEditor.h"
#include "gui/Palette.h"

const juce::Rectangle<int> PluginEditor::headerArea { 12, 8, 1256, 36 };
const juce::Rectangle<int> PluginEditor::canvasArea { 12, 48, 776, 540 };
const juce::Rectangle<int> PluginEditor::adsrArea  { 12, 594, 776, 230 };
const juce::Rectangle<int> PluginEditor::harmonyArea { 12, 830, 776, 82 };
const juce::Rectangle<int> PluginEditor::waveArea  { 804, 48, 464, 168 };
const juce::Rectangle<int> PluginEditor::potArea   { 804, 224, 464, 168 };
const juce::Rectangle<int> PluginEditor::envArea   { 804, 400, 464, 168 };
const juce::Rectangle<int> PluginEditor::filterArea { 804, 576, 464, 168 };
const juce::Rectangle<int> PluginEditor::fxArea    { 804, 752, 464, 160 };

juce::Colour PluginEditor::accentFor (Group g)
{
    const Palette& pal = Skins::current();
    switch (g)
    {
        case Group::Wave:      return pal.wave;
        case Group::Potential: return pal.potential;
        case Group::Env:       return pal.env;
        case Group::Filter:    return pal.filter;
        case Group::Fx:        return pal.fx;
        case Group::Harmony:   return pal.harmony;
    }
    return pal.accent;
}

namespace
{
    using Fn = ValueSlider::FormatFn;

    Fn percent   = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    Fn signedPercent = [] (double v) { return (v >= 0.0 ? "+" : "") + juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    Fn msText    = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    Fn hzText    = [] (double v) { if (v >= 1000.0) return juce::String (v / 1000.0, 1) + " kHz";
                                   return juce::String (juce::roundToInt (v)) + " Hz"; };
    Fn dtText    = [] (double v) { return juce::String (v, 3); };
    Fn stepText  = [] (double v) { return juce::String (v, 1) + " smp"; };
    Fn gainText  = [] (double v) { return juce::String (v, 2); };
    Fn bitsText  = [] (double v) { return juce::String (juce::roundToInt (v)) + " bit"; };
    Fn rateText  = [] (double v) { return juce::String (v, 2) + " Hz"; };
    Fn semitoneText = [] (double v) { return (v >= 0.0 ? "+" : "") + juce::String (v / 100.0, 1) + " st"; };
    Fn plainPct  = [] (double v) { return juce::String (v, 2); };

    struct KnobSpec
    {
        const char* id;
        const char* title;
        const char* tip;
        ValueSlider::FormatFn format;
    };
}

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), canvas (p), adsrCanvas (p), presetBrowser (p.apvts)
{
    setLookAndFeel (&lnf);
    setResizable (true, true);
    setSize (baseW, baseH);
    setResizeLimits (1024, 740, 1700, 1100);

    title.setText ("SPIRALQUANT", juce::dontSendNotification);
    addAndMakeVisible (title);
    addAndMakeVisible (logo);

    versionLabel.setText ("v" SPIRALQUANT_VERSION, juce::dontSendNotification);
    addAndMakeVisible (versionLabel);

    status.setText ("quantum wavetable synth", juce::dontSendNotification);
    addAndMakeVisible (status);

    addAndMakeVisible (canvas);
    addAndMakeVisible (adsrCanvas);
    addAndMakeVisible (presetBrowser);

    for (int i = 0; i < (int) Skins::all().size(); ++i)
        skinCombo.addItem (Skins::all()[(size_t) i].name, i + 1);
    skinCombo.setTooltip ("Interface colour scheme (saved with the preset).");
    addAndMakeVisible (skinCombo);

    waveTypeChip.setButtonText ("WAVE");
    potentialTypeChip.setButtonText ("POT");
    for (auto* b : { &waveTypeChip, &potentialTypeChip })
    {
        b->setConnectedEdges (juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
        addAndMakeVisible (*b);
    }
    waveTypeChip.onClick = [this] { canvas.cycleWaveType(); repaint(); };
    potentialTypeChip.onClick = [this] { canvas.cyclePotentialType(); repaint(); };
    waveTypeChip.setTooltip ("Initial wavefunction shape (click to cycle).");
    potentialTypeChip.setTooltip ("Potential the wavefunction evolves in (click to cycle).");

    showFFT.setButtonText ("FFT");
    applyQm.setButtonText ("QM");
    chordChip.setButtonText ("CHORD");
    tunnelChip.setButtonText ("TUNNEL");
    superposeChip.setButtonText ("SUPERPOSE");
    walkChip.setButtonText ("WALK");
    eprChip.setButtonText ("EPR");
    limiterChip.setButtonText ("LIMIT");
    for (auto* b : { &showFFT, &applyQm, &chordChip, &tunnelChip, &superposeChip, &walkChip, &limiterChip, &eprChip })
        addAndMakeVisible (*b);
    showFFT.setTooltip ("Freeze evolution and read the momentum-domain (FFT) wave.");
    applyQm.setTooltip ("Master switch for real-time Schrodinger evolution. Off freezes the wave.");
    chordChip.setTooltip ("Play harmonically tuned chords from every single note.");
    tunnelChip.setTooltip ("Quantum tunneling: the wavefunction leaks through potential barriers.");
    superposeChip.setTooltip ("Chord superposition: each note holds two voicings at once; note-off measures (collapses) it to one.");
    walkChip.setTooltip ("Quantum random walk: the engine composes its own melody, measuring a Hadamard walk over pentatonic degrees at every tick.");
    eprChip.setTooltip ("Entangled voices: a quantum jump on one voice instantly collapses its partners sharing the same note (needs COLLAPSE > 0).");
    limiterChip.setTooltip ("Output limiter: pulls peaks above -1 dBFS down with a fast attack and slow release, so chords and drive can never spike the volume.");

    sampleMode.addItemList ({ "Real","Imag","Magnitude","Phase","PhononQND" }, 1);
    fxType.addItemList ({ "Off","Flanger","Phaser","Bitcrush","Fractal" }, 1);
    fxSolo.addItemList ({ "Full","Dry","Wet" }, 1);
    chordType.addItemList ({ "Power","Major","Minor","Sus4","Major7","Tritone","Cluster","Pythagorean" }, 1);
    for (auto* c : { &sampleMode, &fxType, &fxSolo, &chordType })
        addAndMakeVisible (*c);
    sampleMode.setTooltip ("Which part of the complex wave becomes audio.");
    fxType.setTooltip ("Master effect applied after the voice mix.");
    fxSolo.setTooltip ("Full = dry+wet, Dry = effect off, Wet = effect only.");
    chordType.setTooltip ("Chord voicing. Consonant chords use small-integer (just) ratios, "
                          "dissonant ones use large coprime ratios.");

    struct GroupSpecs
    {
        Group group;
        std::initializer_list<KnobSpec> specs;
    };
    const GroupSpecs groups[] = {
        { Group::Wave, {
            { "waveShift",    "SHIFT",   "Centre of the initial wavefunction in the well.", percent },
            { "waveScale",    "WIDTH",   "Width of the wave packet / spatial frequency.", percent },
            { "waveImpulse",  "IMPULSE", "Momentum kick baked into the initial wave.", percent },
            { "timestepDelta","DT",      "Schrodinger evolution step dt. Smaller = smoother wave, more CPU.", dtText },
            { "samplesPerTimestep", "STEP", "Audio samples between evolution steps. Lower = faster wave.", stepText },
            { "jumpRate",   "COLLAPSE", "Phonon-collapse rate: random quantum jumps squeeze the wave (Monte-Carlo wavefunction).", percent },
            { "frftAngle",  "QUAD",     "Quadrature rotation of the readout: 0 reads psi(x), 1 reads the momentum state psi(k).", percent },
            { "zeno",       "ZENO",     "Quantum Zeno effect: frequent position measurements freeze the wave's evolution.", percent },
            { "squeeze",    "SQUEEZE",  "Squeezed states: positive narrows the wave in space (wider in k), negative narrows it in momentum.", signedPercent } } },
        { Group::Potential, {
            { "potentialShift",  "CENTER", "Where the potential sits.", percent },
            { "potentialScale",  "WIDTH",  "Barrier width / curvature.", percent },
            { "potentialHeight", "HEIGHT", "Barrier depth.", percent },
            { "memoryAmt",   "MEMORY", "Non-Markovian self-coupling: the wave feels its own density from LAG ago.", percent },
            { "memoryTime",  "LAG",    "Delay tau of the resonator memory.", msText },
            { "ringdown",    "RING",   "Cavity ringdown: phonon loss fades the readout with a T1 tail.", percent } } },
        { Group::Env, {
            { "attack",  "ATTACK",  "Time to full amplitude in ms.", msText },
            { "decay",   "DECAY",   "Time to the sustain level in ms.", msText },
            { "sustain", "SUSTAIN", "Held level after decay.", percent },
            { "release", "RELEASE", "Time to silence in ms.", msText } } },
        { Group::Filter, {
            { "filterFreq", "CUTOFF", "Low-pass cutoff applied to every voice.", hzText },
            { "filterRes",  "RESO",   "Resonance peak at the cutoff frequency.", percent },
            { "drive",      "DRIVE",  "Pre-emphasis feeding the soft-clipper.", percent },
            { "satMix",     "SAT",    "Dry/wet balance of the saturation.", percent },
            { "gain",       "GAIN",   "Per-voice output gain, applied after saturation.", gainText } } },
        { Group::Fx, {
            { "fxDepth",    "DEPTH",  "Effect depth (delay time / phase spread). On Fractal: how many tonic periods the tail may span.", percent },
            { "fxRate",     "RATE",   "Effect modulation rate. On Fractal: how often the comb re-tunes to a new tonic period.", rateText },
            { "fxFeedback", "FDBK",   "Effect feedback.", percent },
            { "fxMix",      "MIX",    "Dry/wet balance of the master effect.", percent },
            { "fxBits",     "BITS",   "Bitcrush quantisation depth.", bitsText },
            { "fxEntropy",  "CHAOS",  "Fractal entropy of the chaos effect.", percent } } },
        { Group::Harmony, {
            { "pitchShift", "PITCH",  "Global transposition in semitones (-48 to +48 st), applied as 2^(st/12).", semitoneText },
            { "walkRate",   "RATE",   "Quantum-walk measurement ticks per second.", rateText } } },
    };

    for (const auto& g : groups)
        for (const auto& s : g.specs)
            addKnob (s.id, s.title, s.tip, g.group, s.format);

    showFFTAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "showFFT", showFFT);
    applyQmAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "applyWavefunction", applyQm);
    chordAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "chordMode", chordChip);
    superposeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "chordSuperpose", superposeChip);
    tunnelAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "tunnel", tunnelChip);
    walkAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "walkOn", walkChip);
    limiterAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "limiter", limiterChip);
    eprAtt      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "entangle", eprChip);
    sampleModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "sampleMode", sampleMode);
    fxTypeAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "fxType", fxType);
    fxSoloAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "fxSolo", fxSolo);
    chordTypeAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "chordType", chordType);
    uiSkinAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "uiSkin", skinCombo);

    applySkin();
    resized();  // setSize() above ran before the knobs existed; lay out once now.
    startTimerHz (20);
}

PluginEditor::~PluginEditor() { setLookAndFeel (nullptr); stopTimer(); }

void PluginEditor::addKnob (const char* id, const juce::String& titleText, const juce::String& tooltip,
                            Group group, ValueSlider::FormatFn format)
{
    auto* param = processor.apvts.getParameter (id);
    if (param == nullptr)
        return;
    Knob k;
    k.id = id;
    k.group = group;
    k.slider = std::make_unique<ValueSlider>();
    const auto& nr = param->getNormalisableRange();
    k.slider->setRange (nr.convertFrom0to1 (0.0), nr.convertFrom0to1 (1.0), 0.0);
    k.slider->format = format;
    k.slider->caption = titleText;
    k.slider->setTooltip (tooltip);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, *k.slider);

    k.nameLabel = std::make_unique<juce::Label>();
    k.nameLabel->setText (titleText.toUpperCase(), juce::dontSendNotification);
    k.nameLabel->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (*k.nameLabel);

    k.valueLabel = std::make_unique<juce::Label>();
    k.valueLabel->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (*k.valueLabel);

    addAndMakeVisible (*k.slider);
    if (group == Group::Fx)
        fxKnobs.push_back (k.slider.get());
    if (juce::String (id) == "pitchShift")
        pitchKnob = k.slider.get();
    knobs.push_back (std::move (k));
}

juce::Rectangle<int> PluginEditor::scaled (juce::Rectangle<int> r) const
{
    return { originX + (int) (r.getX() * uiScale), originY + (int) (r.getY() * uiScale),
             juce::jmax (1, (int) (r.getWidth() * uiScale)), juce::jmax (1, (int) (r.getHeight() * uiScale)) };
}

void PluginEditor::layoutKnob (const Knob& k, juce::Rectangle<int> cell)
{
    const int textH = juce::jmax (10, (int) (Skins::fontValue * uiScale) + 2);
    const int knobD = juce::jmax (20, juce::jmin (cell.getWidth() - 4, cell.getHeight() - textH * 2 - 4));
    k.slider->setBounds (cell.getX() + (cell.getWidth() - knobD) / 2, cell.getY(), knobD, knobD);
    auto below = cell.withY (cell.getY() + knobD).withHeight (cell.getHeight() - knobD);
    k.nameLabel->setBounds (below.removeFromTop (textH));
    k.valueLabel->setBounds (below);
}

void PluginEditor::resized()
{
    uiScale = juce::jmax (0.001f, juce::jmin ((float) getWidth() / (float) baseW,
                                              (float) getHeight() / (float) baseH));
    originX = (getWidth()  - (int) ((float) baseW * uiScale)) / 2;
    originY = (getHeight() - (int) ((float) baseH * uiScale)) / 2;
    lnf.uiScale = uiScale;

    auto header = scaled (headerArea);
    logo.setBounds (header.removeFromLeft (34));
    title.setBounds (header.removeFromLeft ((int) (240 * uiScale)).withY (header.getY() - 8));
    title.setFont (Skins::font (Skins::fontTitle * uiScale * 1.4f, juce::Font::bold));
    versionLabel.setBounds (header.removeFromLeft ((int) (90 * uiScale)).withY (header.getY() + 2));
    versionLabel.setFont (Skins::font (Skins::fontTick * uiScale));
    status.setBounds (header.withHeight (20));
    status.setFont (Skins::font (Skins::fontTick * uiScale));
    auto right = header;
    skinCombo.setBounds (scaled ({ 1136, 8, 132, 28 }));
    presetBrowser.setBounds (scaled ({ 672, 8, 456, 28 }));

    canvas.setBounds (scaled (canvasArea));
    adsrCanvas.setBounds (scaled (adsrArea));

    auto layoutPanel = [this] (juce::Rectangle<int> area, bool hasChips,
                               std::function<void (juce::Rectangle<int>)> chips,
                               std::initializer_list<const char*> ids)
    {
        auto inner = scaled (area).reduced (juce::jmax (2, (int) (6 * uiScale)));
        inner.removeFromTop (juce::jmax (6, (int) (18 * uiScale)));  // title zone
        if (hasChips && chips)
        {
            auto chipRow = inner.removeFromTop (juce::jmax (16, (int) (26 * uiScale)));
            chipRow.removeFromTop (2);
            chips (chipRow);
            inner.removeFromTop (4);
        }
        const int n = (int) ids.size();
        if (n == 0) return;
        const int cellW = inner.getWidth() / n;
        int i = 0;
        for (auto* id : ids)
        {
            for (const auto& k : knobs)
                if (k.id == id)
                    layoutKnob (k, inner.withX (inner.getX() + i * cellW).withWidth (cellW));
            ++i;
        }
    };

    layoutPanel (waveArea, true, [this] (juce::Rectangle<int> r) {
        waveTypeChip.setBounds (r.removeFromLeft (juce::jmin (r.getWidth() - 220, (int) (150 * uiScale))).reduced (2));
        showFFT.setBounds (r.removeFromLeft ((int) (52 * uiScale)).reduced (2));
        applyQm.setBounds (r.removeFromLeft ((int) (56 * uiScale)).reduced (2));
        sampleMode.setBounds (r.removeFromRight ((int) (116 * uiScale)).reduced (2));
    }, { "waveShift", "waveScale", "waveImpulse", "timestepDelta", "samplesPerTimestep", "jumpRate", "frftAngle", "zeno", "squeeze" });

    layoutPanel (potArea, true, [this] (juce::Rectangle<int> r) {
        potentialTypeChip.setBounds (r.removeFromLeft (juce::jmin (r.getWidth() - 120, (int) (160 * uiScale))).reduced (2));
        tunnelChip.setBounds (r.removeFromRight ((int) (92 * uiScale)).reduced (2));
    }, { "potentialShift", "potentialScale", "potentialHeight", "memoryAmt", "memoryTime", "ringdown" });

    layoutPanel (envArea, false, nullptr, { "attack", "decay", "sustain", "release" });
    layoutPanel (filterArea, false, nullptr, { "filterFreq", "filterRes", "drive", "satMix", "gain" });

    layoutPanel (fxArea, true, [this] (juce::Rectangle<int> r) {
        fxType.setBounds (r.removeFromLeft ((int) (130 * uiScale)).reduced (2));
        limiterChip.setBounds (r.removeFromRight ((int) (72 * uiScale)).reduced (2));
        fxSolo.setBounds (r.removeFromRight ((int) (100 * uiScale)).reduced (2));
    }, { "fxDepth", "fxRate", "fxFeedback", "fxMix", "fxBits", "fxEntropy" });

    // harmony strip
    {
        auto h = scaled (harmonyArea).reduced (juce::jmax (2, (int) (6 * uiScale)));
        h.removeFromLeft ((int) (86 * uiScale)); // title zone
        chordChip.setBounds (h.removeFromLeft ((int) (76 * uiScale)).reduced (2));
        chordType.setBounds (h.removeFromLeft ((int) (140 * uiScale)).reduced (2));
        superposeChip.setBounds (h.removeFromLeft ((int) (112 * uiScale)).reduced (2));
        walkChip.setBounds (h.removeFromLeft ((int) (70 * uiScale)).reduced (2));
        eprChip.setBounds (h.removeFromLeft ((int) (58 * uiScale)).reduced (2));
        for (const char* id : { "pitchShift", "walkRate" })
        {
            auto knobCell = h.withWidth (juce::jmin (h.getWidth(), (int) (90 * uiScale)));
            if (knobCell.getWidth() >= (int) (50 * uiScale))
                for (const auto& k : knobs)
                    if (k.id == id)
                        layoutKnob (k, knobCell);
            h.removeFromLeft (knobCell.getWidth() + (int) (6 * uiScale));
        }
    }

    setGroupAlpha();
}

void PluginEditor::setGroupAlpha()
{
    const bool fxOff = fxType.getSelectedId() <= 1;
    for (auto* s : fxKnobs)
        s->setAlpha (fxOff ? 0.45f : 1.0f);
    fxSolo.setAlpha (fxOff ? 0.45f : 1.0f);
}

void PluginEditor::applySkin()
{
    const int idx = juce::roundToInt (processor.apvts.getRawParameterValue ("uiSkin")->load());
    Skins::select (idx);
    const Palette& pal = Skins::current();
    lnf.refreshColours();
    lnf.uiScale = uiScale;

    title.setColour (juce::Label::textColourId, pal.wave);
    title.setFont (Skins::font (Skins::fontTitle * uiScale * 1.4f, juce::Font::bold));
    versionLabel.setColour (juce::Label::textColourId, pal.textDim);
    status.setColour (juce::Label::textColourId, pal.textDim);

    waveTypeChip.setColour (juce::TextButton::buttonOnColourId, accentFor (Group::Wave).withAlpha (0.16f));
    potentialTypeChip.setColour (juce::TextButton::buttonOnColourId, accentFor (Group::Potential).withAlpha (0.16f));
    showFFT.setColour (juce::ToggleButton::tickColourId, accentFor (Group::Wave));
    applyQm.setColour (juce::ToggleButton::tickColourId, accentFor (Group::Wave));
    tunnelChip.setColour (juce::ToggleButton::tickColourId, accentFor (Group::Potential));
    chordChip.setColour (juce::ToggleButton::tickColourId, accentFor (Group::Harmony));
    superposeChip.setColour (juce::ToggleButton::tickColourId, accentFor (Group::Harmony));
    walkChip.setColour (juce::ToggleButton::tickColourId, accentFor (Group::Harmony));
    eprChip.setColour (juce::ToggleButton::tickColourId, accentFor (Group::Harmony));
    limiterChip.setColour (juce::ToggleButton::tickColourId, accentFor (Group::Fx));
    for (auto* b : { &showFFT, &applyQm, &tunnelChip, &chordChip, &superposeChip, &walkChip, &limiterChip, &eprChip })
        b->setColour (juce::ToggleButton::textColourId, pal.text);

    for (auto& k : knobs)
    {
        const auto accent = accentFor (k.group);
        k.slider->setColour (juce::Slider::rotarySliderFillColourId, accent);
        k.nameLabel->setColour (juce::Label::textColourId, pal.textTick);
        k.nameLabel->setFont (Skins::font (Skins::fontValue * uiScale + 1.0f, juce::Font::bold));
        k.valueLabel->setColour (juce::Label::textColourId, accent.brighter (0.5f));
        k.valueLabel->setFont (Skins::font (Skins::fontValue * uiScale));
        k.lastText.clear();
    }

    repaint();
}

void PluginEditor::timerCallback()
{
    const int skinIdx = juce::roundToInt (processor.apvts.getRawParameterValue ("uiSkin")->load());
    if (skinIdx != Skins::currentIndex())
    {
        applySkin();
        resized();
    }

    if (auto* p = processor.apvts.getParameter ("waveType"))
    {
        const juce::String t = "WAVE: " + p->getText (juce::roundToInt (p->getValue() * 5.0f), 0).toUpperCase();
        if (waveTypeChip.getButtonText() != t)
            waveTypeChip.setButtonText (t);
    }
    if (auto* p = processor.apvts.getParameter ("potentialType"))
    {
        const juce::String t = "POT: " + p->getText (juce::roundToInt (p->getValue() * 5.0f), 0).toUpperCase();
        if (potentialTypeChip.getButtonText() != t)
            potentialTypeChip.setButtonText (t);
    }

    for (auto& k : knobs)
    {
        const juce::String t = k.slider->valueText();
        if (t != k.lastText)
        {
            k.lastText = t;
            k.valueLabel->setText (t, juce::dontSendNotification);
        }
    }

    setGroupAlpha();

    const bool chordOn = chordChip.getToggleState();
    if (chordType.isEnabled() != chordOn)
    {
        chordType.setAlpha (chordOn ? 1.0f : 0.45f);
        chordType.setEnabled (chordOn);
        superposeChip.setAlpha (chordOn ? 1.0f : 0.45f);
    }

    const bool jumpsOn = processor.apvts.getRawParameterValue ("jumpRate")->load() > 0.0f;
    if (eprChip.isEnabled() != jumpsOn)
    {
        eprChip.setAlpha (jumpsOn ? 1.0f : 0.45f);
        eprChip.setEnabled (jumpsOn);
    }
}

void PluginEditor::paint (juce::Graphics& g)
{
    const Palette& pal = Skins::current();
    g.fillAll (pal.background);
    const float fs = Skins::fontLabel * uiScale;

    drawSkinnedPanel (g, scaled (waveArea).toFloat(),    pal, "Wavefunction", accentFor (Group::Wave),      fs);
    drawSkinnedPanel (g, scaled (potArea).toFloat(),     pal, "Potential",    accentFor (Group::Potential), fs);
    drawSkinnedPanel (g, scaled (envArea).toFloat(),     pal, "Envelope",     accentFor (Group::Env),       fs);
    drawSkinnedPanel (g, scaled (filterArea).toFloat(),  pal, "Filter & Drive", accentFor (Group::Filter),  fs);
    drawSkinnedPanel (g, scaled (fxArea).toFloat(),      pal, "Master FX",    accentFor (Group::Fx),        fs);
    drawSkinnedPanel (g, scaled (harmonyArea).toFloat(), pal, "Harmony",      accentFor (Group::Harmony),   fs);

    const auto line = scaled ({ 12, 44, 1256, 1 }).toFloat();
    g.setColour (pal.accent.withAlpha (0.35f));
    g.fillRect (line);
}
