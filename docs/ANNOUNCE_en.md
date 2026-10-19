# SpiralQuant — Announcement (EN)

## Short (stories / X / reel caption)

**🌀 SpiralQuant — a synthesizer that solves the Schrödinger equation while you
play.**

Every voice is a real 128-point complex wavefunction ψ(x), evolved at audio rate
by split-step Fourier. You don't draw waveforms here — you build the box the
wave lives in: pick a particle shape, choose a potential, and listen to it slosh,
reflect, interfere with itself and *tunnel through the wall*. Quantum jumps,
Zeno pinning, squeezed uncertainty, entangled voices and a quantum walk that
writes its own melody. VST3 + AU + Standalone, macOS / Windows.

Author: **Spiralfractal**

---

## Detailed (post)

**🌀 SpiralQuant — the timbre *is* the interference.**

Not another wavetable with a physics sticker on it. Each of the eight voices
carries a genuine 128-point complex ψ(x), and every sample the engine takes a
split-step Fourier leap: half a step of the potential in position space, a
kinetic step in momentum space, renormalise, read out. The wave accelerates,
bounces, splits and recombines with itself — and what you hear is that
interference, not an imitation of it.

🎛 **What you get**
- **A wavefunction per voice** — 128-point complex ψ(x) evolved live; DT and STEP
  set the physics clock, so the same patch can crawl as a slow morph or run at
  full audio rate
- **6 waves × 6 potentials** — Gaussian, Sine, Cos, Parabola, Saw, Square inside
  Flat / Harmonic / Infinite Well / Barrier / Double Well / Linear boxes, with
  centre, width and height you drag on the graph; read out Re, Im, |ψ|, phase,
  the PhononQND staircase, or freeze evolution and take the momentum FFT as audio
- **Tunnelling** — Gamow transparency T = exp(−2κL) leaks the wave through the
  barrier instead of hard-reflecting it: long, shimmering, non-repeating tails
- **Measurement and quantum jumps** — COLLAPSE fires stochastic phonon
  collapses (percussive snaps and granular decay on top of the smooth wave),
  ZENO pins a watched packet in place, SQUEEZE trades position spread for
  momentum spread, EPR makes voices on one note collapse together
- **It composes for you** — WALK runs a continuous-time quantum walk on a
  16-node two-octave pentatonic ring and samples a note per measurement tick
  with Born probabilities; CHORD voices just-intonation ratios and SUPERPOSE
  sounds two competing voicings at once, then releases the key as a measurement
  that keeps one winner
- **Cavity behaviour** — MEMORY self-phase-modulates ψ through its own delayed
  density (echoes and chirps), RING leaks an exponential cavity tail after the
  release
- **Master FX on the summed mix** — Flanger / Phaser / Bitcrush, plus Fractal: a
  chaos map that picks a feedback comb whose length is a whole number of periods
  of the note you are playing, so the tail rings tonally and never louder than
  the dry signal
- **Real scope** — 128-point |ψ(x)| and the |ψ̃(k)| momentum spectrum with a
  harmonic frequency grid, fed by a double-buffered snapshot of the actual DSP;
  the UI never blocks the audio thread
- ADSR, resonant low-pass with drive and saturation, ±48 st transposition,
  master limiter, 3 skins (Dark / SciFi Neon / Console), scalable UI, preset
  browser
- **18 factory presets** and 8 voices at ≈ 0.06× realtime CPU
- VST3 + AU + Standalone, macOS universal / Windows

📐 **Measured, not promised.** The whole engine runs offline in tests: 6 waves ×
6 potentials × 5 readouts, every FX type, chord and quantum-measurement
combinations swept across sample-rate and block-size settings, Parseval energy
checked between position and momentum readouts, the limiter, squeeze, ringdown
and fractal levels asserted — and all 18 factory presets rendered out and
required to be finite and non-silent.

Try: **Infinite Well + narrow Gaussian packet + IMPULSE + TUNNEL** — and listen
to a wavefunction diffract.

Author: **Spiralfractal**

---

## Artwork

| file | size | for |
|---|---|---|
| `docs/marketing/product-card.png` | 2560×1440 | post (Telegram / VK) |
| `docs/marketing/product-card-ru.png` | 2560×1440 | same, Russian |
| `docs/marketing/stories.png` | 1080×1920 | stories / reel cover |
| `docs/marketing/stories-ru.png` | 1080×1920 | same, Russian |

The screenshot in the card is the real UI (`docs/screenshots/ui-hud.png`).
Regenerate the cards after a release:

```sh
python docs/marketing/make_cards.py
```

Card copy and version live in `COPY` / `VERSION` inside `make_cards.py`.
