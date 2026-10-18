# SpiralQuant

**SpiralQuant** is a polyphonic software synthesizer plug-in built with [JUCE](https://juce.com/).
Instead of a classic wavetable, each voice's oscillator is a complex-valued
solution of the 1D time-dependent Schrödinger equation, evolved in real time with
a split-step Fourier method on a 128-point grid.

**Docs**: [User manual (EN)](docs/MANUAL_en.md) · [Руководство (RU)](docs/MANUAL_ru.md)
· [Announcement text](docs/ANNOUNCE_en.md) / [анонс](docs/ANNOUNCE_ru.md)

## UI

![SpiralQuant HUD interface](docs/screenshots/ui-hud.png)

## Formats

- VST3
- AU (macOS)
- Standalone application
## Sound engine

- **8-voice polyphony** with oldest-voice stealing.
- **MIDI note input**; pitch mapped from MIDI note to frequency.
- **Oscillator (`QuantumOsc`)**: 128-sample complex wavetable (`|ψ|`, `Re ψ`, `Im ψ`,
  magnitude, phase), normalised after every timestep.
- **Initial wave shapes**: Gaussian, Sine, Cos, Parabola, Saw, Square.
- **Potentials**: Flat, Harmonic, Infinite Well, Barrier, Double Well, Linear
  (with scale/shift/height controls).
- **Evolution**: split-step Fourier (potential half-step in position space, kinetic
  step in momentum space); optional FFT view keeps the table in momentum space.
- **Readout modes**: Real, Imaginary, Magnitude, Phase.
- **Per-voice envelope**: ADSR.
- **Filter/saturation**: resonant low-pass, drive, saturation mix, master gain.
- **Scope**: 128-point `|ψ(x)|` plus a `|ψ(k)|` momentum spectrum with a harmonic
  frequency grid, fed by a double-buffered snapshot of the first active voice.
- **Chord mode**: each key triggers a just-intonation voicing — consonant chords
  (Power, Major, Minor, Sus4, Major 7th) and dissonant ones (Tritone, Cluster,
  Pythagorean); extra degrees get a velocity falloff.
- **Pitch shift**: ±2400 cents applied as `f · 2^(cents/1200)` at note start and
  on every change.
- **Tunneling**: Gamow transparency `T = exp(-2κL)` leaks the wave through the
  potential barrier instead of hard-reflecting it (κ from `√(2m(V−E))`).
- **Preset browser**: factory presets embedded at build time, user presets saved
  as `.sp303preset` JSON in the app-support folder.

## Parameters

| ID                   | Type     | Range / choices                               | Default |
| -------------------- | -------- | --------------------------------------------- | ------- |
| `waveType`           | choice   | Gaussian / Sine / Cos / Parabola / Saw / Square | 0 (Gaussian) |
| `waveShift`          | float    | 0.0 – 1.0                                     | 0.5     |
| `waveScale`          | float    | 0.0 – 1.0                                     | 0.5     |
| `waveImpulse`        | float    | 0.0 – 1.0                                     | 0.2     |
| `potentialType`      | choice   | Flat / Harmonic / InfiniteWell / Barrier / DoubleWell / Linear | 1 (Harmonic) |
| `potentialShift`     | float    | 0.0 – 1.0                                     | 0.5     |
| `potentialScale`     | float    | 0.0 – 1.0                                     | 0.5     |
| `potentialHeight`    | float    | 0.0 – 1.0                                     | 0.5     |
| `showFFT`            | bool     | —                                             | false   |
| `sampleMode`         | choice   | Real / Imag / Magnitude / Phase / PhononQND   | 0 (Real) |
| `timestepDelta`      | float    | 0.001 – 0.2                                   | 0.01    |
| `samplesPerTimestep` | float    | 0.1 – 64                                      | 8       |
| `applyWavefunction`  | bool     | — (off freezes evolution)                     | true    |
| `attack`             | float    | 1 – 2000 ms                                   | 10      |
| `decay`              | float    | 1 – 2000 ms                                   | 100     |
| `sustain`            | float    | 0.0 – 1.0                                     | 0.8     |
| `release`            | float    | 1 – 5000 ms                                   | 200     |
| `filterFreq`         | float    | 20 – 20000 Hz                                 | 4000    |
| `filterRes`          | float    | 0.0 – 1.0                                     | 0.3     |
| `drive`              | float    | 0.0 – 1.0                                     | 0.0     |
| `satMix`             | float    | 0.0 – 1.0                                     | 0.0     |
| `gain`               | float    | 0.0 – 2.0                                     | 0.8     |
| `fxType`             | choice   | Off / Flanger / Phaser / Bitcrush / Fractal  | 0 (Off) |
| `fxDepth`            | float    | 0.0 – 1.0                                     | 0.5     |
| `fxRate`             | float    | 0.01 – 20 Hz                                  | 1.0     |
| `fxFeedback`         | float    | 0.0 – 0.95                                    | 0.3     |
| `fxMix`              | float    | 0.0 – 1.0                                     | 0.5     |
| `fxBits`             | float    | 2 – 16                                        | 8       |
| `fxEntropy`          | float    | 0.0 – 1.0                                     | 0.5     |
| `chordMode`          | bool     | —                                             | false   |
| `chordType`          | choice   | Power / Major / Minor / Sus4 / Major7 / Tritone / Cluster / Pythagorean | 1 (Major) |
| `pitchShift`         | float    | -2400 – +2400 cents                           | 0       |
| `tunnel`             | bool     | —                                             | false   |
| `uiSkin`             | choice   | Dark / SciFi Neon / Console (UI only)         | 0 (Dark) |
| `chordSuperpose`     | bool     | — (chords in quantum superposition)           | false   |
| `jumpRate`           | float    | 0.0 – 1.0 (phonon-collapse rate)              | 0.0     |
| `frftAngle`          | float    | 0.0 – 1.0 (quadrature rotation x↔k)           | 0.0     |
| `memoryAmt`          | float    | 0.0 – 1.0 (resonator memory depth)            | 0.0     |
| `memoryTime`         | float    | 5 – 2000 ms (memory delay)                    | 80      |
| `ringdown`           | float    | 0.0 – 1.0 (cavity decay rate)                 | 0.0     |
| `walkOn`             | bool     | — (quantum-walk melody generator)             | false   |
| `walkRate`           | float    | 0.25 – 8 Hz (walk measurement ticks)          | 2.0     |
| `limiter`            | bool     | — (master output limiter)                     | true    |

Parameters are stored in an `AudioProcessorValueTreeState`, so host automation and
state save/restore are supported.

## How the sound is made

SpiralQuant is not an oscillator with a waveform — every voice holds a **complex
128-point wavefunction ψ(x)** that is evolved in real time with the Schrödinger
equation, and the audio is whatever you "measure" out of it.

**1. Initial state (per note-on).** `QuantumOsc::generateInitial` fills ψ with the
selected shape on a grid x ∈ [−1, +1]: a Gaussian packet, sine, cosine, parabola,
saw or square. `waveShift` moves the shape's centre, `waveScale` its width/spatial
frequency, and `waveImpulse` multiplies by `e^(i·k·x)` — a momentum kick that makes
the packet travel.

**2. The potential V(x).** Six shapes (Flat, Harmonic oscillator, Infinite well,
Barrier, Double well, Linear ramp) are sampled onto the same grid with
`potentialShift/Scale/Height`. Cells where V exceeds the wall threshold get
`mask[i] = false`: the wave is hard-reflected there (infinite wall). With
**Tunnel** on, masked cells instead get `V *= exp(−2κL)` (Gamow transparency,
κ = √(2m(V−E))) and are un-masked — the wave leaks through barriers the classical
particle could never cross, which is where most of the "living" timbre comes from.

**3. Evolution — split-step Fourier.** Every `samplesPerTimestep` audio samples
(`timestepDelta` = dt) `doTimestep` applies one second-order Strang step:

```
ψ ← e^(−i V dt) · F⁻¹[ e^(−i k² dt/2m) · F[ψ] ]
```

i.e. a phase twist in position space for the potential, an FFT (order-7, 128
points, via JUCE/Accelerate) to momentum space where the kinetic term is diagonal,
a phase twist per wavenumber k², and an inverse FFT back. The transform is
normalised by 1/√N so the step is unitary; afterwards ψ is renormalised to
∑|ψ|² = 1 and the barrier mask is applied. Because the update is a *phase*
rotation, no energy is created or destroyed — the wave sloshes, interferes with
itself, diffracts around the well edges, and that interference is the sound.

**4. Readout.** The output sample is ψ sampled at a moving read position
(`sampleWave`, linear interpolation): the note's frequency sets how fast the
read head circles the 128-point table (`phaseInc = freq/sampleRate · 128`), so
the wave shape is the timbre and the MIDI pitch is the scan rate. `sampleMode`
picks which part becomes audio: **Real**, **Imag**, **Magnitude** (rectified,
bright/harsh), **Phase** (imag/|ψ| — the instantaneous phase, very FM-like) or
**PhononQND** (an 8-step staircase quantisation of Re ψ — a "quantum non-demolition"
readout that clicks as the wave crosses discrete amplitude levels). `frftAngle`
rotates the readout quadrature, blending the position-space sample with the
momentum-space one (the last FFT, so it is free) — 0 hears ψ(x), 1 hears ψ̃(k),
with fractional Fourier-like mixtures in between. With **showFFT** on, the table is
kept in momentum space instead, so you hear |ψ̃(k)| — the frequency content of the
wave as the waveform itself.
**Apply QM** off freezes evolution: the wave stops mid-shape and the voice becomes
a plain wavetable of that frozen state.

**5. Per-voice chain.** The readout goes through an ADSR (velocity-scaled gain,
per-note frequency glide), a resonant low-pass (Q from `filterRes`), cubic
soft-clip saturation driven by `drive`/`satMix`, and per-voice `gain`.
Chord mode clones a note into a just-intonation voicing with velocity falloff on
upper degrees; `pitchShift` transposes all voices by 2^(cents/1200).

**Phonon collapse (`jumpRate`).** With the rate above zero, each evolution step also
runs a Monte-Carlo quantum-trajectory measurement: the phonon-annihilation operator
â ψ = (x/σ)ψ + σ·∂ψ/x is applied, and with probability ∝ `jumpRate`·‖â ψ‖²·dt the
wavefunction *collapses* onto â ψ and is renormalised. This is the stochastic
"quantum jump of sound" — the smooth wave periodically snaps to a lower-energy
shape, producing percussive transients and granular decay that pure unitary
evolution can't reach. Collapses are seeded per voice, so renders are reproducible.

**Superposed chords (`chordSuperpose`).** With chord mode on, instead of one fixed
voicing the engine spawns *two* competing chord types per note, weighted cos²φ /
sin²φ by a random phase φ. The note rings in a superposition of both voicings until
you release it: note-off is a *measurement* that collapses to one winner (prob.
cos²φ) — the loser is killed instantly, the winner releases normally — so the same
note can resolve to different harmony each take.

**Resonator memory (`memoryAmt` / `memoryTime`).** A real cavity does not react to
the wave instantaneously: its refractive index follows the *past* density. The
oscillator keeps a delayed copy of |ψ|² in a two-slot ring buffer (pushed every
`memoryTime` ms of evolution time) and each step applies a Kerr-like cross-phase
modulation `ψ → ψ·e^(i·dt·6·memoryAmt·|ψ_delayed|²)`. The self-interaction lags,
so fast shapes smear into echoes and slow chirps instead of ringing clean. 0 is
pure instantaneous Schrödinger evolution; the effect is entirely in the phase,
so `frftAngle`>0 makes it audible in the readout too.

**Cavity ringdown (`ringdown`).** Photons leak out of the resonator at rate T₁⁻¹,
so each evolution step decays an internal envelope `damp` and the readout is
scaled by it — notes fade with a cavity-shaped exponential tail even after the
ADSR has released. The wavefunction *shape* stays normalised (only the readout
is scaled), so evolution remains unitary; 0 = perfectly reflecting cavity.

**Quantum-walk melody (`walkOn` / `walkRate`).** A continuous-time quantum walk
runs on a 16-node ring tuned to a two-octave pentatonic scale: the engine holds a
16-amplitude state |ψ_walk⟩, spreads it Hadamard-style to the neighbours every
tick (`walkRate` Hz) and *measures* the walker's position — the sampled node is
played as a note with velocity ∝ detection probability. Where a random walk
diffuses, the walk's interference keeps returning the melody to its start, so
lines loop, cluster and suddenly leap an octave. Fully deterministic given the
engine's own RNG stream; switch it off and the walk's voice releases cleanly.

**Output limiter (`limiter`).** On by default: a stereo-linked brickwall just
under full scale sits at the very end of the master bus. Quiet blocks skip it
entirely (one vectorized block-peak check); when a chord or drive spike pushes
a block over the threshold, gain is pulled down with a ~1 ms attack and an
~80 ms release, and a final clamp guarantees the output never leaves [−1, +1].
Switch the LIMIT chip off for unprotected (potentially clipping) output.

**6. Master section.** Up to 8 voices are summed, then a master effect
(Flanger/Phaser via JUCE, or custom Bitcrush/Fractal-chaos) with dry/wet solo,
and the stereo pair is written to the bus.

The UI scope shows the same data the audio uses: `|ψ(x)|` from the first active
voice on top, and the momentum spectrum `|ψ̃(k)|` with a harmonic grid (bin n =
n·f₀, labelled in Hz) below — the audio and the picture come from one snapshot of
the engine, never from a look-alike animation.

## Building

Prerequisites:

- CMake 3.24+
- C++17 compiler
- JUCE is vendored under `vendor/JUCE/` (no separate install needed).

```sh
cmake -S . -B build
cmake --build build --config Release --parallel
```

On macOS the project builds a universal binary (`arm64;x86_64`) by default and
copies built plug-ins next to the artefacts (`JUCE_COPY_PLUGIN_AFTER_BUILD`).

## Tests

A console test target is built from `tests/TestRender.cpp`:

```sh
cmake --build build --target SpiralQuantTests --config Release
./build/SpiralQuantTests_artefacts/Release/SpiralQuantTests
```

The suite:

- renders every `waveType × potentialType × sampleMode` combination (6×6×5),
  checking for NaN/inf and non-silent output;
- renders the chord × pitch-shift × tunneling matrix;
- renders the phonon features: collapse (deterministic across identical seeds),
  quadrature rotation, PhononQND staircase, and superposed chords (verifying notes
  go silent after release — no stuck voices);
- exercises all master FX types and a state save/restore roundtrip;
- `--bench` runs a deterministic 20 000-block, 8-voice CPU benchmark;
- `--render-logo <dir>` regenerates `resources/logo.png` and the iconset PNGs
  from the vector `LogoMark` (used to refresh `resources/icons/SpiralQuant.icns`
  via `iconutil`).

## Build options

```sh
# Aggressive math (fastest; changes denormal/FPA behaviour, NaN guards remain)
cmake -S . -B build -DSPIRALQUANT_OFAST=ON

# Profile-guided optimization:
cmake -S . -B build-pgo -DSPIRALQUANT_PGO=generate   # instrumented build
cmake --build build-pgo --target SpiralQuantTests
./build-pgo/SpiralQuantTests_artefacts/Release/SpiralQuantTests --bench   # train
xcrun llvm-profdata merge -output=pgo/default.profdata pgo/*.profraw
cmake -S . -B build-pgo -DSPIRALQUANT_PGO=use       # rebuild with profile
cmake --build build-pgo --target SpiralQuantTests
```

The default is `-O3 -fno-math-errno -ffp-contract=fast`; `-ffast-math`/-Ofast are
opt-in because the DSP relies on `isfinite` guards for NaN rejection.

It must print `ALL TESTS PASSED`.

## Build artefacts (macOS)

- `build/SpiralQuant_artefacts/Release/VST3/SpiralQuant.vst3`
- `build/SpiralQuant_artefacts/Release/AU/SpiralQuant.component`
- `build/SpiralQuant_artefacts/Release/Standalone/SpiralQuant.app`

See [How it works](docs/HOW_IT_WORKS.md) for the signal path and sound generation details.

## Presets

Preset I/O is implemented in `Source/Presets.cpp` using a small JSON format
(`PresetFormat`, version 1):

```json
{
  "format": 1,
  "name": "Example",
  "params": { "waveType": 0, "gain": 0.8 },
  "sequence": "...",
  "gate": "...",
  "credit": "..."
}
```

`presets/` is embedded at build time (when populated); an empty folder produces
no factory presets and no errors.

## Project layout

```
CMakeLists.txt
Source/
  PluginProcessor.{h,cpp}   APVTS + process block
  PluginEditor.{h,cpp}     editor + attachments
  CreateEditor.cpp         JUCE plugin entry point
  dsp/
    QuantumEngine.{h,cpp}  voice pool (8 voices)
    QuantumVoice.{h,cpp}   envelope, filter, saturation
    QuantumOsc.{h,cpp}     Schrödinger wavetable
  gui/                   palette/skins, scope canvas, ADSR editor, look-and-feel
tests/TestRender.cpp
vendor/JUCE/            vendored JUCE (do not edit)
docs/                   manuals (EN/RU), announcements, HOW_IT_WORKS, screenshots
scripts/make_dist.sh    bundles VST3/AU/Standalone + docs into dist/ + zip
dist/                   generated release distributions (gitignored except README)
```

## Known limitations

- When no note is held the scope shows the resting wavefunction of voice 0 rather
  than a dedicated idle animation.
