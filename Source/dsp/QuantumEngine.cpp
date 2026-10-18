#include "QuantumEngine.h"

void QuantumEngine::prepare (double sampleRateHz)
{
    sampleRate = sampleRateHz <= 0.0 ? 48000.0 : sampleRateHz;
    fx.prepare (sampleRate);
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
}
void QuantumEngine::setParams (float attack, float decay, float sustain, float release,
                    float filterFreq, float filterRes, float drive, float satMix, float gain,
                    int waveType, float waveShift, float waveScale, float waveImpulse,
                    int potentialType, float potentialShift, float potentialScale, float potentialHeight,
                    int showFFT, int sampleMode, float timestepDelta, float samplesPerTimestep, bool applyWavefunction,
                    bool chordOn, int chordTypeSel, float pitchCents, bool tunnel)
{
    chordMode = chordOn;
    chordType = juce::jlimit (0, 7, chordTypeSel);
    for (int i = 0; i < MAX_VOICES; ++i)
        voices[i].setParams (attack, decay, sustain, release, filterFreq, filterRes, drive, satMix, gain,
                             waveType, waveShift, waveScale, waveImpulse,
                             potentialType, potentialShift, potentialScale, potentialHeight,
                             showFFT, sampleMode, timestepDelta, samplesPerTimestep, applyWavefunction,
                             pitchCents, tunnel);
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
    const int degrees = chordMode ? chordSize (chordType) : 1;
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
        const float velScale = 1.0f / (1.0f + 0.35f * (float) d);
        voices[idx].noteOn (note, juce::jlimit (1, 127, juce::roundToInt ((float) velocity * velScale)),
                            chordMode ? chordRatio (chordType, d) : 1.0f);
        voiceNote[idx] = note;
        voiceAge[idx] = nextAge++;
    }
}

void QuantumEngine::handleNoteOff (int note)
{
    for (int i = 0; i < MAX_VOICES; ++i)
        if (voiceNote[i] == note && voices[i].isActive())
            voices[i].noteOff();
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
        fx.process (buffer);

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