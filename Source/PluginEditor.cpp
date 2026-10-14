#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), canvas (p), adsrCanvas (p), presetBrowser (p.apvts)
{
    setLookAndFeel (&lnf);
    setSize (1280, 840);
    setResizeLimits (980, 680, 1700, 1000);

    title.setText ("SPIRALQUANT", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, SciFiTheme::neon);
    addAndMakeVisible (title);
    addAndMakeVisible (logo);

    status.setText ("", juce::dontSendNotification);
    status.setFont (juce::Font (juce::FontOptions (11.0f)));
    status.setColour (juce::Label::textColourId, SciFiTheme::textDim);
    addAndMakeVisible (status);

    versionLabel.setText ("v" SPIRALQUANT_VERSION "  quantum wavetable synth", juce::dontSendNotification);
    versionLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    versionLabel.setColour (juce::Label::textColourId, SciFiTheme::textDim);
    versionLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (versionLabel);

    addAndMakeVisible (canvas);
    addAndMakeVisible (adsrCanvas);

    waveTypeChip.setButtonText ("WAVE: GAUSSIAN");
    potentialTypeChip.setButtonText ("POT: HARMONIC");
    for (auto* b : { &waveTypeChip, &potentialTypeChip })
    {
        b->setLookAndFeel (&lnf);
        addAndMakeVisible (*b);
    }
    waveTypeChip.onClick = [this] { canvas.cycleWaveType(); repaint(); };
    potentialTypeChip.onClick = [this] { canvas.cyclePotentialType(); repaint(); };

    showFFT.setButtonText ("FFT");
    applyQm.setButtonText ("QM");
    chordChip.setButtonText ("CHORD");
    tunnelChip.setButtonText ("TUNNEL");
    for (auto* b : { &showFFT, &applyQm, &chordChip, &tunnelChip }) { b->setLookAndFeel (&lnf); addAndMakeVisible (*b); }
    chordChip.setTooltip ("Play harmonically tuned chords from every single note.");
    tunnelChip.setTooltip ("Quantum tunneling: the wavefunction leaks through potential barriers.");

    sampleMode.addItemList ({ "Real","Imag","Magnitude","Phase" }, 1);
    fxType.addItemList ({ "Off","Flanger","Phaser","Bitcrush","Fractal" }, 1);
    fxSolo.addItemList ({ "Full","Dry","Wet" }, 1);
    chordType.addItemList ({ "Power","Major","Minor","Sus4","Major7","Tritone","Cluster","Pythagorean" }, 1);
    for (auto* c : { &sampleMode, &fxType, &fxSolo, &chordType }) { c->setLookAndFeel (&lnf); addAndMakeVisible (*c); }
    chordType.setTooltip ("Chord voicing. Consonant chords use small-integer (just) ratios, "
                          "dissonant ones use large coprime ratios.");
    presetBrowser.setLookAndFeel (&lnf);
    addAndMakeVisible (presetBrowser);

    auto config = [this] (ValueSlider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
        addAndMakeVisible (s);
    };
    for (auto* s : { &filterFreq,&filterRes,&gain,&fxMix,&timestepDelta,&samplesPerTimestep,&pitchShift })
        config (*s);

    filterFreq.setRange (20.0, 20000.0, 0.0);
    filterRes.setRange (0.0, 1.0, 0.0);
    gain.setRange (0.0, 2.0, 0.0);
    fxMix.setRange (0.0, 1.0, 0.0);
    timestepDelta.setRange (0.001, 0.2, 0.0);
    samplesPerTimestep.setRange (0.1, 64.0, 0.0);
    pitchShift.setRange (-2400.0, 2400.0, 0.0);

    filterFreq.setLabelText ("FILTER FREQ");
    filterRes.setLabelText ("RESO");
    gain.setLabelText ("OUTPUT");
    fxMix.setLabelText ("FX MIX");
    timestepDelta.setLabelText ("TIMESTEP");
    samplesPerTimestep.setLabelText ("EVO RATE");
    pitchShift.setLabelText ("PITCH");

    pitchShift.setTooltip ("Global transposition in cents (100 = one semitone), applied as 2^(cents/1200).");

    filterFreq.setTooltip ("Low-pass cutoff applied to every voice.");
    filterRes.setTooltip ("Resonance peak at the cutoff frequency.");
    gain.setTooltip ("Per-voice output gain, applied after saturation.");
    fxMix.setTooltip ("Dry/wet balance of the master effect.");
    timestepDelta.setTooltip ("Schrodinger evolution step dt. Smaller = smoother wave, more CPU.");
    samplesPerTimestep.setTooltip ("Audio samples between evolution steps. Lower = faster wave.");
    waveTypeChip.setTooltip ("Initial wavefunction shape (click to cycle).");
    potentialTypeChip.setTooltip ("Potential the wavefunction evolves in (click to cycle).");
    showFFT.setTooltip ("Freeze evolution and read the momentum-domain (FFT) wave.");
    applyQm.setTooltip ("Master switch for real-time Schrodinger evolution.");
    sampleMode.setTooltip ("Which part of the complex wave becomes audio.");
    fxType.setTooltip ("Master effect applied after the voice mix.");
    fxSolo.setTooltip ("Full = dry+wet, Dry = effect off, Wet = effect only.");

    showFFTAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "showFFT", showFFT);
    applyQmAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "applyWavefunction", applyQm);
    sampleModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "sampleMode", sampleMode);
    fxTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "fxType", fxType);
    fxSoloAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "fxSolo", fxSolo);
    filterFreqAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "filterFreq", filterFreq);
    filterResAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "filterRes", filterRes);
    gainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "gain", gain);
    fxMixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "fxMix", fxMix);
    timestepAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "timestepDelta", timestepDelta);
    evolRateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "samplesPerTimestep", samplesPerTimestep);
    chordAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "chordMode", chordChip);
    tunnelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "tunnel", tunnelChip);
    chordTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "chordType", chordType);
    pitchAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "pitchShift", pitchShift);

    auto percent = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    filterRes.format = percent;
    fxMix.format = percent;
    filterFreq.format = [] (double v) { if (v >= 1000.0) return juce::String (v/1000.0, 1) + "k"; return juce::String (juce::roundToInt (v)); };
    gain.format = [] (double v) { return juce::String (v, 2); };
    timestepDelta.format = [] (double v) { return juce::String (v, 3); };
    samplesPerTimestep.format = [] (double v) { return juce::String (v, 1); };
    pitchShift.format = [] (double v) { return (v >= 0.0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct"; };

    // Text boxes were created before the formatters existed; refresh them.
    for (auto* s : { &filterFreq,&filterRes,&gain,&fxMix,&timestepDelta,&samplesPerTimestep,&pitchShift })
        s->updateText();

    startTimer (120);
}

PluginEditor::~PluginEditor() { setLookAndFeel (nullptr); stopTimer(); }

void PluginEditor::timerCallback()
{
    const int wt = juce::roundToInt (processor.apvts.getRawParameterValue ("waveType")->load());
    const int pt = juce::roundToInt (processor.apvts.getRawParameterValue ("potentialType")->load());
    const juce::StringArray waveNames { "GAUSSIAN","SINE","COS","PARABOLA","SAW","SQUARE" };
    const juce::StringArray potNames  { "FLAT","HARMONIC","WELL","BARRIER","DOUBLEWELL","LINEAR" };
    waveTypeChip.setButtonText ("WAVE: " + waveNames[wt]);
    potentialTypeChip.setButtonText ("POT: " + potNames[pt]);

    const float waveShift = processor.apvts.getRawParameterValue ("waveShift")->load();
    const float waveScale = processor.apvts.getRawParameterValue ("waveScale")->load();
    const float potHeight = processor.apvts.getRawParameterValue ("potentialHeight")->load();
    status.setText ("wave shift " + juce::String (waveShift, 2) + "  scale " + juce::String (waveScale, 2) +
                    "    |    pot height " + juce::String (potHeight, 2), juce::dontSendNotification);
}

void PluginEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient grad (SciFiTheme::panelDeep.withAlpha (0.55f), 0.0f, 0.0f,
                               SciFiTheme::background, 0.0f, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillRect (getLocalBounds().toFloat());

    auto line = getLocalBounds().toFloat().removeFromTop (92);
    juce::ColourGradient glow (SciFiTheme::neon.withAlpha (0.55f), line.getX(), 0.0f,
                               SciFiTheme::neon.withAlpha (0.0f), line.getRight(), 0.0f, false);
    g.setGradientFill (glow);
    g.fillRect (line.removeFromBottom (1.0f));
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (14);

    auto header = area.removeFromTop (76);
    auto titleRow = header.removeFromTop (28);
    logo.setBounds (titleRow.removeFromLeft (32).reduced (2));
    title.setBounds (titleRow.withRight (titleRow.getRight() - 340));
    auto statusRow = header.removeFromTop (18).reduced (2, 0);
    versionLabel.setBounds (statusRow.removeFromLeft (300));
    versionLabel.setJustificationType (juce::Justification::centredLeft);
    status.setBounds (statusRow);
    presetBrowser.setBounds (header.withLeft (header.getRight() - 420).reduced (0, 2));
    area.removeFromTop (6);

    auto topRow = area.removeFromTop (30).reduced (0, 2);
    waveTypeChip.setBounds     (topRow.removeFromLeft (150).reduced (2));
    potentialTypeChip.setBounds(topRow.removeFromLeft (150).reduced (2));
    showFFT.setBounds          (topRow.removeFromLeft (64).reduced (2));
    applyQm.setBounds          (topRow.removeFromLeft (64).reduced (2));
    chordChip.setBounds        (topRow.removeFromLeft (80).reduced (2));
    tunnelChip.setBounds       (topRow.removeFromLeft (88).reduced (2));
    topRow.removeFromLeft (10);
    chordType.setBounds        (topRow.removeFromLeft (130).reduced (2));
    sampleMode.setBounds       (topRow.removeFromLeft (110).reduced (2));
    fxType.setBounds           (topRow.removeFromLeft (120).reduced (2));
    fxSolo.setBounds           (topRow.removeFromLeft (100).reduced (2));
    area.removeFromTop (10);

    const float canvasH = juce::jmax (220.0f, (float) area.getHeight() * 0.42f);
    canvas.setBounds (area.removeFromTop ((int) canvasH));
    area.removeFromTop (8);
    adsrCanvas.setBounds (area.removeFromTop (juce::jmax (120, area.getHeight() / 3)));
    area.removeFromTop (10);

    auto knobRow = [this] (juce::Rectangle<int> row, std::initializer_list<ValueSlider*> sliders, int labelH)
    {
        const int n = (int) sliders.size();
        const int cell = row.getWidth() / n;
        int i = 0;
        for (auto* s : sliders)
        {
            auto cellArea = row.withX (row.getX() + i * cell).withWidth (cell);
            const int knob = juce::jmin (cell - 12, row.getHeight() - labelH);
            s->setBounds (cellArea.withWidth (knob).withX (cellArea.getCentreX() - knob / 2)
                              .withHeight (knob).withY (row.getY() + labelH));
            ++i;
        }
    };

    auto row1 = area.removeFromTop (juce::jmin (120, area.getHeight() / 2));
    knobRow (row1.reduced (0, 2), { &filterFreq, &filterRes, &gain, &fxMix }, 14);
    area.removeFromTop (4);
    if (area.getHeight() > 96)
    {
        auto row2 = area.removeFromTop (juce::jmin (120, area.getHeight()));
        knobRow (row2.reduced (0, 2), { &timestepDelta, &samplesPerTimestep, &pitchShift }, 14);
    }
}
