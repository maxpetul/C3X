#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Civ 3 day-night PCX transformer (SIMPLE, recursive; refined land-to-blue at night)
- Recursively walks Data/1200 (or provided noon folder) and writes siblings 0100..2400.
- Preserves #ff00ff and ALL provided --light-key colors exactly (index-stable).
- GREEN (#00ff00) pixels are written out as MAGENTA at index 255 (so green never leaks).
- Blue split: --blue-water and --blue-land.
- Stronger, clearer sunset warmth (water + vegetation), mountains/deserts protected.
"""

import argparse, math, os
from typing import List, Tuple, Dict
from PIL import Image
import colorsys

MAGENTA = (255, 0, 255)   # ff00ff
GREEN   = (0, 255, 0)     # 00ff00

# --------- helpers ---------

def parse_rgb_one(s: str) -> Tuple[int,int,int]:
    s = s.strip()
    if s.startswith("#"):
        s = s[1:]
        if len(s) != 6: raise ValueError("Hex color must be #rrggbb")
        return (int(s[0:2],16), int(s[2:4],16), int(s[4:6],16))
    parts = [int(p) for p in s.split(",")]
    if len(parts) != 3: raise ValueError("RGB must be R,G,B")
    return tuple(max(0,min(255,v)) for v in parts)  # type: ignore

def parse_rgb_list(values: List[str]) -> List[Tuple[int,int,int]]:
    out: List[Tuple[int,int,int]] = []
    for raw in values or []:
        tokens = [t for t in raw.replace(";", " ").split() if t]
        for tok in tokens:
            out.append(parse_rgb_one(tok))
    if not out:
        out = [(0,254,255)]  # sensible default: #00feff
    dedup: List[Tuple[int,int,int]] = []
    seen = set()
    for c in out:
        if c not in seen:
            seen.add(c); dedup.append(c)
    return dedup

def hour_factor(hour_1_24: int) -> float:
    """0 at 12 (no change), 1 at 24/0 (midnight). Smooth cosine easing."""
    d = abs(hour_1_24 - 12)
    if d > 12: d = 24 - d
    return 0.5 * (1.0 - math.cos(math.pi * (d / 12.0)))

def evening_weight(hour_1_24: int, center: float = 19.0, width: float = 3.5) -> float:
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
    palette = get_palette(image)
    to_add = [c for c in colors if find_color_index(palette, c) == -1]
    if not to_add: return palette
    hist = image.histogram() if image.mode == 'P' else None
    candidates = sorted(range(256), key=(lambda i: hist[i])) if (hist and len(hist)==256) else list(range(255, -1, -1))
    # protect magic + wanted colors
    protected = set()
    for rgb in colors + [MAGENTA]:
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

def quantize_to_p_256(img: Image.Image) -> Image.Image:
    """Convert to RGB then quantize with FASTOCTREE (works on RGB/RGBA)."""
    try:
        dnone = Image.Dither.NONE
    except Exception:
        dnone = getattr(Image, "NONE", 0)
    img_rgb = img.convert('RGB')
    try:
        return img_rgb.quantize(colors=256, method=Image.FASTOCTREE, dither=dnone)
    except Exception:
        return img_rgb.quantize(colors=256, dither=dnone)

# --------- palette pinning for magenta & multiple light-keys ---------

def force_magenta_at_255(im: Image.Image) -> int:
    """
    Ensure palette index 255 is #ff00ff (MAGENTA).
    Returns replacement_idx: where the old slot-255 color now lives (or 255 if none).
    """
    pal = get_palette(im)
    old255 = (pal[3*255], pal[3*255+1], pal[3*255+2])
    idx_mag = find_color_index(pal, MAGENTA)
    if idx_mag == -1:
        pal = ensure_palette_has_colors(im, [MAGENTA])
        put_palette(im, pal)
        pal = get_palette(im)
        idx_mag = find_color_index(pal, MAGENTA)
    if idx_mag == 255:
        return 255
    pal[3*255:3*255+3] = list(MAGENTA)
    pal[3*idx_mag:3*idx_mag+3] = list(old255)
    put_palette(im, pal)
    return idx_mag

def choose_fallback_slot(im: Image.Image, reserved: set) -> int:
    """Pick a slot from high to low avoiding reserved (255, etc.)."""
    for i in range(254, -1, -1):
        if i not in reserved:
            return i
    return 254

def pin_lightkeys_to_source_indices(im: Image.Image,
                                    src_palette: List[int],
                                    light_keys: List[Tuple[int,int,int]]
                                   ) -> Tuple[Dict[Tuple[int,int,int],int], Dict[int,int]]:
    """
    Try to keep each light-key at the SAME index as in the source palette.
    Returns:
      lk_to_target_index: mapping rgb -> desired target index
      swap_partner_for_target: mapping target_index -> partner_index (if we swapped)
    """
    # ensure keys exist
    pal = ensure_palette_has_colors(im, light_keys)
    put_palette(im, pal)
    pal = get_palette(im)

    reserved = {255}
    lk_to_target: Dict[Tuple[int,int,int], int] = {}
    for rgb in light_keys:
        src_idx = find_color_index(src_palette, rgb)
        if src_idx == -1 or src_idx == 255:
            t = choose_fallback_slot(im, reserved)
        else:
            t = src_idx
        lk_to_target[rgb] = t
        reserved.add(t)

    # perform swaps if needed
    swap_map: Dict[int,int] = {}  # target -> partner
    pal = get_palette(im)
    for rgb, target in lk_to_target.items():
        cur = find_color_index(pal, rgb)
        if cur == -1 or cur == target:
            continue
        rT,gT,bT = pal[3*target], pal[3*target+1], pal[3*target+2]
        rC,gC,bC = pal[3*cur],    pal[3*cur+1],    pal[3*cur+2]
        pal[3*target:3*target+3] = [rC,gC,bC]
        pal[3*cur:3*cur+3]       = [rT,gT,bT]
        put_palette(im, pal)
        pal = get_palette(im)
        swap_map[target] = cur

    return lk_to_target, swap_map

# --------- refined color grading ---------

def _wrap_delta(a: float, b: float) -> float:
    """Shortest signed delta on a unit hue circle (both in [0,1))."""
    d = (b - a) % 1.0
    if d > 0.5: d -= 1.0
    return d

def transform_rgb_triplet(
    r: int, g: int, b: int,
    hour_1_24: int,
    night: float,
    blue_water: float,
    blue_land: float,
    sunset: float
) -> Tuple[int,int,int]:
    """
    Oceans: as before (you said they look good).
    Vegetation (55°–140°): now hue-lerps strongly toward ~230° at night, so grass/tundra read BLUE.
    Mountains/deserts (≈20°–55°) remain protected from purple.
    """
    # clamp
    night      = max(0.0, min(1.0, night))
    blue_water = max(0.0, min(1.0, blue_water))
    blue_land  = max(0.0, min(1.0, blue_land))
    sunset     = max(0.0, min(1.0, sunset))

    f  = hour_factor(hour_1_24)               # 0..1
    ew = evening_weight(hour_1_24, 19.0, 3.5) # 0..1

    h0, s0, v0 = colorsys.rgb_to_hsv(r/255.0, g/255.0, b/255.0)
    h,  s,  v  = h0, s0, v0
    hdeg = h0 * 360.0

    # Darkness (same as before)
    max_darken = 0.78
    floor_mid  = 0.02
    v = v * (1 - (max_darken*night)*f) + (floor_mid*night)*f
    v = pow(max(0.0, min(1.0, v)), 1.0 + 0.25*night*f)

    # Categories
    warm_min, warm_max = 20/360.0, 55/360.0   # desert/mountain protection
    is_warm_narrow = (warm_min <= h <= warm_max)

    is_waterish = (130.0 <= hdeg <= 190.0) and (s0 > 0.12)
    is_veg_yellowgreen = (55.0 <= hdeg < 95.0)  and (s0 > 0.12)
    is_veg_green       = (95.0 <= hdeg <= 140.0) and (s0 > 0.12)
    is_vegetation      = is_veg_yellowgreen or is_veg_green

    # Mild global cool-down (unchanged, but weaker baseline for non-veg)
    if not is_warm_narrow and s > 0.20 and v > 0.20:
        target_cool = 220/360.0
        d = _wrap_delta(h, target_cool)
        local_blue = blue_water if is_waterish else (blue_land if is_vegetation else 0.25*(blue_water + blue_land))
        h = (h + (0.35*local_blue)*f * d) % 1.0

    # WATER: keep your “good” behavior (slight tweaks retained)
    if is_waterish:
        t_lin = max(0.0, min(1.0, (hdeg - 140.0) / (190.0 - 140.0)))
        t = t_lin * t_lin * (3 - 2*t_lin)  # smoothstep
        target = ((230.0/360.0) * (1 - t)) + ((238.0/360.0) * t)
        pull   = ((0.55)          * (1 - t)) + ((0.95)          * t)
        satb   = ((0.30)          * (1 - t)) + ((0.58)          * t)
        d = _wrap_delta(h, target)
        h = (h + (pull * blue_water * f) * d) % 1.0
        s = s + (1.0 - s) * (satb * blue_water * f)

    # LAND: push vegetation decisively toward blue at night (hue-lerp)
    # We compute a night-scaled lerp amount with a small baseline so even
    # blue_land~0.3 shows clear blue at midnight.
    if is_vegetation:
        # Target hue: slightly variable so green (95–140) goes ~232°, yellow-green (55–95) ~226°
        if is_veg_green:
            target = 232/360.0
        else:
            target = 226/360.0

        # Lerp strength: baseline + user control, all scaled by night factor.
        # Midnight w/ blue_land=1.0 -> ~0.80; with 0.3 -> ~0.55; with 0.1 -> ~0.45
        base_push = 0.30
        user_push = 0.50 * blue_land
        amt = f * (base_push + user_push)

        # Apply hue lerp on the circle
        d = _wrap_delta(h, target)
        h = (h + amt * d) % 1.0

        # Ensure it reads as BLUE not teal: add sat boost in blue sector
        if _hue_in_range(h, 190, 260):
            s = s + (1.0 - s) * (0.30 + 0.25*blue_land) * f

        # Slight value roll-off to avoid neon grass
        v = v * (1.0 - 0.06 * f * (0.5 + 0.5*blue_land))

    # Extra blue-sector pop (common for ocean/sky/now-blue land)
    if _hue_in_range(h, 200, 255):
        s = s + (1.0 - s) * (0.06 + 0.24*blue_water) * f

    # Sunset warmth (visible now): water strongest, veg moderate, deserts/mountains skipped
    if ew > 0:
        if is_waterish:
            target_warm = 22/360.0
            dW = _wrap_delta(h, target_warm)
            h = (h + (0.40*sunset) * ew * dW) % 1.0
            s = s + (1.0 - s) * (0.10*sunset) * ew
            v = v + (1.0 - v) * (0.04*sunset) * ew
        elif is_vegetation and not is_warm_narrow:
            target_warm = 28/360.0
            dL = _wrap_delta(h, target_warm)
            h = (h + (0.20*sunset) * ew * dL) % 1.0
            s = s + (1.0 - s) * (0.06*sunset) * ew

    # clamp & back to RGB
    s = max(0.0, min(1.0, s))
    v = max(0.0, min(1.0, v))
    rr, gg, bb = colorsys.hsv_to_rgb(h, s, v)
    return int(round(rr*255.0)), int(round(gg*255.0)), int(round(bb*255.0))

# --------- processing ---------

def process_indexed_pcx(in_path: str, out_path: str, hour_1_24: int,
                        night: float, blue_water: float, blue_land: float,
                        sunset: float, light_keys: List[Tuple[int,int,int]]) -> None:
    print(f"Processing {in_path} → {out_path} [hour {hour_1_24}]")
    # Load source (for palette indices) and working image
    src_img = Image.open(in_path)
    src_pal = get_palette(src_img if src_img.mode == 'P' else src_img.convert('P'))

    im = Image.open(in_path)
    if im.mode != 'P':
        im = quantize_to_p_256(im)  # robust palettization

    palette = get_palette(im)
    new_palette = palette.copy()

    # remap palette (skip magenta + all light-keys)
    skip = set([MAGENTA] + light_keys)
    for i in range(256):
        r,g,b = palette[3*i], palette[3*i+1], palette[3*i+2]
        if (r,g,b) in skip:
            continue
        nr,ng,nb = transform_rgb_triplet(r,g,b, hour_1_24, night, blue_water, blue_land, sunset)
        new_palette[3*i:3*i+3] = [nr,ng,nb]

    put_palette(im, new_palette)

    # ensure magic + keys exist
    pal_after = get_palette(im)
    need = []
    if find_color_index(pal_after, MAGENTA) == -1: need.append(MAGENTA)
    for lk in light_keys:
        if find_color_index(pal_after, lk) == -1: need.append(lk)
    if need:
        pal_after = ensure_palette_has_colors(im, need)
        put_palette(im, pal_after)

    # pin magenta at 255
    replacement_idx_255 = force_magenta_at_255(im)

    # pin each light-key to the same index as source (when possible)
    lk_to_target, swap_partner = pin_lightkeys_to_source_indices(im, src_pal, light_keys)

    # Apply pixel remap: magic to 255, GREEN->255, keys to their pinned slots
    src_rgb = Image.open(in_path).convert('RGB')
    w,h = src_rgb.size
    src_px = src_rgb.load()
    dst_px = im.load()

    lk_index_map: Dict[Tuple[int,int,int], int] = {rgb: tgt for rgb, tgt in lk_to_target.items()}

    for y in range(h):
        for x in range(w):
            r,g,b = src_px[x,y]
            cur = dst_px[x,y]
            if (r,g,b) == MAGENTA or (r,g,b) == GREEN:
                dst_px[x,y] = 255
            elif (r,g,b) in lk_index_map:
                dst_px[x,y] = lk_index_map[(r,g,b)]
            else:
                if cur in swap_partner:
                    dst_px[x,y] = swap_partner[cur]
                elif cur == 255 and replacement_idx_255 != 255:
                    dst_px[x,y] = replacement_idx_255

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    im.save(out_path, format='PCX')

def process_tree(data_dir:str, noon_subfolder:str, night:float, blue_water:float, blue_land:float,
                 sunset:float, only_hour:int=None, only_file:str=None,
                 light_keys: List[Tuple[int,int,int]] = [(0,254,255)]) -> None:
    base_dir = os.path.join(data_dir, noon_subfolder)
    if not os.path.isdir(base_dir): raise SystemExit(f"No such folder: {base_dir}")

    hours = [h for h in range(100, 2500, 100) if h != 1200]
    if only_hour is not None:
        if only_hour == 1200: raise SystemExit("1200 is the base (left untouched). Choose another hour.")
        if only_hour % 100 != 0 or only_hour < 100 or only_hour > 2400:
            raise SystemExit("--only-hour must be one of 100, 200, ..., 2400.")
        hours = [only_hour]

    norm_only = os.path.normcase(only_file) if only_file else None

    for dirpath, _, filenames in os.walk(base_dir):
        rel_dir = os.path.relpath(dirpath, base_dir)
        rel_dir = "" if rel_dir == "." else rel_dir

        for hhh in hours:
            out_dir = os.path.join(data_dir, f"{hhh:04d}", rel_dir)
            os.makedirs(out_dir, exist_ok=True)

        for name in filenames:
            if not name.lower().endswith(".pcx"):
                continue
            if norm_only:
                rel_file = os.path.normcase(os.path.join(rel_dir, name))
                if norm_only != os.path.normcase(name) and norm_only != rel_file:
                    continue

            in_path = os.path.join(dirpath, name)
            for hhh in hours:
                hour_1_24 = hhh // 100
                out_path = os.path.join(data_dir, f"{hhh:04d}", rel_dir, name)
                process_indexed_pcx(
                    in_path, out_path, hour_1_24, night, blue_water, blue_land, sunset, light_keys
                )

def main():
    ap = argparse.ArgumentParser(description="Civ 3 day-night (recursive; bluer land at night; multi light-keys)")
    ap.add_argument("--data", required=True, help="Path to Data root (contains the noon subfolder)")
    ap.add_argument("--noon", default="1200", help="Name of noon subfolder under --data (default: 1200)")
    ap.add_argument("--only-hour", type=int, help="Process a single hour (e.g., 2400)")
    ap.add_argument("--only-file", help="Process a single PCX filename or relative path")
    ap.add_argument("--night",  type=float, default=0.60, help="Midnight darkness strength (0..1). Default: 0.60")
    ap.add_argument("--blue-water", type=float, default=0.85, help="Blue emphasis for ocean/shoreline (0..1). Default: 0.85")
    ap.add_argument("--blue-land",  type=float, default=0.85, help="Blue emphasis for land/greens (0..1). Default: 0.85")
    ap.add_argument("--blue", type=float, default=None, help="[Deprecated] Sets both --blue-water and --blue-land if they are not provided.")
    ap.add_argument("--sunset", type=float, default=0.20, help="Evening warmth at ~19:00 (0..1). Default: 0.20 (stronger)")
    ap.add_argument("--light-key", action="append", default=["#00feff"],
                    help="Placeholder color(s) to preserve. Repeat flag for multiple. Accepts #rrggbb or R,G,B.")

    args = ap.parse_args()

    bw = args.blue_water
    bl = args.blue_land
    if args.blue is not None:
        if bw == ap.get_default("blue_water"): bw = args.blue
        if bl == ap.get_default("blue_land"):  bl = args.blue
        print("Note: --blue is deprecated; prefer --blue-water and --blue-land.")

    light_keys = parse_rgb_list(args.light_key)

    process_tree(args.data, args.noon, args.night, bw, bl, args.sunset,
                 args.only_hour, args.only_file, light_keys)

if __name__ == "__main__":
    main()
