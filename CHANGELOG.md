# Changelog

All notable changes to SpiralQuant are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

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
