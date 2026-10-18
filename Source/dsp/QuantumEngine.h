#pragma once
#include <JuceHeader.h>
#include "QuantumVoice.h"
#include "MasterFx.h"

class QuantumEngine
{
public:
    static constexpr int MAX_VOICES = 8;
    void prepare (double sampleRateHz);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, bool running);
    void setParams (float attack, float decay, float sustain, float release,
                    float filterFreq, float filterRes, float drive, float satMix, float gain,
                    int waveType, float waveShift, float waveScale, float waveImpulse,
                    int potentialType, float potentialShift, float potentialScale, float potentialHeight,
                    int showFFT, int sampleMode, float timestepDelta, float samplesPerTimestep, bool applyWavefunction,
                    bool chordOn = false, int chordType = 0, float pitchCents = 0.0f, bool tunnel = false);
    void setFxParams (int type, float depth, float rate, float feedback, float mix, float bits, float entropy, int soloMode);

    static constexpr int SCOPE_SIZE = QuantumOsc::SIZE;

    /** Lock-free copy of the latest published scope snapshot (message thread). */
    void snapshotScope (float* pos, float* mom, float* pot) const;
    unsigned getScopeSeq() const { return scopeSeq.load (std::memory_order_acquire); }
    float getUiNoteFreqHz() const;

private:
    QuantumVoice voices[MAX_VOICES];
    MasterFx fx;
    int voiceNote[MAX_VOICES] = { -1,-1,-1,-1,-1,-1,-1,-1 };
    unsigned long long voiceAge[MAX_VOICES] = {};
    unsigned long long nextAge = 1;
    double sampleRate = 48000.0;

    // Double-buffered scope: audio thread fills the back slot then flips scopeSlot;
    // the UI reads the published slot without ever blocking the audio thread.
    alignas (64) float scopePos [2][SCOPE_SIZE] {};
    alignas (64) float scopeMom [2][SCOPE_SIZE] {};
    alignas (64) float scopePot [2][SCOPE_SIZE] {};
    std::atomic<int> scopeSlot { 0 };
    std::atomic<unsigned> scopeSeq { 0 };
    std::atomic<int> uiNote { -1 };
    bool chordMode = false;
    int chordType = 0;

    void handleNoteOn (int note, int velocity);
    void handleNoteOff (int note);
    static int chordSize (int type);
    static float chordRatio (int type, int degree);
};