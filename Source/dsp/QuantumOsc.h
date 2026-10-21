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
    /** Non-Markovian self-coupling: the potential sees |psi|^2 from delayMs ago. */
    void setMemory (float amount, float delayMs)
    {
        memoryAmt = juce::jlimit (0.0f, 1.0f, amount);
        memoryTimeMs = juce::jlimit (5.0f, 2000.0f, delayMs);
    }
    /** Cavity ringdown (T1 phonon loss): fades the readout while the wave persists. */
    void setRingdown (float amount) { ringdown = juce::jlimit (0.0f, 1.0f, amount); }
    /** Quantum Zeno: probability of a projective position measurement per step. */
    void setZeno (float rate) { zenoAmt = juce::jlimit (0.0f, 1.0f, rate); }
    /** Squeeze of the prepared state: >0 narrows position, <0 narrows momentum. */
    void setSqueeze (float amount) { squeezeAmt = juce::jlimit (-1.0f, 1.0f, amount); }
    /** Gross-Pitaevskii self-interaction: the wave feels an extra potential
        g|psi|^2 (defocusing saturation of peaks). 0 = linear evolution. */
    void setGpSaturation (float amount) { gpAmt = juce::jlimit (0.0f, 1.0f, amount); }
    /** WKB tunneling filter: momentum modes transmit through a virtual barrier
        with T(k) = exp(-2 kappa(k) L); slow components reflect. 0 = off. */
    void setWkbFilter (float amount) { wkbAmt = juce::jlimit (0.0f, 1.0f, amount); }
    /** Dispersive readout: the wavetable is probed after an extra free
        propagation e^{-i alpha k^2}, sweeping quadratic-phase notches
        (quantum phaser). 0 = read the bare wave. */
    void setQPhaser (float amount) { dispAmt = juce::jlimit (0.0f, 1.0f, amount); }
    /** Global dynamical phase e^{-iEt/hbar}: the readout slowly orbits between
        psi and i*psi (per-voice frequency-shift flavour). Berry phase from
        adiabatic potential sweeps is folded into the same rotation. */
    void setPhaseAccum (float amount) { phaseAccAmt = juce::jlimit (0.0f, 1.0f, amount); }
    /** True once after a phonon jump until read (engine uses it for EPR pairing). */
    bool consumeJumped() { const bool j = jumped; jumped = false; return j; }
    /** EPR partner response: apply the annihilation collapse without rolling. */
    void collapseNow();
    void markTablesDirty() { tablesDirty = true; }
    /** Reset the per-block evolution-step budget (call once per audio block). */
    void beginBlock() { stepsUsed = 0; }
    void trigger ();

    /** Returns one wavetable sample advance. Phase is in [0, SIZE). */
    float getNextSample (float phaseIncrement);

    float getPhase() const { return phase; }
    void setPhase (float p) { phase = p; }
    /** Raw table read for cross-voice interference (Bose/Fermi exchange):
        returns the same readout mode the voice itself uses. (Defined in the
        .cpp: it goes through the private inline table samplers.) */
    float readAt (float index) const;

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
    int memoryPushInterval() const;
    void zenoMeasure();
    void applySqueeze();
    void rebuildWkbTable();
    void rebuildDispTable();
    void advanceRotation();
    float buildJumpBuf();
    void applyJumpBuf();
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
    float memoryAmt = 0.0f;
    float memoryTimeMs = 80.0f;
    float ringdown = 0.0f;
    float zenoAmt = 0.0f;
    float squeezeAmt = 0.0f;
    // Quantum Lab (wave 4): evolution-level effects.
    float gpAmt = 0.0f;          // Gross-Pitaevskii g|psi|^2 strength
    float wkbAmt = 0.0f;         // WKB virtual-barrier transmission depth
    float dispAmt = 0.0f;        // dispersive (quantum phaser) readout mix
    float phaseAccAmt = 0.0f;    // global dynamical + Berry phase rotation
    bool jumped = false;
    float damp = 1.0f;
    float memBuf[2][SIZE] {};
    int memHead = 0;
    int memCountdown = 0;
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

    // Quantum Lab tables/state (rebuilt lazily, audio thread only).
    float wkbT[SIZE] {};                  // WKB transmission per momentum bin
    float wkbBuiltAmt = -1.0f;            // wkbAmt the table was built for
    float wkbBuiltVmax = -1.0f;           // and potential height it saw
    std::complex<float> dispPhase[SIZE];  // exp(-i alpha k^2)
    float dispBuiltAmt = -1.0f;           // dispAmt the table was built for
    std::complex<float> dispPsi[SIZE];    // dispersion-rotated readout table
    std::complex<float> dispTmp[SIZE];
    bool dispValid = false;
    // Global phase rotation: (rotC + i rotS) advances by (rotDC + i rotDS)
    // per sample; recomputed after every evolution step from <H>.
    float rotC = 1.0f, rotS = 0.0f;
    float rotDC = 1.0f, rotDS = 0.0f;
    float meanEnergy = 10.0f;             // <H> estimate in simulation units
    float berryPhase = 0.0f;              // geometric phase from adiabatic sweeps
    float lastBerryShift = 0.0f;

    float phase = 0.0f;
    float timestepCounter = 0.0f;
    float timestepCountTo = 0.0f;

    // Real-time load guards: a sample may burst at most maxStepsPerSample
    // evolution steps, and a voice at most blockStepBudget steps per block.
    // Defaults demand 1 step per STEP=8 samples, so the budget only engages on
    // extreme settings (samplesPerTimestep < 1) that would otherwise explode.
    static constexpr int maxStepsPerSample = 4;
    static constexpr int blockStepBudget = 128;
    int stepsUsed = 0;

    juce::dsp::FFT fft { 7 }; // 2^7 = 128
};