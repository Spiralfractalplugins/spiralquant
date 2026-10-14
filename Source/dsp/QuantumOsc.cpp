#include "QuantumOsc.h"
#include <cmath>

void QuantumOsc::prepare (double sampleRateHz)
{
    sampleRate = sampleRateHz <= 0.0 ? 48000.0 : sampleRateHz;
}

void QuantumOsc::reset (int wt, float ws, float wsc, float wi,
                         int pt, float ps, float psc, float ph,
                         int sf, int sm, float td, float spt, float psSteps)
{
    waveType = wt; waveShift = ws; waveScale = wsc; waveImpulse = wi;
    potentialType = pt; potentialShift = ps; potentialScale = psc; potentialHeight = ph;
    showFFT = sf; sampleMode = sm; timestepDelta = td; samplesPerTimestep = spt; preStartSteps = psSteps;
    applyWavefunction = true;
}

void QuantumOsc::setPotential (int pt, float ps, float psc, float ph)
{
    if (pt == potentialType && ps == potentialShift && psc == potentialScale && ph == potentialHeight)
        return;
    potentialType = pt; potentialShift = ps; potentialScale = psc; potentialHeight = ph;
    computePotential();
    tablesDirty = true;
}

void QuantumOsc::setWaveParameters (int wt, float ws, float wsc, float wi)
{
    waveType = wt; waveShift = ws; waveScale = wsc; waveImpulse = wi;
}

void QuantumOsc::setEvolution (int sf, int sm, float td, float spt)
{
    showFFT = sf; sampleMode = sm; timestepDelta = td; samplesPerTimestep = spt;
}

void QuantumOsc::rebuildTables()
{
    const float dt = timestepDelta;
    const float PRE = std::pow (juce::MathConstants<float>::twoPi / (float) SIZE, 2) * dt;

    for (int i = 0; i < SIZE; ++i)
        potPhase[i] = std::polar (1.0f, dt * potential[i]);

    for (int i = 0; i < SIZE; ++i)
    {
        const int k = (i <= SIZE / 2) ? i : i - SIZE;
        kinPhase[i] = std::polar (1.0f, PRE * (float) (k * k));
    }

    tableDt = dt;
    tablesDirty = false;
}

void QuantumOsc::trigger ()
{
    generateInitial();
    computePotential();
    inFrequencyDomain = (showFFT != 0);
    const int steps = (int) std::max (0.0f, preStartSteps);
    for (int i = 0; i < steps; ++i)
        doTimestep (timestepDelta);
    phase = 0.0f;
    timestepCounter = 0.0f;
    timestepCountTo = samplesPerTimestep <= 0.0f ? 1.0f : samplesPerTimestep;
}

void QuantumOsc::generateInitial ()
{
    for (int i = 0; i < SIZE; ++i)
    {
        const float x = (SIZE <= 1) ? 0.0f : (2.0f * (float) i / (float) (SIZE - 1) - 1.0f);
        std::complex<float> v (0.0f, 0.0f);

        if (waveType == 0) // Gaussian
        {
            const float width = 0.05f + 0.45f * juce::jlimit (0.0f, 1.0f, waveScale);
            const float c = juce::jlimit (-1.0f, 1.0f, waveShift);
            const float d = x - c;
            v = std::exp (-d * d / (2.0f * width * width)) *
                std::polar (1.0f, juce::MathConstants<float>::pi * waveImpulse * x);
        }
        else if (waveType == 1) // Sine
        {
            const float k = 1.0f + 4.0f * juce::jlimit (0.0f, 1.0f, waveScale);
            v = std::sin (juce::MathConstants<float>::pi * k * (x + 1.0f));
        }
        else if (waveType == 2) // Cos
        {
            const float k = 1.0f + 4.0f * juce::jlimit (0.0f, 1.0f, waveScale);
            v = std::cos (juce::MathConstants<float>::pi * k * (x + 1.0f));
        }
        else if (waveType == 3) // Parabola
        {
            const float c = juce::jlimit (-1.0f, 1.0f, waveShift);
            v = (x - c) * (x - c);
        }
        else if (waveType == 4) // Saw ramp
        {
            v = x;
        }
        else // Square
        {
            v = (x < juce::jlimit (-1.0f, 1.0f, waveShift)) ? 1.0f : -1.0f;
        }

        psi[i] = v;
    }

    // normalise
    float s = 0.0f;
    for (int i = 0; i < SIZE; ++i) s += std::norm (psi[i]);
    s = std::sqrt (std::max (1.0e-12f, s));
    for (int i = 0; i < SIZE; ++i) psi[i] /= s;
}

void QuantumOsc::setTunneling (bool enabled)
{
    if (tunnelOn == enabled)
        return;
    tunnelOn = enabled;
    computePotential();
}

