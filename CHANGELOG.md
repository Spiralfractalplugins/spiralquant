# Changelog

All notable changes to SpiralQuant are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [0.2.0] - 2026-10-21

### Added
- **Quantum Lab band (wave 4, from the research report)**: a full-width panel
  under both columns with eight knobs and two selectors, implementing ten
  quantum effects from the literature study end to end:
  - *Evolution-level (per voice)*: **GP SAT** — Gross-Pitaevskii self-interaction
    g|psi|^2*psi (spectral peaks self-repel, a nonlinear quantum saturation
    ceiling); **WKB** — tunneling filter T(k)=exp(-2*kappa(k)*L) in the momentum
    step (fast components transmit the virtual barrier, slow ones reflect);
    **PHASER** — dispersive quantum phaser, the readout probes the wave after an
    extra e^(-i*alpha*k^2) free propagation, sweeping quadratic-phase notches;
    **PHASE** — global dynamical phase e^(-i*E*t/hbar) with <H> = <T>+<V>
    measured from the evolving state, plus a Berry-phase winding on adiabatic
    potential sweeps.
  - *Exchange symmetry*: **BOSE / FERMI** — neighbouring voices read each
    other's wavetables at their own scan phase (symmetrised pair states):
    Bose adds constructively, Fermi subtracts (exchange holes).
  - *Master lab effect* (one at a time, bit-exact when Off): **Anderson
    Reverb** — 16-line unitary Hadamard FDN whose length disorder plays the
    random potential (CHAR = disorder W, SIZE = T60); **Adiabatic Pitch** —
    dual-tap grain shifter whose ratio glides on the adiabatic timescale
    (CHAR = -12..+12 semitones, SIZE = glide time); **Born Events** — Poisson
    gate engine whose rate follows the smoothed signal power |psi|^2
    (CHAR = rate, SIZE = recovery); **Mode Clip** — 256-point STFT spectral
    saturation, a projection onto the lowest modes with a magnitude ceiling
    (CHAR = modes kept, AMT = ceiling); **Loschmidt Delay** — time-reversed
    echo window U(-t) with a Loschmidt-fidelity envelope (SIZE = window,
    CHAR = fidelity decay). The AMT / CHAR / SIZE / MIX knobs remap per
    effect; the shared controls dim when the lab is off.
- **Six new factory presets** (19-24) demoing each Quantum Lab effect.
- **Test tooling**: `--bench-lab` benchmarks all eight voices with every
  evolution effect engaged, once per master lab effect.
- **New base window size 1280x1142** (the lab band matches the hero-panel
  height of 208 px); the editor stays freely resizable and scales down to
  compact hosts.

- **Fourth UI skin "Golden"**: warm gold/bronze accents on graphite, selected as
  the new last entry of the `uiSkin` choice (existing saved 0..2 values keep
  their meaning; presets are unaffected).
- **Golden-section HUD layout**: the base 1280×920 grid is now derived from φ.
  Columns split 768:474 (1.620), the left column splits 520 : 190 : 118 with
  ADSR : Harmony at 1.610, right-column hero panels (208 px) sit φ above the
  utility rows (128 px), and the scope splits |ψ(x)| : |ψ(k)| at 0.618.
- **Regrouped controls**: the Wavefunction panel now holds three hero knobs
  (SHIFT / WIDTH / IMPULSE, ~φ× larger than the utility knobs) directly under
  the scope handles; the physics knobs moved to their own "Quantum Evolution"
  row (DT / STEP / COLLAPSE / QUAD / ZENO / SQUEEZE); the four ADSR knobs sit
  in a 2×2 block beside the envelope curve (golden split between curve and
  knobs); Master FX controls live in the panel title band so all six FX knobs
  regain full panel height.
- **Pill buttons with LED indicators**: all buttons and toggles render as
  fully-rounded chips with a soft drop shadow, vertical gradient, accent
  outline when engaged, a lit LED dot with glow, hover ring and pressed state;
  combo boxes match the pill language.
- **Test tooling**: `SpiralQuantTests --render-ui <dir> [w h]` offscreen-renders
  the HUD once per skin for layout review, and `--bench-extreme` measures the
  worst-case STEP=0.1 load to verify the evolution-step budget.

### Changed
- **Performance (bit-exact DSP)**: the per-timestep 1/√N FFT rescale loops are
  folded into the kinetic phase table (and the inverse-FFT rescale is dropped
  as the following renormalisation is scale-invariant), the voice sum loop
  walks a dense pointer table instead of double-indexing, and the 20/30 Hz UI
  timers use cached parameter pointers instead of name lookups per tick.
- **Stability under extreme settings**: `samplesPerTimestep < 1` (up to 10
  demanded evolution steps per sample) is now capped at 4 steps per sample and
  128 steps per voice per block, so a CPU-exploding setting saturates the
  evolution rate instead of overflowing the real-time budget; the master
  limiter drops NaN from the gain follower and re-arms instead of latching.
- **Correct tail reporting**: `getTailLengthSeconds` now returns 5 s (the
  RELEASE range) instead of 0, so hosts keep rendering release/FX tails.

### Fixed
- **ADSR canvas**: the title band ("ADSR" and the drag hint) is now reserved,
  so the attack peak and its value label no longer collide with the heading in
  the shorter golden-grid ADSR zone.

## [0.1.2] - 2026-10-21

The plug-in shipped as VST3 / AU / Standalone; CLAP hosts (Bitwig, Reaper,
Falcon, Ardour, LMMS, …) had nothing to load. Now they do — same DSP, same
parameters, same presets.

### Added
- **CLAP format** via [clap-juce-extensions](https://github.com/dsp56300/clap-juce-extensions)
  (fork dsp56300, pinned to `e1f6789` by the CMake guard), mirroring the Spiral303
  setup: the same shared code as VST3/AU, parameters still driven by
  `AudioProcessorValueTreeState`, state transferred through the standard
  `setStateInformation`, so presets open in a CLAP host as they do in VST3.
  `CLAP_ID = com.spiralquant.plugin` (matches `BUNDLE_ID`), features
  `instrument synth polyphonic bass lead pad "wavetable synthesis"`. New target
  `SpiralQuant_CLAP` emits `CLAP/SpiralQuant.clap` next to the other artefacts and
  auto-copies to `~/Library/Audio/Plug-Ins/CLAP/` on macOS and `~/.clap/` on Linux
  (manual copy to `C:\Program Files\Common Files\CLAP` on Windows). Vendored into
  `vendor/clap-juce-extensions` — outside git like JUCE, with a CMake guard and the
  fetch commands in `docs/BUILDING.md`. No DSP code changed, not one sample.
- CLAP rides into the releases: `scripts/make_dist.sh` bundles the bundle into the
  macOS and Linux archives and `INSTALL.txt` gained the CLAP install line; the
  container Linux target builds `SpiralQuant_CLAP` (needs `libcurl4-openssl-dev`,
  now in the image), the Windows cross-build still stops at VST3 + Standalone.
- Cross-build tooling: `tools/windows-cross-build/cross-build.sh` compiles the
  Windows VST3 + Standalone with MSVC under Wine and the native Linux build in one
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
