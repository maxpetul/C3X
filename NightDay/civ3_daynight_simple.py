#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Civ 3 day-night PCX transformer (SIMPLE, recursive; split blue controls + multiple light-keys)

- Recursively walks Data/1200 (or provided noon folder) and writes siblings 0100..2400.
- Preserves #ff00ff (MAGENTA) and ALL provided --light-key colors exactly.
- NEW: Converts GREEN (#00ff00) pixels to MAGENTA and pins MAGENTA to palette index 255.
  (GREEN remains usable for detection but is not used in outputs.)
- Pins #ff00ff to palette index 255 and tries to keep each light-key at the SAME
  palette index as in the source PCX (when possible), for index-stability.
- Blue is split: --blue-water and --blue-land.
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
        out = [(0,254,255)]  # default #00feff
    dedup: List[Tuple[int,int,int]] = []
    seen = set()
    for c in out:
        if c not in seen:
            seen.add(c); dedup.append(c)
    return dedup

def hour_factor(hour_1_24: int) -> float:
    d = abs(hour_1_24 - 12)
    if d > 12: d = 24 - d
    return 0.5 * (1.0 - math.cos(math.pi * (d / 12.0)))

def evening_weight(hour_1_24: int, center: float = 19.0, width: float = 3.0) -> float:
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
    try:
        dnone = Image.Dither.NONE
    except Exception:
        dnone = getattr(Image, "NONE", 0)
    rgb = img.convert('RGB')
    try:
        return rgb.quantize(colors=256, method=Image.FASTOCTREE, dither=dnone)
    except Exception:
        return rgb.quantize(colors=256, dither=dnone)

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
        pal = ensure_palette_has_colors(im, [MAGENTA]); put_palette(im, pal)
        pal = get_palette(im); idx_mag = find_color_index(pal, MAGENTA)
    if idx_mag == 255: return 255
    pal[3*255:3*255+3] = list(MAGENTA)
    pal[3*idx_mag:3*idx_mag+3] = list(old255)
    put_palette(im, pal)
    return idx_mag

def choose_fallback_slot(im: Image.Image, reserved: set) -> int:
    for i in range(254, -1, -1):
        if i not in reserved: return i
    return 254

def pin_lightkeys_to_source_indices(im: Image.Image,
                                    src_palette: List[int],
                                    light_keys: List[Tuple[int,int,int]]
                                   ) -> Tuple[Dict[Tuple[int,int,int],int], Dict[int,int]]:
    pal = ensure_palette_has_colors(im, light_keys); put_palette(im, pal); pal = get_palette(im)
    reserved = {255}
    lk_to_target: Dict[Tuple[int,int,int], int] = {}
    for rgb in light_keys:
        src_idx = find_color_index(src_palette, rgb)
        t = choose_fallback_slot(im, reserved) if (src_idx in (-1, 255)) else src_idx
        lk_to_target[rgb] = t; reserved.add(t)
    swap_map: Dict[int,int] = {}
    pal = get_palette(im)
    for rgb, target in lk_to_target.items():
        cur = find_color_index(pal, rgb)
        if cur == -1 or cur == target: continue
        rT,gT,bT = pal[3*target:3*target+3]
        rC,gC,bC = pal[3*cur:3*cur+3]
        pal[3*target:3*target+3] = [rC,gC,bC]
        pal[3*cur:3*cur+3]       = [rT,gT,bT]
        put_palette(im, pal); pal = get_palette(im)
        swap_map[target] = cur
    return lk_to_target, swap_map

# --------- color grading (your reverted version) ---------

def transform_rgb_triplet(r:int,g:int,b:int,hour_1_24:int,night:float,blue_water:float,blue_land:float,sunset:float)->Tuple[int,int,int]:
    night=max(0.0,min(1.0,night)); blue_water=max(0.0,min(1.0,blue_water)); blue_land=max(0.0,min(1.0,blue_land)); sunset=max(0.0,min(1.0,sunset))
    f=hour_factor(hour_1_24); ew=evening_weight(hour_1_24,19.0,3.0)
    h0,s0,v0=colorsys.rgb_to_hsv(r/255.0,g/255.0,b/255.0); h,s,v=h0,s0,v0
    is_greenish=_hue_in_range(h0,95,140) and s0>0.18
    is_waterish=(_hue_in_range(h0,140,215) or _hue_in_range(h0,190,260)) and s0>0.12
    max_darken=0.78; floor_at_midnight=0.02
    v=v*(1-(max_darken*night)*f)+(floor_at_midnight*night)*f
    v=pow(max(0.0,min(1.0,v)),1.0+0.25*night*f)
    warm_min,warm_max=10/360.0,75/360.0; is_warm= warm_min<=h<=warm_max
    if not is_warm and s>0.20 and v>0.20:
        target_cool=220/360.0; delta=target_cool-h
        if delta>0.5: delta-=1.0
        elif delta<-0.5: delta+=1.0
        local_blue= blue_water if is_waterish else (blue_land if is_greenish else 0.5*(blue_water+blue_land))
        h=(h+(0.40*local_blue)*f*delta)%1.0
    if _hue_in_range(h0,140,215) and s0>0.12:
        h0deg=h0*360.0; t_lin = max(0.0, min(1.0, (h0deg - 140.0) / (215.0 - 140.0))); t=t_lin*t_lin*(3-2*t_lin)
        target=((232.0/360.0)*(1-t))+((238.0/360.0)*t); pull=((0.55)*(1-t))+((0.95)*t); satb=((0.35)*(1-t))+((0.60)*t)
        d=target-h
        if d>0.5: d-=1.0
        elif d<-0.5: d+=1.0
        h=(h+(pull*blue_water*f)*d)%1.0; s=s+(1.0-s)*(satb*blue_water*f)
    if _hue_in_range(h0,95,140) and s0>0.18:
        green_target=232/360.0; d3=green_target-h
        if d3>0.5: d3-=1.0
        elif d3<-0.5: d3+=1.0
        h=(h+(0.95*blue_land)*(f**1.20)*d3)%1.0; s=s+(1.0-s)*(0.40*blue_land)*(f**1.10)
    if _hue_in_range(h,190,260): s=s+(1.0-s)*(0.35*blue_water)*f
    if ew>0 and not is_warm:
        target_warm=15/360.0; d4=target_warm-h
        if d4>0.5: d4-=1.0
        elif d4<-0.5: d4+=1.0
        h=(h+(0.20*sunset)*ew*d4)%1.0; s=s+(1.0-s)*(0.05*sunset)*ew
    s=max(0.0,min(1.0,s)); v=max(0.0,min(1.0,v)); rr,gg,bb=colorsys.hsv_to_rgb(h,s,v)
    return int(round(rr*255.0)),int(round(gg*255.0)),int(round(bb*255.0))

# --------- processing ---------

def process_indexed_pcx(in_path: str, out_path: str, hour_1_24: int,
                        night: float, blue_water: float, blue_land: float,
                        sunset: float, light_keys: List[Tuple[int,int,int]]) -> None:
    print(f"Processing {in_path} → {out_path} [hour {hour_1_24}]")
    src_img = Image.open(in_path)
    src_pal = get_palette(src_img if src_img.mode == 'P' else src_img.convert('P'))
    im = Image.open(in_path)
    if im.mode != 'P':
        im = quantize_to_p_256(im)

    palette = get_palette(im); new_palette = palette.copy()
    # Skip MAGENTA and all light-keys (GREEN is not “preserved”; it will be remapped to MAGENTA on pixels)
    skip = set([MAGENTA] + light_keys)
    for i in range(256):
        r,g,b = palette[3*i:3*i+3]
        if (r,g,b) in skip:
            continue
        nr,ng,nb = transform_rgb_triplet(r,g,b,hour_1_24,night,blue_water,blue_land,sunset)
        new_palette[3*i:3*i+3] = [nr,ng,nb]
    put_palette(im, new_palette)

    # Ensure MAGENTA + all light-keys exist (GREEN not required)
    pal_after = get_palette(im)
    need = []
    if find_color_index(pal_after, MAGENTA) == -1: need.append(MAGENTA)
    for lk in light_keys:
        if find_color_index(pal_after, lk) == -1: need.append(lk)
    if need:
        pal_after = ensure_palette_has_colors(im, need); put_palette(im, pal_after)

    # Pin magenta at 255
    replacement_idx_255 = force_magenta_at_255(im)

    # Pin each light-key to the same index as source (when possible)
    lk_to_target, swap_partner = pin_lightkeys_to_source_indices(im, src_pal, light_keys)

    # Final index correction on pixels:
    # - MAGENTA → 255
    # - GREEN   → 255  (convert green to magenta)
    # - each light-key → its pinned target index
    # - any non-key pixel at 255 (due to quantizer) → move to replacement idx
    pal_after = get_palette(im)
    lk_index_map: Dict[Tuple[int,int,int],int] = {rgb:idx for rgb,idx in lk_to_target.items()}

    src_rgb = Image.open(in_path).convert('RGB')
    w,h = src_rgb.size; src_px = src_rgb.load(); dst_px = im.load()

    for y in range(h):
        for x in range(w):
            r,g,b = src_px[x,y]; cur = dst_px[x,y]
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
                 light_keys: List[Tuple[int,int,int]]=[(0,254,255)]) -> None:
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
        rel_dir = os.path.relpath(dirpath, base_dir); rel_dir = "" if rel_dir == "." else rel_dir
        for hhh in hours:
            os.makedirs(os.path.join(data_dir, f"{hhh:04d}", rel_dir), exist_ok=True)

        for name in filenames:
            if not name.lower().endswith(".pcx"): continue
            if norm_only:
                rel_file = os.path.normcase(os.path.join(rel_dir, name))
                if norm_only != os.path.normcase(name) and norm_only != rel_file: continue
            in_path = os.path.join(dirpath, name)
            for hhh in hours:
                hour_1_24 = hhh // 100
                out_path = os.path.join(data_dir, f"{hhh:04d}", rel_dir, name)
                process_indexed_pcx(in_path, out_path, hour_1_24, night, blue_water, blue_land, sunset, light_keys)

