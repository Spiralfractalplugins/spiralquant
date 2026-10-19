# SpiralQuant presets

Each `*.sp303preset` file is a JSON object with format 1:

```json
{
  "format": 1,
  "name": "Preset name",
  "params": { "waveType": 0, "gain": 0.8 },
  "credit": "Factory"
}
```

Parameter keys must match the plug-in parameter IDs in `Source/PluginProcessor.cpp`,
and their values are the **scaled** ones shown on the UI (cents, ms, Hz, 0–1),
never the host's normalised 0–1 values. `params` may be omitted to put the values
at the top level.

Factory presets carry the full parameter set, so recalling one leaves nothing of the
previous sound behind. User presets are written by the `SAVE` button and stored in
`~/Library/Application Support/SpiralQuant/presets`.

## Factory bank

| File | Preset |
| --- | --- |
| `01_Init.sp303preset` | Init |
| `02_Neon_Flanger.sp303preset` | Neon Flanger |
| `03_Bitcrush_Fractal.sp303preset` | Bitcrush Fractal |
| `04_Tonic_Fractal.sp303preset` | Tonic Fractal |
| `05_Fractal_Octave.sp303preset` | Fractal Octave Lift |
| `06_Deep_Sub.sp303preset` | Deep Sub Drone |
| `07_Quantum_Walk.sp303preset` | Quantum Walk |
| `08_Zeno_Freeze.sp303preset` | Zeno Freeze |
| `09_Squeezed_Phase.sp303preset` | Squeezed Phase |
| `10_EPR_Chorale.sp303preset` | EPR Chorale |
| `11_Superposed_Chords.sp303preset` | Superposed Chords |
| `12_Tunneling_Bell.sp303preset` | Tunneling Bell |
| `13_DoubleWell_Pluck.sp303preset` | Double Well Pluck |
| `14_Phonon_Bass.sp303preset` | Phonon Bass |
| `15_Cavity_Ring.sp303preset` | Cavity Ring |
| `16_SciFi_Sine.sp303preset` | SciFi Sine Lead |
| `17_Console_Chip.sp303preset` | Console Chip |
| `18_Momentum_Readout.sp303preset` | Momentum Readout |

`SpiralQuantTests` checks every embedded preset: numeric values, known parameter IDs,
in-range results, and a clean non-silent render.
