#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "gui/SciFiLookAndFeel.h"
#include "gui/ValueSlider.h"
#include "gui/QuantumCanvas.h"
#include "gui/AdsrCanvas.h"
#include "gui/PresetBrowser.h"
#include "gui/LogoMark.h"

class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    PluginProcessor& processor;
    SciFiLookAndFeel lnf;

    juce::Label title;
    LogoMark logo;
    juce::Label versionLabel;
    juce::Label status;
    juce::Label presetLabel;
    juce::ComboBox presetBox;
    QuantumCanvas canvas;
    AdsrCanvas adsrCanvas;

    juce::TextButton waveTypeChip, potentialTypeChip;
    juce::ToggleButton showFFT, applyQm, chordChip, tunnelChip;

    ValueSlider filterFreq, filterRes, gain, fxMix;
    ValueSlider timestepDelta, samplesPerTimestep, pitchShift;
    juce::ComboBox sampleMode, fxType, fxSolo, chordType;
    PresetBrowser presetBrowser;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showFFTAtt, applyQmAtt, chordAtt, tunnelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterFreqAtt, filterResAtt, gainAtt, fxMixAtt, timestepAtt, evolRateAtt, pitchAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sampleModeAtt, fxTypeAtt, fxSoloAtt, chordTypeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
