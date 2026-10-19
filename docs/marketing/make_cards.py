#!/usr/bin/env python3
"""Render social-media cards for SpiralQuant from the committed UI screenshot.

    python docs/marketing/make_cards.py

Reads docs/screenshots/ui-hud.png and emits product-card{,-ru}.png (16:9) and
stories{,-ru}.png (9:16) next to this file. Fonts resolve per-OS so the same
script runs on macOS and a plain Windows checkout.
"""

import math
import os
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
SHOT = ROOT / "docs" / "screenshots" / "ui-hud.png"

# Palette taken from the plugin's Dark skin: graphite plate, cyan wavefunction,
# amber potential, violet envelope, red FX.
BG_TOP = (9, 12, 16)
BG_BOTTOM = (18, 21, 26)
GRID = (40, 46, 56)
INK = (200, 208, 220)
MUTED = (122, 132, 146)
WAVE = (74, 200, 232)    # primary accent — psi
POT = (232, 160, 60)     # secondary — the box the wave lives in
ENV = (176, 106, 232)    # chips / measurement


def _font_candidates():
    if sys.platform == "darwin":
        reg = ["/System/Library/Fonts/HelveticaNeue.ttc",
               "/System/Library/Fonts/Supplemental/Arial.ttf"]
        bold = ["/System/Library/Fonts/HelveticaNeue.ttc",
                "/System/Library/Fonts/Supplemental/Arial Bold.ttf"]
    elif os.name == "nt":
        fdir = Path(os.environ.get("WINDIR", "C:\\Windows")) / "Fonts"
        reg = [fdir / "arial.ttf", fdir / "segoeui.ttf"]
        bold = [fdir / "arialbd.ttf", fdir / "segoeuib.ttf"]
    else:
        d = Path("/usr/share/fonts")
        reg = list(d.rglob("DejaVuSans.ttf")) + list(d.rglob("Arial.ttf"))
        bold = list(d.rglob("DejaVuSans-Bold.ttf")) + list(d.rglob("Arial Bold.ttf"))
    return [str(p) for p in reg], [str(p) for p in bold]


REG, BOLD = _font_candidates()
_CACHE = {}


def font(size=32, bold=False):
    key = (size, bold)
    if key not in _CACHE:
        path = (BOLD if bold else REG)
        chosen = next((p for p in path if Path(p).exists()), None)
        if chosen is None:
            _CACHE[key] = ImageFont.load_default(size)
        elif chosen.endswith(".ttc") and "HelveticaNeue" in chosen:
            _CACHE[key] = ImageFont.truetype(chosen, size, index=1 if bold else 0)
        else:
            _CACHE[key] = ImageFont.truetype(chosen, size)
    return _CACHE[key]


def gradient(size, top=BG_TOP, bottom=BG_BOTTOM):
    img = Image.new("RGB", size, top)
    d = ImageDraw.Draw(img)
    for y in range(size[1]):
        t = y / max(1, size[1] - 1)
        d.line([(0, y), (size[0], y)],
               fill=tuple(int(a + (b - a) * t) for a, b in zip(top, bottom)))
    return img.convert("RGBA")


def grid(img, step=88):
    d = ImageDraw.Draw(img)
    for x in range(0, img.width, step):
        d.line([(x, 0), (x, img.height)], fill=GRID, width=1)
    for y in range(0, img.height, step):
        d.line([(0, y), (img.width, y)], fill=GRID, width=1)
    return img


