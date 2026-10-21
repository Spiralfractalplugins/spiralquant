#pragma once
#include <JuceHeader.h>
#include <random>
#include <vector>

/** Quantum Lab: master-bus effects translated from the research report
    "Quantum effects in sound synthesis" (SpiralQuant, wave 4).

    Every type reads the same four controls, remapped per effect:

    type            | AMT            | CHAR ((ctrl+1)/2 -> 0..1)  | SIZE
    ----------------+----------------+----------------------------+-----------------
    Anderson reverb | wet send       | length disorder W          | T60 (0.3..8 s)
    Adiabatic pitch | shift scale    | shift, -12..+12 semitones  | glide time
    Born events     | duck depth     | mean event rate            | recovery time
    Mode clip       | spectral ceiling| modes kept (2..64)        | (window fixed)
    Loschmidt delay | echo level     | fidelity decay             | window length

    The lab sits after MasterFx and before the output limiter; with the type
    selector on Off the class is not called at all, so the bypass stays
    bit-exact with the pre-lab signal path. */
class QuantumLab
{
public:
    enum Type
    {
        off = 0,
        andersonReverb = 1,
        adiabaticPitch = 2,
        bornEvents = 3,
        modeClip = 4,
        loschmidtDelay = 5
    };

    void prepare (double sampleRateHz);
    void reset();
    void setParams (int newType, float newAmt, float newCtrl, float newSize, float newMix);
    void process (juce::AudioBuffer<float>& buffer);

private:
    // 1) Anderson-localisation reverb: a 16-line feedback delay network whose
    //    loop mixing is an orthogonal Hadamard matrix (a closed, unitary
    //    system); the disorder in the line lengths plays the role of the
    //    random potential, and the localised eigenmodes give a dense tail
    //    without metallic ring.
    static constexpr int fdnLines = 16;
    void rebuildLines();
    void processAnderson (int ch, const float* in, float* out, int n);
    std::vector<float> fdnBuf[2][fdnLines];
    int   fdnWrite[2][fdnLines] {};
    int   fdnLen[2][fdnLines] {};
    int   fdnMaxLen[2][fdnLines] {};
    float fdnLp[2][fdnLines] {};
    float fdnU[2][fdnLines] {};      // fixed disorder pattern per channel
    float fdnSign[2][fdnLines] {};   // fixed input signs
    float fdnBuiltW = -1.0f;

    // 2) Adiabatic pitch shifter: two Hann-windowed delay taps whose read
    //    rate is the shift ratio. The ratio itself glides with a long time
    //    constant — the adiabatic theorem's slow parameter change, where the
    //    level E ~ 1/L^2 moves without popping. All phase state is per
    //    channel, so the stereo grains decorrelate slightly.
    void processPitch (int ch, const float* in, float* out, int n);
    std::vector<float> pitchBuf[2];
    int   pitchMax = 0;
    int   pitchWrite[2] {};
    float pitchP[2] {};              // grain phase in samples
    float pitchRatio[2] {};          // currently gliding ratio
    float pitchG = 4800.0f;          // grain length in samples

    // 3) Born-rule events: a Poisson event engine whose local rate scales
    //    with the smoothed signal power (loud = larger |psi|^2 = more likely
    //    measurement), producing gated dips. One measurement clock per block;
    //    the resulting duck trajectory is applied to both channels.
    void computeEventTrajectory (const float* in0, int n);
    void applyEventTrajectory (int ch, const float* in, float* out, int n);
    std::mt19937 eventRng { 31337u };
    std::vector<float> eventTraj;
    double eventCountdown = 0.0;
    float  eventGain = 1.0f;
    int    eventAtkLeft = 0;
    float  eventLevel = 0.0f;

    // 4) Mode clip: 256-point STFT, magnitudes clipped at a ceiling and modes
    //    beyond kmax zeroed — the expansion of the report's section 10, a
    //    projection of the overloaded signal onto its lowest energy modes.
    void processModeClip (int ch, const float* in, float* out, int n);
    static constexpr int clipN = 256;
    static constexpr int clipHop = 64;
    juce::dsp::FFT clipFft { 8 };    // 2^8 = 256
    std::vector<float> clipWin;      // Hann analysis = synthesis window
    std::vector<float> clipInvNorm;  // per-hop-phase OLA normalisation
    float clipInvGain = 1.0f;        // engine inverse-FFT gain, pinned at prepare()
    std::vector<float> clipIn[2];    // circular input
    std::vector<float> clipAcc[2];   // circular overlap-add
    std::complex<float> clipSpec[clipN];
    std::complex<float> clipSpec2[clipN];
    long long clipTin = 0;

    // 5) Loschmidt echo delay: the wet path plays the last SIZE window
    //    backwards — the audio analogue of evolving the state with the
    //    reversed Hamiltonian U(-t) = U+(t) — with a Loschmidt-fidelity
    //    envelope exp(-G t) shaping the echo. Phase state is per channel.
    void processLoschmidt (int ch, const float* in, float* out, int n);
    std::vector<float> loschBuf[2];
    int  loschMax = 0;
    int  loschWrite[2] {};
    long long loschT[2] {};

    int type = off;
    float amt = 0.5f;
    float ctrl = 0.0f;
    float size = 0.5f;
    float mix = 0.5f;
    double sampleRate = 48000.0;

    inline float ctrl01() const noexcept { return (ctrl + 1.0f) * 0.5f; }
};
