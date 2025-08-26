#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Civ 3 day-night PCX transformer (SIMPLE, recursive; split blue controls + light-key safe)

- Root layout:
    Data/
      1200/                <-- base (noon) tree with nested subfolders & PCX
      0100/ 0200/ ...      <-- siblings created by this script (mirroring 1200)

- What it does:
  * Recursively walks Data/1200 and processes every .pcx it finds.
  * Writes results to matching paths in Data/0100..Data/2400 (except 1200).
  * Preserves #ff00ff, #00ff00, AND --light-key exactly (no grading applied).

- Controls:
    --night       (0..1)  midnight darkness amount
    --blue-water  (0..1)  ocean/shoreline blue emphasis & blue-sector boost
    --blue-land   (0..1)  land/greens cooling toward dusk blue
    --sunset      (0..1)  evening warmth around ~19:00 (7pm)
    --light-key           placeholder window color to preserve exactly

- Legacy:
    --blue        (0..1)  alias for setting both blue-water and blue-land
"""

import argparse, math, os
from typing import List, Tuple
from PIL import Image

MAGENTA = (255, 0, 255)   # ff00ff
GREEN   = (0, 255, 0)     # 00ff00

# --------- helpers ---------
def force_magenta_at_255(im: Image.Image) -> int:
    """
    Ensure palette index 255 is #ff00ff (MAGENTA).
    Returns replacement_idx: if we had to move the old 255-color somewhere,
    this is the index where it now lives; else returns 255 when no swap was needed.
    """
    pal = get_palette(im)
    # current color stored at slot 255 (before we change it)
    old255 = (pal[3*255], pal[3*255+1], pal[3*255+2])

    # where is magenta now?
    idx_mag = find_color_index(pal, MAGENTA)
    if idx_mag == -1:
        # make sure magenta exists at all
        pal = ensure_palette_has_colors(im, [MAGENTA])
        put_palette(im, pal)
        pal = get_palette(im)
        idx_mag = find_color_index(pal, MAGENTA)

    # If magenta already at 255, done
    if idx_mag == 255:
        return 255

    # Otherwise: swap the RGB triplets (put magenta at 255, move old-255 color to idx_mag)
    pal[3*255:3*255+3] = list(MAGENTA)
    pal[3*idx_mag:3*idx_mag+3] = list(old255)
    put_palette(im, pal)
    return idx_mag  # non-255 index where the old 255-color now lives

def parse_rgb_hex_or_csv(s: str) -> Tuple[int,int,int]:
    s = s.strip()
    if s.startswith("#"):
        s = s[1:]
        if len(s) != 6:
            raise ValueError("Hex color must be #rrggbb")
        return (int(s[0:2],16), int(s[2:4],16), int(s[4:6],16))
    parts = [int(p) for p in s.split(",")]
    if len(parts) != 3:
        raise ValueError("RGB must be R,G,B")
    return tuple(max(0,min(255,v)) for v in parts)  # type: ignore

def hour_factor(hour_1_24: int) -> float:
    """0 at 12 (no change), 1 at 24/0 (midnight). Smooth cosine easing."""
    d = abs(hour_1_24 - 12)
    if d > 12: d = 24 - d
    return 0.5 * (1.0 - math.cos(math.pi * (d / 12.0)))

def evening_weight(hour_1_24: int, center: float = 19.0, width: float = 3.0) -> float:
    """Cosine window centered at `center`, 1 at center, 0 at center±width."""
    d = abs(hour_1_24 - center)
    d = min(d, 24.0 - d)
    if d >= width or width <= 0: return 0.0
    t = d / width
    return 0.5 * (1.0 + math.cos(math.pi * t))

def get_palette(image: Image.Image) -> List[int]:
    pal = image.getpalette() or []
    if len(pal) < 256*3: pal += [0] * (256*3 - len(pal))
    return pal[:256*3]

def put_palette(image: Image.Image, palette: List[int]) -> None:
    if len(palette) < 256*3: palette += [0] * (256*3 - len(palette))
    image.putpalette(palette[:256*3])

def find_color_index(palette: List[int], rgb: Tuple[int,int,int]) -> int:
    r,g,b = rgb
    for i in range(256):
        if palette[3*i]==r and palette[3*i+1]==g and palette[3*i+2]==b:
            return i
    return -1

def ensure_palette_has_colors(image: Image.Image, colors: List[Tuple[int,int,int]]) -> List[int]:
    """Ensure given colors exist in the palette; replace least-used slots if needed."""
    palette = get_palette(image)
    to_add = [c for c in colors if find_color_index(palette, c) == -1]
    if not to_add: return palette

    hist = image.histogram() if image.mode == 'P' else None
    if hist is None or len(hist) != 256:
        candidates = list(range(255, -1, -1))
    else:
        candidates = sorted(range(256), key=lambda i: hist[i])

    # Protect requested colors + Civ3 magenta/green
    protected = set()
    for rgb in colors + [MAGENTA, GREEN]:
        idx = find_color_index(palette, rgb)
        if idx != -1: protected.add(idx)

    replace_slots = []
    for i in candidates:
        if i in protected: continue
        replace_slots.append(i)
        if len(replace_slots) >= len(to_add): break
    if len(replace_slots) < len(to_add):
        tail = [i for i in range(255, -1, -1) if i not in protected][:len(to_add)-len(replace_slots)]
        replace_slots.extend(tail)

    for slot, rgb in zip(replace_slots, to_add):
        palette[3*slot:3*slot+3] = list(rgb)
    return palette

def _hue_in_range(h01: float, start_deg: float, end_deg: float) -> bool:
    a = (start_deg % 360.0) / 360.0
    b = (end_deg   % 360.0) / 360.0
    return (a <= b and a <= h01 <= b) or (a > b and (h01 >= a or h01 <= b))

# --------- color grading (simple) ---------

def transform_rgb_triplet(
    r: int, g: int, b: int,
    hour_1_24: int,
    night: float,
    blue_water: float,
    blue_land: float,
    sunset: float
) -> Tuple[int,int,int]:
    import colorsys

    # clamp controls
    night      = max(0.0, min(1.0, night))
    blue_water = max(0.0, min(1.0, blue_water))
    blue_land  = max(0.0, min(1.0, blue_land))
    sunset     = max(0.0, min(1.0, sunset))

    # global curves
    f  = hour_factor(hour_1_24)                  # 0 at noon, 1 at midnight
    ew = evening_weight(hour_1_24, 19.0, 3.0)    # 0..1 around ~7pm

    h0, s0, v0 = colorsys.rgb_to_hsv(r/255.0, g/255.0, b/255.0)
    h,  s,  v  = h0, s0, v0

    # Precompute loose categories from original hue
    is_greenish = _hue_in_range(h0, 95, 140) and s0 > 0.18
    is_waterish = (_hue_in_range(h0, 140, 215) or _hue_in_range(h0, 190, 260)) and s0 > 0.12

    # 1) DARKNESS: deeper nights + gentle gamma for midtone roll-off
    max_darken = 0.78
    floor_at_midnight = 0.02
    v = v * (1 - (max_darken*night)*f) + (floor_at_midnight*night)*f
    v = pow(max(0.0, min(1.0, v)), 1.0 + 0.25*night*f)

    # 2) Mild global cool-down for non-warm hues (sky/water/greens)
    warm_min, warm_max = 10/360.0, 75/360.0
    is_warm = warm_min <= h <= warm_max
    if not is_warm and s > 0.20 and v > 0.20:
        target_cool = 220/360.0
        delta = target_cool - h
        if delta > 0.5: delta -= 1.0
        elif delta < -0.5: delta += 1.0
        local_blue = blue_water if is_waterish else (blue_land if is_greenish else 0.5*(blue_water + blue_land))
        h = (h + (0.40*local_blue)*f * delta) % 1.0

    # 3) WATER CONTINUUM (teal↔ocean), blended (uses blue_water)
    if _hue_in_range(h0, 140, 215) and s0 > 0.12:
        h0deg = h0 * 360.0
        t_lin = max(0.0, min(1.0, (h0deg - 140.0) / (215.0 - 140.0)))
        t = t_lin * t_lin * (3 - 2*t_lin)  # smoothstep
        target = ((232.0/360.0) * (1 - t)) + ((238.0/360.0) * t)
        pull   = ((0.55)          * (1 - t)) + ((0.95)          * t)
        satb   = ((0.35)          * (1 - t)) + ((0.60)          * t)
        d = target - h
        if d > 0.5: d -= 1.0
        elif d < -0.5: d += 1.0
        h = (h + (pull * blue_water * f) * d) % 1.0
        s = s + (1.0 - s) * (satb * blue_water * f)

    # 4) GREEN LAND → cooler/blue (uses blue_land)
    if is_greenish:
        green_target = 232/360.0
        d3 = green_target - h
        if d3 > 0.5: d3 -= 1.0
        elif d3 < -0.5: d3 += 1.0
        h = (h + (0.95*blue_land)*(f**1.20) * d3) % 1.0
        s = s + (1.0 - s) * (0.40*blue_land)*(f**1.10)

    # 5) Blue-sector chroma boost (after shifts) — ocean/sky (uses blue_water)
    if _hue_in_range(h, 190, 260):
        s = s + (1.0 - s) * (0.35*blue_water)*f

    # 6) Sunset warmth (evenings only), skip warm hues
    if ew > 0 and not is_warm:
        target_warm = 15/360.0
        d4 = target_warm - h
        if d4 > 0.5: d4 -= 1.0
        elif d4 < -0.5: d4 += 1.0
        h = (h + (0.20*sunset) * ew * d4) % 1.0
        s = s + (1.0 - s) * (0.05*sunset) * ew

    # clamp & back to RGB
    s = max(0.0, min(1.0, s))
    v = max(0.0, min(1.0, v))
    rr, gg, bb = colorsys.hsv_to_rgb(h, s, v)
    return int(round(rr*255.0)), int(round(gg*255.0)), int(round(bb*255.0))

# --------- processing ---------

def process_indexed_pcx(
    in_path: str,
    out_path: str,
    hour_1_24: int,
    night: float,
    blue_water: float,
    blue_land: float,
    sunset: float,
    light_key: Tuple[int,int,int]
) -> None:
    print(f"Processing {in_path} → {out_path} [hour {hour_1_24}]")
    im = Image.open(in_path)
    # quantize to palette if needed
    if im.mode != 'P':
        try:
            dnone = Image.Dither.NONE
        except Exception:
            dnone = getattr(Image, "NONE", 0)
        im = im.convert('RGBA').quantize(colors=256, method=Image.MEDIANCUT, dither=dnone)

    palette = get_palette(im)
    new_palette = palette.copy()

    # remap palette (skip magenta/green/light_key)
    for i in range(256):
        r,g,b = palette[3*i], palette[3*i+1], palette[3*i+2]
        if (r,g,b) == MAGENTA or (r,g,b) == GREEN or (r,g,b) == light_key:
            continue
        nr,ng,nb = transform_rgb_triplet(r,g,b, hour_1_24, night, blue_water, blue_land, sunset)
        new_palette[3*i:3*i+3] = [nr,ng,nb]

    put_palette(im, new_palette)

    # ensure magic + light_key exist & pixels point to them exactly
    pal_after = get_palette(im)

    # Ensure magic + light_key exist
    need = []
    if find_color_index(pal_after, MAGENTA) == -1: need.append(MAGENTA)
    if find_color_index(pal_after, GREEN)   == -1: need.append(GREEN)
    if find_color_index(pal_after, light_key) == -1: need.append(light_key)
    if need:
        pal_after = ensure_palette_has_colors(im, need)
        put_palette(im, pal_after)
        pal_after = get_palette(im)

    # Pin magenta to slot 255 and remember where the old-255 color moved to
    replacement_idx = force_magenta_at_255(im)  # 255 if no swap, else index containing the old 255-color

    # Refresh indices after palette edits
    pal_after = get_palette(im)
    idx_mag = 255  # by construction
    idx_grn = find_color_index(pal_after, GREEN)
    idx_lit = find_color_index(pal_after, light_key)

    # Remap pixels. Also reroute any non-magenta pixels that still carry index 255.
    src_rgb = Image.open(in_path).convert('RGB')
    w,h = src_rgb.size
    src_px = src_rgb.load()
    dst_px = im.load()

    for y in range(h):
        for x in range(w):
            r,g,b = src_px[x,y]
            cur = dst_px[x,y]
            if (r,g,b) == MAGENTA:
                dst_px[x,y] = 255
            elif (r,g,b) == GREEN and idx_grn != -1:
                dst_px[x,y] = idx_grn
            elif (r,g,b) == light_key and idx_lit != -1:
                dst_px[x,y] = idx_lit
            elif cur == 255 and replacement_idx != 255:
                # Pixel was using old slot 255 for some other color; move it to the replacement slot
                dst_px[x,y] = replacement_idx


    # ensure out dir
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    im.save(out_path, format='PCX')

def process_tree(
    data_dir: str,
    noon_subfolder: str,
    night: float,
    blue_water: float,
    blue_land: float,
    sunset: float,
    only_hour: int = None,
    only_file: str = None,
    light_key: Tuple[int,int,int] = (255,0,254)
) -> None:

    base_dir = os.path.join(data_dir, noon_subfolder)
    if not os.path.isdir(base_dir): raise SystemExit(f"No such folder: {base_dir}")

    # Prepare hour list
    hours = [h for h in range(100, 2500, 100) if h != 1200]
    if only_hour is not None:
        if only_hour == 1200: raise SystemExit("1200 is the base (left untouched). Choose another hour.")
        if only_hour % 100 != 0 or only_hour < 100 or only_hour > 2400:
            raise SystemExit("--only-hour must be one of 100, 200, ..., 2400.")
        hours = [only_hour]

    # Normalize only_file (relative path within noon), or allow bare filename match
    norm_only = os.path.normcase(only_file) if only_file else None

    for dirpath, dirnames, filenames in os.walk(base_dir):
        rel_dir = os.path.relpath(dirpath, base_dir)
        rel_dir = "" if rel_dir == "." else rel_dir

        # Mirror directory structure for each hour
        for hhh in hours:
            out_dir = os.path.join(data_dir, f"{hhh:04d}", rel_dir)
            os.makedirs(out_dir, exist_ok=True)

        for name in filenames:
            if not name.lower().endswith(".pcx"):
                continue

            # only-file filtering (match by bare filename OR by relative path)
            if norm_only:
                rel_file = os.path.normcase(os.path.join(rel_dir, name))
                if norm_only != os.path.normcase(name) and norm_only != rel_file:
                    continue

            in_path = os.path.join(dirpath, name)
            for hhh in hours:
                hour_1_24 = hhh // 100
                out_path = os.path.join(data_dir, f"{hhh:04d}", rel_dir, name)
                process_indexed_pcx(
                    in_path, out_path, hour_1_24, night, blue_water, blue_land, sunset, light_key
                )

def main():
    ap = argparse.ArgumentParser(description="Civ 3 day-night (recursive; split blue controls; light-key safe)")
    ap.add_argument("--data", required=True, help="Path to Data root (which contains the noon subfolder, e.g., Data)")
    ap.add_argument("--noon", default="1200", help="Name of noon subfolder under --data (default: 1200)")
    ap.add_argument("--only-hour", type=int, help="Process a single hour (e.g., 2400)")
    ap.add_argument("--only-file", help="Process a single PCX filename or relative path within the noon tree")
    ap.add_argument("--night",  type=float, default=0.60, help="Midnight darkness strength (0..1). Default: 0.60")
    # Split controls
    ap.add_argument("--blue-water", type=float, default=0.85, help="Blue emphasis for ocean/shoreline (0..1). Default: 0.85")
    ap.add_argument("--blue-land",  type=float, default=0.85, help="Blue emphasis for land/greens (0..1). Default: 0.85")
    # Legacy alias (optional)
    ap.add_argument("--blue", type=float, default=None, help="[Deprecated] Sets both --blue-water and --blue-land if they are not provided.")
    ap.add_argument("--sunset", type=float, default=0.12, help="Evening warmth at ~19:00 (0..1). Default: 0.12")
    ap.add_argument("--light-key", type=str, default="#ff00fe",
                    help="Placeholder window color to preserve (R,G,B or #rrggbb). Default: #ff00fe")

    args = ap.parse_args()

    # Resolve split/legacy blue flags
    bw = args.blue_water
    bl = args.blue_land
    if args.blue is not None:
        if bw == ap.get_default("blue_water"): bw = args.blue
        if bl == ap.get_default("blue_land"):  bl = args.blue
        print("Note: --blue is deprecated; prefer --blue-water and --blue-land.")

    light_key = parse_rgb_hex_or_csv(args.light_key)

    process_tree(args.data, args.noon, args.night, bw, bl, args.sunset,
                 args.only_hour, args.only_file, light_key)

if __name__ == "__main__":
    main()
