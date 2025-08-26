#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Civ 3 city lights glow compositor for *_lights.pcx (recursive)

- Root layout:
    Data/
      1200/                <-- base (noon) tree with nested subfolders
      0100/ 0200/ ...      <-- siblings created by this script (mirroring 1200)

- Behavior:
  * Recursively scans Data/1200 for files ending with "_lights.pcx".
  * Always processes the *_lights file with a placeholder color mask to add
    warm window cores + soft halo (time-of-day weighted).
  * NIGHT hours (19:00..24:00 and 01:00..06:00): the processed result ALSO
    replaces the paired non-lights file (same path/name without "_lights").
  * DAY hours: leave the non-lights file as-is. If missing, copy from noon.
  * Preserves Civ3 magic colors #ff00ff and #00ff00 in outputs.
"""

import argparse, os, math, shutil
from typing import List, Tuple, Optional
from PIL import Image, ImageChops, ImageFilter

MAGENTA = (255, 0, 255)   # ff00ff
GREEN   = (0, 255, 0)     # 00ff00

# ---------- small utils ----------

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

def parse_rgb(s: str) -> Tuple[int,int,int]:
    s = s.strip()
    if s.startswith("#"):
        s = s[1:]
        if len(s) != 6: raise ValueError("Hex color must be #rrggbb")
        return int(s[0:2],16), int(s[2:4],16), int(s[4:6],16)
    if "," in s:
        parts = [int(p) for p in s.split(",")]
        if len(parts)!=3: raise ValueError("RGB must be R,G,B")
        return tuple(max(0,min(255,v)) for v in parts)  # type: ignore
    raise ValueError("Color must be '#rrggbb' or 'R,G,B'")

def hour_weight_lights(hour_1_24: int, peak: float=24.0) -> float:
    h = float(hour_1_24) % 24.0
    d = min((h - peak) % 24.0, (peak - h) % 24.0)  # 0..12
    half = 6.05
    if d > half: return 0.0
    t = d / half
    return 0.5 * (1.0 + math.cos(math.pi * t))

def is_night_hour(hour_1_24: int) -> bool:
    return (19 <= hour_1_24 <= 24) or (1 <= hour_1_24 <= 6)

def get_palette(image: Image.Image) -> List[int]:
    pal = image.getpalette() or []
    if len(pal) < 256*3: pal += [0]*(256*3 - len(pal))
    return pal[:256*3]

def put_palette(image: Image.Image, palette: List[int]) -> None:
    if len(palette) < 256*3: palette += [0]*(256*3 - len(palette))
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
    if hist is None or len(hist) != 256:
        candidates = list(range(255, -1, -1))
    else:
        candidates = sorted(range(256), key=lambda i: hist[i])

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

def quantize_to_p_256(img: Image.Image) -> Image.Image:
    """Convert to RGB then quantize with FASTOCTREE; avoids RGBA+MEDIANCUT errors."""
    try:
        dnone = Image.Dither.NONE
    except Exception:
        dnone = getattr(Image, "NONE", 0)
    img_rgb = img.convert('RGB')
    try:
        return img_rgb.quantize(colors=256, method=Image.FASTOCTREE, dither=dnone)
    except Exception:
        return img_rgb.quantize(colors=256, dither=dnone)

def color_equal(img_rgb: Image.Image, color: Tuple[int,int,int]) -> Image.Image:
    w,h = img_rgb.size
    solid = Image.new('RGB', (w,h), color)
    diff = ImageChops.difference(img_rgb, solid)
    diffL = diff.convert('L')
    mask = diffL.point(lambda v: 255 if v == 0 else 0, mode='L')
    return mask

def scale_L(imgL: Image.Image, factor: float) -> Image.Image:
    factor = max(0.0, min(10.0, factor))
    return imgL.point(lambda v: int(round(max(0, min(255, v*factor)))))

def subtract_L(a: Image.Image, b: Image.Image) -> Image.Image:
    return ImageChops.subtract(a, b, scale=1.0, offset=0)

def any_true(maskL: Image.Image) -> bool:
    return maskL.getbbox() is not None

def average_color_in_ring(base_rgb: Image.Image, ring_mask: Image.Image) -> Optional[Tuple[int,int,int]]:
    base_data = base_rgb.getdata()
    ring_data = ring_mask.getdata()
    total = [0,0,0]
    count = 0
    for (r,g,b), m in zip(base_data, ring_data):
        if m == 255:
            total[0]+=r; total[1]+=g; total[2]+=b
            count += 1
    if count == 0:
        return None
    return (total[0]//count, total[1]//count, total[2]//count)

def strip_lights_suffix(filename: str) -> str:
    lower = filename.lower()
    if not lower.endswith("_lights.pcx"):
        raise ValueError("Expected filename to end with '_lights.pcx'")
    return filename[:-len("_lights.pcx")] + ".pcx"

# ---------- core compositor ----------

def build_composite(
    base_bg: Image.Image,             # RGBA non-lights art
    mask_source_rgb: Image.Image,     # RGB from NOON *_lights to find placeholder
    hour_1_24: int,
    light_key: Tuple[int,int,int],
    core_radius: float,
    halo_radius: float,
    core_gain: float,
    halo_gain: float,
    core_color: Tuple[int,int,int],
    glow_color: Tuple[int,int,int],
    shadow_mode: str,
    shadow_color_const: Tuple[int,int,int]
) -> Image.Image:
    """Return RGBA composite for this hour, with glow confined to the city interior."""
    # 0) Interior mask = everything that's NOT magenta or green in the base (the actual sprite)
    base_rgb = base_bg.convert('RGB')
    mag_mask = color_equal(base_rgb, MAGENTA)          # 255 at magenta bg
    grn_mask = color_equal(base_rgb, GREEN)            # 255 at special green (just in case)
    import PIL.ImageChops as IC
    interior = IC.invert(IC.add(mag_mask, grn_mask, scale=1.0))  # 255 where not magenta/green

    # light erosion so glow can't hug the outer silhouette too hard
    try:
        #interior = interior.filter(ImageFilter.MinFilter(3))  # erode by ~1px
        pass
    except Exception:
        pass

    # 1) Build window mask from the NOON *_lights file
    mask_bin = color_equal(mask_source_rgb, light_key)  # 255 only where placeholder color is
    if not any_true(mask_bin):
        return base_bg.copy()

    # Confine mask to interior so we never use background edges
    mask_bin = IC.multiply(mask_bin, interior)

    # 2) Blurs for core + halo
    blur_core = mask_bin.filter(ImageFilter.GaussianBlur(radius=core_radius))
    blur_halo_full = mask_bin.filter(ImageFilter.GaussianBlur(radius=halo_radius))
    halo_only = IC.subtract(blur_halo_full, blur_core)  # avoids double-counting core

    # 3) Time-of-day
    wtime = hour_weight_lights(hour_1_24)

    comp = base_bg.copy()

    if wtime > 0.0:
        # Core fill
        core_alpha = scale_L(mask_bin, wtime * max(0.0, min(1.0, 0.95)))
        core_alpha = IC.multiply(core_alpha, interior)
        core_solid = Image.new('RGBA', base_bg.size, (*core_color, 255))
        comp = Image.composite(core_solid, comp, core_alpha)

        # Halo
        # allow >1.0 to really boost opacity; scale_L caps internally (0..10)
        core_glow_alpha = scale_L(blur_core, wtime * max(0.0, core_gain))
        halo_glow_alpha = scale_L(halo_only, wtime * max(0.0, halo_gain))

        glow_alpha = IC.add(core_glow_alpha, halo_glow_alpha, scale=1.0, offset=0)
        glow_alpha = IC.multiply(glow_alpha, interior)
        glow_solid = Image.new('RGBA', base_bg.size, (*glow_color, 255))
        comp = Image.composite(glow_solid, comp, glow_alpha)
    else:
        # Lights off → fill placeholders with sampled shadow (still clipped to interior)
        dilated = mask_bin.filter(ImageFilter.GaussianBlur(radius=1.0)).point(lambda v: 255 if v>0 else 0, mode='L')
        ring_mask = IC.subtract(dilated, mask_bin)
        base_rgb2 = comp.convert('RGB')
        sampled = average_color_in_ring(base_rgb2, ring_mask) if shadow_mode == "auto" else None
        if sampled is None:
            sr, sg, sb = shadow_color_const
        else:
            sr, sg, sb = sampled
            sr, sg, sb = int(sr*0.85), int(sg*0.83), int(sb*0.80)
        fill_color = Image.new('RGBA', base_bg.size, (sr, sg, sb, 255))
        comp = Image.composite(fill_color, comp, mask_bin)

    return comp

def save_with_palette_and_magic(comp_rgba:Image.Image, out_path:str, base_for_magic_rgb:Image.Image) -> None:
    imP = quantize_to_p_256(comp_rgba)

    # Ensure magic exist, then pin magenta to index 255
    pal_after = ensure_palette_has_colors(imP, [MAGENTA, GREEN])
    put_palette(imP, pal_after)
    replacement_idx = force_magenta_at_255(imP)

    # Refresh indices after edits
    pal_after = get_palette(imP)
    idx_mag = 255
    idx_grn = find_color_index(pal_after, GREEN)

    w,h = imP.size
    dst_px = imP.load()
    src_px = base_for_magic_rgb.load()

    for y in range(h):
        for x in range(w):
            r,g,b = src_px[x,y]
            cur = dst_px[x,y]
            if (r,g,b) == MAGENTA:
                dst_px[x,y] = 255
            elif (r,g,b) == GREEN and idx_grn != -1:
                dst_px[x,y] = idx_grn
            elif cur == 255 and replacement_idx != 255:
                # Non-magenta pixel that held old index 255 → send to the moved color
                dst_px[x,y] = replacement_idx

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    imP.save(out_path, format='PCX')


# ---------- folder driver ----------

def process_tree(data_dir:str, noon_subfolder:str, light_key:Tuple[int,int,int],
                 core_radius:float, halo_radius:float, core_gain:float, halo_gain:float,
                 core_color:Tuple[int,int,int], glow_color:Tuple[int,int,int],
                 shadow_mode:str, shadow_color:Tuple[int,int,int],
                 only_hour:int=None, only_file:str=None) -> None:

    base_dir = os.path.join(data_dir, noon_subfolder)
    if not os.path.isdir(base_dir): raise SystemExit(f"No such folder: {base_dir}")

    hours = [h for h in range(100, 2500, 100) if h != 1200]
    if only_hour is not None:
        if only_hour == 1200: raise SystemExit("1200 is the base (left untouched). Choose another hour.")
        if only_hour % 100 != 0 or only_hour < 100 or only_hour > 2400:
            raise SystemExit("--only-hour must be one of 100, 200, ..., 2400.")
        hours = [only_hour]

    norm_only = os.path.normcase(only_file) if only_file else None

    for dirpath, dirnames, filenames in os.walk(base_dir):
        rel_dir = os.path.relpath(dirpath, base_dir)
        rel_dir = "" if rel_dir == "." else rel_dir

        for hhh in hours:
            out_dir = os.path.join(data_dir, f"{hhh:04d}", rel_dir)
            os.makedirs(out_dir, exist_ok=True)

        lights_files = [f for f in filenames if f.lower().endswith("_lights.pcx")]

        for lights_name in lights_files:
            if norm_only:
                rel_file = os.path.normcase(os.path.join(rel_dir, lights_name))
                if norm_only != os.path.normcase(lights_name) and norm_only != rel_file:
                    continue

            plain_name = strip_lights_suffix(lights_name)

            noon_lights_path = os.path.join(dirpath, lights_name)
            noon_plain_path  = os.path.join(dirpath, plain_name)

            for hhh in hours:
                hour_1_24 = hhh // 100
                hour_dir = os.path.join(data_dir, f"{hhh:04d}", rel_dir)
                hour_lights_path = os.path.join(hour_dir, lights_name)
                hour_plain_path  = os.path.join(hour_dir, plain_name)
                print(f"Processing hour {hour_1_24:02d}: {os.path.join(rel_dir, lights_name)}")

                if os.path.exists(hour_plain_path):
                    base_bg_img = Image.open(hour_plain_path).convert('RGBA')
                    base_for_magic_rgb = Image.open(hour_plain_path).convert('RGB')
                elif os.path.exists(noon_plain_path):
                    base_bg_img = Image.open(noon_plain_path).convert('RGBA')
                    base_for_magic_rgb = Image.open(noon_plain_path).convert('RGB')
                else:
                    base_bg_img = Image.open(noon_lights_path).convert('RGBA')
                    base_for_magic_rgb = Image.open(noon_lights_path).convert('RGB')

                mask_source_rgb = Image.open(noon_lights_path).convert('RGB')

                comp = build_composite(base_bg=base_bg_img, mask_source_rgb=mask_source_rgb,
                                       hour_1_24=hour_1_24, light_key=light_key,
                                       core_radius=core_radius, halo_radius=halo_radius,
                                       core_gain=core_gain, halo_gain=halo_gain,
                                       core_color=core_color, glow_color=glow_color,
                                       shadow_mode=shadow_mode, shadow_color_const=shadow_color)

                save_with_palette_and_magic(comp, hour_lights_path, base_for_magic_rgb)

                if is_night_hour(hour_1_24):
                    save_with_palette_and_magic(comp, hour_plain_path, base_for_magic_rgb)
                else:
                    if not os.path.exists(hour_plain_path):
                        if os.path.exists(noon_plain_path):
                            os.makedirs(os.path.dirname(hour_plain_path), exist_ok=True)
                            shutil.copyfile(noon_plain_path, hour_plain_path)
                        else:
                            save_with_palette_and_magic(base_bg_img, hour_plain_path, base_for_magic_rgb)

def main():
    ap = argparse.ArgumentParser(description="Civ 3 city lights glow compositor for *_lights.pcx (recursive with night-hour replacement)")
    ap.add_argument("--data", required=True, help="Path to Data root (which contains the noon subfolder, e.g., Data)")
    ap.add_argument("--noon", default="1200", help="Name of noon subfolder under --data (default: 1200)")
    ap.add_argument("--only-hour", type=int, help="Process a single hour (e.g., 2400)")
    ap.add_argument("--only-file", help="Process a single *_lights.pcx filename or relative path within the noon tree")

    ap.add_argument("--light-key", type=str, default="#00feff", help="Placeholder RGB (R,G,B or #rrggbb). Default: #00feff")
    ap.add_argument("--core-radius", type=float, default=1.6, help="Gaussian blur radius (px) for inner glow. Default: 1.6")
    ap.add_argument("--halo-radius", type=float, default=6.0, help="Gaussian blur radius (px) for soft halo. Default: 6.0")
    ap.add_argument("--core-gain", type=float, default=0.95, help="Inner glow strength (0..1). Default: 0.95")
    ap.add_argument("--halo-gain", type=float, default=0.55, help="Halo strength (0..1). Default: 0.55")
    ap.add_argument("--core-color", type=str, default="#fff87a", help="Core window color (lit). Default: #ffd27a")
    ap.add_argument("--glow-color", type=str, default="#e78e21", help="Glow tint color. Default: #e78e21")
    ap.add_argument("--shadow", type=str, default="auto", help="Shadow fill when lights off: 'auto' to sample surrounding ring, or a color (#rrggbb). Default: auto")
    ap.add_argument("--shadow-fallback", type=str, default="#5a4e44", help="Fallback shadow color if sampling fails. Default: #5a4e44")

    args = ap.parse_args()

    light_key   = parse_rgb(args.light_key)
    core_color  = parse_rgb(args.core_color)
    glow_color  = parse_rgb(args.glow_color)

    shadow_mode = "auto"
    shadow_const = parse_rgb(args.shadow_fallback)
    if args.shadow.lower() != "auto":
        shadow_mode = "const"
        shadow_const = parse_rgb(args.shadow)

    process_tree(data_dir=args.data, noon_subfolder=args.noon,
                 light_key=light_key, core_radius=args.core_radius, halo_radius=args.halo_radius,
                 core_gain=args.core_gain, halo_gain=args.halo_gain,
                 core_color=core_color, glow_color=glow_color,
                 shadow_mode=shadow_mode, shadow_color=shadow_const,
                 only_hour=args.only_hour, only_file=args.only_file)

if __name__ == "__main__":
    main()
