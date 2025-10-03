#!/usr/bin/env python3
"""
make_civ3_autumn_indexed.py

Create an autumn-variant Civ 3 terrain PCX with a 256-color palette.
- Preserves separator pixels: pure magenta (#FF00FF) and pure green (#00FF00)
- Writes them as the LAST TWO palette entries (index 254 = green, 255 = magenta)
- All other pixels are quantized adaptively to 254 colors

Usage:
  python make_civ3_autumn_indexed.py <terrain.pcx> [--overall 1.0]
                                     [--grass-blend 0.5] [--forest-blend 0.9]
                                     [--grass-sat 0.90]  [--forest-sat 1.20]
                                     [--grass-val 0.96]  [--forest-val 0.95]
                                     [--forest-hue 28]   [--grass-hue 48]
                                     [--jitter 14]
                                     [--no-dither]

Knobs:
  --overall       Multiplier on ALL effects (0 = off, 1 = default, 2 = strong)
  --grass-blend   How far grass hue moves toward grass-hue target (0..1)
  --forest-blend  How far forest hue moves toward forest-hue target (0..1)
  --grass-sat     Multiplier for grass saturation
  --forest-sat    Multiplier for forest saturation
  --grass-val     Multiplier for grass value (brightness)
  --forest-val    Multiplier for forest value
  --forest-hue    Target hue for forest (degrees, ~20–35 is orange)
  --grass-hue     Target hue for grass (degrees, ~45–55 is yellow-brown)
  --jitter        ±degrees of hue noise for forest (leaf color variety)
  --no-dither     Disable Floyd–Steinberg when building the 254-color palette

Requires: Pillow (PIL). Numpy optional for speed.
"""

import argparse
import os
import sys
from PIL import Image
import math
import random

try:
    import numpy as np
except Exception:
    np = None


# ---- Fixed detection thresholds (work well with Civ3 tiles) ----
GREEN_HUE_MIN = 60
GREEN_HUE_MAX = 155
MIN_SAT_FOR_VEG = 0.17
MIN_VAL_FOR_VEG = 0.10
FOREST_SAT_THRESHOLD = 0.33
FOREST_VAL_THRESHOLD = 0.58

# ---- Utility: color math ----
def rgb_to_hsv_pixel(r, g, b):
    r_, g_, b_ = r/255.0, g/255.0, b/255.0
    mx = max(r_, g_, b_)
    mn = min(r_, g_, b_)
    diff = mx - mn
    if diff == 0:
        h = 0.0
    elif mx == r_:
        h = (60 * ((g_ - b_) / diff) + 360) % 360
    elif mx == g_:
        h = (60 * ((b_ - r_) / diff) + 120) % 360
    else:
        h = (60 * ((r_ - g_) / diff) + 240) % 360
    s = 0.0 if mx == 0 else diff / mx
    v = mx
    return h, s, v

def hsv_to_rgb_pixel(h, s, v):
    h = h % 360
    c = v * s
    x = c * (1 - abs(((h / 60.0) % 2) - 1))
    m = v - c
    if   0 <= h < 60:   rp, gp, bp = c, x, 0
    elif 60 <= h < 120: rp, gp, bp = x, c, 0
    elif 120 <= h < 180: rp, gp, bp = 0, c, x
    elif 180 <= h < 240: rp, gp, bp = 0, x, c
    elif 240 <= h < 300: rp, gp, bp = x, 0, c
    else:                rp, gp, bp = c, 0, x
    r = int(round((rp + m) * 255))
    g = int(round((gp + m) * 255))
    b = int(round((bp + m) * 255))
    return r, g, b

def blend_toward_hue(h_src, h_dst, alpha):
    d = ((h_dst - h_src + 180) % 360) - 180
    return (h_src + alpha * d) % 360

def is_separator(rgb):
    return rgb == (255, 0, 255) or rgb == (0, 255, 0)

def classify_veg(h, s, v):
    if not (GREEN_HUE_MIN <= h <= GREEN_HUE_MAX):
        return 'none'
    if s < MIN_SAT_FOR_VEG or v < MIN_VAL_FOR_VEG:
        return 'none'
    if s >= FOREST_SAT_THRESHOLD or v <= FOREST_VAL_THRESHOLD:
        return 'forest'
    return 'grass'

# ---- Color transformation (with strength controls) ----
def apply_grass(h, s, v, args):
    hb = max(0.0, min(1.0, args.grass_blend * args.overall))
    h2 = blend_toward_hue(h, args.grass_hue, hb)
    s2 = max(0.0, min(1.0, s * (args.grass_sat ** args.overall)))
    v2 = max(0.0, min(1.0, v * (args.grass_val ** args.overall)))
    return h2, s2, v2

