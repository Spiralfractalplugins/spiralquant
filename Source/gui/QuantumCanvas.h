#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

class QuantumCanvas : public juce::Component, private juce::Timer
{
public:
    QuantumCanvas (PluginProcessor& p) : processor (p) { startTimerHz (30); }
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override;

    void cycleWaveType();
    void cyclePotentialType();

private:
    void timerCallback() override { repaint(); }

    PluginProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts() { return processor.apvts; }

    float getRaw (const char* id) const { return *processor.apvts.getRawParameterValue (id); }
    void setNorm (const char* id, float norm);

    enum Drag { None, WaveCenter, WaveScale, PotCenter, PotHeight, PotScale };
    Drag dragging = None;
    juce::Point<float> lastMouse;

    juce::Point<float> waveCenterH, waveScaleH;
    juce::Point<float> potCenterH, potHeightH, potScaleH;

    void updateHandles();
    bool hit (juce::Point<float> p, juce::Point<float> h) const { return p.getDistanceFrom (h) <= 11.0f; }

    float xNormToUnit (float xNorm) const { return juce::jlimit (0.0f, 1.0f, (xNorm + 1.0f) * 0.5f); }
    float unitToXNorm (float u) const { return juce::jlimit (-1.0f, 1.0f, u * 2.0f - 1.0f); }
};
