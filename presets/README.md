# SpiralQuant presets

Each `*.sp303preset` file is a JSON object with format 1:

```json
{
  "format": 1,
  "name": "Preset name",
  "params": { "waveType": 0, "gain": 0.8 },
  "sequence": "",
  "gate": "",
  "credit": "Factory"
}
```

Parameter keys must match the plug-in parameter IDs in `Source/PluginProcessor.cpp`.
Example presets included here:

- `01_Init.sp303preset`
- `02_Neon_Flanger.sp303preset`
- `03_Bitcrush_Fractal.sp303preset`
