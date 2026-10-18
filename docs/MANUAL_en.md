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
|   (harmonic grid, Hz labels) |          DT STEP              |
+------------------------------+-------------------------------+
|                              |  POTENTIAL                    |
|   ADSR editor (drag handles) |  chips: POT, TUNNEL           |
|                              |  knobs: CENTER WIDTH HEIGHT   |
+------------------------------+-------------------------------+
| HARMONY: CHORD chip, chord   |  ENVELOPE: A D S R            |
| type combo, PITCH knob       |  FILTER & DRIVE: CUTOFF RESO  |
|                              |    DRIVE SAT GAIN             |
|                              |  MASTER FX: type/solo +       |
|                              |    DEPTH RATE FDBK MIX BITS   |
|                              |    CHAOS                      |
+------------------------------+-------------------------------+
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
| **FFT** toggle | Freeze evolution and read ψ̃(k): the momentum spectrum becomes the audio. Very metallic/organ-like. |
| **QM** toggle | Master switch for evolution. Off = the wave freezes mid-shape and the voice becomes a static wavetable. |
| **Sample mode** combo | Which part of complex ψ becomes audio: Real / Imag / Magnitude / Phase. Magnitude = bright and buzzy; Phase = FM-like. |

### POTENTIAL panel

| Control | What it does |
|---|---|
| **POT** chip | The "box": Flat, Harmonic, InfiniteWell, Barrier, DoubleWell, Linear. Determines how the wave accelerates, reflects and interferes. |
| **CENTER / WIDTH / HEIGHT** | Position, scale and depth of the potential. HEIGHT also sets the barrier wall threshold. |
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
| DEPTH | Delay time / phase spread |
| RATE | Modulation rate (Hz) |
| FDBK | Feedback |
| MIX | Dry/wet |
| BITS | Bitcrush resolution (2–16 bit) |
| CHAOS | Fractal entropy of the chaos effect |

**Solo** selects Full / Dry-only / Wet-only output.

### HARMONY strip

- **CHORD**: one key triggers a just-intonation voicing. Consonant sets
  (Power, Major, Minor, Sus4, Major7) use small-integer ratios; dissonant sets
  (Tritone, Cluster, Pythagorean) use large coprime ratios. Upper chord degrees
  get a velocity falloff.
- **PITCH**: global transpose ±2400 cents, applied as 2^(cents/1200).

---

## 5. Presets

- The header combo lists factory presets (embedded at build time) and your own.
- **LOAD** applies the selected preset; **SAVE** writes a `.sp303preset` JSON to
  the app-support folder; **DEL** removes a user preset.
- Presets store all parameters *including the UI skin*.

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
