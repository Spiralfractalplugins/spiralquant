# SpiralQuant

**SpiralQuant** is a polyphonic software synthesizer plug-in built with [JUCE](https://juce.com/).
Instead of a classic wavetable, each voice's oscillator is a complex-valued
solution of the 1D time-dependent Schrödinger equation, evolved in real time with
a split-step Fourier method on a 128-point grid.

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
- **Scope**: 128-point display of `|ψ|` from the first active voice.

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
| `sampleMode`         | choice   | Real / Imag / Magnitude / Phase               | 0 (Real) |
| `timestepDelta`      | float    | 0.001 – 0.2                                   | 0.01    |
| `samplesPerTimestep` | float    | 0.1 – 64                                      | 8       |
| `applyWavefunction`  | bool     | —                                             | true    |
| `attack`             | float    | 1 – 2000 ms                                   | 10      |
| `decay`              | float    | 1 – 2000 ms                                   | 100     |
| `sustain`            | float    | 0.0 – 1.0                                     | 0.8     |
| `release`            | float    | 1 – 5000 ms                                   | 200     |
| `filterFreq`         | float    | 20 – 20000 Hz                                 | 4000    |
| `filterRes`          | float    | 0.0 – 1.0                                     | 0.3     |
| `drive`              | float    | 0.0 – 1.0                                     | 0.0     |
| `satMix`             | float    | 0.0 – 1.0                                     | 0.0     |
| `gain`               | float    | 0.0 – 2.0                                     | 0.8     |

Parameters are stored in an `AudioProcessorValueTreeState`, so host automation and
state save/restore are supported.

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

- renders every `waveType × potentialType × sampleMode` combination (6×6×4),
  checking for NaN/inf and non-silent output;
- exercises a state save/restore roundtrip.

It must print `ALL TESTS PASSED`.

## Build artefacts (macOS)

- `build/SpiralQuant_artefacts/Release/VST3/SpiralQuant.vst3`
- `build/SpiralQuant_artefacts/Release/AU/SpiralQuant.component`
- `build/SpiralQuant_artefacts/Release/Standalone/SpiralQuant.app`

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
  gui/                   look-and-feel + (legacy, not all wired) widgets
tests/TestRender.cpp
vendor/JUCE/            vendored JUCE (do not edit)
```

## Known limitations

- `applyWavefunction` is exposed and automated but the DSP currently always applies
  evolution (the toggle is ignored).
- The potential-type label differs between the parameter combo (`InfiniteWell`)
  and some UI strings (`Well`).
- `Source/gui/SequenceEditorComponent.h` and the Gate widgets are not compiled
  into the current build; treat them as legacy/experimental.
