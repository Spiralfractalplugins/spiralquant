#include "QuantumLab.h"
#include <cmath>

void QuantumLab::prepare (double sampleRateHz)
{
    sampleRate = sampleRateHz <= 0.0 ? 48000.0 : sampleRateHz;
    const double sr = sampleRate;

    // Anderson FDN: line i spans base*(1+i) with +-35% disorder headroom.
    for (int ch = 0; ch < 2; ++ch)
    {
        std::mt19937 rng (9101u + (unsigned) ch * 77u);
        std::uniform_real_distribution<float> uni (-0.35f, 0.35f);
        for (int i = 0; i < fdnLines; ++i)
        {
            const double base = 0.008 * (double) (1 + i);
            fdnMaxLen[ch][i] = juce::jmax (4, (int) std::lround (base * 1.35 * sr));
            fdnBuf[ch][i].assign ((size_t) fdnMaxLen[ch][i], 0.0f);
            fdnWrite[ch][i] = 0;
            fdnLp[ch][i] = 0.0f;
            fdnU[ch][i] = uni (rng);
            fdnSign[ch][i] = uni (rng) > 0.0f ? 1.0f : -1.0f;
        }
    }
    fdnBuiltW = -1.0f;

    // Adiabatic pitch: grain up to 240 ms, buffer a little larger.
    pitchMax = (int) (0.24 * sr) + 16;
    for (int ch = 0; ch < 2; ++ch)
        pitchBuf[ch].assign ((size_t) pitchMax, 0.0f);
    for (int ch = 0; ch < 2; ++ch)
    {
        pitchWrite[ch] = 0;
        pitchP[ch] = 0.0f;
        pitchRatio[ch] = 1.0f;
    }
    pitchG = (float) (0.04 * sr);

    // Born events.
    eventRng.seed (31337u);
    eventCountdown = 0.0;
    eventGain = 1.0f;
    eventAtkLeft = 0;
    eventLevel = 0.0f;

    // Mode clip STFT.
    clipWin.assign (clipN, 0.0f);
    for (int i = 0; i < clipN; ++i)
        clipWin[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                                      * (float) i / (float) clipN);
    clipInvNorm.assign (clipHop, 1.0f);
    {
        // Pin the engine's inverse-FFT scaling with a one-off roundtrip probe:
        // FFTW/MKL/vDSP divide the complex inverse by N, the fallback engine
        // does not. clipInvGain converts the inverse output back to time-domain
        // gain 1 whatever engine this build picked.
        std::complex<float> p1[clipN], p2[clipN];
        for (int i = 0; i < clipN; ++i)
            p1[i] = std::complex<float> (std::sin (juce::MathConstants<float>::twoPi
                                                   * 3.0f * (float) i / (float) clipN), 0.0f);
        clipFft.perform (p1, p2, false);
        clipFft.perform (p2, p1, true);
        double eIn = 0.0, eOut = 0.0;
        for (int i = 0; i < clipN; ++i)
            eIn += (double) std::norm (p2[i]);   // = N * sum|x|^2 (unscaled forward)
        for (int i = 0; i < clipN; ++i)
            eOut += (double) std::norm (p1[i]);  // = g^2 * sum|x|^2
        clipInvGain = eOut > 1.0e-12 ? (float) std::sqrt (eIn / ((double) clipN * eOut)) : 1.0f;
    }
    for (int p = 0; p < clipHop; ++p)
    {
        double s = 0.0;
        for (int j = 0; j * clipHop + p < clipN; ++j)
            s += (double) clipWin[(size_t) (j * clipHop + p)] * clipWin[(size_t) (j * clipHop + p)];
        clipInvNorm[(size_t) p] = s > 1.0e-6 ? clipInvGain / (float) s : clipInvGain;
    }
    for (int ch = 0; ch < 2; ++ch)
    {
        clipIn[ch].assign ((size_t) clipN, 0.0f);
        clipAcc[ch].assign ((size_t) clipN, 0.0f);
    }
    clipTin = 0;

    // Loschmidt delay: window up to 1 s.
    loschMax = (int) (1.0 * sr) + 8;
    for (int ch = 0; ch < 2; ++ch)
        loschBuf[ch].assign ((size_t) loschMax, 0.0f);
    for (int ch = 0; ch < 2; ++ch)
    {
        loschWrite[ch] = 0;
        loschT[ch] = 0;
    }

    reset();
}

