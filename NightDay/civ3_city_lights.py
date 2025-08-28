#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Civ 3 city lights compositor for *_lights.pcx
- Recursive over Data/1200 → writes siblings 0100..2400
- Multiple --light-key values (union mask)
- **Per-light-key styles** via --light-style: per-key core/glow colors and optional overrides
- Night (19..24,1..6): also replaces non-lights file
- Pins magenta #ff00ff to palette index 255; preserves #00ff00
"""

import argparse, os, math, shutil
from typing import List, Tuple, Dict, Optional
from PIL import Image, ImageChops, ImageFilter

MAGENTA = (255, 0, 255)
GREEN   = (0, 255, 0)

# ---------- parsing ----------

def parse_rgb_one(s: str) -> Tuple[int,int,int]:
    s = s.strip()
    if s.startswith("#"):
        s = s[1:];  assert len(s)==6, "Hex color must be #rrggbb"
        return int(s[0:2],16), int(s[2:4],16), int(s[4:6],16)
    if "," in s:
        a = [int(p) for p in s.split(",")]; assert len(a)==3, "RGB must be R,G,B"
        return tuple(max(0,min(255,v)) for v in a)  # type: ignore
    raise ValueError("Color must be '#rrggbb' or 'R,G,B'")

def parse_rgb_list(values: List[str]) -> List[Tuple[int,int,int]]:
    out: List[Tuple[int,int,int]] = []
    for raw in values or []:
        tokens = [t for t in raw.replace(";", " ").split() if t]
        for tok in tokens:
            out.append(parse_rgb_one(tok))
    if not out: out = [(0,254,255)]
    dedup, seen = [], set()
    for c in out:
        if c not in seen: seen.add(c); dedup.append(c)
    return dedup

def parse_styles(values: List[str]) -> Dict[Tuple[int,int,int], Dict[str,object]]:
    """
    --light-style "key=#00feff; core=#fff87a; glow=#ff8a20; core_gain=1.6; halo_gain=4.0; halo_sep=0.75; halo_gamma=1.35; core_radius=1.2; halo_radius=14; highlight=0.6"
    Accepts ';' or ',' separators. Only 'key' is required; others override globals for that key.
    """
    styles: Dict[Tuple[int,int,int], Dict[str,object]] = {}
    for raw in values or []:
        parts = [p.strip() for p in raw.replace(",", ";").split(";") if p.strip()]
        kv: Dict[str,str] = {}
        for p in parts:
            if "=" in p:
                k,v = p.split("=",1); kv[k.strip().lower()] = v.strip()
        if "key" not in kv: raise SystemExit("Each --light-style must include key=<color>")
        key_rgb = parse_rgb_one(kv["key"])
        entry: Dict[str,object] = {}
        if "core" in kv: entry["core_color"] = parse_rgb_one(kv["core"])
        if "glow" in kv: entry["glow_color"] = parse_rgb_one(kv["glow"])
        for numk in ["core_gain","halo_gain","core_radius","halo_radius","halo_sep","halo_gamma","highlight","size_boost","size_radius","size_gamma"]:
            if numk in kv: entry[numk] = float(kv[numk])
        styles[key_rgb] = entry
    return styles

# ---------- time window ----------

def hour_weight_lights(hour_1_24: int, start: float = 19.0, end: float = 6.0, floor: float = 0.80) -> float:
    h = float(hour_1_24) % 24.0
    inwin = (h >= start) or (h <= end)
    if not inwin: return 0.0
    L = (24.0 - start) + end
    p = (h - start) if (h >= start) else ((24.0 - start) + h)
    mid = 0.5*L; t = abs(p - mid)/mid
    w = floor + (1.0 - floor)*0.5*(1.0 + math.cos(math.pi * t))
    return max(0.0, min(1.0, w))

def is_night_hour(hour_1_24: int) -> bool:
    return (19 <= hour_1_24 <= 24) or (1 <= hour_1_24 <= 6)

# ---------- palette ----------

def get_palette(image: Image.Image) -> List[int]:
    pal = image.getpalette() or []
    if len(pal) < 256*3: pal += [0]*(256*3 - len(pal))
    return pal[:256*3]

def put_palette(image: Image.Image, palette: List[int]) -> None:
    if len(palette) < 256*3: palette += [0]*(256*3 - len(palette))
    image.putpalette(palette[:256*3])

def ensure_palette_has_colors(image: Image.Image, colors: List[Tuple[int,int,int]]) -> List[int]:
    pal = get_palette(image)
    to_add = [c for c in colors if find_color_index(pal,c)==-1]
    if not to_add: return pal
    hist = image.histogram() if image.mode=='P' else None
    candidates = sorted(range(256), key=(lambda i: hist[i])) if (hist and len(hist)==256) else list(range(255,-1,-1))
    protected=set()
    for rgb in colors+[MAGENTA,GREEN]:
        idx = find_color_index(pal, rgb)
        if idx!=-1: protected.add(idx)
    replace=[]
    for i in candidates:
        if i in protected: continue
        replace.append(i)
        if len(replace)>=len(to_add): break
    if len(replace)<len(to_add):
        tail=[i for i in range(255,-1,-1) if i not in protected][:len(to_add)-len(replace)]
        replace.extend(tail)
    for slot,rgb in zip(replace,to_add):
        pal[3*slot:3*slot+3]=list(rgb)
    return pal

def find_color_index(palette: List[int], rgb: Tuple[int,int,int]) -> int:
    r,g,b = rgb
    for i in range(256):
        if palette[3*i]==r and palette[3*i+1]==g and palette[3*i+2]==b: return i
    return -1

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

def force_magenta_at_255(im: Image.Image) -> int:
    pal = get_palette(im)
    old255 = (pal[3*255], pal[3*255+1], pal[3*255+2])
    idx_mag = find_color_index(pal, MAGENTA)
    if idx_mag == -1:
        pal = ensure_palette_has_colors(im, [MAGENTA]); put_palette(im, pal)
        pal = get_palette(im); idx_mag = find_color_index(pal, MAGENTA)
    if idx_mag == 255: return 255
    pal[3*255:3*255+3]=list(MAGENTA); pal[3*idx_mag:3*idx_mag+3]=list(old255)
    put_palette(im, pal); return idx_mag

# ---------- image ops ----------

def color_equal(img_rgb: Image.Image, color: Tuple[int,int,int]) -> Image.Image:
    w,h = img_rgb.size
    solid = Image.new('RGB',(w,h),color)
    diff = ImageChops.difference(img_rgb, solid)
    return diff.convert('L').point(lambda v: 255 if v==0 else 0, mode='L')

def apply_gamma_L(imgL: Image.Image, gamma: float) -> Image.Image:
    gamma = max(0.05, float(gamma))
    lut = [min(255, int(round((i/255.0) ** gamma * 255))) for i in range(256)]
    return imgL.point(lut, mode='L')

def scale_L(imgL: Image.Image, factor: float) -> Image.Image:
    factor = max(0.0, float(factor))
    return imgL.point(lambda v: 255 if v*factor >= 255 else int(v*factor))

def screen_blend(a: Image.Image, b: Image.Image) -> Image.Image:
    try:
        return ImageChops.screen(a,b)
    except AttributeError:
        return ImageChops.invert(ImageChops.multiply(ImageChops.invert(a), ImageChops.invert(b)))

# ---------- masks & glows ----------

def build_interior_mask(base_bg: Image.Image, clip: bool, erode_px: int) -> Image.Image:
    if not clip: return Image.new('L', base_bg.size, 255)
    base_rgb = base_bg.convert('RGB')
    mag = color_equal(base_rgb, MAGENTA)
    grn = color_equal(base_rgb, GREEN)
    interior = ImageChops.invert(ImageChops.add(mag, grn, scale=1.0))
    if erode_px>0:
        k = max(1,int(erode_px)); interior = interior.filter(ImageFilter.MinFilter(2*k+1))
    return interior

def build_glow_maps(mask_bin: Image.Image, interior_mask: Image.Image, wtime: float,
                    core_radius: float, halo_radius: float,
                    core_gain: float, halo_gain: float,
                    size_boost: float, size_radius: float, size_gamma: float,
                    halo_sep: float, halo_gamma: float):
    blur_core = mask_bin.filter(ImageFilter.GaussianBlur(radius=core_radius))
    blur_halo = mask_bin.filter(ImageFilter.GaussianBlur(radius=halo_radius))

    if size_boost>0.0 and size_radius>0:
        density = mask_bin.filter(ImageFilter.BoxBlur(radius=size_radius)).convert('L')
        density = apply_gamma_L(density, max(0.05, size_gamma))
        boost = scale_L(density, size_boost * wtime)
        blur_core = ImageChops.add(blur_core, boost, scale=1.0)
        blur_halo = ImageChops.add(blur_halo, boost, scale=1.0)

    halo_sep = max(0.0, min(1.0, halo_sep))
    halo_only = ImageChops.subtract(blur_halo, blur_core.point(lambda v:int(v*halo_sep)), scale=1.0, offset=0) if halo_sep>0 else blur_halo
    halo_only = apply_gamma_L(halo_only, halo_gamma)

    core_alpha = scale_L(blur_core, wtime*max(0.0, core_gain))
    halo_alpha = scale_L(halo_only, wtime*max(0.0, halo_gain))
    core_alpha = ImageChops.multiply(core_alpha, interior_mask)
    halo_alpha = ImageChops.multiply(halo_alpha, interior_mask)
    return core_alpha, halo_alpha

def layer_from_alpha(color: Tuple[int,int,int], alphaL: Image.Image) -> Image.Image:
    w,h = alphaL.size
    solid = Image.new('RGB',(w,h),color)
    return Image.composite(solid, Image.new('RGB',(w,h),(0,0,0)), alphaL)

# ---------- compositor with per-key styles ----------

def build_composite_with_styles(base_bg_rgba: Image.Image,
                                mask_source_rgb: Image.Image,
                                hour_1_24: int,
                                keys: List[Tuple[int,int,int]],
                                styles: Dict[Tuple[int,int,int], Dict[str,object]],
                                # global defaults:
                                core_radius: float, halo_radius: float,
                                core_gain: float, halo_gain: float,
                                core_color: Tuple[int,int,int],
                                glow_color: Tuple[int,int,int],
                                size_boost: float, size_radius: float, size_gamma: float,
                                clip_interior: bool, clip_erode: int,
                                highlight_gain: float, blend_mode: str,
                                halo_sep: float, halo_gamma: float) -> Image.Image:
    wtime = hour_weight_lights(hour_1_24)
    if wtime <= 0.0:  # just return unchanged base (still save with palette fix later)
        return base_bg_rgba.copy()

    interior = build_interior_mask(base_bg_rgba, clip_interior, clip_erode)
    comp = base_bg_rgba.convert('RGB')

    # Ensure every styled key is included in keys
    keys_set = {k for k in keys}
    for k in styles.keys():
        keys_set.add(k)
    keys_order = list(keys_set)

    for key_rgb in keys_order:
        # Per-key mask
        mask_bin = color_equal(mask_source_rgb, key_rgb)
        if mask_bin.getbbox() is None:
            continue

        st = styles.get(key_rgb, {})
        k_core_color = st.get("core_color", core_color)
        k_glow_color = st.get("glow_color", glow_color)
        k_core_gain  = float(st.get("core_gain", core_gain))
        k_halo_gain  = float(st.get("halo_gain", halo_gain))
        k_core_rad   = float(st.get("core_radius", core_radius))
        k_halo_rad   = float(st.get("halo_radius", halo_radius))
        k_halo_sep   = float(st.get("halo_sep", halo_sep))
        k_halo_gamma = float(st.get("halo_gamma", halo_gamma))
        k_size_boost = float(st.get("size_boost", size_boost))
        k_size_radius= float(st.get("size_radius", size_radius))
        k_size_gamma = float(st.get("size_gamma", size_gamma))
        k_highlight  = float(st.get("highlight", highlight_gain))

        core_alpha, halo_alpha = build_glow_maps(
            mask_bin, interior, wtime,
            k_core_rad, k_halo_rad,
            k_core_gain, k_halo_gain,
            k_size_boost, k_size_radius, k_size_gamma,
            k_halo_sep, k_halo_gamma
        )
        core_layer = layer_from_alpha(k_core_color, core_alpha)
        halo_layer = layer_from_alpha(k_glow_color, halo_alpha)

        if blend_mode == "add":
            comp = ImageChops.add(comp, core_layer, scale=1.0)
            comp = ImageChops.add(comp, halo_layer, scale=1.0)
        else:
            comp = screen_blend(comp, core_layer)
            comp = screen_blend(comp, halo_layer)

        if k_highlight > 0.0:
            hl = scale_L(core_alpha, k_highlight)
            comp = ImageChops.add(comp, Image.composite(Image.new('RGB',comp.size,(255,255,255)),
                                                        Image.new('RGB',comp.size,(0,0,0)), hl), scale=1.0)

    return comp.convert('RGBA')

# ---------- save with palette fixes ----------

def save_with_palette_and_magic(comp_rgba: Image.Image, out_path: str, base_for_magic_rgb: Image.Image) -> None:
    imP = quantize_to_p_256(comp_rgba)
    pal_after = ensure_palette_has_colors(imP, [MAGENTA, GREEN]); put_palette(imP, pal_after)
    replacement_idx = force_magenta_at_255(imP)
    pal_after = get_palette(imP); idx_grn = find_color_index(pal_after, GREEN)
    dst_px = imP.load(); src_px = base_for_magic_rgb.load(); w,h = imP.size
    for y in range(h):
        for x in range(w):
            r,g,b = src_px[x,y]; cur = dst_px[x,y]
            if (r,g,b)==MAGENTA: dst_px[x,y]=255
            elif (r,g,b)==GREEN and idx_grn!=-1: dst_px[x,y]=idx_grn
            elif cur==255 and replacement_idx!=255: dst_px[x,y]=replacement_idx
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    imP.save(out_path, format='PCX')

# ---------- driver ----------

def process_tree(data_dir: str, noon_subfolder: str,
                 light_keys: List[Tuple[int,int,int]],
                 styles: Dict[Tuple[int,int,int], Dict[str,object]],
                 core_radius: float, halo_radius: float,
                 core_gain: float, halo_gain: float,
                 core_color: Tuple[int,int,int], glow_color: Tuple[int,int,int],
                 size_boost: float, size_radius: float, size_gamma: float,
                 clip_interior: bool, clip_erode: int,
                 highlight_gain: float, blend_mode: str,
                 halo_sep: float, halo_gamma: float,
                 only_hour: Optional[int]=None, only_file: Optional[str]=None) -> None:

    base_dir = os.path.join(data_dir, noon_subfolder)
    if not os.path.isdir(base_dir): raise SystemExit(f"No such folder: {base_dir}")

    hours = [h for h in range(100, 2500, 100) if h != 1200]
    if only_hour is not None:
        if only_hour == 1200: raise SystemExit("1200 is the base. Choose another hour.")
        if only_hour % 100 != 0 or only_hour < 100 or only_hour > 2400:
            raise SystemExit("--only-hour must be one of 100,200,...,2400.")
        hours = [only_hour]

    norm_only = os.path.normcase(only_file) if only_file else None

    # Ensure styled keys are included even if not explicitly listed in --light-key
    for k in styles.keys():
        if k not in light_keys:
            light_keys.append(k)

    for dirpath, _, filenames in os.walk(base_dir):
        rel_dir = os.path.relpath(dirpath, base_dir); rel_dir = "" if rel_dir=="." else rel_dir
        for hhh in hours:
            os.makedirs(os.path.join(data_dir, f"{hhh:04d}", rel_dir), exist_ok=True)

        lights_files = [f for f in filenames if f.lower().endswith("_lights.pcx")]
        for lights_name in lights_files:
            if norm_only:
                rel_file = os.path.normcase(os.path.join(rel_dir, lights_name))
                if norm_only != os.path.normcase(lights_name) and norm_only != rel_file: continue

            plain_name = lights_name[:-len("_lights.pcx")] + ".pcx"
            noon_lights = os.path.join(dirpath, lights_name)
            noon_plain  = os.path.join(dirpath, plain_name)

            for hhh in hours:
                hour_1_24 = hhh // 100
                hour_dir  = os.path.join(data_dir, f"{hhh:04d}", rel_dir)
                out_lights= os.path.join(hour_dir, lights_name)
                out_plain = os.path.join(hour_dir, plain_name)

                if os.path.exists(out_plain):
                    base_bg = Image.open(out_plain).convert('RGBA'); base_rgb = Image.open(out_plain).convert('RGB')
                elif os.path.exists(noon_plain):
                    base_bg = Image.open(noon_plain).convert('RGBA'); base_rgb = Image.open(noon_plain).convert('RGB')
                else:
                    base_bg = Image.open(noon_lights).convert('RGBA'); base_rgb = Image.open(noon_lights).convert('RGB')

                mask_src = Image.open(noon_lights).convert('RGB')

                comp = build_composite_with_styles(
                    base_bg_rgba=base_bg, mask_source_rgb=mask_src, hour_1_24=hour_1_24,
                    keys=light_keys, styles=styles,
                    core_radius=core_radius, halo_radius=halo_radius,
                    core_gain=core_gain, halo_gain=halo_gain,
                    core_color=core_color, glow_color=glow_color,
                    size_boost=size_boost, size_radius=size_radius, size_gamma=size_gamma,
                    clip_interior=clip_interior, clip_erode=clip_erode,
                    highlight_gain=highlight_gain, blend_mode=blend_mode,
                    halo_sep=halo_sep, halo_gamma=halo_gamma
                )

                save_with_palette_and_magic(comp, out_lights, base_rgb)
                if is_night_hour(hour_1_24):
                    save_with_palette_and_magic(comp, out_plain, base_rgb)
                else:
                    if not os.path.exists(out_plain):
                        if os.path.exists(noon_plain):
                            os.makedirs(os.path.dirname(out_plain), exist_ok=True)
                            shutil.copyfile(noon_plain, out_plain)
                        else:
                            save_with_palette_and_magic(base_bg, out_plain, base_rgb)

def main():
    ap = argparse.ArgumentParser(description="Civ 3 city lights compositor (multi light-keys + per-key styles)")
    ap.add_argument("--data", required=True)
    ap.add_argument("--noon", default="1200")
    ap.add_argument("--only-hour", type=int)
    ap.add_argument("--only-file")

    ap.add_argument("--light-key", action="append", default=["#00feff"],
                    help="Repeat for multiple (accepts #rrggbb or R,G,B)")
    ap.add_argument("--light-style", action="append", default=[],
                    help='Per-key overrides: "key=#00feff; core=#fff87a; glow=#ff8a20; core_gain=1.6; halo_gain=4.0; halo_sep=0.75; halo_gamma=1.35; core_radius=1.2; halo_radius=14; highlight=0.6"')

    # Global defaults (each key can override)
    ap.add_argument("--core-radius", type=float, default=1.1)
    ap.add_argument("--halo-radius", type=float, default=13.0)
    ap.add_argument("--core-gain",   type=float, default=2.1)
    ap.add_argument("--halo-gain",   type=float, default=20.0)
    ap.add_argument("--core-color", type=str, default="#ff8a20")
    ap.add_argument("--glow-color", type=str, default="#dc6a00")
    ap.add_argument("--highlight-gain", type=float, default=0.6)

    ap.add_argument("--size-boost",  type=float, default=1.1)
    ap.add_argument("--size-radius", type=float, default=3.5)
    ap.add_argument("--size-gamma",  type=float, default=0.75)

    ap.add_argument("--clip-interior", type=str, default="yes", choices=["yes","no"])
    ap.add_argument("--clip-erode", type=int, default=0)

    ap.add_argument("--halo-sep", type=float, default=0.75)
    ap.add_argument("--halo-gamma", type=float, default=1.4)
    ap.add_argument("--blend-mode", type=str, default="screen", choices=["screen","add"])

    args = ap.parse_args()

    light_keys = parse_rgb_list(args.light_key)
    styles = parse_styles(args.light_style)
    core_color = parse_rgb_one(args.core_color)
    glow_color = parse_rgb_one(args.glow_color)

    process_tree(
        data_dir=args.data, noon_subfolder=args.noon,
        light_keys=light_keys, styles=styles,
        core_radius=args.core_radius, halo_radius=args.halo_radius,
        core_gain=args.core_gain, halo_gain=args.halo_gain,
        core_color=core_color, glow_color=glow_color,
        size_boost=args.size_boost, size_radius=args.size_radius, size_gamma=args.size_gamma,
        clip_interior=(args.clip_interor if hasattr(args,'clip_interor') else args.clip_interior)=="yes",
        clip_erode=args.clip_erode,
        highlight_gain=args.highlight_gain, blend_mode=args.blend_mode,
        halo_sep=args.halo_sep, halo_gamma=args.halo_gamma,
        only_hour=args.only_hour, only_file=args.only_file
    )

if __name__ == "__main__":
    main()
