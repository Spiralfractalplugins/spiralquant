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

    if (buffer.getNumChannels() < 1 || buffer.getNumSamples() <= 0)
        return;

    // ensure stereo working buffer
    if (buffer.getNumChannels() == 1)
    {
        buffer.setSize (2, buffer.getNumSamples(), true, true, true);
    }

    if (type == 1)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }
    else if (type == 2)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        phaser.process (ctx);
    }
    else if (type == 3)
    {
        processBitcrush (buffer);
    }
    else if (type == 4)
    {
        processFractal (buffer);
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
    const int levels = 1 << (int) std::round (bits);
    const int period = 1 + (int) std::round (depth * 15.0f);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : l;

    for (int i = 0; i < numSamples; ++i)
    {
        if (holdCounter >= (float) period)
        {
            heldL = std::round (l[i] * (float) levels) / (float) levels;
            heldR = std::round (r[i] * (float) levels) / (float) levels;
            holdCounter = 0.0f;
        }
        else
        {
            // keep held values while waiting; l[i] still advances position
        }
        ++holdCounter;

        const float dryL = l[i];
        const float dryR = r[i];
        const float wetL = heldL;
        const float wetR = heldR;
        l[i] = dryL * (1.0f - mix) + wetL * mix;
        r[i] = dryR * (1.0f - mix) + wetR * mix;
    }
}

void MasterFx::processFractal (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float lfoInc = (juce::MathConstants<float>::twoPi * rate) / (float) sampleRate;
    const int levels = 1 << (int) std::round (juce::jlimit (2.0f, 16.0f, bits));
    const int period = 1 + (int) std::round (depth * 12.0f);

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : l;

    juce::Random& rng = juce::Random::getSystemRandom();

    for (int i = 0; i < numSamples; ++i)
    {
        if (holdCounter >= (float) period)
        {
            heldL = std::round (l[i] * (float) levels) / (float) levels;
            heldR = std::round (r[i] * (float) levels) / (float) levels;
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
        const float rndL = (rng.nextFloat() - 0.5f) * entropyAmount * 0.04f;
        const float rndR = (rng.nextFloat() - 0.5f) * entropyAmount * 0.04f;

        const float dryL = l[i];
        const float dryR = r[i];
        const float wetL = (heldL * chaosMod * wobble) + rndL;
        const float wetR = (heldR * chaosMod * wobble) + rndR;

        l[i] = dryL * (1.0f - mix) + wetL * mix;
        r[i] = dryR * (1.0f - mix) + wetR * mix;
    }
}