void QuantumLab::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < fdnLines; ++i)
        {
            std::fill (fdnBuf[ch][i].begin(), fdnBuf[ch][i].end(), 0.0f);
            fdnWrite[ch][i] = 0;
            fdnLp[ch][i] = 0.0f;
        }
        std::fill (pitchBuf[ch].begin(), pitchBuf[ch].end(), 0.0f);
        std::fill (clipIn[ch].begin(), clipIn[ch].end(), 0.0f);
        std::fill (clipAcc[ch].begin(), clipAcc[ch].end(), 0.0f);
        std::fill (loschBuf[ch].begin(), loschBuf[ch].end(), 0.0f);
        pitchWrite[ch] = 0;
        pitchP[ch] = 0.0f;
        pitchRatio[ch] = 1.0f;
        loschWrite[ch] = 0;
        loschT[ch] = 0;
    }
    eventCountdown = 0.0;
    eventGain = 1.0f;
    eventAtkLeft = 0;
    eventLevel = 0.0f;
    clipTin = 0;
}

void QuantumLab::setParams (int newType, float newAmt, float newCtrl, float newSize, float newMix)
{
    type = juce::jlimit (0, 5, newType);
    amt = juce::jlimit (0.0f, 1.0f, newAmt);
    ctrl = juce::jlimit (-1.0f, 1.0f, newCtrl);
    size = juce::jlimit (0.0f, 1.0f, newSize);
    mix = juce::jlimit (0.0f, 1.0f, newMix);
}

void QuantumLab::process (juce::AudioBuffer<float>& buffer)
{
    if (type == off)
        return;
    const int n = buffer.getNumSamples();
    const int nc = buffer.getNumChannels();
    if (n <= 0 || nc < 1)
        return;
    const int chans = juce::jmin (2, nc);

    // Born events share one measurement clock: the duck trajectory is rolled
    // once per block (from the channel-0 level) and applied to both channels,
    // so the stereo image gates together.
    if (type == bornEvents)
    {
        if ((int) eventTraj.size() < n)
            eventTraj.resize ((size_t) n);
        computeEventTrajectory (buffer.getReadPointer (0), n);
    }

    for (int ch = 0; ch < chans; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        switch (type)
        {
            case andersonReverb: processAnderson (ch, d, d, n); break;
            case adiabaticPitch: processPitch (ch, d, d, n);    break;
            case bornEvents:     applyEventTrajectory (ch, d, d, n); break;
            case modeClip:       processModeClip (ch, d, d, n); break;
            case loschmidtDelay: processLoschmidt (ch, d, d, n); break;
            default: break;
        }
    }

    // Sanitise: the FDN and OLA loops must never leak a NaN into the master.
    for (int ch = 0; ch < chans; ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            if (! std::isfinite (d[i]))
                d[i] = 0.0f;
    }
}

//==============================================================================
void QuantumLab::rebuildLines()
{
    // Line lengths: L_i = 8ms * (1+i) * (1 + W u_i) with a fixed disorder
    // pattern. W = 0 keeps exact harmonic spacing (metallic, comb-like);
    // W -> 1 spreads the modes the way disorder spreads eigenenergies.
    const float w = juce::jlimit (0.0f, 1.0f, ctrl01());
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < fdnLines; ++i)
        {
            const double base = 0.008 * (double) (1 + i);
            const double len = base * (1.0 + (double) (w * fdnU[ch][i]));
            fdnLen[ch][i] = juce::jlimit (4, fdnMaxLen[ch][i], (int) std::lround (len * sampleRate));
        }
    fdnBuiltW = w;
}

void QuantumLab::processAnderson (int ch, const float* in, float* out, int n)
{
    if (ctrl01() != fdnBuiltW)
        rebuildLines();

    const float t60 = 0.3f + 7.7f * size;           // seconds
    const float dampCoef = 0.6f;                     // one-pole HF loss in the loop
    // The dry signal feeds every line, so the re-injected energy grows with
    // the line count: scale the send down by ~sqrt(lines) to keep wet ~ dry.
    const float inWeight = amt * 0.3f;

    float y[fdnLines];
    float g[fdnLines];
    for (int i = 0; i < fdnLines; ++i)
    {
        const double lsamp = (double) fdnLen[ch][i];
        // Per-line loop gain from T60: g = 10^(-3 L / (T60 sr)).
        g[i] = juce::jlimit (0.0f, 0.98f,
                             std::pow (10.0f, -3.0f * (float) lsamp / (t60 * (float) sampleRate)));
    }

    for (int s = 0; s < n; ++s)
    {
        const float dry = in[s];
        float wet = 0.0f;

        for (int i = 0; i < fdnLines; ++i)
        {
            float* line = fdnBuf[ch][i].data();
            const int maxLen = fdnMaxLen[ch][i];
            int r = fdnWrite[ch][i] - fdnLen[ch][i];
            if (r < 0) r += maxLen;
            const float v = line[r];
            fdnLp[ch][i] += dampCoef * (v - fdnLp[ch][i]);
            y[i] = fdnLp[ch][i];
            wet += y[i];
        }
        wet *= 0.25f;

        // Hadamard mixing of the re-injected vector (orthogonal, unitary —
        // the closed-system evolution inside the loop).
        for (int stage = 0; stage < 4; ++stage)
        {
            const int step = 1 << stage;
            for (int i = 0; i < fdnLines; ++i)
                if (i & step)
                {
                    const float a = y[i ^ step];
                    const float b = y[i];
                    y[i] = a + b;
                    y[i ^ step] = a - b;
                }
        }

        for (int i = 0; i < fdnLines; ++i)
        {
            float* line = fdnBuf[ch][i].data();
            const int maxLen = fdnMaxLen[ch][i];
            const float inject = dry * inWeight * fdnSign[ch][i] + 0.25f * y[i] * g[i];
            line[fdnWrite[ch][i]] = inject;
            fdnWrite[ch][i] = (fdnWrite[ch][i] + 1) % maxLen;
        }

        out[s] = dry * (1.0f - mix) + wet * mix;
    }
}