def apply_forest(h, s, v, args, jitter_seed):
    rng = random.Random(jitter_seed)
    jitter = rng.uniform(-args.jitter, args.jitter) * args.overall
    hb = max(0.0, min(1.0, args.forest_blend * args.overall))
    target = (args.forest_hue + jitter) % 360
    h2 = blend_toward_hue(h, target, hb)
    s2 = max(0.0, min(1.0, s * (args.forest_sat ** args.overall)))
    v2 = max(0.0, min(1.0, v * (args.forest_val ** args.overall)))
    return h2, s2, v2

# ---- Processing to autumn RGB ----
def process_to_autumn_rgb(src_img, args):
    img = src_img.convert("RGBA").convert("RGB")
    w, h = img.size

    if np is None:
        px = img.load()
        for y in range(h):
            for x in range(w):
                r, g, b = px[x, y]
                if is_separator((r, g, b)):
                    continue
                hh, ss, vv = rgb_to_hsv_pixel(r, g, b)
                kind = classify_veg(hh, ss, vv)
                if kind == 'grass':
                    hh, ss, vv = apply_grass(hh, ss, vv, args)
                    px[x, y] = hsv_to_rgb_pixel(hh, ss, vv)
                elif kind == 'forest':
                    hh, ss, vv = apply_forest(hh, ss, vv, args, y*4099 + x)
                    px[x, y] = hsv_to_rgb_pixel(hh, ss, vv)
        return img

    arr = np.array(img, dtype=np.uint8)
    R = arr[..., 0].astype(np.float32)
    G = arr[..., 1].astype(np.float32)
    B = arr[..., 2].astype(np.float32)

    sep_mask = ((R == 255) & (G == 0) & (B == 255)) | ((R == 0) & (G == 255) & (B == 0))

    r = R / 255.0; g = G / 255.0; b = B / 255.0
    mx = np.maximum(np.maximum(r, g), b)
    mn = np.minimum(np.minimum(r, g), b)
    diff = mx - mn

    h = np.zeros_like(mx)
    nz = diff != 0
    mr = (mx == r) & nz
    mg = (mx == g) & nz
    mb = (mx == b) & nz
    h[mr] = (60 * ((g[mr] - b[mr]) / diff[mr]) + 360) % 360
    h[mg] = (60 * ((b[mg] - r[mg]) / diff[mg]) + 120) % 360
    h[mb] = (60 * ((r[mb] - g[mb]) / diff[mb]) + 240) % 360

    s = np.zeros_like(mx); s[mx != 0] = diff[mx != 0] / mx[mx != 0]
    v = mx

    veg = (h >= GREEN_HUE_MIN) & (h <= GREEN_HUE_MAX) & (s >= MIN_SAT_FOR_VEG) & (v >= MIN_VAL_FOR_VEG) & (~sep_mask)
    forest = veg & ((s >= FOREST_SAT_THRESHOLD) | (v <= FOREST_VAL_THRESHOLD))
    grass = veg & (~forest)

    # grass
    if np.any(grass):
        hb = max(0.0, min(1.0, args.grass_blend * args.overall))
        delta = (((args.grass_hue - h[grass] + 180) % 360) - 180)
        h[grass] = (h[grass] + hb * delta) % 360
        s[grass] = np.clip(s[grass] * (args.grass_sat ** args.overall), 0, 1)
        v[grass] = np.clip(v[grass] * (args.grass_val ** args.overall), 0, 1)

    # forest with jitter
    if np.any(forest):
        yy, xx = np.indices(h.shape)
        # deterministic “noise” in [-1,1]
        noise = np.sin(xx[forest] * 12.9898 + yy[forest] * 78.233) * 43758.5453
        noise = (noise - np.floor(noise)) * 2 - 1
        targets = (args.forest_hue + noise * args.jitter * args.overall) % 360
        hb = max(0.0, min(1.0, args.forest_blend * args.overall))
        delta = (((targets - h[forest] + 180) % 360) - 180)
        h[forest] = (h[forest] + hb * delta) % 360
        s[forest] = np.clip(s[forest] * (args.forest_sat ** args.overall), 0, 1)
        v[forest] = np.clip(v[forest] * (args.forest_val ** args.overall), 0, 1)

    # HSV -> RGB
    hh = (h % 360) / 60.0
    c = v * s
    x = c * (1 - np.abs((hh % 2) - 1))
    z = np.zeros_like(hh)

    r1 = np.select([(0<=hh)&(hh<1),(1<=hh)&(hh<2),(2<=hh)&(hh<3),(3<=hh)&(hh<4),(4<=hh)&(hh<5),(5<=hh)&(hh<6)],
                   [c, x, z, z, x, c], default=z)
    g1 = np.select([(0<=hh)&(hh<1),(1<=hh)&(hh<2),(2<=hh)&(hh<3),(3<=hh)&(hh<4),(4<=hh)&(hh<5),(5<=hh)&(hh<6)],
                   [x, c, c, x, z, z], default=z)
    b1 = np.select([(0<=hh)&(hh<1),(1<=hh)&(hh<2),(2<=hh)&(hh<3),(3<=hh)&(hh<4),(4<=hh)&(hh<5),(5<=hh)&(hh<6)],
                   [z, z, x, c, c, x], default=z)
    m = v - c
    R2 = np.clip(((r1 + m) * 255.0).round(), 0, 255).astype(np.uint8)
    G2 = np.clip(((g1 + m) * 255.0).round(), 0, 255).astype(np.uint8)
    B2 = np.clip(((b1 + m) * 255.0).round(), 0, 255).astype(np.uint8)

    out = np.stack([R2, G2, B2], axis=-1)
    # keep separators unchanged
    out[sep_mask] = arr[sep_mask]
    return Image.fromarray(out, mode="RGB")

