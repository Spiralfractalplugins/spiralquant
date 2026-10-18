#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class AdsrCanvas : public juce::Component, private juce::Timer
{
public:
    AdsrCanvas (PluginProcessor& p) : processor (p) { startTimerHz (30); }
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    PluginProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts() { return processor.apvts; }
    float getRaw (const char* id) const { return processor.apvts.getRawParameterValue (id)->load(); }
    void setRaw (const char* id, float raw, float min, float max);

    enum Drag { None, Attack, Decay, Sustain, Release };
    Drag dragging = None;

    juce::Point<float> attackH, decayH, sustainH, releaseH;
    void updateHandles (float w, float h);
    bool hit (juce::Point<float> p, juce::Point<float> h) const { return p.getDistanceFrom (h) <= 12.0f; }

    float lastSig = -1.0f;
};
