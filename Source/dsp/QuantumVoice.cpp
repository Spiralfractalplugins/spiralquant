#include "QuantumVoice.h"
#include <cmath>

QuantumVoice::QuantumVoice() = default;

void QuantumVoice::prepare (double sampleRateHz)
{
    sampleRate = sampleRateHz <= 0.0 ? 48000.0 : sampleRateHz;
    osc.prepare (sampleRate);
    filter.reset();
}

void QuantumVoice::noteOn (int note, int velocity)
{
    midiNote = note;
    targetFreq = 440.0f * std::pow (2.0f, (note - 69) / 12.0f);
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
                              int sf, int smode, float td, float spt, bool apply)
{
    attackMs = juce::jlimit (1.0f, 5000.0f, a);
    decayMs = juce::jlimit (1.0f, 5000.0f, d);
    sustainLevel = juce::jlimit (0.0f, 1.0f, s);
    releaseMs = juce::jlimit (1.0f, 5000.0f, r);
    filterFreq = juce::jlimit (20.0f, 20000.0f, ff);
    filterRes = juce::jlimit (0.0f, 0.98f, fr);
    drive = juce::jlimit (0.0f, 1.0f, dr);
    satMix = juce::jlimit (0.0f, 1.0f, sm);
    gain = juce::jlimit (0.0f, 2.0f, g);

    osc.setWaveParameters (wt, ws, wsc, wi);
    osc.setPotential (pt, ps, psc, ph);
    osc.setEvolution (sf, smode, td, spt);
    // apply toggle not stored; keep always true for now
    (void) apply;
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

    // lowpass
    const float q = 0.5f + filterRes * filterRes * 12.0f;
    const float w = 2.0f * juce::MathConstants<float>::pi * filterFreq / (float) sampleRate;
    juce::IIRCoefficients c = juce::IIRCoefficients::makeLowPass ((double) sampleRate, (double) filterFreq, (double) q);
    filter.setCoefficients (c);
    sample = filter.processSingleSampleRaw (sample);

    // saturation
    if (drive > 0.0f)
    {
        const float x = sample * (1.0f + drive * 12.0f);
        const float sat = std::tanh (x);
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
        const float inc = 1.0f / std::max (1.0f, attackMs * 0.001f * (float) sampleRate);
        envelope += inc;
        if (envelope >= 1.0f) { envelope = 1.0f; state = State::Decay; }
    }
    else if (state == State::Decay)
    {
        const float diff = envelope - sustainLevel;
        const float rate = 1.0f / std::max (1.0f, decayMs * 0.001f * (float) sampleRate);
        envelope -= diff * rate;
        if (std::fabs (envelope - sustainLevel) < 0.001f) { envelope = sustainLevel; state = State::Sustain; }
    }
    else if (state == State::Sustain)
    {
        envelope = sustainLevel;
    }
    else if (state == State::Release)
    {
        const float rate = 1.0f / std::max (1.0f, releaseMs * 0.001f * (float) sampleRate);
        envelope *= (1.0f - rate);
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