void QuantumOsc::computePotential ()
{
    for (int i = 0; i < SIZE; ++i)
    {
        const float x = (SIZE <= 1) ? 0.0f : (2.0f * (float) i / (float) (SIZE - 1) - 1.0f);
        const float sc = juce::jlimit (0.0f, 1.0f, potentialScale);
        const float h = juce::jlimit (0.0f, 1.0f, potentialHeight);
        float v = 0.0f;
        bool m = true;

        if (potentialType == 1) // Harmonic
        {
            v = 0.5f * (5.0f + 40.0f * sc) * (x - juce::jlimit (-1.0f, 1.0f, potentialShift)) *
                (x - juce::jlimit (-1.0f, 1.0f, potentialShift));
        }
        else if (potentialType == 2) // Infinite well
        {
            v = (std::fabs (x) > 0.9f) ? 1000.0f : 0.0f;
            m = (std::fabs (x) <= 0.9f);
        }
        else if (potentialType == 3) // Barrier
        {
            const float pos = juce::jlimit (-1.0f, 1.0f, potentialShift);
            const float width = 0.05f + 0.4f * sc;
            if (std::fabs (x - pos) < width)
            {
                v = 1000.0f;
                m = false;
            }
        }
        else if (potentialType == 4) // Double well
        {
            v = 5.0f * (5.0f + 40.0f * sc) * ((x * x - 0.25f) * (x * x - 0.25f));
        }
        else if (potentialType == 5) // Linear
        {
            v = (10.0f + 40.0f * sc) * x + 5.0f * juce::jlimit (-1.0f, 1.0f, potentialShift);
        }
        else
        {
            v = 0.0f;
        }

        v *= (0.25f + 0.75f * h);
        potential[i] = v;
        mask[i] = m;
    }

    if (tunnelOn)
    {
        // Gamow transparency T = exp(-2 kappa L), kappa = sqrt(2m(V-E)).
        // Wave energy E ~ vmax/3 (dominant confined eigenmode); the grid
        // dx is mapped to an effective barrier width so narrow barriers
        // leak audibly while wide ones stay near-opaque.
        float vmax = 0.0f;
        for (int i = 0; i < SIZE; ++i) vmax = juce::jmax (vmax, potential[i]);
        const float e = vmax / 3.0f;
        for (int i = 0; i < SIZE; ++i)
        {
            if (mask[i]) continue;
            const float v = juce::jmax (0.0f, potential[i] - e);
            const float kappa = std::sqrt (2.0f * v) * 0.01f;
            potential[i] *= std::exp (-2.0f * kappa);
            mask[i] = true; // partial transmission instead of hard zero
        }
    }
}

void QuantumOsc::doTimestep (float dt)
{
    if (tablesDirty || dt != tableDt)
        rebuildTables();

    // potential part in position domain
    for (int i = 0; i < SIZE; ++i)
        psi[i] *= potPhase[i];

    // FFT to momentum domain
    fft.perform (psi, psiFFT, false);
    const float invSqrt = 1.0f / std::sqrt ((float) SIZE);
    for (int i = 0; i < SIZE; ++i) psiFFT[i] *= invSqrt;

    // kinetic part
    for (int i = 0; i < SIZE; ++i)
        psiFFT[i] *= kinPhase[i];

    if (! showFFT)
    {
        fft.perform (psiFFT, psi, true);
        for (int i = 0; i < SIZE; ++i) psi[i] *= invSqrt;

        // barrier mask + norm accumulation in one pass
        float s = 0.0f;
        for (int i = 0; i < SIZE; ++i)
        {
            if (! mask[i]) psi[i] = std::complex<float> (0.0f, 0.0f);
            s += std::norm (psi[i]);
        }
        s = 1.0f / std::sqrt (std::max (1.0e-12f, s));
        for (int i = 0; i < SIZE; ++i) psi[i] *= s;
    }
    else
    {
        for (int i = 0; i < SIZE; ++i) psi[i] = psiFFT[i];

        float s = 0.0f;
        for (int i = 0; i < SIZE; ++i) s += std::norm (psi[i]);
        s = 1.0f / std::sqrt (std::max (1.0e-12f, s));
        for (int i = 0; i < SIZE; ++i) psi[i] *= s;
    }
    inFrequencyDomain = (showFFT != 0);
}

float QuantumOsc::getNextSample (float phaseIncrement)
{
    if (applyWavefunction)
    {
        if (samplesPerTimestep >= 1.0f)
        {
            timestepCounter += 1.0f;
            if (timestepCounter >= samplesPerTimestep)
            {
                doTimestep (timestepDelta);
                timestepCounter = 0.0f;
            }
        }
        else
        {
            while (timestepCounter < 1.0f)
            {
                timestepCounter += samplesPerTimestep;
                doTimestep (timestepDelta);
            }
            timestepCounter = std::fmod (timestepCounter, 1.0f);
        }
    }

    const std::complex<float> v = sampleWave (phase);
    phase += phaseIncrement;
    if (phase >= (float) SIZE) phase -= (float) SIZE;
    if (phase < 0.0f) phase = 0.0f;

    return toReal (v);
}

std::complex<float> QuantumOsc::sampleWave (float index) const
{
    int i0 = (int) index;
    const float frac = index - (float) i0;
    if (i0 < 0) i0 = 0;
    if (i0 >= SIZE) i0 = SIZE - 1;
    const int i1 = (i0 + 1 == SIZE) ? 0 : i0 + 1;
    return psi[i0] * (1.0f - frac) + psi[i1] * frac;
}

float QuantumOsc::toReal (std::complex<float> v) const
{
    if (sampleMode == 1) return v.imag();
    if (sampleMode == 2) return std::abs (v);
    if (sampleMode == 3) return std::sin (std::arg (v));
    return v.real();
}

void QuantumOsc::fillScope (float* dest, int maxN) const
{
    const int n = juce::jmin (maxN, SIZE);
    for (int i = 0; i < n; ++i) dest[i] = std::abs (psi[i]);
}