void QuantumLab::processPitch (int ch, const float* in, float* out, int n)
{
    // Shift in semitones; AMT scales the CHAR setting toward zero.
    const float st = ctrl * 12.0f * amt;
    const float target = std::pow (2.0f, st / 12.0f);
    const float glideTau = 0.03f + 1.5f * size * size;   // seconds (adiabatic)
    const float glide = 1.0f - std::exp (-1.0f / (glideTau * (float) sampleRate));
    const float grain = (0.04f + 0.20f * size) * (float) sampleRate;
    pitchG = juce::jlimit (64.0f, (float) (pitchMax - 16), grain);

    float* buf = pitchBuf[ch].data();
    const int bufLen = pitchMax;
    float p = pitchP[ch];
    float ratio = pitchRatio[ch];
    int write = pitchWrite[ch];

    for (int s = 0; s < n; ++s)
    {
        const float dry = in[s];
        buf[write] = dry;

        if (std::abs (st) < 0.005f)
        {
            // No shift requested: the taps would freeze, so pass the dry
            // signal straight through (also the click-free rest state).
            out[s] = dry;
            ratio = 1.0f;
            write = (write + 1) % bufLen;
            continue;
        }

        ratio += (target - ratio) * glide;
        p += ratio - 1.0f;
        if (p >= pitchG) p -= pitchG;
        if (p < 0.0f)    p += pitchG;

        // Two Hann-complementary taps one half-grain apart.
        auto tap = [&] (float pos)
        {
            float r = (float) write - pitchG + pos;
            while (r < 0.0f) r += (float) bufLen;
            while (r >= (float) bufLen) r -= (float) bufLen;
            const int i0 = (int) r;
            const int i1 = (i0 + 1) % bufLen;
            const float f = r - (float) i0;
            return buf[i0] * (1.0f - f) + buf[i1] * f;
        };
        float p1 = p + pitchG * 0.5f;
        if (p1 >= pitchG) p1 -= pitchG;
        const float w0 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * p / pitchG);
        const float w1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * p1 / pitchG);
        const float wet = w0 * tap (p) + w1 * tap (p1);

        out[s] = dry * (1.0f - mix) + wet * mix;
        write = (write + 1) % bufLen;
    }

    pitchP[ch] = p;
    pitchRatio[ch] = ratio;
    pitchWrite[ch] = write;
}

void QuantumLab::computeEventTrajectory (const float* in0, int n)
{
    // One shared measurement clock: a Poisson process whose local rate is
    // weighted by the smoothed signal power — louder passages carry a larger
    // |psi|^2, so the "measurement" fires more often there (Born rule).
    const float rateHz = 0.3f + 14.7f * ctrl01() * ctrl01();
    const float depth = juce::jlimit (0.0f, 1.0f, amt);
    const float relTau = 0.02f + 0.45f * size * size;
    const float atkCoef = 1.0f - std::exp (-1.0f / (0.003f * (float) sampleRate));
    const float relCoef = 1.0f - std::exp (-1.0f / (relTau * (float) sampleRate));
    const float folCoef = 1.0f - std::exp (-1.0f / (0.02f * (float) sampleRate));
    std::uniform_real_distribution<float> uni (0.0f, 1.0f);

    for (int s = 0; s < n; ++s)
    {
        eventLevel += folCoef * (std::abs (in0[s]) - eventLevel);
        eventCountdown -= 1.0;
        if (eventCountdown <= 0.0)
        {
            eventAtkLeft = juce::jmax (1, (int) (0.003f * sampleRate));
            const float lam = rateHz * (0.25f + 0.75f * juce::jmin (1.0f, eventLevel * eventLevel * 8.0f));
            const float u = juce::jlimit (1.0e-6f, 1.0f, uni (eventRng));
            eventCountdown = juce::jmax (1.0, (double) (-std::log (u) * sampleRate / lam));
        }
        if (eventAtkLeft > 0)
        {
            eventGain += (1.0f - depth - eventGain) * atkCoef;
            --eventAtkLeft;
        }
        else
        {
            eventGain += (1.0f - eventGain) * relCoef;
        }
        eventTraj[(size_t) s] = eventGain;
    }
}

