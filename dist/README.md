# dist/

Collected release builds (generated, not committed).

Run from the repository root:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
scripts/make_dist.sh build-release
```

Pass the directory that was actually configured for Release: `make_dist.sh`
copies whatever bundles it finds, so pointing it at a Debug build tree ships a
Debug plug-in.

Produces `dist/SpiralQuant-v<version>-<os>-<arch>/` with VST3, AU (macOS) and
Standalone bundles plus `INSTALL.txt`, `README.md` and the `docs/` manuals, and
a matching `.zip` next to it.
