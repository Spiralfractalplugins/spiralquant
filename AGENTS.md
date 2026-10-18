# AGENTS.md

Guidance for coding agents working in this repository.

## Overview

SpiralQuant is a JUCE 7-based audio plug-in (VST3/AU/Standalone, C++17).
The core DSP is a real-time Schrödinger-equation wavetable synthesizer.
The project has no commits yet; treat the working tree as the source of truth.

## Repository layout

- `CMakeLists.txt` — build definition; plugin + `SpiralQuantTests` console app.
- `Source/PluginProcessor.{h,cpp}` — APVTS parameter layout and processing entry.
- `Source/PluginEditor.{h,cpp}` — UI layout and parameter attachments.
- `Source/dsp/` — engine (`QuantumEngine`), voice (`QuantumVoice`), oscillator
  (`QuantumOsc`).
- `Source/gui/` — palette/skins (`Palette.h`), look-and-feel, scope canvas,
  ADSR editor, preset browser.
- `tests/TestRender.cpp` — golden render/state test.
- `vendor/JUCE/` — **vendored dependency; never edit.**
- `presets/` — JSON presets embedded at build time.

## Commands

Configure (macOS universal by default):

```sh
cmake -S . -B build
```

Build plug-in:

```sh
cmake --build build --config Release --parallel
```

Build and run tests:

```sh
cmake --build build --target SpiralQuantTests --config Release
./build/SpiralQuantTests_artefacts/SpiralQuantTests          # single-config generators
./build/SpiralQuantTests_artefacts/Release/SpiralQuantTests  # Xcode/multi-config
```

`--bench` appended prints µs/audio-block for the 8-voice case; compare against
~645 µs/block on Apple Silicon to catch perf regressions.

Expected test output ends with `ALL TESTS PASSED`.

## Architecture notes

- `QuantumEngine` owns 8 `QuantumVoice`s, tracks note numbers/ages for voice
  stealing, renders the sum into the output buffer (channel 0 then copied).
- Each voice runs an ADSR, a `QuantumOsc`, an IIR low-pass, drive/saturation
  and gain. Parameters are pushed globally before `processBlock`.
- `QuantumOsc` is a 128-point complex DSP updated by split-step Fourier;
  `getNextSample(phaseInc)` advances the wave if `samplesPerTimestep >= 1`.
- Master effects live in `MasterFx` (JUCE Chorus/Phaser + custom Bitcrush/Fractal)
  and run after voice mixing in `QuantumEngine::process`.
- Scope data is double-buffered in `QuantumEngine` (`scopePos/scopeMom/scopePot
  [2][128]` + atomic slot/sequence flip); the UI polls
  `processor.snapshotScope()` on a 30 Hz timer and repaints only when the
  sequence counter changes. The reader never blocks the audio thread.
- UI colours come from `Source/gui/Palette.h` (`Skins::current()`); skins are
  Dark / SciFi Neon / Console, selected by the UI-only `uiSkin` APVTS param.
  `PluginEditor` lays out a fixed 1280×920 base grid scaled by `uiScale`.

## Coding conventions

- C++17, include via `JuceHeader.h` then project headers.
- Use `juce::jlimit` / `juce::MathConstants` rather than hand-rolled clamps.
- Clamp parameter ranges in DSP as `QuantumVoice::setParams` does; keep UI ranges
  in `PluginEditor` consistent.
- Tabs/spaces: follow existing file (4 spaces in most Source files).
- Do not rename or insert parameters in the middle of `createLayout()` without
  considering host index stability (AUv2 addresses params by index; new params
  go at the end, as `uiSkin` does).

## Adding/changing parameters

1. Add to `createLayout()` in `PluginProcessor.cpp`.
2. Read in `processBlock()` and forward through `QuantumEngine::setParams`.
3. Update `QuantumVoice::setParams` and `QuantumOsc` if needed.
4. Add slider/attachment/range/layout in `PluginEditor.cpp`.
5. Update `README.md` parameter table.
6. Re-run tests.

## Known pitfalls

- `vendor/JUCE/` is huge; search within project Source instead.

## Safety checks

Run the full render suite after any DSP or parameter change. A change that
introduces NaN/inf or silence for any combination is a regression.