void QuantumLab::applyEventTrajectory (int ch, const float* in, float* out, int n)
{
    juce::ignoreUnused (ch);
    for (int s = 0; s < n; ++s)
    {
        const float dry = in[s];
        out[s] = dry * (1.0f - mix) + dry * eventTraj[(size_t) s] * mix;
    }
}

void QuantumLab::processModeClip (int ch, const float* in, float* out, int n)
{
    const float ceiling = 2.0f + 96.0f * amt * amt;
    const int kMax = 2 + (int) (ctrl01() * 62.0f);   // 2..64 modes kept
    float* ibuf = clipIn[ch].data();
    float* acc = clipAcc[ch].data();

    for (int s = 0; s < n; ++s)
    {
        // Capture first: in and out may alias the same channel buffer.
        const float dry = in[s];
        const int pos = (int) (clipTin % clipN);

        // 1) hop boundary: analyse the window that just completed, while the
        //    slot it overlaps is still holding the oldest input sample.
        if (clipTin >= clipN && clipTin % clipHop == 0)
        {
            const int start = (int) ((clipTin - clipN) % clipN);
            // JUCE's perform is strictly out-of-place: forward into spec2,
            // process, then inverse back into spec (its input is consumed).
            for (int i = 0; i < clipN; ++i)
                clipSpec[i] = std::complex<float> (ibuf[(size_t) ((start + i) % clipN)]
                                                       * clipWin[(size_t) i], 0.0f);
            clipFft.perform (clipSpec, clipSpec2, false);

            // Mode projection: keep the lowest kMax modes, clip their
            // magnitudes at the spectral ceiling (warm, quantised overlay).
            for (int i = 0; i < clipN; ++i)
            {
                const int k = (i <= clipN / 2) ? i : i - clipN;
                if (std::abs (k) > kMax)
                    clipSpec2[i] = std::complex<float> (0.0f, 0.0f);
                else
                {
                    const float m = std::abs (clipSpec2[i]);
                    if (m > ceiling)
                        clipSpec2[i] *= ceiling / m;
                }
            }
            clipFft.perform (clipSpec2, clipSpec, true);

            for (int i = 0; i < clipN; ++i)
                acc[(size_t) ((start + i) % clipN)] += clipSpec[i].real() * clipWin[(size_t) i];
        }

        // 2) the sample that leaves the overlap-add is N samples old: the
        //    frame processed just above added its final contribution.
        out[s] = acc[pos] * clipInvNorm[(size_t) (pos % clipHop)];
        acc[pos] = 0.0f;

        // 3) write the new input sample into the freed slot.
        ibuf[(size_t) pos] = dry;
        ++clipTin;
    }
}

void QuantumLab::processLoschmidt (int ch, const float* in, float* out, int n)
{
    const float window = juce::jlimit (0.06f, 0.999f, 0.06f + 0.94f * size) * (float) sampleRate;
    const int winSamp = juce::jmax (1, (int) window);
    const float fadeT = juce::jmax (1.0f, 0.08f * window);
    const float gamma = ctrl01() * ctrl01() * 6.0f / window;   // fidelity decay
    float* buf = loschBuf[ch].data();
    const int bufLen = loschMax;
    int write = loschWrite[ch];
    long long t = loschT[ch];

    for (int s = 0; s < n; ++s)
    {
        const float dry = in[s];
        buf[write] = dry;

        // Reversed read of the last `window` samples (the U(-t) branch),
        // faded at the window edges and weighted by the Loschmidt fidelity.
        const float p = (float) (t % (long long) winSamp);
        float r = (float) write - 1.0f - p;
        while (r < 0.0f) r += (float) bufLen;
        while (r >= (float) bufLen) r -= (float) bufLen;
        const int i0 = (int) r;
        const int i1 = (i0 + 1) % bufLen;
        const float f = r - (float) i0;
        const float wet = buf[i0] * (1.0f - f) + buf[i1] * f;

        const float edge = juce::jmin (1.0f, juce::jmin (p, window - p) / fadeT);
        const float fid = std::exp (-gamma * p);

        out[s] = dry * (1.0f - mix) + wet * edge * fid * amt * mix;

        write = (write + 1) % bufLen;
        ++t;
    }

    loschWrite[ch] = write;
    loschT[ch] = t;
}
