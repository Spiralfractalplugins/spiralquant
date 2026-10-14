#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), scope (p)
{
    setLookAndFeel (&lnf);
    setSize (1280, 860);
    setResizeLimits (1000, 700, 1700, 1000);

    title.setText ("SPIRALQUANT", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, SciFiTheme::neon);
    addAndMakeVisible (title);

    subtitle.setText ("QUANTUM WAVEFUNCTION SYNTHESIZER // HUD", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (11.0f)));
    subtitle.setColour (juce::Label::textColourId, SciFiTheme::textDim);
    addAndMakeVisible (subtitle);

    addAndMakeVisible (scope);

    // combos
    waveType.addItemList ({ "Gaussian","Sine","Cos","Parabola","Saw","Square" }, 1);
    potentialType.addItemList ({ "Flat","Harmonic","Well","Barrier","DoubleWell","Linear" }, 1);
    sampleMode.addItemList ({ "Real","Imag","Magnitude","Phase" }, 1);
    fxType.addItemList ({ "Off","Flanger","Phaser","Bitcrush","Fractal" }, 1);
    fxSolo.addItemList ({ "Full","Dry","Wet" }, 1);
    for (auto* c : { &waveType, &potentialType, &sampleMode, &fxType, &fxSolo })
    {
        c->setLookAndFeel (&lnf);
        addAndMakeVisible (*c);
    }
    showFFT.setButtonText ("Show FFT");
    applyQm.setButtonText ("Apply QM");
    addAndMakeVisible (showFFT);
    addAndMakeVisible (applyQm);

    auto configSlider = [this] (ValueSlider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 18);
        addAndMakeVisible (s);
    };

    for (auto* s : { &waveShift,&waveScale,&waveImpulse,&potentialShift,&potentialScale,&potentialHeight,
                     &timestepDelta,&samplesPerTimestep,&attack,&decay,&sustain,&release,
                     &filterFreq,&filterRes,&drive,&satMix,&gain,
                     &fxDepth,&fxRate,&fxFeedback,&fxMix,&fxBits,&fxEntropy })
        configSlider (*s);

    timestepDelta.setRange (0.001, 0.2, 0.0);
    samplesPerTimestep.setRange (0.1, 64.0, 0.0);
    attack.setRange (1.0, 2000.0, 0.0);
    decay.setRange (1.0, 2000.0, 0.0);
    sustain.setRange (0.0, 1.0, 0.0);
    release.setRange (1.0, 5000.0, 0.0);
    filterFreq.setRange (20.0, 20000.0, 0.0);
    filterRes.setRange (0.0, 1.0, 0.0);
    drive.setRange (0.0, 1.0, 0.0);
    satMix.setRange (0.0, 1.0, 0.0);
    gain.setRange (0.0, 2.0, 0.0);
    fxDepth.setRange (0.0, 1.0, 0.0);
    fxRate.setRange (0.01, 20.0, 0.0);
    fxFeedback.setRange (0.0, 0.95, 0.0);
    fxMix.setRange (0.0, 1.0, 0.0);
    fxBits.setRange (2.0, 16.0, 0.0);
    fxEntropy.setRange (0.0, 1.0, 0.0);

    // attachments
    waveTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "waveType", waveType);
    potentialTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "potentialType", potentialType);
    sampleModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "sampleMode", sampleMode);
    fxTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "fxType", fxType);
    fxSoloAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "fxSolo", fxSolo);
    showFFTAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "showFFT", showFFT);
    applyQmAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "applyWavefunction", applyQm);

    waveShiftAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "waveShift", waveShift);
    waveScaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "waveScale", waveScale);
    waveImpulseAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "waveImpulse", waveImpulse);
    potentialShiftAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "potentialShift", potentialShift);
    potentialScaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "potentialScale", potentialScale);
    potentialHeightAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "potentialHeight", potentialHeight);
    timestepAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "timestepDelta", timestepDelta);
    evolRateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "samplesPerTimestep", samplesPerTimestep);
    attackAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "attack", attack);
    decayAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "decay", decay);
    sustainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "sustain", sustain);
    releaseAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "release", release);
    filterFreqAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "filterFreq", filterFreq);
    filterResAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "filterRes", filterRes);
    driveAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "drive", drive);
    satMixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "satMix", satMix);
    gainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "gain", gain);
    fxDepthAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "fxDepth", fxDepth);
    fxRateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "fxRate", fxRate);
    fxFeedbackAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "fxFeedback", fxFeedback);
    fxMixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "fxMix", fxMix);
    fxBitsAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "fxBits", fxBits);
    fxEntropyAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "fxEntropy", fxEntropy);

    // labels
    auto configureLabel = [] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        l.setColour (juce::Label::textColourId, SciFiTheme::textDim);
        l.setJustificationType (juce::Justification::centred);
    };
    configureLabel (waveShiftLabel, "WAVE SHIFT");
    configureLabel (waveScaleLabel, "WAVE SCALE");
    configureLabel (waveImpulseLabel, "IMPULSE");
    configureLabel (potentialShiftLabel, "POT SHIFT");
    configureLabel (potentialScaleLabel, "POT SCALE");
    configureLabel (potentialHeightLabel, "POT HEIGHT");
    configureLabel (timestepLabel, "TIMESTEP");
    configureLabel (evolRateLabel, "EVOL RATE");
    configureLabel (attackLabel, "ATTACK");
    configureLabel (decayLabel, "DECAY");
    configureLabel (sustainLabel, "SUSTAIN");
    configureLabel (releaseLabel, "RELEASE");
    configureLabel (filterFreqLabel, "FILTER");
    configureLabel (filterResLabel, "RES");
    configureLabel (driveLabel, "DRIVE");
    configureLabel (satMixLabel, "SAT MIX");
    configureLabel (gainLabel, "GAIN");
    configureLabel (fxDepthLabel, "DEPTH");
    configureLabel (fxRateLabel, "RATE");
    configureLabel (fxFeedbackLabel, "FEEDBACK");
    configureLabel (fxMixLabel, "MIX");
    configureLabel (fxBitsLabel, "BITS");
    configureLabel (fxEntropyLabel, "ENTROPY");

    for (auto* l : { &waveShiftLabel,&waveScaleLabel,&waveImpulseLabel,&potentialShiftLabel,&potentialScaleLabel,&potentialHeightLabel,
                     &timestepLabel,&evolRateLabel,&attackLabel,&decayLabel,&sustainLabel,&releaseLabel,
                     &filterFreqLabel,&filterResLabel,&driveLabel,&satMixLabel,&gainLabel,
                     &fxDepthLabel,&fxRateLabel,&fxFeedbackLabel,&fxMixLabel,&fxBitsLabel,&fxEntropyLabel })
        addAndMakeVisible (*l);

    // formats
    auto percent = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
    waveShift.format = percent;
    waveScale.format = percent;
    waveImpulse.format = percent;
    potentialShift.format = percent;
    potentialScale.format = percent;
    potentialHeight.format = percent;
    sustain.format = percent;
    filterRes.format = percent;
    drive.format = percent;
    satMix.format = percent;
    fxDepth.format = percent;
    fxMix.format = percent;
    fxEntropy.format = percent;

    timestepDelta.format = [] (double v) { return juce::String (v, 3); };
    samplesPerTimestep.format = [] (double v) { return juce::String (v, 1); };
    attack.format = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    decay.format = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    release.format = [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    filterFreq.format = [] (double v)
    {
        if (v >= 1000.0) return juce::String (v / 1000.0, 2) + " kHz";
        return juce::String (juce::roundToInt (v)) + " Hz";
    };
    gain.format = [] (double v) { return juce::String (v, 2); };
    fxRate.format = [] (double v) { return juce::String (v, 2) + " Hz"; };
    fxFeedback.format = [] (double v) { return juce::String (v, 2); };
    fxBits.format = [] (double v) { return juce::String (juce::roundToInt (v)) + " bit"; };
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (SciFiTheme::background);

    for (auto r : { waveArea, potArea, evolArea, voiceArea, fxArea })
    {
        g.setColour (SciFiTheme::panel);
        g.fillRoundedRectangle (r.toFloat(), SciFiTheme::corner);
        g.setColour (SciFiTheme::stroke);
        g.drawRoundedRectangle (r.toFloat(), SciFiTheme::corner, 1.0f);
    }

    paintPanel (g, waveArea, "WAVE GENERATOR");
    paintPanel (g, potArea, "POTENTIAL FIELD");
    paintPanel (g, evolArea, "EVOLUTION");
    paintPanel (g, voiceArea, "VOICE / FILTER");
    paintPanel (g, fxArea, "MASTER FX");
}