def main():
    ap = argparse.ArgumentParser(description="Civ 3 day-night (recursive; split blue controls; multiple light-keys)")
    ap.add_argument("--data", required=True, help="Path to Data root (contains the noon subfolder)")
    ap.add_argument("--noon", default="1200", help="Name of noon subfolder under --data (default: 1200)")
    ap.add_argument("--only-hour", type=int, help="Process a single hour (e.g., 2400)")
    ap.add_argument("--only-file", help="Process a single PCX filename or relative path")
    ap.add_argument("--night",  type=float, default=0.60, help="Midnight darkness strength (0..1)")
    ap.add_argument("--blue-water", type=float, default=0.85, help="Blue emphasis for ocean/shoreline (0..1)")
    ap.add_argument("--blue-land",  type=float, default=0.85, help="Blue emphasis for land/greens (0..1)")
    ap.add_argument("--blue", type=float, default=None, help="[Deprecated] Set both --blue-water and --blue-land if not individually provided.")
    ap.add_argument("--sunset", type=float, default=0.12, help="Evening warmth at ~19:00 (0..1)")
    ap.add_argument("--light-key", action="append", default=["#00feff"],
                    help="Placeholder color(s) to preserve. Repeat flag for multiple. Accepts #rrggbb or R,G,B.")

    args = ap.parse_args()
    bw, bl = args.blue_water, args.blue_land
    if args.blue is not None:
        if bw == ap.get_default("blue_water"): bw = args.blue
        if bl == ap.get_default("blue_land"):  bl = args.blue
        print("Note: --blue is deprecated; prefer --blue-water and --blue-land.")
    light_keys = parse_rgb_list(args.light_key)

    process_tree(args.data, args.noon, args.night, bw, bl, args.sunset,
                 args.only_hour, args.only_file, light_keys)

if __name__ == "__main__":
    main()
