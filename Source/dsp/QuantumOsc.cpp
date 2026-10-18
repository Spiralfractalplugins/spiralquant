#include "QuantumOsc.h"
#include <cmath>

unsigned QuantumOsc::instanceCounter = 0;

QuantumOsc::QuantumOsc()
{
    // Mask keeps voice i of every engine instance on the same seed while the
    // eight voices still jump independently (test reproducibility).
    rng.seed (20261018u + (instanceCounter++ & 7u));
}

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
    applySqueeze();
    // Warm the momentum buffer so a quadrature readout is valid before the
    // very first timestep.
    fft.perform (psi, psiFFT, false);
    const float invSqrt = 1.0f / std::sqrt (SIZE_F);
    for (int i = 0; i < SIZE; ++i) psiFFT[i] *= invSqrt;
    inFrequencyDomain = (showFFT != 0);
    for (int s = 0; s < 2; ++s)
        for (int i = 0; i < SIZE; ++i)
            memBuf[s][i] = 0.0f;
    memHead = 0;
    memCountdown = memoryPushInterval();
    damp = 1.0f;
    const int steps = (int) std::max (0.0f, preStartSteps);
    for (int i = 0; i < steps; ++i)
        doTimestep (timestepDelta);
    phase = 0.0f;
    timestepCounter = 0.0f;
    timestepCountTo = samplesPerTimestep <= 0.0f ? 1.0f : samplesPerTimestep;
}

int QuantumOsc::memoryPushInterval() const
{
    const float stepSeconds = (samplesPerTimestep > 0.0f ? samplesPerTimestep : 1.0f) / sampleRate;
    return juce::jmax (1, juce::roundToInt (memoryTimeMs * 0.001f / juce::jmax (1.0e-6f, stepSeconds)));
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

    // Non-Markovian memory: the wave also feels its own density from
    // ~memoryTime ago (delayed Kerr-like self-phase modulation).
    if (memoryAmt > 0.0f)
    {
        const float* delayed = memBuf[1 - memHead];
        const float g = dt * 6.0f * memoryAmt;
        for (int i = 0; i < SIZE; ++i)
            psi[i] *= std::polar (1.0f, g * delayed[i]);
    }

    if (ringdown > 0.0f)
        damp = juce::jmax (1.0e-6f, damp * (1.0f - ringdown * 0.033f * dt));

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

        if (jumpRate > 0.0f)
            maybeJump (dt);

        // Quantum Zeno: frequent projective position measurements freeze the
        // spreading — the wave is pinned near a randomly detected bin.
        if (zenoAmt > 0.0f)
        {
            std::uniform_real_distribution<float> zd (0.0f, 1.0f);
            if (zd (rng) < zenoAmt)
                zenoMeasure();
        }
    }
    else
    {
        for (int i = 0; i < SIZE; ++i) psi[i] = psiFFT[i];

        float s = 0.0f;
        for (int i = 0; i < SIZE; ++i) s += std::norm (psi[i]);
        s = 1.0f / std::sqrt (std::max (1.0e-12f, s));
        for (int i = 0; i < SIZE; ++i) psi[i] *= s;
    }

    // Publish this step's density into the ring once per delay slice, so the
    // slot the next step reads back is ~memoryTime old.
    if (memoryAmt > 0.0f && --memCountdown <= 0)
    {
        float* back = memBuf[memHead];
        for (int i = 0; i < SIZE; ++i)
            back[i] = std::norm (psi[i]) * SIZE_F;
        memHead = 1 - memHead;
        memCountdown = memoryPushInterval();
    }
    inFrequencyDomain = (showFFT != 0);
}