def mark(img, cx, cy, r):
    """The SpiralQuant signature: a dispersing wave packet, cyan into violet."""
    d = ImageDraw.Draw(img)
    n = 260
    pts = []
    for i in range(n):
        t = i / (n - 1)
        centre = 0.30 + 0.28 * t
        spread = 0.075 + 0.20 * t
        env = math.exp(-((t - centre) ** 2) / (2 * spread * spread))
        x = cx - r + t * 2 * r
        y = cy - env * math.sin((t - centre) * 46.0) * r * 0.92
        pts.append((x, y))
    w = max(2, r // 8)
    d.line(pts[: n * 3 // 5], fill=WAVE, width=w, joint="curve")
    d.line(pts[n * 3 // 5 - 1:], fill=ENV, width=w, joint="curve")
    d.ellipse([pts[0][0] - r * .16, pts[0][1] - r * .16,
               pts[0][0] + r * .16, pts[0][1] + r * .16], fill=WAVE)


def panel(img, box, radius=28, glow=WAVE, border=(51, 58, 68)):
    """Glowing rounded card: blurred halo behind a stroked dark plate."""
    halo = Image.new("RGBA", img.size, (0, 0, 0, 0))
    hd = ImageDraw.Draw(halo)
    pad = 6
    hd.rounded_rectangle([box[0] - pad, box[1] - pad, box[2] + pad, box[3] + pad],
                         radius=radius + pad, fill=glow + (80,))
    img.alpha_composite(halo.filter(ImageFilter.GaussianBlur(26)))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle(box, radius=radius, fill=(18, 21, 26, 235),
                        outline=border, width=3)


def text(d, xy, s, f, fill=INK, tracking=0):
    x, y = xy
    for ch in s:
        d.text((x, y), ch, font=f, fill=fill)
        x += d.textlength(ch, font=f) + tracking
    return x - xy[0]


def width(s, f, tracking=0):
    m = Image.new("RGBA", (8, 8))
    d = ImageDraw.Draw(m)
    return sum(d.textlength(c, font=f) + tracking for c in s) - tracking


def bullets(img, items, x, y, w, f_head, f_sub, gap, dot_r=9):
    d = ImageDraw.Draw(img)
    for head, sub in items:
        d.ellipse([x, y + f_head.size * 0.30 - dot_r, x + dot_r * 2,
                   y + f_head.size * 0.30 + dot_r], fill=POT)
        text(d, (x + dot_r * 2 + 22, y), head, f_head, INK)
        yy = y + f_head.size + 10
        line = ""
        for word in sub.split():
            if width(line + " " + word, f_sub) > w - dot_r * 2 - 22:
                text(d, (x + dot_r * 2 + 22, yy), line, f_sub, MUTED)
                yy += int(f_sub.size * 1.32)
                line = word
            else:
                line = (line + " " + word).strip()
        if line:
            text(d, (x + dot_r * 2 + 22, yy), line, f_sub, MUTED)
            yy += f_sub.size
        y = yy + gap
    return y


def chips(img, labels, x, y, f, colour=WAVE, h=76, pad=30, gap=18):
    d = ImageDraw.Draw(img)
    for s in labels:
        w = width(s, f) + pad * 2
        d.rounded_rectangle([x, y, x + w, y + h], radius=14, outline=colour, width=3)
        text(d, (x + pad, y + (h - f.size) // 2 - 4), s, f, colour)
        x += w + gap
    return x


def shot(img, box):
    """Paste the real UI paint, keeping its aspect inside box."""
    im = Image.open(SHOT).convert("RGBA")
    bw, bh = box[2] - box[0], box[3] - box[1]
    scale = min(bw / im.width, bh / im.height)
    size = (round(im.width * scale), round(im.height * scale))
    ox, oy = box[0] + (bw - size[0]) // 2, box[1] + (bh - size[1]) // 2
    img.alpha_composite(im.resize(size, Image.LANCZOS), (ox, oy))
    return (ox, oy, ox + size[0], oy + size[1])


COPY = {
    "en": {
        "tagline": "REAL-TIME SCHRÖDINGER SYNTHESIZER",
        "bullets": [
            ("A wavefunction per voice", "128-point complex ψ(x) evolved at audio "
                                         "rate by split-step Fourier; the interference "
                                         "of the wave with itself is the timbre"),
            ("6 waves × 6 potentials", "Gaussian, Sine, Cos, Parabola, Saw, Square "
                                        "inside Flat / Harmonic / Well / Barrier / "
                                        "Double Well / Linear; Re, Im, |ψ|, phase or "
                                        "momentum FFT readout"),
            ("Tunnelling", "Gamow transparency T = exp(−2κL) leaks the wave "
                           "through the wall instead of hard-reflecting it; long "
                           "shimmering tails"),
            ("Measurement & quantum jumps", "COLLAPSE phonon rate, ZENO pinning, "
                                             "SQUEEZE uncertainty, EPR entangled "
                                             "voices: percussive snaps and granular "
                                             "decay on top of the smooth wave"),
            ("It plays itself", "WALK: a quantum walk on a 16-node pentatonic ring "
                                "samples notes with Born probabilities; CHORD just "
                                "intonation with SUPERPOSE that collapses on release"),
            ("Cavity & master FX", "MEMORY lag resonator, RING cavity leak, Flanger / "
                                   "Phaser / Bitcrush / Fractal tonal tail, limiter; "
                                   "live |ψ(x)| scope and |ψ(k)| spectrum"),
        ],
        "chips": ["VST3", "AU", "STANDALONE", "macOS", "Windows"],
        "foot": "8 voices  ·  3 skins  ·  18 factory presets",
        "author": "by Spiralfractal",
    },
    "ru": {
        "tagline": "СИНТЕЗАТОР РЕШАЮЩИЙ УРАВНЕНИЕ ШРЕДИНГЕРА",
        "bullets": [
            ("Волновая функция в каждом голосе", "128-точечная комплексная ψ(x), "
                                                  "эволюционирующая на аудио-скорости "
                                                  "методом split-step Fourier; тембр — "
                                                  "это интерференция волны с собой"),
            ("6 форм × 6 потенциалов", "Gaussian, Sine, Cos, Parabola, Saw, Square в "
                                        "ящиках Flat / Harmonic / Well / Barrier / "
                                        "Double Well / Linear; чтение Re, Im, |ψ|, "
                                        "фаза или импульсный FFT"),
            ("Туннелирование", "прозрачность Гамова T = exp(−2κL): волна "
                               "просачивается сквозь стену, а не отражается жёстко — "
                               "длинные мерцающие хвосты"),
            ("Измерение и квантовые скачки", "COLLAPSE-скорость фононов, ZENO-наблюдение, "
                                              "SQUEEZE неопределённость, EPR-сплетённые "
                                              "голоса: перкуссионные щелчки и гранулярный "
                                              "распад"),
            ("Синтезатор сочиняет сам", "WALK — квантовое блуждание по 16-узльному "
                                        "пентатоническому кольцу, ноты по вероятностям "
                                        "Борна; CHORD с чистой интонацией и SUPERPOSE, "
                                        "схлопывающийся при отпускании"),
            ("Резонатор и мастер-эффекты", "MEMORY со запаздыванием, RING-утечка "
                                           "полости, Flanger / Phaser / Bitcrush / "
                                           "Fractal, лимитер; живой осциллограф "
                                           "|ψ(x)| и спектр |ψ(k)|"),
        ],
        "chips": ["VST3", "AU", "STANDALONE", "macOS", "Windows"],
        "foot": "8 голосов  ·  3 скина  ·  18 фабричных пресетов",
        "author": "автор Spiralfractal",
    },
}

VERSION = "v0.1.1"


def post(lang):
    c = COPY[lang]
    img = grid(gradient((2560, 1440)))
    d = ImageDraw.Draw(img)

    mark(img, 96, 168, 34)
    text(d, (150, 148), "S P I R A L Q U A N T", font(size=34, bold=True), MUTED,
         tracking=2)
    panel(img, (120, 236, 1500, 1236))
    shot(img, (152, 268, 1468, 1204))

    chips(img, c["chips"], 124, 1280, font(size=27, bold=True),
          h=66, pad=26, gap=16)
    text(d, (126, 1368), c["foot"], font(size=28), MUTED)

    x = 1580
    text(d, (x, 176), "SpiralQuant", font(size=104, bold=True), INK)
    text(d, (x + 6, 316), c["tagline"], font(size=27, bold=True), WAVE, tracking=2)
    d.line([(x + 4, 370), (2452, 370)], fill=(52, 66, 84), width=3)

    y = bullets(img, c["bullets"], x + 6, 396, 930,
                font(size=30, bold=True), font(size=24), gap=16)
    y = min(y + 30, 1216)
    text(d, (x + 6, y), c["author"], font(size=42, bold=True), WAVE, tracking=1)
    text(d, (x + 8, y + 74), VERSION, font(size=28, bold=True), MUTED)
    return img


def stories(lang):
    c = COPY[lang]
    img = grid(gradient((1080, 1920)), step=72)
    d = ImageDraw.Draw(img)

    mark(img, 128, 146, 44)
    text(d, (196, 98), "SpiralQuant", font(size=78, bold=True), INK)
    text(d, (200, 196), c["tagline"], font(size=22, bold=True), WAVE, tracking=2)
    d.line([(80, 248), (1000, 248)], fill=(52, 66, 84), width=3)

    panel(img, (56, 278, 1024, 846))
    shot(img, (76, 298, 1004, 826))

    y = bullets(img, c["bullets"], 84, 896, 878,
                font(size=29, bold=True), font(size=23), gap=15)

    x = 84
    for s in c["chips"]:
        f = font(size=23, bold=True)
        w = width(s, f) + 36
        if x + w > 1004:
            x, y = 84, y + 74
        d = ImageDraw.Draw(img)
        d.rounded_rectangle([x, y, x + w, y + 56], radius=12, outline=ENV, width=3)
        text(d, (x + 18, y + 13), s, f, ENV)
        x += w + 14
    y += 84

    d = ImageDraw.Draw(img)
    d.rounded_rectangle([56, y, 1024, y + 168], radius=24, fill=(14, 18, 23, 235),
                        outline=WAVE, width=4)
    text(d, (92, y + 26), c["author"], font(size=38, bold=True), WAVE, tracking=1)
    text(d, (94, y + 98), f"{VERSION}  ·  {c['foot']}", font(size=23), MUTED)
    return img


def main():
    if not SHOT.exists():
        raise SystemExit(f"missing {SHOT}")
    jobs = [("product-card.png", post("en")), ("product-card-ru.png", post("ru")),
            ("stories.png", stories("en")), ("stories-ru.png", stories("ru"))]
    for name, img in jobs:
        out = HERE / name
        img.convert("RGB").save(out)
        print(f"wrote {out} {img.size} {round(os.path.getsize(out)/1024)} KB")


if __name__ == "__main__":
    main()
