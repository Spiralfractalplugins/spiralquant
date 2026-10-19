# Changelog

All notable changes to SpiralQuant are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Cross-build tooling: `tools/windows-cross-build/cross-build.sh` compiles the
  Windows VST3 + Standalone with MSVC under Wine and a native Linux build in one
  amd64 Docker/OrbStack container (no matching machine needed), then runs the
  render test suite on the produced PE binary as a smoke test; `docs/BUILDING.md`
  covers the container, native Windows/Linux instructions and the JUCE
  VST3-helper patching it relies on. `scripts/make_dist.sh` now takes the
  cross-build output directory (`build/windows`, `build/linux`) and names the
  package accordingly.

## [0.1.1] - 2026-10-19

### Added
- Phonon-collapse readout: `jumpRate` drives a Monte-Carlo quantum-trajectory
  collapse (â ψ = (x/σ)ψ + σ·∂ψ/∂x) applied per evolution step, seeded per voice
  for reproducible renders — stochastic "quantum jumps of sound".
- Quadrature rotation `frftAngle`: blends the position-space sample with the
  (already-computed, free) momentum-space one for fractional-Fourier timbres.
- `PhononQND` sample mode: 8-step amplitude staircase — a quantum non-demolition
  style readout that clicks as ψ crosses discrete levels.
- Superposed chords (`chordSuperpose`): each note spawns two competing voicings
  weighted cos²φ/sin²φ; note-off is a measurement that collapses to one winner
  (probability cos²φ) and kills the loser, so a note resolves differently each take.
- Master FX module: Off / Flanger / Phaser / Bitcrush / Fractal Entropy.
- HUD UI redesign with panel layout, oscilloscope grid, knobs and value labels.
- Screenshot preview in README.
- Full psykick-style UI overhaul: base 1280×920 scaled grid, per-group accent
  colors, name+value labels under every knob, panel chips for wave/potential
  types, header with preset browser and skin selector.
- Multi-skin system (Dark / SciFi Neon / Console) driven by the UI-only
  `uiSkin` parameter; palette singleton `Skins::current()`, repaint on switch.
- Real-DSP scope: double-buffered |ψ(x)|, |ψ(k)| spectrum and potential
  snapshot from the audio thread (atomic slot + sequence counter), frequency
  grid labeled in multiples of the playing fundamental, repaint-on-change
  30 Hz canvas.
- Fully rendered rotary knobs: gradient cap, rim highlight, groove track,
  glow value arc, tick table, value dot and focus halo (skin-aware).
- Controls for previously unexposed parameters: drive, satMix, gain, fxBits,
  fxRate, fxFeedback, fxEntropy.
- User manuals in English and Russian (`docs/MANUAL_{en,ru}.md`), announcement
  texts (`docs/ANNOUNCE_{en,ru}.md`), updated `HOW_IT_WORKS.md`.
- Release packaging: `scripts/make_dist.sh` bundles VST3/AU/Standalone + docs
  into versioned, zipped distributions under `dist/`.
- Parseval/finiteness assertion for scope snapshots in the render test suite.
- Non-Markovian resonator memory (`memoryAmt`/`memoryTime`): delayed |ψ|²
  Kerr-like self-phase modulation through a two-slot density ring buffer —
  lagged echoes and slow chirps instead of clean ringing.
- Cavity ringdown (`ringdown`): T₁-shaped decay applied to the readout only,
  so notes leak out of the "cavity" with an exponential tail while ψ stays
  normalised and evolution remains unitary.
- Continuous-time quantum-walk melody generator (`walkOn`/`walkRate`): a
  16-node pentatonic ring walked Hadamard-style, position measured per tick
  with Born probabilities — the plugin composes its own self-returning lines.
- ADSR graph: attack edge is truly vertical at attack = 0 (curve and drag
  handle both start at the envelope origin).
- Master output limiter (`limiter`, LIMIT chip, on by default): stereo-linked
  brickwall just under full scale after the FX — fast attack, 80 ms release,
  hard ±1 clamp; quiet blocks skip processing via a vectorized block-peak test.
- Quantum Zeno effect (`zeno`, ZENO knob): per-step Born-sampled position
  projections with Gaussian windows — watching the wave freezes its evolution.
