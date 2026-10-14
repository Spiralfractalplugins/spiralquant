#include <JuceHeader.h>
#include <cmath>
#include "PluginProcessor.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    for (int wt = 0; wt < 6; ++wt)
    for (int pt = 0; pt < 6; ++pt)
    for (int sm = 0; sm < 4; ++sm)
    {
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);

        if (auto* p = proc.apvts.getParameter ("waveType")) p->setValueNotifyingHost ((float) wt / 5.0f);
        if (auto* p = proc.apvts.getParameter ("potentialType")) p->setValueNotifyingHost ((float) pt / 5.0f);
        if (auto* p = proc.apvts.getParameter ("sampleMode")) p->setValueNotifyingHost ((float) sm / 3.0f);
        if (auto* p = proc.apvts.getParameter ("samplesPerTimestep")) p->setValueNotifyingHost (0.2f);
        if (auto* p = proc.apvts.getParameter ("applyWavefunction")) p->setValueNotifyingHost (1.0f);

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        midi.addEvent (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 90), 0);
        midi.addEvent (juce::MidiMessage::noteOn (1, 67, (juce::uint8) 90), 0);
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        proc.processBlock (buffer, midi);

        float rms = 0.0f;
        const float* d = buffer.getReadPointer (0);
        for (int i = 0; i < 512; ++i) { if (! std::isfinite (d[i])) { std::cerr << "NaN at wt=" << wt << " pt=" << pt << " sm=" << sm << "\n"; return 1; } rms += d[i]*d[i]; }
        rms = std::sqrt (rms / 512.0f);
        if (rms <= 0.0f) { std::cerr << "silent at wt=" << wt << " pt=" << pt << " sm=" << sm << "\n"; return 1; }
    }

    // state roundtrip
    {
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        PluginProcessor proc2;
        proc2.prepareToPlay (48000.0, 512);
        proc2.setStateInformation (mb.getData(), (int) mb.getSize());
    }

    // FX stress
    for (int fxt = 0; fxt < 5; ++fxt)
    {
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);

        if (auto* p = proc.apvts.getParameter ("waveType")) p->setValueNotifyingHost (0.0f);
        if (auto* p = proc.apvts.getParameter ("potentialType")) p->setValueNotifyingHost (1.0f / 5.0f);
        if (auto* p = proc.apvts.getParameter ("sampleMode")) p->setValueNotifyingHost (0.0f);
        if (auto* p = proc.apvts.getParameter ("samplesPerTimestep")) p->setValueNotifyingHost (0.2f);
        if (auto* p = proc.apvts.getParameter ("applyWavefunction")) p->setValueNotifyingHost (1.0f);
        if (auto* p = proc.apvts.getParameter ("fxType")) p->setValueNotifyingHost ((float) fxt / 4.0f);
        if (auto* p = proc.apvts.getParameter ("fxDepth")) p->setValueNotifyingHost (0.6f);
        if (auto* p = proc.apvts.getParameter ("fxRate")) p->setValueNotifyingHost (0.1f);
        if (auto* p = proc.apvts.getParameter ("fxFeedback")) p->setValueNotifyingHost (0.4f);
        if (auto* p = proc.apvts.getParameter ("fxMix")) p->setValueNotifyingHost (0.7f);
        if (auto* p = proc.apvts.getParameter ("fxBits")) p->setValueNotifyingHost (0.4f);
        if (auto* p = proc.apvts.getParameter ("fxEntropy")) p->setValueNotifyingHost (0.5f);

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        proc.processBlock (buffer, midi);

        float rms = 0.0f;
        const float* d = buffer.getReadPointer (0);
        for (int i = 0; i < 512; ++i)
        {
            if (! std::isfinite (d[i])) { std::cerr << "FX NaN fxt=" << fxt << "\n"; return 1; }
            rms += d[i] * d[i];
        }
        rms = std::sqrt (rms / 512.0f);
        if (rms <= 0.0f) { std::cerr << "FX silent fxt=" << fxt << "\n"; return 1; }
    }

    std::cout << "ALL TESTS PASSED" << std::endl;
    return 0;
}