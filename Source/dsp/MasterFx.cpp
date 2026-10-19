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

    fractalMax = juce::jmax (1024, (int) (sampleRate * 0.5));
    fractalLine.setSize (2, fractalMax);
    fractalLine.clear();

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
    fractalWrite = 0;
    fractalLp [0] = 0.0f;
    fractalLp [1] = 0.0f;
    if (fractalMax > 0)
        fractalLine.clear();
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

void MasterFx::setReferenceHz (float hz)
{
    if (std::isfinite (hz) && hz > 20.0f && hz < 20000.0f)
        refHz = hz;
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

void MasterFx::processFractal (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());
    if (numSamples <= 0 || numChannels <= 0 || fractalMax <= 0)
        return;

    // Chaos moves the *harmonic content*, never the level: the tent map picks a
    // comb length that is a whole number of tonic periods and steers the tail's
    // brightness. Wet gain is normalised by (1 - feedback) and trimmed, so this
    // path can only ever be quieter than the dry signal.
    lfoPhase += (juce::MathConstants<float>::twoPi * rate * (float) numSamples) / (float) sampleRate;
    if (lfoPhase > juce::MathConstants<float>::twoPi)
        lfoPhase -= juce::MathConstants<float>::twoPi;

    // Tent map on [0,1]: x -> 1 - |2x - 1|. It must stay inside the unit
    // interval, otherwise the tap index below pins to one period forever.
    chaosState = 1.0f - std::fabs (2.0f * chaosState - 1.0f);
    chaosState = juce::jlimit (0.0f, 1.0f, chaosState);

    const float tonicPeriod = (float) sampleRate / refHz;
    // DEPTH opens the tap range: the tent map may reach 2 tonic periods at the
    // bottom and 8 at the top. Never past a delay line that still holds whole
    // periods, or the tail stops reinforcing the tonic.
    const int periodCap = juce::jmax (1, (int) ((float) (fractalMax - 1) / tonicPeriod));
    const int maxPeriods = juce::jmin (2 + juce::roundToInt (juce::jlimit (0.0f, 1.0f, depth) * 6.0f),
                                       periodCap);
    const int tapPeriods = juce::jlimit (1, maxPeriods,
                                         1 + (int) (chaosState * (float) maxPeriods));
    const int delay = juce::jlimit (1, fractalMax - 1,
                                    (int) std::lround (tonicPeriod * (float) tapPeriods));

    const int levels = 1 << juce::roundToInt (juce::jlimit (5.0f, 14.0f, bits));
    const float invLevels = 1.0f / (float) levels;
    const float fb = juce::jlimit (0.0f, 0.85f, feedback);
    const float dryWet = 1.0f - mix * 0.35f;
    const float wetWet = mix * 0.35f * (1.0f - fb);
    // Entropy opens the tail: low chaos keeps it dark and consonant, high chaos
    // lets the upper harmonics of the tonic ring.
    const float lpCoef = juce::jlimit (0.01f, 0.9f, 0.12f + entropyAmount * 0.7f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        float* line = fractalLine.getWritePointer (ch);
        float& lp = fractalLp [ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const int readIdx = (fractalWrite - delay + fractalMax) % fractalMax;
            const float quantised = std::round (d[i] * (float) levels) * invLevels;
            float out = quantised + fb * line[readIdx];
            if (! std::isfinite (out)) out = 0.0f;

            line[fractalWrite] = out;
            fractalWrite = (fractalWrite + 1) % fractalMax;

            lp += lpCoef * (out - lp);
            d[i] = d[i] * dryWet + lp * wetWet;
        }
    }
}
