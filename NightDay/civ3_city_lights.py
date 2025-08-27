#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Civ 3 city lights compositor for *_lights.pcx (recursive, high-intensity, orange-edge)

What this does
- Recursively scans Data/1200 for files ending with "_lights.pcx".
- Builds bright yellow cores + wide orange halos around exact --light-key pixels.
- Night hours (19:00..06:00): also replaces the paired non-lights file.
- Day hours: leaves non-lights as-is (ensures a copy exists).
- Preserves Civ3 magic colors (#ff00ff, #00ff00) and pins magenta to palette index 255.

Controls you’ll likely tweak
  --light-key         Exact RGB in *_lights.pcx marking windows (default: #00feff).
  --core-radius       Core blur size in px (feathering).
  --halo-radius       Halo blur size in px (bigger = farther reach).
  --core-gain         Core strength (no artificial cap).
  --halo-gain         Halo strength (no artificial cap).
  --core-color        Core tint (yellow).
  --glow-color        Halo tint (orange).
  --highlight-gain    Extra additive white pop (strong “lamp” sparkle).
  --halo-sep          0..1. Subtract this fraction of core from halo (creates ring outside).
  --halo-gamma        >1 pushes halo emphasis outward; <1 fattens near core.
  --size-boost        Boost intensity when light clusters are larger (0 disables).
  --size-radius       Radius for cluster detection (px).
  --size-gamma        Curve shaping for the cluster strength.
  --clip-interior     yes/no: confine glow inside the sprite (avoids magenta border bleed).
  --clip-erode        Erode interior mask by N px (0..3). 0 = no erosion.
  --blend-mode        screen|add. 'screen' keeps colors rich; 'add' is punchier/brighter.

Layout
  Data/
    1200/            <-- base noon tree (nested)
    0100/ 0200/ ...  <-- siblings created by this script

Tip: To get “yellow core → orange edge → black sky”, use a light yellow core,
a darker orange halo, nonzero --halo-sep, and halo_gamma ~1.25–1.5.
"""

import argparse, os, math, shutil
from typing import List, Tuple
from PIL import Image, ImageChops, ImageFilter

MAGENTA = (255, 0, 255)   # ff00ff
GREEN   = (0, 255, 0)     # 00ff00

# ---------- small utilities ----------

def parse_rgb(s: str) -> Tuple[int,int,int]:
    s = s.strip()
    if s.startswith("#"):
        s = s[1:]
        if len(s) != 6: raise ValueError("Hex color must be #rrggbb")
        return int(s[0:2],16), int(s[2:4],16), int(s[4:6],16)
    if "," in s:
        a = [int(p) for p in s.split(",")]
        if len(a) != 3: raise ValueError("RGB must be R,G,B")
        return tuple(max(0,min(255,v)) for v in a)  # type: ignore
    raise ValueError("Color must be '#rrggbb' or 'R,G,B'")

def hour_weight_lights(hour_1_24: int, start: float = 19.0, end: float = 6.0, floor: float = 0.80) -> float:
    """
    Nighttime weight with a high, flat plateau:
      - Visible from `start` (default 19:00) through `end` (default 06:00, across midnight).
      - Returns ~`floor` at the edges (e.g., 0.80 at 19:00 and 06:00) and 1.0 near midnight.
      - Outside the window: 0.0.

    Tweak `floor` if you want more/less variation (e.g., 0.70 or 0.90).
    """
    import math

    h = float(hour_1_24) % 24.0

    # inside the night window?
    in_window = (h >= start) or (h <= end)
    if not in_window:
        return 0.0

    # Map hour to position p in [0, L] along the night window
    L = (24.0 - start) + end               # total “night length” in hours (default 11h)
    if h >= start:
        p = h - start                       # 19:00..24:00  -> 0..(24-start)
    else:
        p = (24.0 - start) + h              # 00:00..06:00  -> (24-start)..L

    # Distance from the middle of the window (0 at center, 1 at edges)
    mid = 0.5 * L
    t = abs(p - mid) / mid                  # 0..1

    # Smooth “flat-top” cosine from `floor` at edges to 1.0 at center
    w = floor + (1.0 - floor) * 0.5 * (1.0 + math.cos(math.pi * t))
    return max(0.0, min(1.0, w))


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
    pal = get_palette(image)
    to_add = [c for c in colors if find_color_index(pal, c) == -1]
    if not to_add: return pal

    hist = image.histogram() if image.mode == 'P' else None
    if hist is None or len(hist) != 256:
        candidates = list(range(255, -1, -1))
    else:
        candidates = sorted(range(256), key=lambda i: hist[i])

    protected = set()
    for rgb in colors + [MAGENTA, GREEN]:
        idx = find_color_index(pal, rgb)
        if idx != -1: protected.add(idx)

    replace = []
    for i in candidates:
        if i in protected: continue
        replace.append(i)
        if len(replace) >= len(to_add): break
    if len(replace) < len(to_add):
        tail = [i for i in range(255, -1, -1) if i not in protected][:len(to_add)-len(replace)]
        replace.extend(tail)

    for slot, rgb in zip(replace, to_add):
        pal[3*slot:3*slot+3] = list(rgb)
    return pal

def quantize_to_p_256(img: Image.Image) -> Image.Image:
    """Convert to RGB then quantize with FASTOCTREE to 256-color P-mode."""
    try:
        dnone = Image.Dither.NONE
    except Exception:
        dnone = getattr(Image, "NONE", 0)
    rgb = img.convert('RGB')
    try:
        return rgb.quantize(colors=256, method=Image.FASTOCTREE, dither=dnone)
    except Exception:
        return rgb.quantize(colors=256, dither=dnone)

def color_equal(img_rgb: Image.Image, color: Tuple[int,int,int]) -> Image.Image:
    """Return 'L' mask 255 where pixel == color else 0."""
    w,h = img_rgb.size
    solid = Image.new('RGB', (w,h), color)
    diff = ImageChops.difference(img_rgb, solid)
    diffL = diff.convert('L')
    return diffL.point(lambda v: 255 if v == 0 else 0, mode='L')

def force_magenta_at_255(im: Image.Image) -> int:
    """
    Ensure palette index 255 is #ff00ff (MAGENTA).
    Returns replacement_idx: where the old slot-255 color was moved (or 255 if none).
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

def apply_gamma_L(imgL: Image.Image, gamma: float) -> Image.Image:
    gamma = max(0.05, float(gamma))
    lut = [min(255, int(round((i/255.0) ** gamma * 255))) for i in range(256)]
    return imgL.point(lut, mode='L')

def scale_L(imgL: Image.Image, factor: float) -> Image.Image:
    """Multiply an L image by a scalar (no cap apart from 255 clip)."""
    factor = max(0.0, float(factor))
    return imgL.point(lambda v: 255 if v*factor >= 255 else int(v*factor))

def screen_blend(a: Image.Image, b: Image.Image) -> Image.Image:
    """Screen blend with Pillow fallback."""
    try:
        return ImageChops.screen(a, b)
    except AttributeError:
        return ImageChops.invert(ImageChops.multiply(ImageChops.invert(a), ImageChops.invert(b)))

# ---------- masks & glow maps ----------

def build_interior_mask(base_bg: Image.Image, clip: bool, erode_px: int) -> Image.Image:
    """
    Returns 'L' mask (255 where glow is allowed). If clip=False, returns full white.
    """
    w,h = base_bg.size
    if not clip:
        return Image.new('L', (w,h), 255)
    base_rgb = base_bg.convert('RGB')
    mag = color_equal(base_rgb, MAGENTA)
    grn = color_equal(base_rgb, GREEN)
    interior = ImageChops.invert(ImageChops.add(mag, grn, scale=1.0))
    if erode_px > 0:
        k = max(1, int(erode_px))
        try:
            interior = interior.filter(ImageFilter.MinFilter(2*k+1))
        except Exception:
            pass
    return interior

def build_glow_maps(
    mask_bin: Image.Image,
    interior_mask: Image.Image,
    wtime: float,
    core_radius: float,
    halo_radius: float,
    core_gain: float,
    halo_gain: float,
    size_boost: float,
    size_radius: float,
    size_gamma: float,
    halo_sep: float,
    halo_gamma: float
) -> Tuple[Image.Image, Image.Image]:
    """
    Returns (core_alpha, halo_alpha) as 'L' images (0..255), clipped to interior,
    time-scaled, with optional size-aware boost and halo separation/gamma shaping.
    """
    # Base blurs (overlap is OK; we’ll shape halo into a ring)
    blur_core = mask_bin.filter(ImageFilter.GaussianBlur(radius=core_radius))
    blur_halo = mask_bin.filter(ImageFilter.GaussianBlur(radius=halo_radius))

    # Local size/density boost (bigger window clusters glow more)
    if size_boost > 0.0 and size_radius > 0:
        density = mask_bin.filter(ImageFilter.BoxBlur(radius=size_radius)).convert('L')
        density = apply_gamma_L(density, max(0.05, size_gamma))
        # Map density 0..255 → multiplier added as extra alpha (simple & effective)
        boost = scale_L(density, size_boost * wtime)
        blur_core = ImageChops.add(blur_core, boost, scale=1.0)
        blur_halo = ImageChops.add(blur_halo, boost, scale=1.0)

    # Halo separation: make halo a ring outside the core (prevents yellow everywhere)
    halo_sep = max(0.0, min(1.0, halo_sep))
    if halo_sep > 0.0:
        inner = blur_core.point(lambda v: int(v * halo_sep))
        halo_only = ImageChops.subtract(blur_halo, inner, scale=1.0, offset=0)
    else:
        halo_only = blur_halo

    # Push halo outward / control falloff
    halo_only = apply_gamma_L(halo_only, halo_gamma)

    # Apply gains and time-of-day
    core_alpha = scale_L(blur_core, wtime * max(0.0, core_gain))
    halo_alpha = scale_L(halo_only, wtime * max(0.0, halo_gain))

    # Clip to interior
    core_alpha = ImageChops.multiply(core_alpha, interior_mask)
    halo_alpha = ImageChops.multiply(halo_alpha, interior_mask)

    return core_alpha, halo_alpha

def layer_from_alpha(color: Tuple[int,int,int], alphaL: Image.Image) -> Image.Image:
    """Create an RGB emission layer by compositing 'color' over black using alpha mask."""
    w,h = alphaL.size
    solid = Image.new('RGB', (w,h), color)
    black = Image.new('RGB', (w,h), (0,0,0))
    return Image.composite(solid, black, alphaL)

# ---------- compositor ----------

def build_composite(
    base_bg_rgba: Image.Image,
    mask_source_rgb: Image.Image,
    hour_1_24: int,
    light_key: Tuple[int,int,int],
    core_radius: float,
    halo_radius: float,
    core_gain: float,
    halo_gain: float,
    core_color: Tuple[int,int,int],
    glow_color: Tuple[int,int,int],
    size_boost: float,
    size_radius: float,
    size_gamma: float,
    clip_interior: bool,
    clip_erode: int,
    highlight_gain: float,
    blend_mode: str,
    halo_sep: float,
    halo_gamma: float
) -> Image.Image:
    """Return RGBA composite for this hour."""
    w,h = base_bg_rgba.size
    mask_bin = color_equal(mask_source_rgb, light_key)
    if mask_bin.getbbox() is None:
        return base_bg_rgba.copy()

    interior = build_interior_mask(base_bg_rgba, clip_interior, clip_erode)
    wtime = hour_weight_lights(hour_1_24)

    core_alpha, halo_alpha = build_glow_maps(
        mask_bin, interior, wtime,
        core_radius, halo_radius,
        core_gain, halo_gain,
        size_boost, size_radius, size_gamma,
        halo_sep, halo_gamma
    )

    core_layer = layer_from_alpha(core_color, core_alpha)
    halo_layer = layer_from_alpha(glow_color, halo_alpha)

    comp = base_bg_rgba.convert('RGB')
    if blend_mode == "add":
        comp = ImageChops.add(comp, core_layer, scale=1.0)
        comp = ImageChops.add(comp, halo_layer, scale=1.0)
    else:
        comp = screen_blend(comp, core_layer)
        comp = screen_blend(comp, halo_layer)

    # Optional white sparkle from the core
    if highlight_gain > 0.0 and wtime > 0.0:
        hl = scale_L(core_alpha, highlight_gain)
        white = Image.new('RGB', (w,h), (255,255,255))
        highlight = Image.composite(white, Image.new('RGB',(w,h),(0,0,0)), hl)
        comp = ImageChops.add(comp, highlight, scale=1.0)

    return comp.convert('RGBA')

# ---------- palette-safe save ----------

def save_with_palette_and_magic(comp_rgba: Image.Image, out_path: str, base_for_magic_rgb: Image.Image) -> None:
    imP = quantize_to_p_256(comp_rgba)

    # Ensure magic colors exist, then pin magenta at index 255
    pal_after = ensure_palette_has_colors(imP, [MAGENTA, GREEN])
    put_palette(imP, pal_after)
    replacement_idx = force_magenta_at_255(imP)

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
                # Non-magenta pixel that held old slot 255 → send to the moved color
                dst_px[x,y] = replacement_idx

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    imP.save(out_path, format='PCX')

# ---------- folder driver ----------

def process_tree(data_dir: str, noon_subfolder: str,
                 light_key: Tuple[int,int,int],
                 core_radius: float, halo_radius: float,
                 core_gain: float, halo_gain: float,
                 core_color: Tuple[int,int,int],
                 glow_color: Tuple[int,int,int],
                 size_boost: float, size_radius: float, size_gamma: float,
                 clip_interior: bool, clip_erode: int,
                 highlight_gain: float, blend_mode: str,
                 halo_sep: float, halo_gamma: float,
                 only_hour: int = None, only_file: str = None) -> None:

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

            plain_name = lights_name[:-len("_lights.pcx")] + ".pcx"
            noon_lights_path = os.path.join(dirpath, lights_name)
            noon_plain_path  = os.path.join(dirpath, plain_name)

            for hhh in hours:
                hour_1_24 = hhh // 100
                hour_dir = os.path.join(data_dir, f"{hhh:04d}", rel_dir)
                hour_lights_path = os.path.join(hour_dir, lights_name)
                hour_plain_path  = os.path.join(hour_dir, plain_name)

                # Background to composite onto (prefer hour, else noon)
                if os.path.exists(hour_plain_path):
                    base_bg_img = Image.open(hour_plain_path).convert('RGBA')
                    base_for_magic_rgb = Image.open(hour_plain_path).convert('RGB')
                elif os.path.exists(noon_plain_path):
                    base_bg_img = Image.open(noon_plain_path).convert('RGBA')
                    base_for_magic_rgb = Image.open(noon_plain_path).convert('RGB')
                else:
                    base_bg_img = Image.open(noon_lights_path).convert('RGBA')
                    base_for_magic_rgb = Image.open(noon_lights_path).convert('RGB')

                # Mask comes from NOON *_lights for consistency
                mask_source_rgb = Image.open(noon_lights_path).convert('RGB')

                comp = build_composite(
                    base_bg_rgba=base_bg_img,
                    mask_source_rgb=mask_source_rgb,
                    hour_1_24=hour_1_24,
                    light_key=light_key,
                    core_radius=core_radius, halo_radius=halo_radius,
                    core_gain=core_gain, halo_gain=halo_gain,
                    core_color=core_color, glow_color=glow_color,
                    size_boost=size_boost, size_radius=size_radius, size_gamma=size_gamma,
                    clip_interior=clip_interior, clip_erode=clip_erode,
                    highlight_gain=highlight_gain, blend_mode=blend_mode,
                    halo_sep=halo_sep, halo_gamma=halo_gamma
                )

                # Always write *_lights variant
                save_with_palette_and_magic(comp, hour_lights_path, base_for_magic_rgb)

                # Night rule: replace non-lights; Day rule: ensure exists
                if is_night_hour(hour_1_24):
                    save_with_palette_and_magic(comp, hour_plain_path, base_for_magic_rgb)
                else:
                    if not os.path.exists(hour_plain_path):
                        if os.path.exists(noon_plain_path):
                            os.makedirs(os.path.dirname(hour_plain_path), exist_ok=True)
                            shutil.copyfile(noon_plain_path, hour_plain_path)
                        else:
                            save_with_palette_and_magic(base_bg_img, hour_plain_path, base_for_magic_rgb)

# ---------- CLI ----------

def main():
    ap = argparse.ArgumentParser(description="Civ 3 city lights compositor (recursive, orange-edge, size-aware)")
    ap.add_argument("--data", required=True, help="Path to Data root (contains the noon subfolder)")
    ap.add_argument("--noon", default="1200", help="Name of noon subfolder under --data (default: 1200)")
    ap.add_argument("--only-hour", type=int, help="Process a single hour (e.g., 2400)")
    ap.add_argument("--only-file", help="Process a single *_lights.pcx filename or relative path")

    ap.add_argument("--light-key", type=str, default="#00feff", help="Placeholder RGB in *_lights.pcx. Default: #00feff")

    # Geometry & strength
    ap.add_argument("--core-radius", type=float, default=1.2, help="Gaussian blur radius for core (px). Default: 1.2")
    ap.add_argument("--halo-radius", type=float, default=12.0, help="Gaussian blur radius for halo (px). Default: 12")
    ap.add_argument("--core-gain",   type=float, default=1.5, help="Core strength multiplier (no cap). Default: 1.5")
    ap.add_argument("--halo-gain",   type=float, default=4.0, help="Halo strength multiplier (no cap). Default: 4.0")

    # Colors
    ap.add_argument("--core-color", type=str, default="#ff8a20", help="Core window color (yellow). Default: #ff8a20")
    ap.add_argument("--glow-color", type=str, default="#dc6a00", help="Halo glow color (orange). Default: #dc6a00")

    # Extra punch
    ap.add_argument("--highlight-gain", type=float, default=0.6, help="Extra additive white highlight from core (0..6+). Default: 0.6")

    # Size-aware boost
    ap.add_argument("--size-boost",  type=float, default=0.8, help="Add alpha based on local light density (0 disables). Default: 0.8")
    ap.add_argument("--size-radius", type=float, default=2.0, help="Radius for density blur (px). Default: 2.0")
    ap.add_argument("--size-gamma",  type=float, default=0.75, help="Gamma for density curve. Default: 0.75")

    # Interior clip
    ap.add_argument("--clip-interior", type=str, default="yes", choices=["yes","no"],
                    help="Confine glow to non-magenta/green sprite interior. Default: yes")
    ap.add_argument("--clip-erode", type=int, default=0, help="Erode interior mask by N px (0..3). Default: 0")

    # Halo shaping
    ap.add_argument("--halo-sep", type=float, default=0.75,
                    help="Fraction of core blur to subtract from halo (0..1). Default: 0.75")
    ap.add_argument("--halo-gamma", type=float, default=1.35,
                    help="Gamma shaping for halo ring (>1 pushes emphasis outward). Default: 1.35")

    # Blend mode
    ap.add_argument("--blend-mode", type=str, default="screen", choices=["screen","add"],
                    help="Light blend mode. 'screen' = rich color, 'add' = very bright. Default: screen")

    args = ap.parse_args()

    light_key   = parse_rgb(args.light_key)
    core_color  = parse_rgb(args.core_color)
    glow_color  = parse_rgb(args.glow_color)

    process_tree(
        data_dir=args.data, noon_subfolder=args.noon,
        light_key=light_key,
        core_radius=args.core_radius, halo_radius=args.halo_radius,
        core_gain=args.core_gain, halo_gain=args.halo_gain,
        core_color=core_color, glow_color=glow_color,
        size_boost=args.size_boost, size_radius=args.size_radius, size_gamma=args.size_gamma,
        clip_interior=(args.clip_interior=="yes"), clip_erode=args.clip_erode,
        highlight_gain=args.highlight_gain, blend_mode=args.blend_mode,
        halo_sep=args.halo_sep, halo_gamma=args.halo_gamma,
        only_hour=args.only_hour, only_file=args.only_file
    )

if __name__ == "__main__":
    main()