float QuantumOsc::buildJumpBuf()
{
    // Monte-Carlo wavefunction: emission of one phonon applies the annihilation
    // operator L = (u + d/du)/sqrt(2), u = x/sigma. Returns ||L psi||^2.
    const float sigma = 0.2f;
    const float dx = 2.0f / (float) (SIZE - 1);
    const float inv2dx = sigma / (2.0f * dx);

    float s2 = 0.0f;
    for (int i = 0; i < SIZE; ++i)
    {
        const float x = 2.0f * (float) i / (float) (SIZE - 1) - 1.0f;
        const int im = juce::jmax (0, i - 1);
        const int ip = juce::jmin (SIZE - 1, i + 1);
        jumpBuf[i] = (x / sigma) * psi[i] + inv2dx * (psi[ip] - psi[im]);
        s2 += std::norm (jumpBuf[i]);
    }
    return s2;
}

void QuantumOsc::applyJumpBuf()
{
    float s = 0.0f;
    for (int i = 0; i < SIZE; ++i)
    {
        psi[i] = jumpBuf[i];
        s += std::norm (psi[i]);
    }
    s = 1.0f / std::sqrt (std::max (1.0e-12f, s));
    for (int i = 0; i < SIZE; ++i) psi[i] *= s;
}

void QuantumOsc::maybeJump (float dt)
{
    const float s2 = buildJumpBuf();

    // 2e-5 tunes jumpRate=1 to tens of collapses per second at the default dt/spt.
    const float p = juce::jlimit (0.0f, 0.25f, jumpRate * s2 * dt * 2.0e-5f * SIZE_F);
    std::uniform_real_distribution<float> dist (0.0f, 1.0f);
    if (dist (rng) >= p)
        return;

    applyJumpBuf();
    jumped = true; // engine propagates this to EPR partners of the same note
}

void QuantumOsc::collapseNow()
{
    buildJumpBuf();
    applyJumpBuf();
}

void QuantumOsc::zenoMeasure()
{
    // Born-sampled position, then Gaussian projection window around the hit.
    float s2 = 0.0f;
    for (int i = 0; i < SIZE; ++i)
        s2 += std::norm (psi[i]);
    if (s2 <= 0.0f)
        return;

    std::uniform_real_distribution<float> dist (0.0f, 1.0f);
    float target = dist (rng) * s2;
    int c = SIZE - 1;
    for (int i = 0; i < SIZE; ++i)
    {
        target -= std::norm (psi[i]);
        if (target <= 0.0f) { c = i; break; }
    }

    const float inv2w2 = 1.0f / (2.0f * 2.5f * 2.5f); // sigma ~ 2.5 bins
    for (int i = 0; i < SIZE; ++i)
    {
        const float d = (float) (i - c);
        psi[i] *= std::exp (-d * d * inv2w2);
    }
    float s = 0.0f;
    for (int i = 0; i < SIZE; ++i) s += std::norm (psi[i]);
    s = 1.0f / std::sqrt (std::max (1.0e-12f, s));
    for (int i = 0; i < SIZE; ++i) psi[i] *= s;
}

