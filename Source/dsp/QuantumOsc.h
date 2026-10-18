#pragma once

#include <JuceHeader.h>
#include <complex>
#include <random>

/** Quantum wavefunction oscillator.

    A voice's wavetable is a complex-valued solution of the 1D time-dependent
    Schrödinger equation. The table is evolved with a split-step Fourier method
    and read back as an audio wavetable by scanning it at the note frequency. */
class QuantumOsc
{
public:
    static constexpr int SIZE = 128;
    static constexpr float SIZE_F = (float) SIZE;

    /** Per-instance RNG seed so the eight voices do not collapse in lockstep. */
    QuantumOsc();

    void prepare (double sampleRateHz);
    void reset (int waveType, float waveShift, float waveScale, float waveImpulse,
                int potentialType, float potentialShift, float potentialScale, float potentialHeight,
                int showFFT, int sampleMode, float timestepDelta, float samplesPerTimestep,
                float preStartSteps);
    void setPotential (int potentialType, float potentialShift, float potentialScale, float potentialHeight);
    void setWaveParameters (int waveType, float waveShift, float waveScale, float waveImpulse);
    void setEvolution (int showFFT, int sampleMode, float timestepDelta, float samplesPerTimestep);
    void setTunneling (bool enabled);
    bool hasTunneling() const { return tunnelOn; }
    void setApplyWavefunction (bool enabled) { applyWavefunction = enabled; }
    /** Monte-Carlo-wavefunction phonon-emission rate (0 = unitary evolution). */
    void setJumpRate (float rate) { jumpRate = juce::jlimit (0.0f, 1.0f, rate); }
    /** Readout quadrature rotation: 0 = psi(x), 1 = psi(k). */
    void setQuadrature (float q) { quadrature = juce::jlimit (0.0f, 1.0f, q); }
    void markTablesDirty() { tablesDirty = true; }
    void trigger ();

    /** Returns one wavetable sample advance. Phase is in [0, SIZE). */
    float getNextSample (float phaseIncrement);

    float getPhase() const { return phase; }
    void setPhase (float p) { phase = p; }

    /** Copy latest |psi| outline for the scope (thread-unsafe; caller guards). */
    void fillScope (float* dest, int maxN) const;
    void fillScopeMomentum (float* dest, int maxN) const;
    void fillPotential (float* dest, int maxN) const;

private:
    void generateInitial();
    void computePotential();
    void rebuildTables();
    void doTimestep (float dt);
    void maybeJump (float dt);
    inline std::complex<float> sampleWave (float index) const;
    inline std::complex<float> sampleArray (const std::complex<float>* arr, float index) const;
    inline float toReal (std::complex<float> v) const;

    double sampleRate = 48000.0;
    int waveType = 0;
    float waveShift = 0.0f;
    float waveScale = 0.5f;
    float waveImpulse = 0.0f;
    int potentialType = 0;
    float potentialShift = 0.0f;
    float potentialScale = 0.5f;
    float potentialHeight = 0.5f;
    int showFFT = 0;
    int sampleMode = 0;
    float timestepDelta = 0.01f;
    float samplesPerTimestep = 8.0f;
    float preStartSteps = 4.0f;
    bool applyWavefunction = true;
    bool tunnelOn = false;
    float jumpRate = 0.0f;
    float quadrature = 0.0f;
    std::mt19937 rng;
    static unsigned instanceCounter;

    std::complex<float> psi[SIZE];
    std::complex<float> psiFFT[SIZE];
    std::complex<float> jumpBuf[SIZE];
    float potential[SIZE];
    bool mask[SIZE];
    std::complex<float> potPhase[SIZE];   // exp(j * dt * potential[i])
    std::complex<float> kinPhase[SIZE];   // exp(j * PRE * k^2)
    bool tablesDirty = true;
    float tableDt = -1.0f;
    bool inFrequencyDomain = false;

    float phase = 0.0f;
    float timestepCounter = 0.0f;
    float timestepCountTo = 0.0f;

    juce::dsp::FFT fft { 7 }; // 2^7 = 128
};