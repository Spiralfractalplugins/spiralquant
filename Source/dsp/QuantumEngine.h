#pragma once
#include <JuceHeader.h>
#include "QuantumVoice.h"
#include "MasterFx.h"

class QuantumEngine
{
public:
    static constexpr int MAX_VOICES = 8;
    void prepare (double sampleRateHz);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, bool running);
    void setParams (float attack, float decay, float sustain, float release,
                    float filterFreq, float filterRes, float drive, float satMix, float gain,
                    int waveType, float waveShift, float waveScale, float waveImpulse,
                    int potentialType, float potentialShift, float potentialScale, float potentialHeight,
                    int showFFT, int sampleMode, float timestepDelta, float samplesPerTimestep, bool applyWavefunction);
    void setFxParams (int type, float depth, float rate, float feedback, float mix, float bits, float entropy, int soloMode);

    const float* getScopeWave() const { return scopeWave; }
    static constexpr int SCOPE_SIZE = QuantumOsc::SIZE;

private:
    QuantumVoice voices[MAX_VOICES];
    MasterFx fx;    int voiceNote[MAX_VOICES] = { -1,-1,-1,-1,-1,-1,-1,-1 };
    unsigned long long voiceAge[MAX_VOICES] = {};
    unsigned long long nextAge = 1;
    double sampleRate = 48000.0;
    float scopeWave[QuantumOsc::SIZE] {};
    void handleNoteOn (int note, int velocity);
    void handleNoteOff (int note);
};