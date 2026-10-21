#pragma once
#include <JuceHeader.h>
#include "dsp/QuantumEngine.h"

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SpiralQuant"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    /** Release tails run up to 5 s (RELEASE max) and the Fractal comb rings on
        after that, so hosts that honour the tail must keep rendering. */
    double getTailLengthSeconds() const override { return 5.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;
    QuantumEngine engine;

    void snapshotScope (float* pos, float* mom, float* pot) const { engine.snapshotScope (pos, mom, pot); }
    unsigned getScopeSeq() const { return engine.getScopeSeq(); }
    float getUiNoteFreqHz() const { return engine.getUiNoteFreqHz(); }

private:
    struct ParamCache
    {
        std::atomic<float>* waveType;
        std::atomic<float>* waveShift;
        std::atomic<float>* waveScale;
        std::atomic<float>* waveImpulse;
        std::atomic<float>* potentialType;
        std::atomic<float>* potentialShift;
        std::atomic<float>* potentialScale;
        std::atomic<float>* potentialHeight;
        std::atomic<float>* showFFT;
        std::atomic<float>* sampleMode;
        std::atomic<float>* timestepDelta;
        std::atomic<float>* samplesPerTimestep;
        std::atomic<float>* applyWavefunction;
        std::atomic<float>* attack;
        std::atomic<float>* decay;
        std::atomic<float>* sustain;
        std::atomic<float>* release;
        std::atomic<float>* filterFreq;
        std::atomic<float>* filterRes;
        std::atomic<float>* drive;
        std::atomic<float>* satMix;
        std::atomic<float>* gain;
        std::atomic<float>* fxType;
        std::atomic<float>* fxDepth;
        std::atomic<float>* fxRate;
        std::atomic<float>* fxFeedback;
        std::atomic<float>* fxMix;
        std::atomic<float>* fxBits;
        std::atomic<float>* fxEntropy;
        std::atomic<float>* fxSolo;
        std::atomic<float>* chordMode;
        std::atomic<float>* chordType;
        std::atomic<float>* pitchShift;
        std::atomic<float>* tunnel;
        std::atomic<float>* chordSuperpose;
        std::atomic<float>* jumpRate;
        std::atomic<float>* frftAngle;
        std::atomic<float>* memoryAmt;
        std::atomic<float>* memoryTime;
        std::atomic<float>* ringdown;
        std::atomic<float>* walkOn;
        std::atomic<float>* walkRate;
        std::atomic<float>* limiter;
        std::atomic<float>* zeno;
        std::atomic<float>* entangle;
        std::atomic<float>* squeeze;
        std::atomic<float>* gpSat;
        std::atomic<float>* wkbFilter;
        std::atomic<float>* qPhaser;
        std::atomic<float>* phaseAccum;
        std::atomic<float>* interfere;
        std::atomic<float>* labType;
        std::atomic<float>* labAmt;
        std::atomic<float>* labChar;
        std::atomic<float>* labSize;
        std::atomic<float>* labMix;
    };

    ParamCache makeParamCache() const;
    ParamCache params;
    double currentSampleRate = 48000.0;
};