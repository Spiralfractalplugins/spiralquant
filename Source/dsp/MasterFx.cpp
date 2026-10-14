#include "MasterFx.h"
#include <cmath>

void MasterFx::prepare (double sampleRateHz)
{
    sampleRate = sampleRateHz <= 0.0 ? 48000.0 : sampleRateHz;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 2048;
    spec.numChannels = 2;

    chorus.prepare (spec);
    phaser.prepare (spec);
    reset();
}

void MasterFx::reset()
{
    chorus.reset();
    phaser.reset();
    lfoPhase = 0.0f;
    holdCounter = 0.0f;
    heldL = 0.0f;
    heldR = 0.0f;
    holdPeriod = 1;
    chaosState = 0.37f;
    fractalAcc = 0.0f;
}

void MasterFx::setParams (int t, float d, float r, float fb, float m, float b, float e, int s)
{
    type = juce::jlimit (0, 4, t);
    depth = juce::jlimit (0.0f, 1.0f, d);
    rate = juce::jlimit (0.01f, 20.0f, r);
    feedback = juce::jlimit (0.0f, 0.95f, fb);
    mix = juce::jlimit (0.0f, 1.0f, m);
    bits = juce::jlimit (2.0f, 16.0f, b);
    entropyAmount = juce::jlimit (0.0f, 1.0f, e);
    soloMode = juce::jlimit (0, 2, s);

    const float effectiveMix = (soloMode == 2) ? 1.0f : mix;
    mix = effectiveMix;

    const float centreDelay = 1.5f + depth * 6.0f;    chorus.setRate (rate);
    chorus.setDepth (depth);
    chorus.setCentreDelay (centreDelay);
    chorus.setFeedback (feedback);
    chorus.setMix (mix);

    phaser.setRate (rate);
    phaser.setDepth (depth);
    phaser.setCentreFrequency (800.0f + depth * 2400.0f);
    phaser.setFeedback (feedback);
    phaser.setMix (mix);
}

void MasterFx::process (juce::AudioBuffer<float>& buffer)
{
    if (type == 0 || soloMode == 1)
        return;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numChannels < 1 || numSamples <= 0)
        return;

    // Render through a persistent stereo scratch buffer: never resize the
    // caller's buffer in the audio callback (setSize can reallocate).
    if (scratch.getNumSamples() < numSamples || scratch.getNumChannels() != 2)
        scratch.setSize (2, numSamples, true, false, false);

    scratch.clear();
    scratch.copyFrom (0, 0, buffer, 0, 0, numSamples);
    if (numChannels > 1)
        scratch.copyFrom (1, 0, buffer, 1, 0, numSamples);
    if (type == 1)
    {
        juce::dsp::AudioBlock<float> block (scratch);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }
    else if (type == 2)
    {
        juce::dsp::AudioBlock<float> block (scratch);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        phaser.process (ctx);
    }
    else if (type == 3)
    {
        processBitcrush (scratch);
    }
    else if (type == 4)
    {
        processFractal (scratch);
    }

    float* dst0 = buffer.getWritePointer (0);
    const float* src0 = scratch.getReadPointer (0);
    const float* src1 = scratch.getReadPointer (1);
    if (numChannels == 1)
        for (int i = 0; i < numSamples; ++i) dst0[i] = src0[i];
    else
    {
        for (int c = 1; c < numChannels; ++c)
            if (c < 2)
                juce::FloatVectorOperations::copy (buffer.getWritePointer (c), src1, numSamples);
            else
                juce::FloatVectorOperations::clear (buffer.getWritePointer (c), numSamples);
        for (int i = 0; i < numSamples; ++i) dst0[i] = src0[i];
    }

    // sanitize
    for (int c = 0; c < buffer.getNumChannels(); ++c)
    {
        float* d = buffer.getWritePointer (c);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
    }
}

void MasterFx::processBitcrush (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float levels = (float) (1 << (int) std::round (bits));
    const int period = 1 + (int) std::round (depth * 15.0f);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);
    const float dryWet = 1.0f - mix;

    for (int i = 0; i < numSamples; ++i)
    {
        if (holdCounter >= (float) period)
        {
            heldL = std::round (l[i] * levels) / levels;
            heldR = std::round (r[i] * levels) / levels;
            holdCounter = 0.0f;
        }
        ++holdCounter;

        const float dryL = l[i];
        const float dryR = r[i];
        l[i] = dryL * dryWet + heldL * mix;
        r[i] = dryR * dryWet + heldR * mix;
    }
}

float MasterFx::nextNoise()
{
    // xorshift32: ~3 ops per sample, no shared global state
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return ((float) (rngState >> 8) / 8388608.0f) - 0.5f; // [-0.5, 0.5)
}

void MasterFx::processFractal (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float lfoInc = (juce::MathConstants<float>::twoPi * rate) / (float) sampleRate;
    const int period = 1 + (int) std::round (depth * 12.0f);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);

    const float levelsF = (float) (1 << (int) std::round (juce::jlimit (2.0f, 16.0f, bits)));
    const float invLevels = 1.0f / levelsF;
    const float dryWet = 1.0f - mix;
    const float noiseAmp = entropyAmount * 0.04f;

    for (int i = 0; i < numSamples; ++i)
    {
        if (holdCounter >= (float) period)
        {
            heldL = std::round (l[i] * levelsF) * invLevels;
            heldR = std::round (r[i] * levelsF) * invLevels;
            holdCounter = 0.0f;
        }
        ++holdCounter;

        // tent map chaos
        chaosState = 1.0f - 2.0f * std::fabs (2.0f * chaosState - 1.0f);
        if (! std::isfinite (chaosState)) chaosState = 0.37f;

        lfoPhase += lfoInc;
        if (lfoPhase > juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        const float chaosMod = 1.0f + entropyAmount * (chaosState - 0.5f) * 0.8f;
        const float wobble = 1.0f + 0.25f * entropyAmount * std::sin (lfoPhase * 3.7f);
        const float gainMod = chaosMod * wobble;
        const float rndL = nextNoise() * noiseAmp;
        const float rndR = nextNoise() * noiseAmp;

        const float dryL = l[i];
        const float dryR = r[i];
        l[i] = dryL * dryWet + (heldL * gainMod + rndL) * mix;
        r[i] = dryR * dryWet + (heldR * gainMod + rndR) * mix;
    }
}