- Squeezed states (`squeeze`, SQUEEZE knob, −1..+1): symplectic rescaling at
  note-on moves uncertainty between position and momentum (+ narrows |ψ(x)|,
  − narrows |ψ(k)|), verified by a variance test in the render suite.
- EPR-entangled voices (`entangle`, EPR chip in HARMONY): a phonon jump on one
  voice instantly collapses its same-note partners within the same block,
  without cascading.
- Scope canvas: the potential now has its own lower lane (ceiling at 55% of
  panel height) and can no longer overlap the |ψ(x)| curve.

### Changed
- Harmony `PITCH` knob spans −48 … +48 st (±4800 cents, four octaves either way)
  and reads in semitones; the voice clamps the resulting frequency to the
  renderable band so extreme transposition stays useful instead of aliasing.
- Fractal master FX is a tonal tail, not a noise generator: white-noise injection
  and the >1 chaos gain boost are gone. The tent map now picks a comb length that
  is a whole number of tonic periods (the engine hands the playing note's root,
  transposed, to the FX), and steers the tail's brightness. Wet gain is
  normalised by (1 − feedback) and trimmed so dry + wet peaks at unity; the
  render suite asserts the effect can no longer be louder than the dry signal.
- Fractal `DEPTH` decides how wide that tap corridor is — two tonic periods at the
  bottom of the knob, eight at the top — instead of the tail being pinned to 1–3.
  The delay line grew to 0.5 s so whole-period taps still fit at the bottom of the
  keyboard; the corridor narrows to match when they would not.
- Fractal `RATE` is the comb's re-tune clock: the map steps once every 1/rate
  seconds rather than every block, so a slow rate holds one tonic relationship for
  bars and a fast one makes the tail hop between harmonics.
- Preset recall applies every parameter in the file and the browser dropdown
  recalls on selection (`LOAD` re-applies). Factory bank grew to 18 presets, each
  carrying the full parameter set.
- `applyWavefunction` (QM toggle) now freezes wavefunction evolution when off
  instead of being ignored.
- Choice-chip captions read from APVTS parameter names, so host-visible
  `InfiniteWell` label and UI can no longer desynchronize.
- Scope data is snapshotted lock-free; the UI reader never blocks the audio
  thread.

### Fixed
- Recalling a preset did nothing: the browser read JSON keys off the top level
  (`format`, `name`, `params`) instead of the `params` object, so no parameter ID
  ever matched. Saving was empty for the same reason — it walked the ValueTree's
  own properties instead of the per-parameter `PARAM` children.
- Factory presets now cover the wave-2/3 parameters, so switching presets is
  deterministic instead of leaving old FX and harmony settings behind.
- The Fractal chaos map is a tent map on [0, 1] again. `1 − 2·|2x − 1|` leaves the
  unit interval, ran away toward negative infinity and left the comb stuck on a
  single period, which made the tap corridor — and `DEPTH` with it — inert.

### Removed
- Dead legacy widgets (Gate/Sequence editors, ScopeComponent, KnobSlider,
  SciFiLookAndFeel) and the stale `PluginParameters.h` ID table.

### Performance
- Cached `AtomicValue`/parameter pointers replace ~36 per-block string
  lookups in `processBlock`.
- Per-voice parameter diffing: IIR coefficients and voice setup only
  recompute when inputs actually change.
- Active-voice list built once per block instead of `isActive()` per sample;
  cheaper phase readout; memoised knob tick table.
  Bench: ~645 µs per 512-sample block, 8 voices @ 48 kHz (Apple Silicon).

## [0.1.0] - 2026-10-14

### Added

- Initial release of SpiralQuant as a VST3 / AU / Standalone synthesizer.
- 8-voice polyphonic engine with oldest-voice stealing.
- Quantum wavetable oscillator (128-point complex wavefunction, split-step
  Fourier evolution) with 6 initial shapes and 6 potentials.
- ADSR envelope, resonant low-pass filter, drive/saturation mix, master gain.
- UI scope of |ψ| and full parameter panel with APVTS automation/state save.
- Preset format v1 (JSON) with compiled-in bank support.
- Console render test covering all wave/potential/sample-mode combinations
  plus state roundtrip.
