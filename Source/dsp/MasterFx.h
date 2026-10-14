#pragma once
#include <JuceHeader.h>

class MasterFx
{
public:
    void prepare (double sampleRateHz);
    void reset();
    void setParams (int type, float depth, float rate, float feedback, float mix, float bits, float entropy, int soloMode);
    void process (juce::AudioBuffer<float>& buffer);

private:
    int type = 0;
    float depth = 0.5f;
    float rate = 1.0f;
    float feedback = 0.3f;
    float mix = 0.5f;
    float bits = 8.0f;
    float entropyAmount = 0.5f;
    int soloMode = 0;
    double sampleRate = 48000.0;
    juce::dsp::ProcessSpec spec {};

    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;

    float lfoPhase = 0.0f;
    float holdCounter = 0.0f;
    float heldL = 0.0f, heldR = 0.0f;
    int holdPeriod = 1;

    float chaosState = 0.37f;
    float fractalAcc = 0.0f;
    std::uint32_t rngState = 0x9e3779b9u;
    juce::AudioBuffer<float> scratch;

    float nextNoise();

    void processBitcrush (juce::AudioBuffer<float>& buffer);
    void processFractal (juce::AudioBuffer<float>& buffer);
};
