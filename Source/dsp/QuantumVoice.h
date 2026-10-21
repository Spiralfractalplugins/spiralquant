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
    /** Hard stop, skipping release (used by superposition collapse). */
    void kill() { state = State::Idle; envelope = 0.0f; }

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
                    float pitchCents = 0.0f, bool tunnel = false, float jumpRate = 0.0f, float quadrature = 0.0f,
                    float memoryAmt = 0.0f, float memoryTimeMs = 80.0f, float ringdown = 0.0f,
                    float zeno = 0.0f, float squeeze = 0.0f);

    /** True once per phonon jump since last call (EPR propagation, engine side). */
    bool jumpedRecently() { return osc.consumeJumped(); }
    void forceCollapse() { osc.collapseNow(); }

    // Quantum Lab (wave 4): evolution-level effects, forwarded to the osc.
    void setEvolutionFx (float gp, float wkb, float disp, float phaseAcc);

    // Cross-voice interference (Bose/Fermi exchange) hooks for the engine.
    float envLevel() const { return envelope * velocityGain; }
    float oscPhase() const { return osc.getPhase(); }
    /** Read this voice's wavetable at an arbitrary scan position. */
    float readTableAt (float phase) const { return osc.readAt (phase); }

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

    // Transposition, clamped to the range the oscillator can actually render:
    // ±4 octaves from the top of the keyboard would otherwise pass Nyquist.
    float applyPitch (float base) const
    {
        return juce::jlimit (8.0f, 0.45f * static_cast<float> (sampleRate),
                             base * std::pow (2.0f, pitchCents / 100.0f));
    }

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
        float jr, qd;
        float mem, memT, rd;
        float zn, sq;

        bool operator== (const LastInputs& o) const
        {
            return a == o.a && d == o.d && s == o.s && r == o.r
                && ff == o.ff && fr == o.fr && dr == o.dr && sm == o.sm && g == o.g
                && wt == o.wt && ws == o.ws && wsc == o.wsc && wi == o.wi
                && pt == o.pt && ps == o.ps && psc == o.psc && ph == o.ph
                && sf == o.sf && smode == o.smode && td == o.td && spt == o.spt
                && apply == o.apply && cents == o.cents && tunnel == o.tunnel
                && jr == o.jr && qd == o.qd
                && mem == o.mem && memT == o.memT && rd == o.rd
                && zn == o.zn && sq == o.sq;
        }
    };
    LastInputs last {};
    bool lastKnown = false;

    // Evolution FX of the last setEvolutionFx call (skip redundant forwarding).
    float lastGp = -1.0f, lastWkb = -1.0f, lastDisp = -1.0f, lastPhaseAcc = -1.0f;

    void updateEnvelope();
    float frequencyToPhaseInc (float freq) const;
};