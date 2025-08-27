#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Civ 3 day-night PCX transformer (pure Python, Pillow) — Blue Hour version
- Palette-safe transform with exact preservation of #ff00ff and #00ff00.
- Creates sibling hour folders 100..2400 (skipping noon folder).
- Adds luminance-aware cool tint + soft fog for a Civ 6–style dusky night.

Example:
  python civ3_daynight_palette_cli.py --terrain "/path/Terrain" --noon 1200 \
    --only-hour 2400 \
    --tint "#a5c8ff" --tint-max 0.35 --fog-max 0.18 --shadow-tint-pow 1.3 \
    --darken-max 0.12 --value-floor 0.08 --desat-max 0.22 \
    --hue-shift 0.15 --hue-target-deg 210 --gamma-mid 0.9
"""

import argparse
import math
import os
from dataclasses import dataclass
from typing import List, Tuple

from PIL import Image

MAGENTA = (255, 0, 255)   # ff00ff
GREEN   = (0, 255, 0)     # 00ff00


# ---------- Look controls ----------

@dataclass
class LookParams:
    # Classic controls (still used, but defaults tuned for dusk)
    darken_max: float = 0.12      # gentle dimming at midnight (0..1)
    value_floor: float = 0.08     # white-ish lift at midnight (0..1)
    desat_max: float = 0.22       # desaturation at midnight (0..1)
    hue_shift: float = 0.15       # fraction toward hue_target at midnight (0..1)
    hue_target_deg: float = 210.0 # cool blue (0..360)

    # Blue-hour controls
    tint_rgb: Tuple[float, float, float] = (0.647, 0.784, 1.000)  # #a5c8ff as 0..1
    tint_max: float = 0.35          # max weight of tint at midnight (0..1)
    fog_max: float = 0.18           # max white blend at midnight (0..1)
    shadow_tint_pow: float = 1.3    # >1 = more tint in darker pixels
    gamma_mid: float = 0.90         # <1 lifts midtones a touch

    fog_tint_mix: float = 0.70   # 0=white fog (current), 1=tinted fog (use --tint)
    tint_base: float = 0.35      # minimum tint even in highlights (0..1)

def _clamp01(x: float) -> float:
    return 0.0 if x < 0.0 else 1.0 if x > 1.0 else x

def hour_factor(hour_1_24: int) -> float:
    """0 at 12 (no change), 1 at 24/0 (midnight). Smooth cosine easing."""
    d = abs(hour_1_24 - 12)
    if d > 12:
        d = 24 - d
    return 0.5 * (1.0 - math.cos(math.pi * (d / 12.0)))

def _to_byte(x: float) -> int:
    return int(min(255, max(0, round(x))))

def _srgb_to_unit(r: int, g: int, b: int) -> Tuple[float, float, float]:
    return r/255.0, g/255.0, b/255.0

def _unit_to_srgb(rr: float, gg: float, bb: float) -> Tuple[int, int, int]:
    return _to_byte(rr*255.0), _to_byte(gg*255.0), _to_byte(bb*255.0)

def _luma_srgb(rr: float, gg: float, bb: float) -> float:
    # sRGB luma approximation
    return 0.299*rr + 0.587*gg + 0.114*bb

def _lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t

def _lerp3(c: Tuple[float, float, float], d: Tuple[float, float, float], t: float):
    return (_lerp(c[0], d[0], t), _lerp(c[1], d[1], t), _lerp(c[2], d[2], t))

def parse_hex_rgb(s: str) -> Tuple[float, float, float]:
    s = s.strip().lstrip("#")
    if len(s) != 6:
        raise ValueError("Hex color must be RRGGBB")
    r = int(s[0:2], 16)/255.0
    g = int(s[2:4], 16)/255.0
    b = int(s[4:6], 16)/255.0
    return (r, g, b)

def transform_rgb_triplet(r: int, g: int, b: int, f: float, p: LookParams) -> Tuple[int, int, int]:
    """
    Blue-hour transform:
      1) Fog blend toward white (uniform).
      2) Shadow-weighted tint toward p.tint_rgb.
      3) Gentle desaturation.
      4) Slight hue nudge toward p.hue_target_deg (to avoid greenish seas).
      5) Overall brightness floor + mild dim.
      6) Midtone gamma lift.
    """
    import colorsys

    # clamp/normalize params
    dm = _clamp01(p.darken_max)
    vf = _clamp01(p.value_floor)
    ds = _clamp01(p.desat_max)
    hs = _clamp01(p.hue_shift)
    ht = (p.hue_target_deg % 360.0) / 360.0
    tint_w = _clamp01(p.tint_max) * f
    fog_w  = _clamp01(p.fog_max)  * f
    pow_k  = max(0.2, p.shadow_tint_pow)
    gm     = max(0.01, p.gamma_mid)

    rr, gg, bb = _srgb_to_unit(r, g, b)
    y = _luma_srgb(rr, gg, bb)

    # 1) Fog (ambient lift) — now can be tinted
    if fog_w > 0.0:
        fog_color = _lerp3((1.0, 1.0, 1.0), p.tint_rgb, _clamp01(p.fog_tint_mix))
        rr, gg, bb = _lerp3((rr, gg, bb), fog_color, fog_w)

    # 2) Cool tint (more in darker pixels, but some in highlights too)
    if tint_w > 0.0:
        shadow_mix = (1.0 - y) ** pow_k
        t = tint_w * (p.tint_base + (1.0 - p.tint_base) * shadow_mix)
        rr, gg, bb = _lerp3((rr, gg, bb), p.tint_rgb, _clamp01(t))


    # 3) Gentle desaturation toward luma
    if ds > 0.0 and f > 0.0:
        y2 = _luma_srgb(rr, gg, bb)
        rr, gg, bb = _lerp3((rr, gg, bb), (y2, y2, y2), ds * f)

    # 4) Micro hue nudge to avoid green cast in seas/grass
    if hs > 0.0 and f > 0.0:
        h, s, v = colorsys.rgb_to_hsv(rr, gg, bb)
        # shortest-arc shift
        delta = ht - h
        if delta > 0.5:   delta -= 1.0
        elif delta < -0.5: delta += 1.0
        h = (h + hs*f*delta) % 1.0
        rr, gg, bb = colorsys.hsv_to_rgb(h, s, v)

    # 5) Brightness floor + gentle darken
    rr = rr * (1 - dm*f) + vf*f
    gg = gg * (1 - dm*f) + vf*f
    bb = bb * (1 - dm*f) + vf*f

    # 6) Midtone gamma lift
    if gm != 1.0:
        rr = max(0.0, min(1.0, rr)) ** gm
        gg = max(0.0, min(1.0, gg)) ** gm
        bb = max(0.0, min(1.0, bb)) ** gm

    return _unit_to_srgb(rr, gg, bb)


# ---------- Palette helpers (Pillow) ----------

def get_palette(image: Image.Image) -> List[int]:
    pal = image.getpalette() or []
    if len(pal) < 256*3:
        pal = pal + [0] * (256*3 - len(pal))
    return pal[:256*3]

def put_palette(image: Image.Image, palette: List[int]) -> None:
    if len(palette) < 256*3:
        palette = palette + [0] * (256*3 - len(palette))
    image.putpalette(palette[:256*3])

def find_color_index(palette: List[int], rgb: Tuple[int, int, int]) -> int:
    r, g, b = rgb
    for i in range(256):
        if palette[3*i] == r and palette[3*i+1] == g and palette[3*i+2] == b:
            return i
    return -1

def ensure_palette_has_colors(image: Image.Image, colors: List[Tuple[int, int, int]]) -> List[int]:
    palette = get_palette(image)
    to_add = [c for c in colors if find_color_index(palette, c) == -1]
    if not to_add:
        return palette

    hist = image.histogram()
    if image.mode != 'P' or len(hist) != 256:
        candidates = list(range(255, -1, -1))
    else:
        candidates = sorted(range(256), key=lambda i: hist[i])

    protected = set()
    for rgb in colors + [MAGENTA, GREEN]:
        idx = find_color_index(palette, rgb)
        if idx != -1:
            protected.add(idx)

    replace_slots = []
    for i in candidates:
        if i in protected:
            continue
        replace_slots.append(i)
        if len(replace_slots) >= len(to_add):
            break

    if len(replace_slots) < len(to_add):
        tail = [i for i in range(255, -1, -1) if i not in protected][:len(to_add)-len(replace_slots)]
        replace_slots.extend(tail)

    for slot, rgb in zip(replace_slots, to_add):
        palette[3*slot:3*slot+3] = list(rgb)

    return palette


# ---------- Image processing ----------

def process_indexed_pcx(in_path: str, out_path: str, hour_1_24: int, params: LookParams) -> None:
    f = hour_factor(hour_1_24)
    im = Image.open(in_path)
    if im.mode != 'P':
        im = im.convert('RGBA')
        im = im.quantize(colors=256, method=Image.MEDIANCUT, dither=Image.NONE)

    palette = get_palette(im)
    new_palette = palette.copy()

    for i in range(256):
        r, g, b = palette[3*i], palette[3*i+1], palette[3*i+2]
        if (r, g, b) == MAGENTA or (r, g, b) == GREEN:
            continue
        nr, ng, nb = transform_rgb_triplet(r, g, b, f, params)
        new_palette[3*i:3*i+3] = [nr, ng, nb]

    put_palette(im, new_palette)

    # Guarantee exact magic colors, then enforce exact indices for magic pixels
    pal_after = get_palette(im)
    idx_mag = find_color_index(pal_after, MAGENTA)
    idx_grn = find_color_index(pal_after, GREEN)

    src_rgb = Image.open(in_path).convert('RGB')
    w, h = src_rgb.size
    src_pixels = src_rgb.load()

    if idx_mag == -1 or idx_grn == -1:
        pal_after = ensure_palette_has_colors(im, [MAGENTA, GREEN])
        put_palette(im, pal_after)
        idx_mag = find_color_index(pal_after, MAGENTA)
        idx_grn = find_color_index(pal_after, GREEN)

    dst_px = im.load()  # palette indices
    for y in range(h):
        for x in range(w):
            r, g, b = src_pixels[x, y]
            if (r, g, b) == MAGENTA:
                dst_px[x, y] = idx_mag
            elif (r, g, b) == GREEN:
                dst_px[x, y] = idx_grn

    im.save(out_path, format='PCX')


def process_folder(terrain_dir: str, noon_subfolder: str,
                   params: LookParams, only_hour: int = None, only_file: str = None) -> None:
    base_dir = os.path.join(terrain_dir, noon_subfolder)
    if not os.path.isdir(base_dir):
        raise SystemExit(f"No such folder: {base_dir}")

    pcx_names = [f for f in os.listdir(base_dir) if f.lower().endswith('.pcx')]
    if not pcx_names:
        raise SystemExit(f"No PCX files found in {base_dir}")

    hours = [h for h in range(100, 2500, 100) if h != 1200]
    if only_hour is not None:
        if only_hour == 1200:
            raise SystemExit("1200 is the base (left untouched). Choose another hour.")
        if only_hour % 100 != 0 or only_hour < 100 or only_hour > 2400:
            raise SystemExit("--only-hour must be one of 100, 200, ..., 2400.")
        hours = [only_hour]

    if only_file is not None:
        pcx_names = [only_file] if only_file in pcx_names else []

    if not pcx_names:
        raise SystemExit("The requested --only-file was not found in the noon folder." if only_file else
                         "No PCX files to process.")

    for hhh in hours:
        hour_1_24 = hhh // 100
        out_dir = os.path.join(terrain_dir, f"{hhh}")
        os.makedirs(out_dir, exist_ok=True)

        for name in pcx_names:
            in_path = os.path.join(base_dir, name)
            out_path = os.path.join(out_dir, name)
            process_indexed_pcx(in_path, out_path, hour_1_24, params)

def main():
    ap = argparse.ArgumentParser(description="Civ 3 day-night PCX transformer (pure Python, Pillow)")
    ap.add_argument("--terrain", required=True, help="Path to Terrain folder (parent of noon subfolder)")
    ap.add_argument("--noon", default="1200", help="Name of noon subfolder (default: 1200)")
    ap.add_argument("--only-hour", type=int, help="Process a single hour folder (e.g., 2400)")
    ap.add_argument("--only-file", help="Process a single PCX filename from the noon folder")

    # Look params (all optional)
    ap.add_argument("--darken-max", type=float, default=0.12,
                    help="Max dimming of brightness at midnight (0..1). Default: 0.12")
    ap.add_argument("--value-floor", type=float, default=0.08,
                    help="Brightness floor added at midnight (0..1). Default: 0.08")
    ap.add_argument("--desat-max", type=float, default=0.22,
                    help="Max desaturation at midnight (0..1). Default: 0.22")
    ap.add_argument("--hue-shift", type=float, default=0.15,
                    help="Fraction of the way toward hue-target at midnight (0..1). Default: 0.15")
    ap.add_argument("--hue-target-deg", type=float, default=210.0,
                    help="Target hue in degrees (0..360). Default: 210 (dusky blue)")

    # Blue-hour controls
    ap.add_argument("--tint", type=str, default="#265db7",
                    help="Hex cool tint (RRGGBB). Default: #265db7")
    ap.add_argument("--tint-max", type=float, default=0.35,
                    help="Max tint blend at midnight (0..1). Default: 0.35")
    ap.add_argument("--fog-max", type=float, default=0.18,
                    help="Max white/ambient blend at midnight (0..1). Default: 0.18")
    ap.add_argument("--shadow-tint-pow", type=float, default=1.3,
                    help="Exponent for stronger tint in shadows. Default: 1.3")
    ap.add_argument("--gamma-mid", type=float, default=0.90,
                    help="Midtone gamma (<1 brightens mids). Default: 0.90")
    
    # Fog controls
    ap.add_argument("--fog-tint-mix", type=float, default=0.70,
                    help="0=white fog, 1=fog toward --tint. Default: 0.70")
    ap.add_argument("--tint-base", type=float, default=0.35,
                    help="Minimum tint in highlights (0..1). Default: 0.35")

    args = ap.parse_args()

    params = LookParams(
        darken_max=args.darken_max,
        value_floor=args.value_floor,
        desat_max=args.desat_max,
        hue_shift=args.hue_shift,
        hue_target_deg=args.hue_target_deg,
        tint_rgb=parse_hex_rgb(args.tint),
        tint_max=args.tint_max,
        fog_max=args.fog_max,
        shadow_tint_pow=args.shadow_tint_pow,
        gamma_mid=args.gamma_mid,
    )

    process_folder(args.terrain, args.noon, params, args.only_hour, args.only_file)


if __name__ == "__main__":
    main()

