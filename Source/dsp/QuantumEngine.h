#pragma once
#include <JuceHeader.h>
#include <random>
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
                    int chordMode = 0, int chordType = 0, float pitchCents = 0.0f, bool tunnel = false,
                    float jumpRate = 0.0f, float quadrature = 0.0f,
                    float memoryAmt = 0.0f, float memoryTimeMs = 80.0f, float ringdown = 0.0f,
                    bool walkOn = false, float walkRateHz = 2.0f, bool limiter = true,
                    float zeno = 0.0f, bool entangle = false, float squeeze = 0.0f);
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
    int voiceVariant[MAX_VOICES] = {};
    unsigned long long voiceAge[MAX_VOICES] = {};
    unsigned long long nextAge = 1;
    double sampleRate = 48000.0;

    // Superposition chords: while a key is held the note lives in a weighted
    // blend of two chord voicings; note-off measures it (collapse to one).
    struct Superposition
    {
        int note = -1;
        float w1 = 1.0f;   // probability amplitude of the selected chordType
        int type2 = 0;     // the superposed alternative voicing
    };
    Superposition supers[4];
    std::mt19937 rng { 4242u };

    // Double-buffered scope: audio thread fills the back slot then flips scopeSlot;
    // the UI reads the published slot without ever blocking the audio thread.
    alignas (64) float scopePos [2][SCOPE_SIZE] {};
    alignas (64) float scopeMom [2][SCOPE_SIZE] {};
    alignas (64) float scopePot [2][SCOPE_SIZE] {};
    std::atomic<int> scopeSlot { 0 };
    std::atomic<unsigned> scopeSeq { 0 };
    std::atomic<int> uiNote { -1 };
    int chordMode = 0;   // 0 off, 1 chord, 2 superposed chord
    int chordType = 0;

    // Stereo-linked peak limiter on the master bus: gain chases the sample
    // peak above the threshold with a ~1 ms attack and ~80 ms release, then a
    // final ±1 clamp guarantees the output can never exceed full scale.
    bool limiterOn = true;
    bool entangleOn = false;
    float jumpRateAmt = 0.0f;
    float limGain = 1.0f;
    float limAttack = 0.0f;
    float limRelease = 0.0f;
    void applyLimiter (juce::AudioBuffer<float>& buffer);

    void handleNoteOn (int note, int velocity);
    void handleNoteOff (int note);
    static int chordSize (int type);
    static float chordRatio (int type, int degree);

    void resetWalk();
    void stepWalk();

    // Continuous-time quantum walk on a 16-node ring of pentatonic degrees:
    // one Hadamard-ish spread per tick, position measured (sampled) each tick.
    std::complex<float> walkPsi[16] {};
    bool walkOn = false;
    float cachedWalkRate = -1.0f;
    int walkPlaying = -1;
    int walkTickSamples = 0;
    int walkCountdown = 0;
};