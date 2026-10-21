# AGENTS.md

Guidance for coding agents working in this repository.

## Overview

SpiralQuant is a JUCE 8-based audio plug-in (VST3/AU/CLAP/Standalone, C++17).
The core DSP is a real-time Schrödinger-equation wavetable synthesizer.
The project has no commits yet; treat the working tree as the source of truth.

## Repository layout

- `CMakeLists.txt` — build definition; plugin + `SpiralQuantTests` console app.
- `Source/PluginProcessor.{h,cpp}` — APVTS parameter layout and processing entry.
- `Source/PluginEditor.{h,cpp}` — UI layout and parameter attachments.
- `Source/dsp/` — engine (`QuantumEngine`), voice (`QuantumVoice`), oscillator
  (`QuantumOsc`), master FX (`MasterFx`), quantum lab (`QuantumLab`: Anderson
  FDN reverb, adiabatic pitch, Born-rule events, mode clip, Loschmidt delay).
- `Source/gui/` — palette/skins (`Palette.h`), look-and-feel, scope canvas,
  ADSR editor, preset browser.
- `tests/TestRender.cpp` — golden render/state test.
- `vendor/JUCE/` — **vendored dependency; never edit.**
- `vendor/clap-juce-extensions/` — fork dsp56300, pinned to SHA
  `e1f67893cc409a40c1154fa2e78c97046da24ce0`; **vendored dependency; never edit.**
  Provides the `SpiralQuant_CLAP` target (CLAP_ID `com.spiralquant.plugin`).
- `presets/` — JSON presets embedded at build time.

## Commands

Fetch the vendored dependencies once (both are gitignored):

```sh
git clone --depth 1 --branch 8.0.14 https://github.com/juce-framework/JUCE.git vendor/JUCE
git clone https://github.com/dsp56300/clap-juce-extensions.git vendor/clap-juce-extensions
git -C vendor/clap-juce-extensions checkout e1f67893cc409a40c1154fa2e78c97046da24ce0
git -C vendor/clap-juce-extensions submodule update --init --recursive
```

Configure (macOS universal by default):

```sh
cmake -S . -B build
```

Build plug-in:

```sh
cmake --build build --config Release --parallel
```

CLAP only (artefact at `build/SpiralQuant_artefacts/<CFG>/CLAP/SpiralQuant.clap`,
auto-copied to `~/Library/Audio/Plug-Ins/CLAP/` on macOS, `~/.clap/` on Linux):

```sh
cmake --build build --target SpiralQuant_CLAP --config Release --parallel
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
  Dark / SciFi Neon / Console / Golden, selected by the UI-only `uiSkin` APVTS param.
  `PluginEditor` lays out a fixed 1280×920 base grid scaled by `uiScale`; the
  grid proportions derive from the golden section (see the phi comments at the
  top of `PluginEditor.cpp`) — keep new panels/knob groups on that grid.
- CLAP reuses the same `AudioProcessor` shared code as VST3/AU; the host
  transfers state and presets through the standard `setStateInformation`, so
  no CLAP-specific code lives in `Source/`.

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
- Configuring requires `vendor/clap-juce-extensions` on disk (CMake guard),
  even when only building VST3/AU.

## Safety checks

Run the full render suite after any DSP or parameter change. A change that
introduces NaN/inf or silence for any combination is a regression.
