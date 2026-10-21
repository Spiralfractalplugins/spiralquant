# SpiralQuant — User Manual

**A quantum-mechanical wavetable synthesizer.** Every voice is a real 128-point
wavefunction ψ(x) evolved live with the Schrödinger equation — you do not shape
a waveform, you shape the *physics* of the box the wave lives in.

Versions: VST3, Audio Unit (AU), Standalone. macOS (universal), Windows, Linux.

---

## 1. Installation

### VST3
Copy `SpiralQuant.vst3` to:

- macOS: `~/Library/Audio/Plug-Ins/VST3/`
- Windows: `C:\Program Files\Common Files\VST3\`
- Linux: `~/.vst3/`

### Audio Unit (macOS)
Copy `SpiralQuant.component` to `~/Library/Audio/Plug-Ins/Components/`
and rescan with Apple's Audio MIDI Setup or your DAW's plug-in manager.

### Standalone
Run `SpiralQuant.app` (or the Windows/Linux executable) directly — no host needed.

---

## 2. Quick start

1. Insert SpiralQuant on a MIDI track and play a note.
2. The wave panel shows |ψ(x)| — the live wavefunction of the first active voice.
3. Turn **POT** to *Harmonic* (default) or *InfiniteWell* and watch the packet
   slosh and reflect.
4. Lower **STEP** (samples per timestep) in the WAVEFUNCTION panel: the wave
   evolves faster and the timbre gets busier.
5. Raise **DT** slightly for coarser, more dramatic evolution.
6. Try **TUNNEL** on with the *Barrier* potential: the wave leaks through the
   wall and the sound gains an evolving, reverb-like tail.

---

## 3. Interface map

```
+--------------------------------------------------------------+
| header: logo | version/status | preset browser | LOAD SAVE DEL | skin |
+------------------------------+-------------------------------+
|                              |  WAVEFUNCTION                 |
|   WAVE |ψ(x)| + POTENTIAL    |  chips: WAVE, FFT, QM, sample |
|   SPECTRUM |ψ̃(k)|            |  knobs: SHIFT WIDTH IMPULSE   |
|   (harmonic grid, Hz labels) |          DT STEP COLLAPSE QUAD ZENO SQUEEZE|
+------------------------------+-------------------------------+
|                              |  POTENTIAL                    |
|   ADSR editor (drag handles) |  chips: POT, TUNNEL           |
|                              |  knobs: CENTER WIDTH HEIGHT   |
|                              |    MEMORY LAG RING            |
+------------------------------+-------------------------------+
| HARMONY: CHORD, SUPERPOSE,   |  ENVELOPE: A D S R            |
| WALK/EPR chips, chord type   |  FILTER & DRIVE: CUTOFF RESO  |
| PITCH + RATE knobs           |    DRIVE SAT GAIN             |
|                              |  MASTER FX: type/solo/LIMIT + |
|                              |    DEPTH RATE FDBK MIX BITS   |
|                              |    CHAOS                      |
+------------------------------+-------------------------------+
| QUANTUM LAB (full width): GP SAT WKB PHASER PHASE AMT CHAR SIZE MIX |
| lab type + exchange-symmetry selectors in the title band             |
+---------------------------------------------------------------------+
```

### Interacting with controls

- **Drag** any knob vertically (or horizontally) to change it.
- **Mouse wheel** over a knob: 1% of range per notch; hold **Shift** for 5%.
- **Double-click** a knob: type an exact value (`55`, `1.2k`, `440 Hz`, `30%`).
- **Click the wave panel** upper half to cycle the wave shape; lower half cycles
  the potential. Both chip buttons do the same and always show the host's own
  parameter name.
- **Drag the scope handles**: cyan = wave centre/width, amber = potential
  centre/height, blue = potential width. Handles write the same parameters the
  knobs do.
- **Drag the ADSR handles** directly on the envelope graph.

---

## 4. Controls in detail

### WAVEFUNCTION panel

| Control | What it does |
|---|---|
| **WAVE** chip | Initial shape of ψ: Gaussian, Sine, Cos, Parabola, Saw, Square. This is the "source waveform" the physics starts from. |
| **SHIFT** | Where the shape sits in the box (−1…+1). Off-centre starts the wave in motion. |
| **WIDTH** | Packet width / spatial frequency. Narrow = broadband = brighter. |
| **IMPULSE** | Momentum kick e^(ikx): makes the packet travel toward a wall at note start. |
| **DT** (`timestepDelta`) | Schrödinger time step. Small = smooth, CPU-neutral; large = wilder evolution per step. |
| **STEP** (`samplesPerTimestep`) | Audio samples between evolution steps. Low = fast wave (more CPU), high = slow morphing. Below 1 runs several steps per sample. |
| **COLLAPSE** (`jumpRate`) | Phonon-collapse rate. Above 0 each step can randomly collapse ψ onto the annihilation operator â ψ and renormalise — stochastic "quantum jumps of sound": percussive snaps and granular decay on top of the smooth wave. 0 = pure unitary evolution. |
| **QUAD** (`frftAngle`) | Quadrature rotation of the readout: 0 hears ψ(x), 1 hears the momentum wave ψ̃(k), in between gives fractional-Fourier mixtures. Free (reuses the last FFT). |
| **ZENO** (`zeno`) | Quantum Zeno effect: with this probability each step performs a position measurement and projects ψ through a narrow Gaussian window. A watched wave stops evolving — high values pin the packet in place (nearly static tone), low values just slow its travel. |
| **SQUEEZE** (`squeeze`) | Squeezed states, −1..+1: at note-on ψ is symplectically rescaled. Positive narrows |ψ(x)| in space (wider momentum spectrum, brighter transient); negative narrows it in momentum (longer, purer ring). Heisenberg is conserved — uncertainty moves, it doesn't vanish. |
| **FFT** toggle | Freeze evolution and read ψ̃(k): the momentum spectrum becomes the audio. Very metallic/organ-like. |
| **QM** toggle | Master switch for evolution. Off = the wave freezes mid-shape and the voice becomes a static wavetable. |
| **Sample mode** combo | Which part of complex ψ becomes audio: Real / Imag / Magnitude / Phase / PhononQND. Magnitude = bright and buzzy; Phase = FM-like; PhononQND = 8-step amplitude staircase (quantum non-demolition readout, clicks as ψ crosses levels). |

### POTENTIAL panel

| Control | What it does |
|---|---|
| **POT** chip | The "box": Flat, Harmonic, InfiniteWell, Barrier, DoubleWell, Linear. Determines how the wave accelerates, reflects and interferes. |
| **CENTER / WIDTH / HEIGHT** | Position, scale and depth of the potential. HEIGHT also sets the barrier wall threshold. |
| **MEMORY** (`memoryAmt`) | Resonator memory depth. The cavity's response lags: ψ self-phase-modulates by the *delayed* density |ψ(t−τ)|² (Kerr-like, phase-only). Smears shapes into echoes and chirps. 0 = instantaneous, textbook Schrödinger. |
| **LAG** (`memoryTime`) | How far back the memory reaches, 5–2000 ms of evolution time. Short = slapback; long = smeared reverberant feedback. |
| **RING** (`ringdown`) | Cavity T₁ leak: each step decays an internal envelope so notes fade with an exponential cavity tail even after release. The wave's *shape* stays normalised — only the readout shrinks. 0 = perfect mirrors. |
| **TUNNEL** | Gamow tunnelling `T = exp(−2κL)`: the wave leaks through barriers instead of hard-reflecting. Long, shimmering tails. |

### ENVELOPE panel

Classic A/D/S/R in ms (sustain is a level). Velocity scales the peak gain.
Note frequency glides to the target over the attack.

### FILTER & DRIVE panel

Per-voice resonant low-pass (CUTOFF, RESO), soft-clip DRIVE with SAT mix and
GAIN. Drive before the filter is more aggressive; after, it just gets louder.

### MASTER FX panel

Applied to the summed 8-voice mix. Type: Off / Flanger / Phaser / Bitcrush /
Fractal. When Off, the whole section dims and is bypassed.

| Knob | Meaning |
|---|---|
| DEPTH | Delay time / phase spread (Fractal: how many tonic periods the tail may span) |
| RATE | Modulation rate (Hz) (Fractal: how often the comb re-tunes) |
| FDBK | Feedback |
| MIX | Dry/wet |
| BITS | Bitcrush resolution (2–16 bit) |
| CHAOS | Fractal entropy: how bright the tail rings, and where inside the span the comb lands |

**Fractal** is a tonal tail, not a noise generator: the chaos map picks a feedback
comb whose length is a whole number of periods of the note you are playing, so the
wet signal reinforces the tonic and its harmonics. DEPTH opens that span from two
periods up to eight, RATE sets how often the comb jumps to a new one (slow = one
relationship held for bars), CHAOS sets how bright the tail rings. Its level is
normalised by feedback, so the effect never gets louder than the dry mix.

**Solo** selects Full / Dry-only / Wet-only output.

- **LIMIT**: master output limiter (on by default). Stereo-linked brickwall just
  under full scale: runaway chord/drive spikes are pulled down with a fast attack
  and slow release, and the output can never leave ±1. Off = unprotected signal.

### QUANTUM LAB band

Ten effects from the quantum-effects research report, in one full-width strip.
The four left knobs act **per voice** on the Schrödinger evolution; the four
right knobs (AMT / CHAR / SIZE / MIX) drive the **master lab effect** selected
in the title band (the row dims when the lab is off). The second selector
chooses the **exchange symmetry** between neighbouring voices.

| Knob | Meaning |
|---|---|
| GP SAT | Gross-Pitaevskii self-interaction g\|psi\|^2*psi: spectral peaks self-repel, a nonlinear quantum saturation ceiling (0 = linear evolution) |
| WKB | Tunneling filter: each momentum mode transmits a virtual barrier with T = exp(-2*kappa*L); fast components pass, slow ones reflect — a high-pass with the WKB skirt |
| PHASER | Dispersive quantum phaser: the readout probes the wave after an extra e^(-i*alpha*k^2) propagation; quadratic-phase notches sweep with the knob |
| PHASE | Global dynamical phase e^(-i*E*t/hbar): the wavetable slowly orbits between the Real and Imag readouts (<H> is measured from the state); sweeping POT CENTER additionally winds a Berry phase |
| AMT | Lab intensity (reverb send / shift scale / duck depth / spectral ceiling / echo level) |
| CHAR | Lab character, -100..+100%: disorder W / semitones (-12..+12) / event rate / modes kept / fidelity decay |
| SIZE | Lab size: T60 (0.3-8 s) / glide time / recovery time / echo window (60-1000 ms) |
| MIX | Lab dry/wet |

**Exchange symmetry**: BOSE (+) adds each voice's ring partner, read at the
voice's own scan phase, constructively; FERMI (-) subtracts it. Two or more
voices needed — try it with CHORD on.

**Anderson Reverb** is a 16-line feedback delay network whose loop mixing is an
orthogonal Hadamard matrix (a closed, unitary system). CHAR sets how much the
16 lengths are disordered: W = 0 keeps harmonic spacing (metallic), large W
localises the modes into a dense diffuse tail. SIZE is the T60, MIX the blend.

**Adiabatic Pitch** shifts with two Hann-complementary delay taps. The ratio
itself glides with a long time constant (SIZE) — the adiabatic theorem: slow
parameter changes move the level E ~ 1/L^2 without pops. CHAR is the shift
(-12..+12 st), AMT scales it toward zero.

**Born Events** runs a Poisson gate engine whose local rate follows the
smoothed signal power (louder = larger |psi|^2 = more measurements). Each event
ducks the wet path by AMT and recovers over SIZE. CHAR is the mean rate.

**Mode Clip** is a 256-point STFT spectral saturation: magnitudes above the
AMT ceiling are clipped and modes beyond CHAR (2..64) are zeroed — a
projection of the overloaded signal onto its lowest energy modes. Warm,
quantised drive with a fixed ~5 ms latency.

**Loschmidt Delay** plays the last SIZE window backwards — the audio analogue
of evolving with the reversed Hamiltonian U(-t) = U+(t) — faded at the window
edges and weighted by a Loschmidt-fidelity envelope exp(-G*t) (CHAR sets G,
AMT the echo level). Rhythmic reversed echoes at the window period.

### HARMONY strip

- **CHORD**: one key triggers a just-intonation voicing. Consonant sets
  (Power, Major, Minor, Sus4, Major7) use small-integer ratios; dissonant sets
  (Tritone, Cluster, Pythagorean) use large coprime ratios. Upper chord degrees
  get a velocity falloff.
- **SUPERPOSE**: with CHORD on, each note sounds *two* competing voicings at once,
  weighted cos²φ / sin²φ by a random phase. Releasing the key is a measurement: it
  collapses to one winner (probability cos²φ) and kills the loser, so the same note
  can resolve to a different harmony on every take. Off = the single selected voicing.
- **PITCH**: global transpose ±48 st (±4800 cents), applied as 2^(cents/1200).
- **WALK**: the plugin composes its own melody. A continuous-time quantum walk
  runs on a 16-node ring (two-octave pentatonic); each measurement tick the
  walker's position — sampled with Born probabilities — becomes a note, velocity
  ∝ detection probability. Interference makes lines return to the start instead
  of diffusing away. Deterministic per instance; turns the walk voice off
  cleanly when switched down.
- **RATE**: walk measurement ticks per second (0.25–8 Hz) — how often a new
  note is sampled from the walker.
- **EPR**: entangled voices (needs COLLAPSE > 0). Voices sharing one MIDI note
  (e.g. the two SUPERPOSE variants) behave like one EPR pair: a phonon jump on
  one partner instantly applies the same collapse to the others in the same
  block, so they snap to consistent states together. No cascade — forced
  collapses never re-trigger propagation.

---

## 5. Presets

- The header combo lists 24 factory presets (embedded at build time) and your own.
  Picking one from the list recalls it straight away.
- **LOAD** re-applies the selected preset; **SAVE** writes a `.sp303preset` JSON to
  the app-support folder; **DEL** removes a user preset.
- Presets store all parameters *including the UI skin*, so recalling one leaves
  nothing of the previous sound behind.

## 6. Skins

The header combo (Dark / SciFi Neon / Console) changes colours only, never
behaviour. The choice is saved with the plugin state.

## 7. MIDI

Any MIDI note on/off works; channel 10 is not special-cased. With chord mode on,
one note-on spawns several voices — keep an eye on the 8-voice limit; oldest /
quietest voices are stolen first.

## 8. Performance notes

- CPU is dominated by `1/dt` FFTs: the evolution rate (STEP) and DT matter,
  the number of knobs does not.
- The scope repaints only when the engine's data actually changes (30 Hz poll,
  sequence-checked).
- Bench reference (`SpiralQuantTests --bench`): ~645 µs per 512-sample block at
  48 kHz with 8 active voices on Apple Silicon.

## 9. Troubleshooting

| Symptom | Fix |
|---|---|
| No sound | Raise GAIN, check the host sends MIDI notes. QM off does not mute — it only freezes the wave. |
| Sound is static | STEP too high (wave evolves slowly) — lower it, or QM is off. |
| Harsh/buzzy | Sample mode Magnitude with narrow WIDTH; lower DRIVE or open CUTOFF. |
| Wave looks flat | You are looking at a square/saw magnitude — that *is* the shape. Play a note and watch it evolve. |
| Spectrum shows "—" | Frequency labels appear after the first note (f₀ comes from the played note). |

---

See also: [README](../README.md) for the DSP math, [RU manual](MANUAL_ru.md).