void PluginEditor::paintPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& name)
{
    g.setColour (SciFiTheme::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (name, r.reduced (10, 4).removeFromTop (16), juce::Justification::centredLeft, false);

    g.setColour (SciFiTheme::neon.withAlpha (0.6f));
    const int k = 8;
    g.drawLine ((float) r.getX(), (float) r.getY(), (float) (r.getX() + k), (float) r.getY(), 1.0f);
    g.drawLine ((float) r.getX(), (float) r.getY(), (float) r.getX(), (float) (r.getY() + k), 1.0f);
    g.drawLine ((float) r.getRight(), (float) r.getY(), (float) (r.getRight() - k), (float) r.getY(), 1.0f);
    g.drawLine ((float) r.getRight(), (float) r.getY(), (float) r.getRight(), (float) (r.getY() + k), 1.0f);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (8);

    title.setBounds (area.removeFromTop (28));
    subtitle.setBounds (area.removeFromTop (14));
    scope.setBounds (area.removeFromTop (140).reduced (2, 0));

    auto comboRow = area.removeFromTop (34);
    waveType.setBounds (comboRow.removeFromLeft (comboRow.getWidth() / 4).reduced (4));
    potentialType.setBounds (comboRow.removeFromLeft (comboRow.getWidth() / 3).reduced (4));
    sampleMode.setBounds (comboRow.removeFromLeft (comboRow.getWidth() / 2).reduced (4));
    fxType.setBounds (comboRow.reduced (4));

    auto toggleRow = area.removeFromTop (28);
    showFFT.setBounds (toggleRow.removeFromLeft (toggleRow.getWidth() / 2).reduced (4));
    applyQm.setBounds (toggleRow.reduced (4));

    waveArea = area.removeFromTop (132);
    potArea = area.removeFromTop (132);
    evolArea = area.removeFromTop (132);
    voiceArea = area.removeFromTop (170);
    fxArea = area;
    fxSolo.setBounds (fxArea.getRight() - 150, fxArea.getY() + 4, 142, 20);
    auto place = [&] (ValueSlider& s, juce::Label& l, juce::Rectangle<int> cell)
    {
        auto labelArea = cell.removeFromBottom (15);
        s.setBounds (cell.reduced (2, 0));
        l.setBounds (labelArea);
    };

    auto layoutRow = [&] (juce::Array<ValueSlider*> sliders, juce::Array<juce::Label*> labels, juce::Rectangle<int> area, int count)
    {
        const int w = area.getWidth() / count;
        for (int i = 0; i < count; ++i)
        {
            if (i >= sliders.size()) break;
            auto cell = juce::Rectangle<int> (area.getX() + i * w, area.getY(), w, area.getHeight());
            place (*sliders[i], *labels[i], cell.reduced (4, 0));
        }
    };

    // Wave
    {
        juce::Array<ValueSlider*> sl; sl.add (&waveShift); sl.add (&waveScale); sl.add (&waveImpulse);
        juce::Array<juce::Label*> lb; lb.add (&waveShiftLabel); lb.add (&waveScaleLabel); lb.add (&waveImpulseLabel);
        auto c = waveArea.reduced (10, 0).withTrimmedTop (22).withTrimmedBottom (4);
        layoutRow (sl, lb, c, 3);
    }
    // Potential
    {
        juce::Array<ValueSlider*> sl; sl.add (&potentialShift); sl.add (&potentialScale); sl.add (&potentialHeight);
        juce::Array<juce::Label*> lb; lb.add (&potentialShiftLabel); lb.add (&potentialScaleLabel); lb.add (&potentialHeightLabel);
        auto c = potArea.reduced (10, 0).withTrimmedTop (22).withTrimmedBottom (4);
        layoutRow (sl, lb, c, 3);
    }
    // Evolution
    {
        auto c = evolArea.reduced (10, 0).withTrimmedTop (22).withTrimmedBottom (4);
        const int w = c.getWidth() / 3;
        place (timestepDelta, timestepLabel, juce::Rectangle<int> (c.getX(), c.getY(), w, c.getHeight()).reduced (4, 0));
        place (samplesPerTimestep, evolRateLabel, juce::Rectangle<int> (c.getX() + w, c.getY(), w, c.getHeight()).reduced (4, 0));
    }
    // Voice ADSR + FX
    {
        auto c = voiceArea.reduced (10, 0).withTrimmedTop (22).withTrimmedBottom (4);
        auto adsrRow = c.removeFromTop (c.getHeight() / 2);
        juce::Array<ValueSlider*> sl; sl.add (&attack); sl.add (&decay); sl.add (&sustain); sl.add (&release);
        juce::Array<juce::Label*> lb; lb.add (&attackLabel); lb.add (&decayLabel); lb.add (&sustainLabel); lb.add (&releaseLabel);
        layoutRow (sl, lb, adsrRow, 4);

        juce::Array<ValueSlider*> fxsl; fxsl.add (&filterFreq); fxsl.add (&filterRes); fxsl.add (&drive); fxsl.add (&satMix); fxsl.add (&gain);
        juce::Array<juce::Label*> fxlb; fxlb.add (&filterFreqLabel); fxlb.add (&filterResLabel); fxlb.add (&driveLabel); fxlb.add (&satMixLabel); fxlb.add (&gainLabel);
        layoutRow (fxsl, fxlb, c, 5);
    }
    // Master FX two rows
    {
        auto c = fxArea.reduced (10, 0).withTrimmedTop (22).withTrimmedBottom (4);
        const int rows = 2;
        const int h = c.getHeight() / rows;
        juce::Array<ValueSlider*> r1; r1.add (&fxDepth); r1.add (&fxRate); r1.add (&fxFeedback);
        juce::Array<juce::Label*> l1; l1.add (&fxDepthLabel); l1.add (&fxRateLabel); l1.add (&fxFeedbackLabel);
        layoutRow (r1, l1, c.removeFromTop (h), 3);

        juce::Array<ValueSlider*> r2; r2.add (&fxMix); r2.add (&fxBits); r2.add (&fxEntropy);
        juce::Array<juce::Label*> l2; l2.add (&fxMixLabel); l2.add (&fxBitsLabel); l2.add (&fxEntropyLabel);
        layoutRow (r2, l2, c, 3);
    }
}