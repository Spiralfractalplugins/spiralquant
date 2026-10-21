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

Produces `dist/SpiralQuant-v<version>-<os>-<arch>/` with VST3, AU (macOS), CLAP
and Standalone bundles plus `INSTALL.txt`, `README.md` and the `docs/` manuals,
and a matching `.zip` next to it. The CLAP bundle only appears if the tree was
built with the `SpiralQuant_CLAP` target (and `vendor/clap-juce-extensions`
fetched); `make_dist.sh` skips it silently otherwise.

Windows and Linux builds come from the container cross-build
(`./tools/windows-cross-build/cross-build.sh windows`, see `docs/BUILDING.md`).
The Linux output carries a `SpiralQuant.clap`; the Windows one does not (the
container scripts build VST3 + Standalone there), so that zip ships VST3 +
Standalone until the Windows CLAP target is added. The platform is taken from
the directory name, so no extra argument is needed:

```sh
scripts/make_dist.sh build/windows
scripts/make_dist.sh build/linux
```
