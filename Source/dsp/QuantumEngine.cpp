#include "QuantumEngine.h"

void QuantumEngine::prepare (double sampleRateHz)
{
    sampleRate = sampleRateHz <= 0.0 ? 48000.0 : sampleRateHz;
    fx.prepare (sampleRate);
    limAttack = 1.0f - (float) std::exp (-1.0 / (0.001 * sampleRate));
    limRelease = 1.0f - (float) std::exp (-1.0 / (0.08 * sampleRate));
    limGain = 1.0f;
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        voices[i].prepare (sampleRate);
        voiceNote[i] = -1;
        voiceAge[i] = 0;
    }
    reset();
    scopeSlot.store (0);
    scopeSeq.store (0);
    uiNote.store (-1);
    for (int s = 0; s < 2; ++s)
        for (int i = 0; i < SCOPE_SIZE; ++i)
        { scopePos[s][i] = 0.0f; scopeMom[s][i] = 0.0f; scopePot[s][i] = 0.0f; }
}

void QuantumEngine::reset()
{
    fx.reset();
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        voices[i].noteOff();
        voiceNote[i] = -1;
    }
    for (auto& sp : supers)
        sp.note = -1;
    walkPlaying = -1;
}
void QuantumEngine::setParams (float attack, float decay, float sustain, float release,
                    float filterFreq, float filterRes, float drive, float satMix, float gain,
                    int waveType, float waveShift, float waveScale, float waveImpulse,
                    int potentialType, float potentialShift, float potentialScale, float potentialHeight,
                    int showFFT, int sampleMode, float timestepDelta, float samplesPerTimestep, bool applyWavefunction,
                    int chordModeSel, int chordTypeSel, float pitchCents, bool tunnel, float jumpRate, float quadrature,
                    float memoryAmt, float memoryTimeMs, float ringdown, bool walkOnSel, float walkRateHz, bool limiter,
                    float zeno, bool entangle, float squeeze)
{
    chordMode = juce::jlimit (0, 2, chordModeSel);
    chordType = juce::jlimit (0, 7, chordTypeSel);
    pitchCents = juce::jlimit (-4800.0f, 4800.0f, pitchCents);
    limiterOn = limiter;
    entangleOn = entangle;
    jumpRateAmt = jumpRate;

    if (walkOnSel && ! walkOn)
        resetWalk();
    if (! walkOnSel && walkOn && walkPlaying >= 0)
    {
        handleNoteOff (walkPlaying);
        walkPlaying = -1;
    }
    walkOn = walkOnSel;

    for (int i = 0; i < MAX_VOICES; ++i)
        voices[i].setParams (attack, decay, sustain, release, filterFreq, filterRes, drive, satMix, gain,
                             waveType, waveShift, waveScale, waveImpulse,
                             potentialType, potentialShift, potentialScale, potentialHeight,
                             showFFT, sampleMode, timestepDelta, samplesPerTimestep, applyWavefunction,
                             pitchCents, tunnel, jumpRate, quadrature,
                             memoryAmt, memoryTimeMs, ringdown, zeno, squeeze);
    if (walkRateHz != cachedWalkRate)
    {
        cachedWalkRate = walkRateHz;
        walkTickSamples = juce::jmax (1, juce::roundToInt (sampleRate / juce::jlimit (0.25f, 8.0f, walkRateHz)));
    }
}

// Chord degrees as just-intonation ratios. Consonant chords use small
// integers (low harmonic-number clash); dissonant ones use large coprimes.
int QuantumEngine::chordSize (int type)
{
    static const int sizes[] = { 2, 3, 3, 3, 3, 2, 3, 4 };
    return sizes[juce::jlimit (0, 7, type)];
}

