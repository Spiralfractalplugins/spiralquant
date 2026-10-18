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
    void fillScopeMomentum (float* dest, int maxN) const;
    void fillPotential (float* dest, int maxN) const;
    int getMidiNote() const { return midiNote; }

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

    // Raw inputs of the last setParams call: an unchanged block skips the
    // whole reassignment, and IIR coefficients are only marked dirty when the
    // filter pair actually moved.
    struct LastInputs
    {
        float a, d, s, r, ff, fr, dr, sm, g;
        int wt; float ws, wsc, wi;
        int pt; float ps, psc, ph;
        int sf, smode; float td, spt;
        bool apply; float cents; bool tunnel;

        bool operator== (const LastInputs& o) const
        {
            return a == o.a && d == o.d && s == o.s && r == o.r
                && ff == o.ff && fr == o.fr && dr == o.dr && sm == o.sm && g == o.g
                && wt == o.wt && ws == o.ws && wsc == o.wsc && wi == o.wi
                && pt == o.pt && ps == o.ps && psc == o.psc && ph == o.ph
                && sf == o.sf && smode == o.smode && td == o.td && spt == o.spt
                && apply == o.apply && cents == o.cents && tunnel == o.tunnel;
        }
    };
    LastInputs last {};
    bool lastKnown = false;

    void updateEnvelope();
    float frequencyToPhaseInc (float freq) const;
};