#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../dsp/QuantumOsc.h"

/** Split scope: top panel |psi(x)| with potential overlay and drag handles,
    bottom panel |psi(k)| momentum spectrum with frequency grid. Both are fed
    by the engine's double-buffered snapshot, never by analytic lookalikes. */
class QuantumCanvas : public juce::Component, private juce::Timer
{
public:
    explicit QuantumCanvas (PluginProcessor& p);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override;

    void cycleWaveType();
    void cyclePotentialType();

private:
    void timerCallback() override;

    PluginProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts() { return processor.apvts; }

    std::atomic<float>* raw (const char* id) const { return processor.apvts.getRawParameterValue (id); }
    void setNorm (const char* id, float norm);

    // Cached for the 30 Hz timer poll and the per-mouse-move handle update.
    std::atomic<float>* pWaveShift = nullptr;
    std::atomic<float>* pWaveScale = nullptr;
    std::atomic<float>* pPotShift = nullptr;
    std::atomic<float>* pPotScale = nullptr;
    std::atomic<float>* pPotHeight = nullptr;
    std::atomic<float>* pShowFFT = nullptr;

    enum Drag { None, WaveCenter, WaveScale, PotCenter, PotHeight, PotScale };
    Drag dragging = None;

    juce::Point<float> waveCenterH, waveScaleH;
    juce::Point<float> potCenterH, potHeightH, potScaleH;
    juce::Rectangle<float> plot;   // |psi(x)| drawing area inside the wave panel
    float potCeiling = 0.0f;       // potential never grows into the wave band

    juce::Rectangle<float> wavePanel() const;
    juce::Rectangle<float> spectrumPanel() const;
    void updateHandles();
    bool hit (juce::Point<float> p, juce::Point<float> h) const { return p.getDistanceFrom (h) <= 11.0f; }

    float xNormToUnit (float xNorm) const { return juce::jlimit (0.0f, 1.0f, (xNorm + 1.0f) * 0.5f); }
    float unitToXNorm (float u) const { return juce::jlimit (-1.0f, 1.0f, u * 2.0f - 1.0f); }

    static constexpr int N = QuantumOsc::SIZE;
    float scopePos[N] {}, scopeMom[N] {}, scopePot[N] {};
    unsigned lastSeq = 0xffffffffu;
    float lastHandleSig = -1.0f;
};
