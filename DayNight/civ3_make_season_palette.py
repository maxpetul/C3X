#!/usr/bin/env python3
"""
Generate a 256-color GIMP .gpl palette from an image (no GIMP needed).

- Biases palette selection toward reds/oranges/pinks/blues and away from greens
- Quantizes to 254 colors, then appends Civ3-reserved colors:
    index 254 -> #00FF00 (green)
    index 255 -> #FF00FF (magenta)
- Ensures those reserved colors do NOT appear earlier in the palette.

Requires: Pillow
  pip install pillow

Usage:
  python3 generate_gpl_palette.py input.png output.gpl --name "Civ3 Seasonal" \
      --boost-red 60 --boost-yellow 40 --boost-blue 35 --boost-magenta 45 --reduce-green -30
"""

import argparse
import os
from PIL import Image
import colorsys

RESERVED_GREEN   = (0, 255, 0)   # #00FF00
RESERVED_MAGENTA = (255, 0, 255) # #FF00FF

def clamp01(x):
    return 0.0 if x < 0.0 else 1.0 if x > 1.0 else x

def bias_hues(img_rgb,
              boost_red=40, boost_yellow=20, boost_blue=30, boost_magenta=30, reduce_green=-30):
    """
    Return a new RGB image with per-hue saturation bias.
    boosts/reductions are in [-100..100] interpreted as percentage of S added.

    Hue bands (in degrees):
      Reds:     [345..360) U [0..15]
      Yellows:  [15..60]
      Greens:   [60..150]
      Blues:    [180..255]
      Mags:     [285..330]
    (We’re lenient; exact bands aren’t critical—goal is to nudge histograms.)
    """
    img = img_rgb.convert("RGB")
    px = img.load()
    w, h = img.size

    # Precompute fractional deltas
    f_red     = boost_red     / 100.0
    f_yellow  = boost_yellow  / 100.0
    f_green   = reduce_green  / 100.0  # typically negative
    f_blue    = boost_blue    / 100.0
    f_mag     = boost_magenta / 100.0

    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            # Normalize to [0..1]
            rf, gf, bf = r/255.0, g/255.0, b/255.0
            h_, s, v = colorsys.rgb_to_hsv(rf, gf, bf)  # h in [0..1]
            hue_deg = (h_ * 360.0) % 360.0

            # Choose band
            if hue_deg >= 345 or hue_deg < 15:
                s = clamp01(s + f_red)
            elif 15 <= hue_deg < 60:
                s = clamp01(s + f_yellow)
            elif 60 <= hue_deg < 150:
                s = clamp01(s + f_green)     # reduce greens (negative)
            elif 180 <= hue_deg < 255:
                s = clamp01(s + f_blue)
            elif 285 <= hue_deg < 330:
                s = clamp01(s + f_mag)

            r2, g2, b2 = colorsys.hsv_to_rgb(h_/1.0, s, v)
            px[x, y] = (int(round(r2*255)), int(round(g2*255)), int(round(b2*255)))
    return img

from PIL import Image

def quantize_254(img_rgb):
    """
    Quantize to up to 254 colors using Pillow and return a list of RGB triples.
    Robust to short palettes and unused entries.
    """
    # Prefer KMEANS if available, else MEDIANCUT
    if hasattr(Image, "Quantize") and hasattr(Image.Quantize, "KMEANS"):
        method = Image.Quantize.KMEANS
    else:
        method = Image.MEDIANCUT

    q = img_rgb.convert("RGB").quantize(colors=254, method=method, dither=Image.NONE)

    # Ensure 'P' mode (palettized)
    if q.mode != "P":
        q = q.convert("P")

    # Get the raw palette and how many entries it actually contains
    pal = q.getpalette() or []          # list[int], may be shorter than 768
    n_avail = len(pal) // 3             # number of RGB triples available

    # Figure out which palette indices are actually used in the image
    # q.getcolors() returns [(count, index), ...] for 'P' images
    used = q.getcolors(maxcolors=256*256) or []
    # Sort by frequency (desc) to prefer the most representative colors first
    used.sort(reverse=True, key=lambda t: t[0])
    used_indices = [idx for _, idx in used if isinstance(idx, int)]

    triples = []
    seen = set()
    for idx in used_indices:
        if 0 <= idx < n_avail:
            r = pal[3*idx + 0]
            g = pal[3*idx + 1]
            b = pal[3*idx + 2]
            t = (r, g, b)
            if t not in seen:
                triples.append(t)
                seen.add(t)
        # Stop early if we already have 254 (we’ll prune/pad later anyway)
        if len(triples) >= 254:
            break

    # Fallback: if palette/used_indices gave us nothing (very rare),
    # sample unique colors from a small downscaled RGB image
    if not triples:
        small = img_rgb.convert("RGB").resize((128, 128), Image.BILINEAR)
        # getcolors(None) can be expensive; cap at a reasonable max
        colors = small.getcolors(maxcolors=128*128) or []
        # Sort by count desc, keep top 254
        colors.sort(reverse=True, key=lambda t: t[0])
        triples = [tuple(map(int, c[1])) for c in colors[:254]]

    return triples