float QuantumEngine::chordRatio (int type, int degree)
{
    switch (juce::jlimit (0, 7, type))
    {
        case 0: { static const float r[] = { 1.0f, 3.0f / 2.0f }; return r[degree]; }            // Power (perfect fifth)
        case 1: { static const float r[] = { 1.0f, 5.0f / 4.0f, 3.0f / 2.0f }; return r[degree]; } // Major
        case 2: { static const float r[] = { 1.0f, 6.0f / 5.0f, 3.0f / 2.0f }; return r[degree]; } // Minor
        case 3: { static const float r[] = { 1.0f, 4.0f / 3.0f, 3.0f / 2.0f }; return r[degree]; } // Sus4
        case 4: { static const float r[] = { 1.0f, 5.0f / 4.0f, 15.0f / 8.0f }; return r[degree]; } // Major 7th
        case 5: { static const float r[] = { 1.0f, 45.0f / 32.0f }; return r[degree]; }          // Tritone (dissonant)
        case 6: { static const float r[] = { 1.0f, 16.0f / 15.0f, 256.0f / 225.0f }; return r[degree]; } // Cluster
        default: { static const float r[] = { 1.0f, 64.0f / 45.0f, 36.0f / 25.0f, 21.0f / 16.0f }; return r[degree]; } // Pythagorean mess
    }
}

void QuantumEngine::setFxParams (int type, float depth, float rate, float feedback, float mix, float bits, float entropy, int soloMode)
{
    fx.setParams (type, depth, rate, feedback, mix, bits, entropy, soloMode);
}

void QuantumEngine::handleNoteOn (int note, int velocity)
{
    uiNote.store (note, std::memory_order_relaxed);
    bool superpose = chordMode == 2;
    int type2 = 0;
    float w1 = 1.0f;
    Superposition* rec = nullptr;

    if (superpose)
    {
        for (auto& sp : supers)
            if (sp.note == note) { rec = &sp; break; }
        if (rec == nullptr)
        {
            for (auto& sp : supers)
                if (sp.note < 0) { rec = &sp; break; }
        }
        if (rec != nullptr)
        {
            std::uniform_real_distribution<float> uni (0.0f, 1.0f);
            const float phi = uni (rng) * juce::MathConstants<float>::halfPi;
            w1 = std::cos (phi);
            rec->note = note;
            rec->w1 = w1;
            rec->type2 = (chordType + 1 + (int) (uni (rng) * 6.99f)) % 8; // != chordType
            type2 = rec->type2;
        }
        else
        {
            superpose = false; // no tracking slot left: plain chord
        }
    }

    for (int variant = 0; variant < (superpose ? 2 : 1); ++variant)
    {
        const int type = variant == 0 ? chordType : type2;
        const float ampW = variant == 0 ? w1 : std::sqrt (1.0f - w1 * w1);
        const int degrees = chordMode >= 1 ? chordSize (type) : 1;
        for (int d = 0; d < degrees; ++d)
        {
            int idx = -1;
            for (int i = 0; i < MAX_VOICES; ++i)
                if (! voices[i].isActive()) { idx = i; break; }
            if (idx == -1)
            {
                // steal oldest voice
                unsigned long long oldest = nextAge;
                int oldestIdx = 0;
                for (int i = 0; i < MAX_VOICES; ++i)
                    if (voices[i].isActive() && voiceAge[i] < oldest) { oldest = voiceAge[i]; oldestIdx = i; }
                idx = oldestIdx;
                voices[idx].noteOff();
            }
            const float velScale = (1.0f / (1.0f + 0.35f * (float) d))
                                   * (superpose ? ampW : 1.0f);
            voices[idx].noteOn (note, juce::jlimit (1, 127, juce::roundToInt ((float) velocity * velScale)),
                                chordMode >= 1 ? chordRatio (type, d) : 1.0f);
            voiceNote[idx] = note;
            voiceVariant[idx] = variant;
            voiceAge[idx] = nextAge++;
        }
    }
}

void QuantumEngine::handleNoteOff (int note)
{
    Superposition* rec = nullptr;
    for (auto& sp : supers)
        if (sp.note == note) { rec = &sp; break; }

    if (rec != nullptr)
    {
        // Measurement: the superposition collapses onto one voicing with
        // probability |c|^2; the losing variant is cut, the winner releases.
        std::uniform_real_distribution<float> uni (0.0f, 1.0f);
        const int winner = uni (rng) < rec->w1 * rec->w1 ? 0 : 1;
        for (int i = 0; i < MAX_VOICES; ++i)
            if (voiceNote[i] == note && voices[i].isActive())
            {
                if (winner == voiceVariant[i]) voices[i].noteOff();
                else voices[i].kill();
            }
        rec->note = -1;
    }
    else
    {
        for (int i = 0; i < MAX_VOICES; ++i)
            if (voiceNote[i] == note && voices[i].isActive())
                voices[i].noteOff();
    }
}

