#pragma once
#include <JuceHeader.h>
#include "QuantumOsc.h"

class QuantumVoice
{
public:
    QuantumVoice();
    void prepare (double sampleRateHz);
    void noteOn (int midiNote, int velocity, float ratio = 1.0f);
    void noteOff();

    bool isActive() const { return state != State::Idle; }
    float process (const float* params); // params not used; reads internal knobs? placeholder

    // Recompute block-rate filter/envelope constants; call once per audio block.
    void prepareBlock();

    // Parameters updated by engine before processBlock
    void setParams (float attack, float decay, float sustain, float release,
                    float filterFreq, float filterRes, float drive, float satMix, float gain,
                    int waveType, float waveShift, float waveScale, float waveImpulse,
                    int potentialType, float potentialShift, float potentialScale, float potentialHeight,
                    int showFFT, int sampleMode, float timestepDelta, float samplesPerTimestep, bool applyWavefunction,
                    float pitchCents = 0.0f, bool tunnel = false);

    void fillScope (float* dest, int maxN) const;

private:
    enum class State { Idle, Attack, Decay, Sustain, Release };
    State state = State::Idle;

    QuantumOsc osc;
    juce::IIRFilter filter;
    double sampleRate = 48000.0;
    int midiNote = -1;

    float envelope = 0.0f;
    float velocityGain = 0.0f;
    float targetFreq = 0.0f;
    float playingFreq = 0.0f;
    float phase = 0.0f;
    float phaseInc = 0.0f;

    float attackMs = 10.0f;
    float decayMs = 100.0f;
    float sustainLevel = 0.8f;
    float releaseMs = 150.0f;
    float filterFreq = 4000.0f;
    float filterRes = 0.3f;
    float drive = 0.0f;
    float satMix = 0.0f;
    float gain = 0.8f;
    float pitchCents = 0.0f;
    float noteRatio = 1.0f;

    float applyPitch (float base) const { return base * std::pow (2.0f, pitchCents / 100.0f); }

    // Block-rate constants (recomputed in prepareBlock).
    float attackInc = 0.0f;
    float decayRate = 0.0f;
    float releaseMul = 0.0f;
    bool coeffsDirty = true;
    float lastTimestepDelta = -1.0f;

    void updateEnvelope();
    float frequencyToPhaseInc (float freq) const;
};