def dedupe_keep_order(seq):
    seen = set()
    out = []
    for c in seq:
        if c not in seen:
            out.append(c)
            seen.add(c)
    return out

def pad_to_254(colors):
    """
    Ensure exactly 254 base colors. If short, pad with neutral grays.
    """
    colors = list(colors)
    colors = colors[:254]
    if len(colors) < 254:
        existing = set(colors)
        needed = 254 - len(colors)
        step = max(1, 256 // max(needed, 1))
        added = 0
        for v in range(0, 256, step):
            cand = (v, v, v)
            if cand not in existing:
                colors.append(cand)
                existing.add(cand)
                added += 1
                if added >= needed:
                    break
    return colors

def write_gpl(name, colors_256, out_path, columns=16):
    if len(colors_256) != 256:
        raise ValueError("Expected 256 colors, got %d" % len(colors_256))
    header = [
        "GIMP Palette",
        f"Name: {name}",
        f"Columns: {int(columns)}",
        "#"
    ]
    lines = []
    for idx, (r, g, b) in enumerate(colors_256):
        lines.append(f"{r:3d} {g:3d} {b:3d}\tc{idx:03d}")
    txt = "\n".join(header + lines) + "\n"
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(txt)

def main():
    ap = argparse.ArgumentParser(description="Generate 256-color GPL palette (Civ3-friendly) without GIMP.")
    ap.add_argument("input", help="Input image (PNG/JPG/etc). Use an RGB export of your XCF.")
    ap.add_argument("output", help="Output .gpl path")
    ap.add_argument("--name", default="Civ3 Seasonal", help="Palette name inside GPL")
    ap.add_argument("--columns", type=int, default=16, help="GPL columns (cosmetic)")
    ap.add_argument("--boost-red", type=int, default=40, help="Saturation %+ for reds [-100..100]")
    ap.add_argument("--boost-yellow", type=int, default=20, help="Saturation %+ for yellows [-100..100]")
    ap.add_argument("--boost-blue", type=int, default=30, help="Saturation %+ for blues [-100..100]")
    ap.add_argument("--boost-magenta", type=int, default=30, help="Saturation %+ for magentas [-100..100]")
    ap.add_argument("--reduce-green", type=int, default=-30, help="Saturation %+ for greens [-100..100] (negative to reduce)")
    args = ap.parse_args()

    img = Image.open(args.input).convert("RGB")

    # Bias hues to favor seasonal tones
    biased = bias_hues(
        img,
        boost_red=args.boost_red,
        boost_yellow=args.boost_yellow,
        boost_blue=args.boost_blue,
        boost_magenta=args.boost_magenta,
        reduce_green=args.reduce_green
    )

    # Quantize to 254 colors
    colors = quantize_254(biased)

    # Remove reserved colors if they slipped in
    colors = [c for c in colors if c not in (RESERVED_GREEN, RESERVED_MAGENTA)]

    # Dedupe and pad to exactly 254 base entries
    colors = dedupe_keep_order(colors)
    colors = pad_to_254(colors)

    # Append reserved entries in required order
    full256 = colors + [RESERVED_GREEN, RESERVED_MAGENTA]

    # Write GPL
    write_gpl(args.name, full256, args.output, columns=args.columns)

    print(f"Wrote 256-color palette '{args.name}' to {args.output}")
    print("Last two entries are #00FF00 and #FF00FF.")

if __name__ == "__main__":
    main()