void QuantumOsc::applySqueeze()
{
    // Symplectic squeeze of the prepared state: (S psi)(y) = sqrt(sf) psi(sf*y),
    // applied on the position grid (squeeze > 0, packet narrows, momentum
    // spreads) or on the momentum grid (< 0, spectrum narrows, position
    // spreads). k-space resampling wraps cyclically around DC so both
    // frequency halves squeeze symmetrically.
    if (squeezeAmt == 0.0f)
        return;

    const float sf = std::exp (std::abs (squeezeAmt) * 1.2f);
    const float amp = std::sqrt (sf);
    const bool inK = squeezeAmt < 0.0f;
    std::complex<float>* a = psi;
    if (inK)
    {
        fft.perform (psi, psiFFT, false);
        const float invSqrt = 1.0f / std::sqrt ((float) SIZE);
        for (int i = 0; i < SIZE; ++i) psiFFT[i] *= invSqrt;
        a = psiFFT;
    }

    if (inK)
    {
        for (int i = 0; i < SIZE; ++i)
        {
            const float fq = (float) (i < SIZE / 2 ? i : i - SIZE);
            const float src = std::fmod (fq * sf + (float) SIZE, (float) SIZE);
            const int i0 = (int) src;
            const int i1 = (i0 + 1) % SIZE;
            const float f = src - (float) i0;
            jumpBuf[i] = (a[i0] + (a[i1] - a[i0]) * f) * amp;
        }
    }
    else
    {
        const float mid = (float) (SIZE - 1) * 0.5f;
        for (int i = 0; i < SIZE; ++i)
        {
            const float src = ((float) i - mid) * sf + mid;
            if (src < 0.0f || src >= (float) (SIZE - 1)) { jumpBuf[i] = {}; continue; }
            const int i0 = (int) src;
            const float f = src - (float) i0;
            jumpBuf[i] = (a[i0] + (a[i0 + 1] - a[i0]) * f) * amp;
        }
    }
    for (int i = 0; i < SIZE; ++i) a[i] = jumpBuf[i];

    if (inK)
    {
        fft.perform (psiFFT, psi, true);
        const float invSqrt = 1.0f / std::sqrt ((float) SIZE);
        for (int i = 0; i < SIZE; ++i) psi[i] *= invSqrt;
    }

    float s = 0.0f;
    for (int i = 0; i < SIZE; ++i) s += std::norm (psi[i]);
    s = 1.0f / std::sqrt (std::max (1.0e-12f, s));
    for (int i = 0; i < SIZE; ++i) psi[i] *= s;
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

    std::complex<float> v = sampleWave (phase);
    // Approximate fractional-Fourier readout: rotate the sampled quadrature
    // between psi(x) and the momentum state of the last step. Degenerate when
    // showFFT already reads psi(k).
    if (quadrature > 0.0f && ! showFFT)
        v = v * (1.0f - quadrature) + sampleArray (psiFFT, phase) * quadrature;

    phase += phaseIncrement;
    if (phase >= (float) SIZE) phase -= (float) SIZE;
    if (phase < 0.0f) phase = 0.0f;

    return toReal (v) * damp;
}

std::complex<float> QuantumOsc::sampleWave (float index) const
{
    return sampleArray (psi, index);
}

std::complex<float> QuantumOsc::sampleArray (const std::complex<float>* arr, float index) const
{
    int i0 = (int) index;
    const float frac = index - (float) i0;
    if (i0 < 0) i0 = 0;
    if (i0 >= SIZE) i0 = SIZE - 1;
    const int i1 = (i0 + 1 == SIZE) ? 0 : i0 + 1;
    return arr[i0] * (1.0f - frac) + arr[i1] * frac;
}

float QuantumOsc::toReal (std::complex<float> v) const
{
    if (sampleMode == 1) return v.imag();
    if (sampleMode == 2) return std::abs (v);
    // Identical to sin(arg(v)) but skips an atan2 plus a sine per sample.
    if (sampleMode == 3) return v.imag () / std::max (1.0e-12f, std::abs (v));
    // Phonon QND: staircase the amplitude onto discrete levels, so the
    // discreteness of the measured energy is audible even mid-sweep.
    if (sampleMode == 4) return std::round (v.real() * 8.0f) / 8.0f;
    return v.real();
}

void QuantumOsc::fillScope (float* dest, int maxN) const
{
    const int n = juce::jmin (maxN, SIZE);
    for (int i = 0; i < n; ++i) dest[i] = std::abs (psi[i]);
}

void QuantumOsc::fillScopeMomentum (float* dest, int maxN) const
{
    const int n = juce::jmin (maxN, SIZE);
    // With showFFT on, psi already holds the (normalised) momentum wavefunction.
    for (int i = 0; i < n; ++i) dest[i] = std::abs (showFFT ? psi[i] : psiFFT[i]);
}

void QuantumOsc::fillPotential (float* dest, int maxN) const
{
    const int n = juce::jmin (maxN, SIZE);
    for (int i = 0; i < n; ++i) dest[i] = potential[i];
}