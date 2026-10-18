# Changelog

All notable changes to SpiralQuant are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

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

### Changed
- `applyWavefunction` (QM toggle) now freezes wavefunction evolution when off
  instead of being ignored.
- Choice-chip captions read from APVTS parameter names, so host-visible
  `InfiniteWell` label and UI can no longer desynchronize.
- Scope data is snapshotted lock-free; the UI reader never blocks the audio
  thread.

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
