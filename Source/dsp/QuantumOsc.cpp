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
    // Berry phase: an adiabatic displacement of the potential centre winds the
    // geometric phase gamma ~ <k> dx; a later sweep back does not unwind it
    // (only the dynamical part is single-valued). The momentum kick of the
    // prepared wave scales the winding, as in the report's eq. for gamma.
    if (phaseAccAmt > 0.0f && ps != lastBerryShift && waveImpulse > 0.0f)
    {
        const float dGamma = (ps - lastBerryShift) * waveImpulse * 4.0f
                             * juce::MathConstants<float>::twoPi;
        berryPhase += dGamma;
        const float b = berryPhase;
        const float cb = std::cos (b), sb = std::sin (b);
        const float nrc = rotC * cb - rotS * sb;
        rotS = rotC * sb + rotS * cb;
        rotC = nrc;
    }
    if (ps != lastBerryShift)
        lastBerryShift = ps;

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
    // The forward-FFT 1/sqrt(N) scale is folded into the kinetic table, so every
    // timestep skips a 128-element complex scale loop. The inverse-FFT scale is
    // dropped entirely: the renormalisation that always follows it makes the
    // result scale-invariant (normalise(c*psi) == normalise(psi)).
    const float invSqrtN = 1.0f / std::sqrt (SIZE_F);

    for (int i = 0; i < SIZE; ++i)
        potPhase[i] = std::polar (1.0f, dt * potential[i]);

    for (int i = 0; i < SIZE; ++i)
    {
        const int k = (i <= SIZE / 2) ? i : i - SIZE;
        kinPhase[i] = std::polar (1.0f, PRE * (float) (k * k)) * invSqrtN;
    }

    rebuildWkbTable();

    tableDt = dt;
    tablesDirty = false;
}

void QuantumOsc::rebuildWkbTable()
{
    // WKB transmission of a virtual rectangular barrier of height Veff:
    //   T(k) = exp(-2 kappa L),  kappa = sqrt(2 (Veff - E_k)),  E_k = ktilde^2 / 2.
    // Fast (high-energy) components pass, slow ones reflect — a high-pass with
    // the characteristic exponential WKB skirt. The knob sweeps Veff, so the
    // cutoff glides from bin ~12 (soft) to ~43 (hard) across the range.
    wkbBuiltAmt = wkbAmt;
    wkbBuiltVmax = 0.0f;
    for (int i = 0; i < SIZE; ++i)
        wkbBuiltVmax = juce::jmax (wkbBuiltVmax, potential[i]);

    const float veff = 0.05f + 2.2f * wkbAmt * wkbAmt;
    const float kFac = juce::MathConstants<float>::twoPi / SIZE_F;
    for (int i = 0; i < SIZE; ++i)
    {
        const int k = std::abs ((i <= SIZE / 2) ? i : i - SIZE);
        const float kt = kFac * (float) k;
        const float ek = 0.5f * kt * kt;
        const float kappa2 = 2.0f * (veff - ek);
        wkbT[i] = kappa2 > 0.0f ? std::exp (-2.0f * std::sqrt (kappa2)) : 1.0f;
    }
}