# ---- Build final paletted PCX with last two entries = green, magenta ----
def to_indexed_with_separators(rgb_img, sep_green_mask, sep_magenta_mask, use_dither):
    """Quantize to 254 colors adaptively, then append green/magenta as palette 254/255."""
    # Prepare a copy where separators are neutralized so they don't consume palette slots.
    src = rgb_img.copy()
    px = src.load()
    w, h = src.size
    for y in range(h):
        for x in range(w):
            if sep_green_mask[y][x] or sep_magenta_mask[y][x]:
                # use nearby-ish neutral value (won't matter; we'll overwrite indices later)
                px[x, y] = (0, 0, 0)

    dither_flag = Image.FLOYDSTEINBERG if use_dither else Image.NONE
    q = src.quantize(colors=254, method=Image.MEDIANCUT, dither=dither_flag)
    q = q.convert("P")  # ensure paletted

    # Fetch current (254*3) palette and extend to 256*3
    pal = q.getpalette()[:254*3]
    # pad if PIL returned fewer than 254*3 (shouldn't, but just in case)
    while len(pal) < 254*3:
        pal.extend([0,0,0])

    # Append required last two entries: index 254=green, 255=magenta
    pal.extend([0, 255, 0])     # 254
    pal.extend([255, 0, 255])   # 255
    q.putpalette(pal)

    # Write separator pixels as indices 254/255
    q_px = q.load()
    for y in range(h):
        for x in range(w):
            if sep_green_mask[y][x]:
                q_px[x, y] = 254
            elif sep_magenta_mask[y][x]:
                q_px[x, y] = 255
    return q

def build_separator_masks(base_rgb):
    w, h = base_rgb.size
    px = base_rgb.load()
    g_mask = [[False]*w for _ in range(h)]
    m_mask = [[False]*w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            r,g,b = px[x,y]
            if (r,g,b) == (0,255,0):
                g_mask[y][x] = True
            elif (r,g,b) == (255,0,255):
                m_mask[y][x] = True
    return g_mask, m_mask

# ---- CLI ----
def parse_args():
    p = argparse.ArgumentParser(description="Convert Civ3 terrain PCX to an autumn variant (indexed, separators at last 2 palette slots).")
    p.add_argument("pcx", help="Input PCX")
    p.add_argument("--overall", type=float, default=1.0, help="Global strength multiplier (0..2)")
    # stronger defaults than before
    p.add_argument("--grass-blend", type=float, default=0.50, help="Grass hue shift amount (0..1)")
    p.add_argument("--forest-blend", type=float, default=0.90, help="Forest hue shift amount (0..1)")
    p.add_argument("--grass-sat", type=float, default=0.90, help="Grass saturation multiplier")
    p.add_argument("--forest-sat", type=float, default=1.20, help="Forest saturation multiplier")
    p.add_argument("--grass-val", type=float, default=0.96, help="Grass value (brightness) multiplier")
    p.add_argument("--forest-val", type=float, default=0.95, help="Forest value (brightness) multiplier")
    p.add_argument("--forest-hue", type=float, default=28.0, help="Target hue for forests (deg)")
    p.add_argument("--grass-hue", type=float, default=48.0, help="Target hue for grass (deg)")
    p.add_argument("--jitter", type=float, default=14.0, help="±Hue jitter for forest (deg)")
    p.add_argument("--no-dither", action="store_true", help="Disable dithering when quantizing to 254 colors")
    return p.parse_args()

def main():
    args = parse_args()

    in_path = args.pcx
    if not os.path.isfile(in_path):
        print(f"File not found: {in_path}")
        sys.exit(1)

    base = Image.open(in_path).convert("RGB")
    # record where the original separators are so we can restore them to palette slots 254/255
    sep_green_mask, sep_magenta_mask = build_separator_masks(base)

    autumn_rgb = process_to_autumn_rgb(base, args)

    paletted = to_indexed_with_separators(
        autumn_rgb,
        sep_green_mask,
        sep_magenta_mask,
        use_dither=(not args.no_dither)
    )

    out_path = os.path.splitext(in_path)[0] + "_fall.pcx"
    paletted.save(out_path, format="PCX")
    print(f"Saved: {out_path}")
    print("Palette indices: 254=#00FF00 (green), 255=#FF00FF (magenta)")

if __name__ == "__main__":
    main()