void QuantumEngine::resetWalk()
{
    for (auto& a : walkPsi)
        a = std::complex<float> (0.0f, 0.0f);
    walkPsi[0] = std::complex<float> (1.0f, 0.0f);
    walkCountdown = 0;
    if (walkTickSamples <= 0)
        walkTickSamples = juce::jmax (1, juce::roundToInt (sampleRate / 2.0f));
}

void QuantumEngine::stepWalk()
{
    // One ring-graph spread of the walk, then a position "measurement" picks
    // the note with probability |a_i|^2 (pentatonic minor lattice from C3).
    static const int offsets[16] = { 0, 3, 5, 7, 10, 12, 15, 17, 19, 22, 24, 27, 29, 31, 34, 36 };
    std::complex<float> next[16];
    const std::complex<float> iu (0.0f, -0.70710678f);
    for (int i = 0; i < 16; ++i)
        next[i] = walkPsi[i] * 0.70710678f
                + (walkPsi[(i + 15) & 15] + walkPsi[(i + 1) & 15]) * iu;

    float s2 = 0.0f;
    for (int i = 0; i < 16; ++i) s2 += std::norm (next[i]);
    s2 = 1.0f / std::sqrt (std::max (1.0e-12f, s2));
    float prob[16];
    for (int i = 0; i < 16; ++i) { next[i] *= s2; prob[i] = std::norm (next[i]); }
    for (int i = 0; i < 16; ++i) walkPsi[i] = next[i];

    std::uniform_real_distribution<float> uni (0.0f, 1.0f);
    const float target = uni (rng);
    float acc = 0.0f;
    int pick = 15;
    for (int i = 0; i < 16; ++i)
    {
        acc += prob[i];
        if (target <= acc) { pick = i; break; }
    }

    const int note = 48 + offsets[pick];
    const int vel = juce::jlimit (35, 120, juce::roundToInt (35.0f + 85.0f * prob[pick] * 16.0f));
    if (walkPlaying >= 0) handleNoteOff (walkPlaying);
    handleNoteOn (note, vel);
    walkPlaying = note;
}

void QuantumEngine::applyLimiter (juce::AudioBuffer<float>& buffer)
{
    // Brickwall just under full scale: ordinary playing levels never touch the
    // limiter (fast path skips the sample loop); runaway spikes are pulled
    // down with a 1 ms attack / 80 ms release, and the clamp guarantees the
    // output stays inside [-1, 1] during the attack ramp.
    static constexpr float threshold = 0.98f;
    const int n = buffer.getNumSamples();
    const int nc = buffer.getNumChannels();
    float* const* ch = buffer.getArrayOfWritePointers();

    float peak = 0.0f;
    for (int c = 0; c < nc; ++c)
    {
        const auto mm = juce::FloatVectorOperations::findMinAndMax (ch[c], n);
        peak = juce::jmax (peak, juce::jmax (std::abs (mm.getStart()), std::abs (mm.getEnd())));
    }
    if (peak <= threshold && limGain >= 0.9995f)
    {
        limGain = 1.0f; // released: re-arm the skip path exactly
        return;
    }

    const bool hot = peak > threshold;
    for (int i = 0; i < n; ++i)
    {
        if (hot)
        {
            float sp = 0.0f;
            for (int c = 0; c < nc; ++c)
                sp = juce::jmax (sp, std::abs (ch[c][i]));
            const float target = sp > threshold ? threshold / sp : 1.0f;
            limGain += (target - limGain) * (target < limGain ? limAttack : limRelease);
        }
        else
        {
            limGain += (1.0f - limGain) * limRelease;
        }
        for (int c = 0; c < nc; ++c)
            ch[c][i] = juce::jlimit (-1.0f, 1.0f, ch[c][i] * limGain);
    }
}