void QuantumOsc::rebuildDispTable()
{
    // Quantum phaser: an extra free propagation by alpha before the readout.
    // e^{-i alpha k^2} is unit-modulus, so the spectral balance is untouched;
    // the quadratic phase sweeps notches whose spacing widens with k — the
    // dispersive analogue of a phaser bank.
    dispBuiltAmt = dispAmt;
    const float alpha = 0.004f * dispAmt * dispAmt;
    const float kFac = juce::MathConstants<float>::twoPi / SIZE_F;
    for (int i = 0; i < SIZE; ++i)
    {
        const int k = (i <= SIZE / 2) ? i : i - SIZE;
        dispPhase[i] = std::polar (1.0f, -alpha * (float) (k * k) * kFac * kFac);
    }
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
    // Quantum Lab state resets with the prepared state.
    rotC = 1.0f; rotS = 0.0f;
    rotDC = 1.0f; rotDS = 0.0f;
    berryPhase = 0.0f;
    lastBerryShift = potentialShift;
    meanEnergy = 10.0f;
    dispValid = false;
    if (wkbAmt != wkbBuiltAmt)
        rebuildWkbTable();
    if (dispAmt != dispBuiltAmt)
        rebuildDispTable();
    const int steps = (int) std::max (0.0f, preStartSteps);
    for (int i = 0; i < steps; ++i)
        doTimestep (timestepDelta);
    if (dispAmt > 0.0f && ! showFFT)
    {
        // One dispersion pass so the phaser readout is valid immediately.
        for (int i = 0; i < SIZE; ++i)
            dispTmp[i] = psiFFT[i] * dispPhase[i];
        fft.perform (dispTmp, dispPsi, true);
        float s = 0.0f;
        for (int i = 0; i < SIZE; ++i) s += std::norm (dispPsi[i]);
        s = 1.0f / std::sqrt (std::max (1.0e-12f, s));
        for (int i = 0; i < SIZE; ++i) dispPsi[i] *= s;
        dispValid = true;
    }
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
    if (wkbAmt != wkbBuiltAmt)
        rebuildWkbTable();
    if (dispAmt != dispBuiltAmt)
        rebuildDispTable();

    const bool gpOn = gpAmt > 0.0f;
    const bool accOn = phaseAccAmt > 0.0f;
    const bool wkbOn = wkbAmt > 0.0f;
    const float gpGain = 400.0f * gpAmt;
    float vE = 0.0f, pW = 0.0f;   // <V> numerator / denominator
    float kE = 0.0f, kW = 0.0f;   // <k^2> numerator / denominator

    // potential part in position domain
    for (int i = 0; i < SIZE; ++i)
    {
        std::complex<float> v = psi[i] * potPhase[i];
        // Gross-Pitaevskii saturation: a defocusing g|psi|^2 term adds an
        // amplitude-dependent phase, so spectral peaks self-repel — the
        // nonlinear ceiling of the report's section 9. Renormalisation below
        // keeps the focusing branch bounded as well.
        if (gpOn)
        {
            const float n = std::norm (v);
            v *= std::polar (1.0f, dt * gpGain * n);
        }
        psi[i] = v;
        if (accOn)
        {
            const float w = std::norm (v);
            vE += w * potential[i];
            pW += w;
        }
    }

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

    // kinetic part (the 1/sqrt(N) forward scale is folded into the table)
    // plus the WKB tunneling filter: each momentum mode loses amplitude
    // according to its transmission through the virtual barrier.
    for (int i = 0; i < SIZE; ++i)
    {
        std::complex<float> v = psiFFT[i] * kinPhase[i];
        if (wkbOn)
            v *= wkbT[i];
        psiFFT[i] = v;
        if (accOn)
        {
            const int k = (i <= SIZE / 2) ? i : i - SIZE;
            const float w = std::norm (v);
            kE += w * (float) (k * k);
            kW += w;
        }
    }

    if (! showFFT)
    {
        fft.perform (psiFFT, psi, true);

        // barrier mask + norm accumulation in one pass. The inverse FFT left
        // psi scaled by sqrt(N); the normalise below is scale-invariant, so no
        // separate rescale pass is needed.
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

        // Quantum phaser: rotate a copy of the momentum wave by e^{-i alpha k^2}
        // and bring it back to the position domain. The bare psi stays the
        // canonical state; only the readout probes the dispersed branch.
        if (dispAmt > 0.0f)
        {
            for (int i = 0; i < SIZE; ++i)
                dispTmp[i] = psiFFT[i] * dispPhase[i];
            fft.perform (dispTmp, dispPsi, true);
            float ds = 0.0f;
            for (int i = 0; i < SIZE; ++i) ds += std::norm (dispPsi[i]);
            ds = 1.0f / std::sqrt (std::max (1.0e-12f, ds));
            for (int i = 0; i < SIZE; ++i) dispPsi[i] *= ds;
            dispValid = true;
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

    // Global dynamical phase: dphi/dt = <H> = <T> + <V> from the expectation
    // values gathered above (e^{-iEt/hbar}). The per-sample rotation increment
    // is precomputed here so the readout loop stays trig-free.
    if (accOn && pW > 0.0f && kW > 0.0f)
    {
        const float kFac = juce::MathConstants<float>::twoPi / SIZE_F;
        const float kinetic = kFac * kFac * (kE / kW);
        const float potentialE = vE / pW;
        meanEnergy = juce::jlimit (0.5f, 200.0f, kinetic + potentialE);
        const float spt = samplesPerTimestep > 0.0f ? samplesPerTimestep : 1.0f;
        const float dPhi = dt * meanEnergy * (2.0f * phaseAccAmt) / spt;
        rotDC = std::cos (dPhi);
        rotDS = -std::sin (dPhi);   // e^{-i phi}: clockwise orbit in Re/Im
        const float rn2 = rotC * rotC + rotS * rotS;
        if (std::abs (rn2 - 1.0f) > 1.0e-4f)
        {
            const float rn = 1.0f / std::sqrt (std::max (1.0e-12f, rn2));
            rotC *= rn; rotS *= rn;
        }
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
    if (applyWavefunction && stepsUsed < blockStepBudget)
    {
        if (samplesPerTimestep >= 1.0f)
        {
            timestepCounter += 1.0f;
            if (timestepCounter >= samplesPerTimestep)
            {
                doTimestep (timestepDelta);
                ++stepsUsed;
                timestepCounter = 0.0f;
            }
        }
        else
        {
            // Load guard: samplesPerTimestep < 1 demands up to 1/spt evolution
            // steps per sample. Cap the burst so an extreme setting saturates
            // the evolution rate instead of overflowing the real-time budget.
            int burst = 0;
            while (timestepCounter < 1.0f)
            {
                timestepCounter += samplesPerTimestep;
                doTimestep (timestepDelta);
                ++stepsUsed;
                if (++burst >= maxStepsPerSample || stepsUsed >= blockStepBudget)
                    break;
            }
            timestepCounter = std::fmod (timestepCounter, 1.0f);
        }
    }

    std::complex<float> v = (dispAmt > 0.0f && dispValid && ! showFFT)
                              ? sampleArray (dispPsi, phase)
                              : sampleWave (phase);
    // Approximate fractional-Fourier readout: rotate the sampled quadrature
    // between psi(x) and the momentum state of the last step. Degenerate when
    // showFFT already reads psi(k).
    if (quadrature > 0.0f && ! showFFT)
        v = v * (1.0f - quadrature) + sampleArray (psiFFT, phase) * quadrature;

    // Global dynamical + Berry phase: orbit the sampled phasor between the
    // real and imaginary readouts (a slow per-voice frequency-shift flavour).
    if (phaseAccAmt > 0.0f)
    {
        const float vr = v.real() * rotC - v.imag() * rotS;
        v = std::complex<float> (vr, v.real() * rotS + v.imag() * rotC);
        advanceRotation();
    }

    phase += phaseIncrement;
    if (phase >= (float) SIZE) phase -= (float) SIZE;
    if (phase < 0.0f) phase = 0.0f;

    return toReal (v) * damp;
}

void QuantumOsc::advanceRotation()
{
    const float nrc = rotC * rotDC - rotS * rotDS;
    rotS = rotC * rotDS + rotS * rotDC;
    rotC = nrc;
}

float QuantumOsc::readAt (float index) const
{
    return toReal (sampleWave (index));
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