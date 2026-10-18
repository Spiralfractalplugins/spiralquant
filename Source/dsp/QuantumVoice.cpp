#include "QuantumVoice.h"
#include <cmath>

QuantumVoice::QuantumVoice() = default;

void QuantumVoice::prepare (double sampleRateHz)
{
    sampleRate = sampleRateHz <= 0.0 ? 48000.0 : sampleRateHz;
    osc.prepare (sampleRate);
    filter.reset();
    coeffsDirty = true;
}

void QuantumVoice::noteOn (int note, int velocity, float ratio)
{
    midiNote = note;
    noteRatio = ratio;
    targetFreq = applyPitch (440.0f * std::pow (2.0f, (note - 69) / 12.0f) * ratio);
    if (playingFreq <= 0.0f) playingFreq = targetFreq;
    phaseInc = frequencyToPhaseInc (playingFreq);

    if (state == State::Idle)
    {
        osc.trigger();
        phase = 0.0f;
    }
    velocityGain = juce::jlimit (0.0f, 1.0f, velocity / 127.0f * 0.8f + 0.2f);
    envelope = (envelope < 0.001f) ? 0.001f : envelope;
    state = State::Attack;
}

void QuantumVoice::noteOff()
{
    if (state != State::Idle) state = State::Release;
}

void QuantumVoice::setParams (float a, float d, float s, float r,
                              float ff, float fr, float dr, float sm, float g,
                              int wt, float ws, float wsc, float wi,
                              int pt, float ps, float psc, float ph,
                              int sf, int smode, float td, float spt, bool apply,
                              float cents, bool tunnel, float jumpRate, float quadrature)
{
    const LastInputs in { a, d, s, r, ff, fr, dr, sm, g,
                          wt, ws, wsc, wi,
                          pt, ps, psc, ph,
                          sf, smode, td, spt, apply, cents, tunnel, jumpRate, quadrature };
    if (lastKnown && in == last)
        return;

    coeffsDirty = ! lastKnown || ff != last.ff || fr != last.fr;
    last = in;
    lastKnown = true;

    attackMs = juce::jlimit (1.0f, 5000.0f, a);
    decayMs = juce::jlimit (1.0f, 5000.0f, d);
    sustainLevel = juce::jlimit (0.0f, 1.0f, s);
    releaseMs = juce::jlimit (1.0f, 5000.0f, r);
    filterFreq = juce::jlimit (20.0f, 20000.0f, ff);
    filterRes = juce::jlimit (0.0f, 0.98f, fr);
    drive = juce::jlimit (0.0f, 1.0f, dr);
    satMix = juce::jlimit (0.0f, 1.0f, sm);
    gain = juce::jlimit (0.0f, 2.0f, g);

    const float clampedCents = juce::jlimit (-2400.0f, 2400.0f, cents);
    if (clampedCents != pitchCents)
    {
        pitchCents = clampedCents;
        if (midiNote >= 0)
            targetFreq = applyPitch (440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f) * noteRatio);
    }

    osc.setWaveParameters (wt, ws, wsc, wi);
    osc.setPotential (pt, ps, psc, ph);
    osc.setTunneling (tunnel);
    osc.setEvolution (sf, smode, td, spt);
    if (td != lastTimestepDelta)
    {
        lastTimestepDelta = td;
        osc.markTablesDirty();
    }
    osc.setApplyWavefunction (apply);
    osc.setJumpRate (jumpRate);
    osc.setQuadrature (quadrature);
}

void QuantumVoice::prepareBlock()
{
    if (coeffsDirty)
    {
        coeffsDirty = false;
        const float q = 0.5f + filterRes * filterRes * 12.0f;
        filter.setCoefficients (juce::IIRCoefficients::makeLowPass ((double) sampleRate,
                                                                    (double) filterFreq, (double) q));
    }

    const float srHz = (float) sampleRate;
    attackInc = 1.0f / std::max (1.0f, attackMs * 0.001f * srHz);
    decayRate = 1.0f / std::max (1.0f, decayMs * 0.001f * srHz);
    releaseMul = 1.0f / std::max (1.0f, releaseMs * 0.001f * srHz);
}

float QuantumVoice::process (const float* /*params*/)
{
    if (state == State::Idle) return 0.0f;

    // frequency glide
    if (playingFreq != targetFreq)
    {
        const float step = std::abs (targetFreq - playingFreq) / std::max (1.0f, attackMs * 2.0f);
        if (playingFreq < targetFreq) { playingFreq = std::min (targetFreq, playingFreq + step); }
        else { playingFreq = std::max (targetFreq, playingFreq - step); }
        phaseInc = frequencyToPhaseInc (playingFreq);
    }

    const float out = osc.getNextSample (phaseInc);
    updateEnvelope();
    if (state == State::Idle) return 0.0f;

    float sample = out * envelope * velocityGain;

    sample = filter.processSingleSampleRaw (sample);

    // saturation: cubic soft-clip approximation of tanh, near-identical for |x| < 1.5
    if (drive > 0.0f)
    {
        const float x = juce::jlimit (-1.5f, 1.5f, sample * (1.0f + drive * 12.0f));
        const float sat = x * (1.0f - (x * x) * (1.0f / 3.0f));
        sample = sample * (1.0f - satMix) + sat * satMix;
    }

    sample *= gain;
    if (! std::isfinite (sample)) sample = 0.0f;

    phase = osc.getPhase();
    return sample;
}

void QuantumVoice::updateEnvelope()
{
    if (state == State::Attack)
    {
        envelope += attackInc;
        if (envelope >= 1.0f) { envelope = 1.0f; state = State::Decay; }
    }
    else if (state == State::Decay)
    {
        envelope -= (envelope - sustainLevel) * decayRate;
        if (std::fabs (envelope - sustainLevel) < 0.001f) { envelope = sustainLevel; state = State::Sustain; }
    }
    else if (state == State::Sustain)
    {
        envelope = sustainLevel;
    }
    else if (state == State::Release)
    {
        envelope *= (1.0f - releaseMul);
        if (envelope < 0.0001f) { envelope = 0.0f; state = State::Idle; }
    }
}

float QuantumVoice::frequencyToPhaseInc (float freq) const
{
    freq = juce::jlimit (1.0f, (float) sampleRate * 0.22f, freq);
    return (freq / (float) sampleRate) * (float) QuantumOsc::SIZE;
}

void QuantumVoice::fillScope (float* dest, int maxN) const
{
    osc.fillScope (dest, maxN);
}

void QuantumVoice::fillScopeMomentum (float* dest, int maxN) const
{
    osc.fillScopeMomentum (dest, maxN);
}

void QuantumVoice::fillPotential (float* dest, int maxN) const
{
    osc.fillPotential (dest, maxN);
}