void QuantumEngine::process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, bool running)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn()) handleNoteOn (msg.getNoteNumber(), msg.getVelocity());
        else if (msg.isNoteOff()) handleNoteOff (msg.getNoteNumber());
    }
    midi.clear();

    if (walkOn && running)
    {
        walkCountdown -= numSamples;
        while (walkCountdown <= 0)
        {
            walkCountdown += walkTickSamples;
            stepWalk();
        }
    }
    else if (walkCountdown < 0)
    {
        walkCountdown = 0;
    }

    float* ch0 = (numChannels > 0) ? buffer.getWritePointer (0) : nullptr;
    if (running && ch0 != nullptr)
    {
        // Voice list resolved once per block; a voice that falls idle mid-block
        // still returns silence from process(), so the sum is unchanged.
        int activeIdx[MAX_VOICES], nActive = 0;
        for (int v = 0; v < MAX_VOICES; ++v)
            if (voices[v].isActive())
            {
                voices[v].prepareBlock();
                activeIdx[nActive++] = v;
            }

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = 0.0f;
            for (int k = 0; k < nActive; ++k)
                sample += voices[activeIdx[k]].process (nullptr);
            ch0[i] = sample;
        }

        // EPR: a quantum jump on one voice instantly collapses its entangled
        // partners (voices sharing the same MIDI note, e.g. superposition
        // variants). collapseNow() never re-arms the flag, so no cascade.
        if (entangleOn && jumpRateAmt > 0.0f && nActive > 1)
        {
            bool jumped[MAX_VOICES] {};
            int notes[MAX_VOICES], nJumped = 0;
            for (int k = 0; k < nActive; ++k)
            {
                const int v = activeIdx[k];
                if (voices[v].jumpedRecently())
                {
                    jumped[v] = true;
                    notes[nJumped++] = voiceNote[v];
                }
            }
            if (nJumped > 0)
                for (int k = 0; k < nActive; ++k)
                {
                    const int v = activeIdx[k];
                    if (jumped[v]) continue;
                    for (int j = 0; j < nJumped; ++j)
                        if (voiceNote[v] == notes[j])
                        {
                            voices[v].forceCollapse();
                            break;
                        }
                }
        }
    }
    else if (ch0 != nullptr)
    {
        juce::FloatVectorOperations::clear (ch0, numSamples);
    }

    if (numChannels > 1)
    {
        for (int c = 1; c < numChannels; ++c)
            juce::FloatVectorOperations::copy (buffer.getWritePointer (c), buffer.getWritePointer (0), numSamples);
    }

    if (running)
    {
        fx.setReferenceHz (tonicHz());
        fx.process (buffer);
    }

    if (running && limiterOn)
        applyLimiter (buffer);

    if (! running)
        uiNote.store (-1, std::memory_order_relaxed);

    // Publish scope snapshot into the back buffer, then flip the slot so the
    // UI can read a stable copy. Falls back to voice 0 when idle so the canvas
    // keeps showing the resting wave/potential instead of going blank.
    const int back = 1 - scopeSlot.load (std::memory_order_relaxed);
    int sV = -1;
    for (int v = 0; v < MAX_VOICES; ++v)
        if (voices[v].isActive()) { sV = v; break; }
    if (sV < 0 && running)
        sV = 0;
    if (sV >= 0)
    {
        voices[sV].fillScope (scopePos[back], SCOPE_SIZE);
        voices[sV].fillScopeMomentum (scopeMom[back], SCOPE_SIZE);
        voices[sV].fillPotential (scopePot[back], SCOPE_SIZE);
    }
    else
    {
        for (int i = 0; i < SCOPE_SIZE; ++i)
        { scopePos[back][i] = 0.0f; scopeMom[back][i] = 0.0f; scopePot[back][i] = 0.0f; }
    }
    scopeSlot.store (back, std::memory_order_release);
    ++scopeSeq;
}

void QuantumEngine::snapshotScope (float* pos, float* mom, float* pot) const
{
    const int front = scopeSlot.load (std::memory_order_acquire);
    if (pos != nullptr) std::memcpy (pos, scopePos[front], sizeof (float) * SCOPE_SIZE);
    if (mom != nullptr) std::memcpy (mom, scopeMom[front], sizeof (float) * SCOPE_SIZE);
    if (pot != nullptr) std::memcpy (pot, scopePot[front], sizeof (float) * SCOPE_SIZE);
}

float QuantumEngine::getUiNoteFreqHz() const
{
    const int note = uiNote.load (std::memory_order_relaxed);
    if (note < 0) return -1.0f;
    return 440.0f * std::pow (2.0f, (note - 69) / 12.0f);
}

float QuantumEngine::tonicHz() const
{
    const int note = uiNote.load (std::memory_order_relaxed);
    if (note < 0)
        return 0.0f;
    // Chord degrees are just ratios of the key note, so the key note is the tonic.
    return 440.0f * std::pow (2.0f, (note - 69) / 12.0f + pitchCents / 1200.0f);
}