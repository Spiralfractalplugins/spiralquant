#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), scope (p)
{
    setLookAndFeel (&lnf);
    setSize (1180, 740);
    setResizeLimits (920, 620, 1500, 920);

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
    for (auto* c : { &waveType, &potentialType, &sampleMode })
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
                     &filterFreq,&filterRes,&drive,&satMix,&gain })
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

    waveTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "waveType", waveType);
    potentialTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "potentialType", potentialType);
    sampleModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "sampleMode", sampleMode);
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
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (SciFiTheme::background);

    // panel backgrounds
    for (auto r : { waveArea, potArea, evolArea, voiceArea })
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
}

void PluginEditor::paintPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& name)
{
    g.setColour (SciFiTheme::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (name, r.reduced (10, 4).removeFromTop (16), juce::Justification::centredLeft, false);

    // small corner brackets
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
    scope.setBounds (area.removeFromTop (150).reduced (2, 0));

    auto comboRow = area.removeFromTop (34);
    waveType.setBounds (comboRow.removeFromLeft (comboRow.getWidth() / 3).reduced (4));
    potentialType.setBounds (comboRow.removeFromLeft (comboRow.getWidth() / 2).reduced (4));
    sampleMode.setBounds (comboRow.reduced (4));

    auto toggleRow = area.removeFromTop (28);
    showFFT.setBounds (toggleRow.removeFromLeft (toggleRow.getWidth() / 2).reduced (4));
    applyQm.setBounds (toggleRow.reduced (4));

    waveArea = area.removeFromTop (126);
    potArea = area.removeFromTop (126);
    evolArea = area.removeFromTop (126);
    voiceArea = area;

    auto layoutRow = [] (juce::Array<juce::Slider*> sliders, juce::Rectangle<int> area, int count)
    {
        const int w = area.getWidth() / count;
        for (int i = 0; i < count; ++i)
            if (i < sliders.size())
                sliders[i]->setBounds (area.getX() + i * w + 4, area.getY(), w - 8, area.getHeight());
    };

    juce::Array<juce::Slider*> waveSl = { &waveShift, &waveScale, &waveImpulse };
    auto wc = waveArea.reduced (10, 0).withTrimmedTop (20).withTrimmedBottom (4);
    layoutRow (waveSl, wc, 3);

    juce::Array<juce::Slider*> potSl = { &potentialShift, &potentialScale, &potentialHeight };
    auto pc = potArea.reduced (10, 0).withTrimmedTop (20).withTrimmedBottom (4);
    layoutRow (potSl, pc, 3);

    juce::Array<juce::Slider*> evlSl = { &timestepDelta, &samplesPerTimestep };
    auto ec = evolArea.reduced (10, 0).withTrimmedTop (20).withTrimmedBottom (4);
    const int eLeftW = ec.getWidth() / 3;
    timestepDelta.setBounds (ec.getX(), ec.getY(), eLeftW - 4, ec.getHeight());
    samplesPerTimestep.setBounds (ec.getX() + eLeftW, ec.getY(), eLeftW - 4, ec.getHeight());

    juce::Array<juce::Slider*> adsrSl = { &attack, &decay, &sustain, &release };
    auto vc = voiceArea.reduced (10, 0).withTrimmedTop (20).withTrimmedBottom (4);
    const int halfH = vc.getHeight() / 2;
    auto adsrRow = vc.removeFromTop (halfH);
    layoutRow (adsrSl, adsrRow, 4);

    juce::Array<juce::Slider*> fxSl = { &filterFreq, &filterRes, &drive, &satMix, &gain };
    auto fxRow = vc;
    layoutRow (fxSl, fxRow, 5);
}
