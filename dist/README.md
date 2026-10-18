# dist/

Collected release builds (generated, not committed).

Run from the repository root:

```sh
cmake --build build --config Release --parallel
scripts/make_dist.sh
```

Produces `dist/SpiralQuant-v<version>-<os>-<arch>/` with VST3, AU (macOS) and
Standalone bundles plus `INSTALL.txt`, `README.md` and the `docs/` manuals, and
a matching `.zip` next to it.
