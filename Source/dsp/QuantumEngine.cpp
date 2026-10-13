#include "QuantumEngine.h"

void QuantumEngine::prepare (double sampleRateHz)
{
    sampleRate = sampleRateHz <= 0.0 ? 48000.0 : sampleRateHz;
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        voices[i].prepare (sampleRate);
        voiceNote[i] = -1;
        voiceAge[i] = 0;
    }
    reset();
    for (int i = 0; i < QuantumOsc::SIZE; ++i) scopeWave[i] = 0.0f;
}

void QuantumEngine::reset()
{
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
                    int showFFT, int sampleMode, float timestepDelta, float samplesPerTimestep, bool applyWavefunction)
{
    for (int i = 0; i < MAX_VOICES; ++i)
        voices[i].setParams (attack, decay, sustain, release, filterFreq, filterRes, drive, satMix, gain,
                             waveType, waveShift, waveScale, waveImpulse,
                             potentialType, potentialShift, potentialScale, potentialHeight,
                             showFFT, sampleMode, timestepDelta, samplesPerTimestep, applyWavefunction);
}

void QuantumEngine::handleNoteOn (int note, int velocity)
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
    voices[idx].noteOn (note, velocity);
    voiceNote[idx] = note;
    voiceAge[idx] = nextAge++;
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
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        if (running)
            for (int v = 0; v < MAX_VOICES; ++v)
                if (voices[v].isActive())
                    sample += voices[v].process (nullptr);
        if (ch0) ch0[i] = sample;
    }

    if (numChannels > 1)
    {
        for (int c = 1; c < numChannels; ++c)
            juce::FloatVectorOperations::copy (buffer.getWritePointer (c), buffer.getWritePointer (0), numSamples);
    }

    // scope from first active voice
    int sV = -1;
    for (int v = 0; v < MAX_VOICES; ++v)
        if (voices[v].isActive()) { sV = v; break; }
    if (sV >= 0) voices[sV].fillScope (scopeWave, QuantumOsc::SIZE);
    else for (int i = 0; i < QuantumOsc::SIZE; ++i) scopeWave[i] = 0.0f;
}