#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "gui/SciFiLookAndFeel.h"
#include "gui/ValueSlider.h"

class QuantumScope : public juce::Component, private juce::Timer
{
public:
    QuantumScope (PluginProcessor& p) : processor (p)
    {
        startTimerHz (30);
    }
    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (SciFiTheme::panel);
        g.fillRoundedRectangle (area, SciFiTheme::corner);
        g.setColour (SciFiTheme::stroke);
        g.drawRoundedRectangle (area, SciFiTheme::corner, 1.0f);

        // oscilloscope grid
        g.setColour (SciFiTheme::grid.withAlpha (0.45f));
        for (int i = 1; i < 4; ++i)
        {
            const float y = area.getY() + area.getHeight() * (float) i / 4.0f;
            g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
        }
        for (int i = 1; i < 6; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / 6.0f;
            g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
        }
        g.setColour (SciFiTheme::grid.withAlpha (0.7f));
        g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());

        // glow trace
        juce::Path glow;
        bool first = true;
        for (int i = 0; i < 128; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / 127.0f;
            const float y = area.getCentreY() - scope[i] * area.getHeight() * 0.42f;
            if (first) { glow.startNewSubPath (x, y); first = false; }
            else glow.lineTo (x, y);
        }
        g.setColour (SciFiTheme::neon.withAlpha (0.18f));
        g.strokePath (glow, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (SciFiTheme::neon);
        g.strokePath (glow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
private:
    void timerCallback() override
    {
        const float* s = processor.getScopeWave();
        for (int i = 0; i < 128; ++i) scope[i] = s[i];
        repaint();
    }
    PluginProcessor& processor;
    float scope[128] {};
};

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& processor;
    SciFiLookAndFeel lnf;

    juce::Label title;
    juce::Label subtitle;
    QuantumScope scope;

    juce::ComboBox waveType, potentialType, sampleMode;
    juce::ToggleButton showFFT, applyQm;

    juce::ComboBox fxType, fxSolo;

    ValueSlider waveShift, waveScale, waveImpulse;
    ValueSlider potentialShift, potentialScale, potentialHeight;
    ValueSlider timestepDelta, samplesPerTimestep;
    ValueSlider attack, decay, sustain, release;
    ValueSlider filterFreq, filterRes, drive, satMix, gain;
    ValueSlider fxDepth, fxRate, fxFeedback, fxMix, fxBits, fxEntropy;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveTypeAtt, potentialTypeAtt, sampleModeAtt, fxTypeAtt, fxSoloAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showFFTAtt, applyQmAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> waveShiftAtt, waveScaleAtt, waveImpulseAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> potentialShiftAtt, potentialScaleAtt, potentialHeightAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timestepAtt, evolRateAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAtt, decayAtt, sustainAtt, releaseAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterFreqAtt, filterResAtt, driveAtt, satMixAtt, gainAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxDepthAtt, fxRateAtt, fxFeedbackAtt, fxMixAtt, fxBitsAtt, fxEntropyAtt;

    juce::Rectangle<int> waveArea, potArea, evolArea, voiceArea, fxArea;

    juce::Label waveShiftLabel, waveScaleLabel, waveImpulseLabel;
    juce::Label potentialShiftLabel, potentialScaleLabel, potentialHeightLabel;
    juce::Label timestepLabel, evolRateLabel;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;
    juce::Label filterFreqLabel, filterResLabel, driveLabel, satMixLabel, gainLabel;
    juce::Label fxDepthLabel, fxRateLabel, fxFeedbackLabel, fxMixLabel, fxBitsLabel, fxEntropyLabel;

    void paintPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
