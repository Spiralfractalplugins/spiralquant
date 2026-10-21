#!/usr/bin/env python3
"""Compose the SpiralQuant marketing product card from the live UI screenshot."""
import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SHOT = ROOT / "docs" / "screenshots" / "ui-hud.png"
OUT = ROOT / "docs" / "marketing" / "product-card.png"

W, H = 2560, 1440
BG = (7, 13, 24)
GRID = (16, 26, 42)
CYAN = (90, 210, 245)
ORANGE = (240, 160, 70)
WHITE = (245, 248, 252)
GREY = (150, 165, 185)
GREEN = (90, 225, 150)
PANEL = (11, 19, 33)

NEUE = "/System/Library/Fonts/HelveticaNeue.ttc"


def f_reg(s):
    return ImageFont.truetype(NEUE, s, index=0)


def f_bold(s):
    return ImageFont.truetype(NEUE, s, index=1)


def f_cond(s):
    return ImageFont.truetype(NEUE, s, index=4)


def spaced(draw, xy, text, font, fill, gap):
    x, y = xy
    for ch in text:
        draw.text((x, y), ch, font=font, fill=fill)
        x += draw.textlength(ch, font=font) + gap
    return x


def logo(img, cx, cy, r, lw):
    d = ImageDraw.Draw(img)
    for k, (col, rad) in enumerate(((CYAN, r), (ORANGE, r * 0.62))):
        d.arc([cx - rad, cy - rad, cx + rad, cy + rad],
              start=200 + k * 160, end=560 + k * 160, fill=col, width=lw)
    d.ellipse([cx - r * 0.16, cy - r * 0.16, cx + r * 0.16, cy + r * 0.16], fill=CYAN)
    d.line([cx - r * 1.15, cy + r * 0.9, cx - r * 0.2, cy + r * 0.9, cx + r * 0.35, cy + r * 0.55],
           fill=ORANGE, width=max(2, lw - 1), joint="curve")


def rounded_box(img, box, radius, fill, outline, width):
    d = ImageDraw.Draw(img)
    d.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def main():
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img, "RGBA")

    for x in range(0, W, 80):
        d.line([x, 0, x, H], fill=GRID, width=1)
    for y in range(0, H, 80):
        d.line([0, y, W, y], fill=GRID, width=1)

    # glowing wave along the bottom
    pts = [(x, int(H * 0.88 + math.sin(x / 260.0) * 46)) for x in range(0, W, 8)]
    for width, alpha in ((9, 40), (5, 90), (2, 220)):
        d.line(pts, fill=CYAN + (alpha,), width=width, joint="curve")

    # ---------------- left: the box, shot almost square to the camera -------
    fx0, fy0, fx1, fy1 = 150, 165, 1150, 1250
    depth = 42  # only a sliver of the side panel shows
    shot_w = fx1 - fx0 - 80
    shot_h = int(shot_w * 1720 / 2400)

    # side panel first, so the front face overlaps it cleanly
    d.polygon([(fx1, fy0), (fx1 + depth, fy0 + depth * 0.55),
               (fx1 + depth, fy1 - depth * 0.55), (fx1, fy1)],
              fill=(5, 9, 16), outline=(30, 60, 84), width=2)
    spine = Image.new("RGBA", (600, depth), (0, 0, 0, 0))
    spaced(ImageDraw.Draw(spine), (8, 2), "SPIRALQUANT", f_cond(30), (70, 150, 185, 255), 4)
    spine = spine.rotate(-90, expand=True)
    img.paste(spine, (fx1 + 2, fy1 - spine.height - 120), spine)

    rounded_box(img, (fx0, fy0, fx1, fy1), 16, PANEL, (56, 120, 150), 3)

    logo(img, fx0 + 78, fy0 + 88, 30, 5)
    d.text((fx0 + 132, fy0 + 52), "SPIRALQUANT", font=f_bold(54), fill=WHITE)
    spaced(d, (fx0 + 134, fy0 + 126), "QUANTUM WAVETABLE SYNTH", f_cond(26), CYAN, 3)

    shot = Image.open(SHOT).convert("RGB")
    shot = shot.resize((shot_w, shot_h), Image.LANCZOS)
    img.paste(shot, (fx0 + 40, fy0 + 185))
    d.rectangle([fx0 + 39, fy0 + 184, fx0 + 41 + shot_w, fy0 + 187 + shot_h], outline=(70, 150, 185), width=2)

    foot = fy0 + 190 + shot_h + 34
    d.text((fx0 + 42, foot), "SCHRÖDINGER DSP ENGINE", font=f_cond(34), fill=WHITE)
    d.text((fx0 + 42, foot + 50), "8 voices  ·  chords  ·  superposition", font=f_reg(27), fill=GREY)
    d.text((fx0 + 42, foot + 94), "VST3  |  AU  |  STANDALONE", font=f_cond(27), fill=CYAN)

    # ---------------- right: the copy --------------------------------------
    rx = 1330
    logo(img, rx + 40, 215, 34, 6)
    d.text((rx + 100, 150), "SpiralQuant", font=f_bold(96), fill=WHITE)
    d.text((rx, 288), "QUANTUM WAVETABLE SYNTHESIZER", font=f_cond(38), fill=CYAN)
    d.line([rx, 348, 2440, 348], fill=(40, 96, 124), width=2)

    bullets = [
        ("Real-time Schrödinger DSP", "split-step Fourier wavefunction, potential barrier, tunnelling"),
        ("8-voice polyphony", "just-intonation chords, superposed voicings that collapse on note-off"),
        ("Quantum controls", "phonon collapse, Zeno freeze, squeeze, EPR entanglement, quantum walk"),
        ("Tonal master FX", "fractal comb locked to the tonic, bitcrush, flanger, phaser, limiter"),
        ("18 factory presets", "user presets as plain JSON, three UI skins, ±48 st global pitch"),
    ]
    y = 400
    for title, detail in bullets:
        d.ellipse([rx + 6, y + 16, rx + 20, y + 30], fill=CYAN)
        d.text((rx + 44, y - 4), title, font=f_bold(37), fill=WHITE)
        d.text((rx + 44, y + 44), detail, font=f_reg(28), fill=GREY)
        y += 116

    chips = ["VST3", "AU", "STANDALONE", "macOS universal"]
    cx = rx
    for text in chips:
        cw = int(d.textlength(text, font=f_cond(31))) + 56
        d.rounded_rectangle([cx, y + 16, cx + cw, y + 84], radius=14,
                            outline=CYAN, width=2)
        d.text((cx + 28, y + 34), text, font=f_cond(31), fill=WHITE)
        cx += cw + 22

    d.text((rx, y + 118), "Made with JUCE 8   ·   ~0.65 ms per 512-sample block, 8 voices @ 48 kHz",
           font=f_reg(29), fill=GREY)
    d.text((rx, y + 186), "FREE & OPEN SOURCE", font=f_bold(46), fill=GREEN)
    d.text((rx, y + 258), "v0.1.1   ·   macOS universal   ·   VST3 / AU / Standalone",
           font=f_reg(29), fill=GREY)

    out = str(OUT)
    img.save(out)
    print("wrote", out, img.size)


if __name__ == "__main__":
    main()
