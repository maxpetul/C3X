#!/usr/bin/env python3
"""
civ3_seasons_pcx.py — v1.1 (strong foliage mask + punchier fall & spring)

Changes vs v1.0:
- New foliage_weight(): catches greens even when low-sat by mixing HSV hue and RGB G-dominance.
- Fall: 3-way ramp by brightness (brown/orange/yellow) with stronger shifts + desat/darken.
- Spring: stronger young-green push; blossom tint improved (near-whites only).
- Tuned defaults keep indices intact and still ignore light keys.
"""

import argparse
import shutil
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple
from PIL import Image
import colorsys
# --------------------------- CLI & helpers ---------------------------

def parse_rgb(s: str) -> Tuple[int, int, int]:
    s = s.strip()
    if s.startswith("#"):
        s = s[1:]
        if len(s) != 6:
            raise ValueError(f"Bad hex color: #{s}")
        return tuple(int(s[i:i+2], 16) for i in (0, 2, 4))  # type: ignore
    if "," in s:
        parts = [p.strip() for p in s.split(",")]
        if len(parts) != 3:
            raise ValueError(f"Bad RGB list: {s}")
        return tuple(int(p) for p in parts)  # type: ignore
    raise ValueError(f"Unrecognized color format: {s}")

def month_labels() -> List[str]:
    return [f"{m:02d}" for m in range(1, 13)]

def clamp_byte(x: float) -> int:
    return int(max(0, min(255, round(x))))

def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * max(0.0, min(1.0, t))

# --------------------------- Palette helpers ---------------------------

def get_palette(img: Image.Image) -> List[int]:
    pal = img.getpalette()
    if pal is None:
        raise ValueError("Image has no palette (mode must be 'P').")
    if len(pal) < 256 * 3:
        pal = pal + [0] * (256 * 3 - len(pal))
    return pal[:256 * 3]

def set_palette(img: Image.Image, pal: Sequence[int]) -> None:
    if len(pal) != 256 * 3:
        raise ValueError("Palette must be exactly 256*3 entries.")
    img.putpalette(list(pal))

def find_color_index(pal: Sequence[int], color: Tuple[int, int, int]) -> int:
    cr, cg, cb = color
    for i in range(256):
        r, g, b = pal[3*i:3*i+3]
        if r == cr and g == cg and b == cb:
            return i
    return -1

def remap_green_to_magenta_indices(img: Image.Image, pal: Sequence[int]) -> Image.Image:
    green_idx = find_color_index(pal, (0, 255, 0))
    magenta_idx = find_color_index(pal, (255, 0, 255))
    if green_idx < 0 or magenta_idx < 0 or green_idx == magenta_idx:
        return img
    lut = [magenta_idx if i == green_idx else i for i in range(256)]
    return img.point(lut, mode="P")

# --------------------------- Circular gaussians by month ---------------------------

def _gauss_circular(x: float, mu: float, sigma: float, period: float = 12.0) -> float:
    from math import exp
    x = ((x - 1) % period) + 1
    mu = ((mu - 1) % period) + 1
    d = abs(x - mu)
    d = min(d, period - d)
    if sigma <= 0.0:
        return 0.0
    return exp(-0.5 * (d / sigma) ** 2)

def seasonal_weights(month_val: int, season_sigma: float) -> dict:
    m = int(month_val)
    w = max(_gauss_circular(m, 12.0, season_sigma),
            _gauss_circular(m, 1.0, season_sigma))
    sp = _gauss_circular(m, 4.0, season_sigma)
    su = _gauss_circular(m, 7.0, season_sigma)
    fa = _gauss_circular(m, 10.0, season_sigma)
    return dict(winter=min(1.0, w),
                spring=min(1.0, sp),
                summer=min(1.0, su),
                fall=min(1.0, fa))

# --------------------------- Global RGB post-processing ---------------------------

def _apply_contrast_rgb(rgb: Tuple[float, float, float], contrast: float) -> Tuple[float, float, float]:
    r, g, b = rgb
    r = 128 + (r - 128) * contrast
    g = 128 + (g - 128) * contrast
    b = 128 + (b - 128) * contrast
    return r, g, b

def _apply_saturation_hsv(h: float, s: float, v: float, sat: float) -> Tuple[float, float, float]:
    return (h, max(0.0, min(1.0, s * sat)), v)

# --------------------------- Utilities for hue ---------------------------

def _deg(h: float) -> float:
    return (h % 1.0) * 360.0

def _h01(deg: float) -> float:
    return (deg % 360.0) / 360.0

def _range_weight_deg(h_deg: float, a: float, b: float) -> float:
    """Soft membership of hue (deg) in [a,b] with 20° feather."""
    a %= 360.0; b %= 360.0; h = h_deg % 360.0
    def seg(x, s, e):
        if s <= e:
            return 1.0 if (s <= x <= e) else 0.0
        return 1.0 if (x >= s or x <= e) else 0.0
    core = seg(h, a, b)
    if core == 1.0:
        return 1.0
    # Feather in/out by 20°
    pad = 20.0
    if seg(h, (a - pad) % 360, a) or seg(h, b, (b + pad) % 360):
        return 0.5
    return 0.0

# --------------------------- NEW: foliage detection ---------------------------

def foliage_weight_rgb_hsv(r: int, g: int, b: int, h: float, s: float, v: float) -> float:
    """
    Blend HSV "greenish" membership with RGB G-dominance so low-sat greens get caught.
    - hue window ~50°..190° (covers yellow-green to cyan-ish greens)
    - g-dominance = max(0, G - max(R,B)) / 255
    """
    hd = _deg(h)
    hue_w = _range_weight_deg(hd, 50, 190)  # 0, 0.5, or 1
    g_dom = max(0, g - max(r, b)) / 255.0
    # Weight s lightly (greens often are mid/low-sat in Civ3 palettes)
    s_w = min(1.0, s * 1.2)
    # Final: ensure at least some response when hue matches OR g is dominant
    return max(hue_w * (0.4 + 0.6 * s_w), 0.6 * g_dom)

# --------------------------- Seasonal HSV operators (stronger) ---------------------------

def _apply_fall_strong(r: int, g: int, b: int, h: float, s: float, v: float, strength: float) -> Tuple[float, float, float]:
    """
    Aggressive fall mapping for foliage:
      - Dark greens → brown (~20°)
      - Mid → orange (~35°)
      - Bright → yellow (~50°)
      - Desaturate slightly; darken a touch for ground/grass feel.
    Applies with weight = foliage_weight * strength (clamped).
    """
    if strength <= 0.0:
        return h, s, v

    w_f = foliage_weight_rgb_hsv(r, g, b, h, s, v) * min(1.0, strength)
    if w_f <= 0.0:
        return h, s, v

    # Bucket by brightness
    if v < 0.35:
        target_deg = 20.0   # brownish
        s_mul = 0.85
        v_mul = 0.92
    elif v < 0.65:
        target_deg = 35.0   # orange
        s_mul = 0.90
        v_mul = 0.95
    else:
        target_deg = 50.0   # yellow
        s_mul = 0.95
        v_mul = 0.98

    # Interpolate hue strongly; keep some original to preserve variation
    hd = _deg(h)
    alpha = 0.65 * w_f + 0.25 * (s)  # extra pull if original was saturated
    new_h = _h01(lerp(hd, target_deg, max(0.0, min(1.0, alpha))))
    new_s = max(0.0, min(1.0, s * lerp(1.0, s_mul, w_f)))
    new_v = max(0.0, min(1.0, v * lerp(1.0, v_mul, w_f)))
    return new_h, new_s, new_v

def _apply_winter(h: float, s: float, v: float, snow: float, cool: float) -> Tuple[float, float, float]:
    if snow <= 0.0 and cool <= 0.0:
        return h, s, v
    hd = _deg(h)
    is_water = (190 <= hd <= 260)
    s_bias = min(1.0, s * 1.5)
    whiten = 0.75 * snow * s_bias
    new_s = max(0.0, s * (1.0 - 0.55 * snow * s_bias))
    new_v = min(1.0, v + (1.0 - v) * whiten)
    if is_water:
        new_s *= (1.0 - 0.25 * snow)
        new_v *= (1.0 - 0.12 * snow)
    if cool > 0.0:
        new_h = _h01(lerp(hd, 210.0, 0.18 * cool))
    else:
        new_h = h
    return new_h, new_s, new_v

def _apply_spring_strong(r: int, g: int, b: int, h: float, s: float, v: float, bloom: float) -> Tuple[float, float, float]:
    """
    Spring:
      - Foliage: push to young yellow-green (~95°) + sat/val boost.
      - Blossom: near-whites (v>0.88 & s<0.10) get a tiny shift toward 330° and a small sat lift.
    """
    if bloom <= 0.0:
        return h, s, v

    w_f = foliage_weight_rgb_hsv(r, g, b, h, s, v) * min(1.0, bloom)
    hd = _deg(h)

    # Foliage push
    if w_f > 0.0:
        target = 95.0
        alpha = min(1.0, 0.60 * w_f + 0.20 * (1.0 - s))  # push harder if original was dull
        h = _h01(lerp(hd, target, alpha))
        s = min(1.0, s * (1.0 + 0.35 * w_f))
        v = min(1.0, v * (1.0 + 0.12 * w_f))

    # Blossom hint (only near-white neutrals so we don't pink snow/ice)
    if v > 0.88 and s < 0.10:
        t = 0.08 * min(1.0, bloom)
        h = _h01(lerp(_deg(h), 330.0, t))
        s = min(1.0, s + 0.05 * bloom)

    return h, s, v

def _apply_summer(h: float, s: float, v: float, dry: float) -> Tuple[float, float, float]:
    if dry <= 0.0:
        return h, s, v
    hd = _deg(h)
    # Only nudge greens
    if 50 <= hd <= 170:
        new_h = _h01(lerp(hd, 80.0, 0.35 * dry))
        new_s = max(0.0, s * (1.0 - 0.10 * dry))
        return new_h, new_s, v
    return h, s, v

# --------------------------- Core per-palette transform ---------------------------

def adjust_palette_for_month(
    pal: List[int],
    month_val: int,
    reserved_colors: Iterable[Tuple[int, int, int]],
    *,
    season_sigma: float,
    fall_strength: float,
    winter_snow: float,
    spring_bloom: float,
    summer_dry: float,
    global_sat: float,
    global_contrast: float,
) -> List[int]:

    W = seasonal_weights(month_val, season_sigma)
    eff_fall   = fall_strength   * W["fall"]
    eff_winter = winter_snow     * W["winter"]
    eff_spring = spring_bloom    * W["spring"]
    eff_summer = summer_dry      * W["summer"]

    out = pal[:]
    reserved = set(reserved_colors)

    for i in range(256):
        r, g, b = pal[3 * i:3 * i + 3]
        if (r, g, b) in reserved:
            out[3 * i + 0] = r
            out[3 * i + 1] = g
            out[3 * i + 2] = b
            continue

        rf, gf, bf = r / 255.0, g / 255.0, b / 255.0
        h, s, v = colorsys.rgb_to_hsv(rf, gf, bf)

        # Stronger seasonal ops
        h, s, v = _apply_fall_strong(r, g, b, h, s, v, eff_fall)
        h, s, v = _apply_winter(h, s, v, eff_winter, eff_winter)
        h, s, v = _apply_spring_strong(r, g, b, h, s, v, eff_spring)
        h, s, v = _apply_summer(h, s, v, eff_summer)

        # Global tweak
        h, s, v = _apply_saturation_hsv(h, s, v, global_sat)

        nr, ng, nb = colorsys.hsv_to_rgb(h, s, max(0.0, min(1.0, v)))
        nr *= 255.0; ng *= 255.0; nb *= 255.0
        nr, ng, nb = _apply_contrast_rgb((nr, ng, nb), global_contrast)

        out[3 * i + 0] = clamp_byte(nr)
        out[3 * i + 1] = clamp_byte(ng)
        out[3 * i + 2] = clamp_byte(nb)

    return out

# --------------------------- File ops ---------------------------

def copy_base_to_label(base_dir: Path, base_month: str, label: str) -> List[Path]:
    src_dir = base_dir / base_month
    dst_dir = base_dir / label
    dst_dir.mkdir(parents=True, exist_ok=True)
    copied: List[Path] = []
    for src in src_dir.glob("*.pcx"):
        dst = dst_dir / src.name
        shutil.copy2(src, dst)
        copied.append(dst)
    return copied

def label_to_month_value(label: str) -> int:
    m = int(label)
    if not (1 <= m <= 12):
        raise ValueError(f"Bad month label: {label}")
    return m

def process_month_label(
    label: str,
    base_dir: Path,
    base_month: str,
    reserved_colors: Iterable[Tuple[int, int, int]],
    *,
    do_index_remap: bool,
    season_sigma: float,
    fall_strength: float,
    winter_snow: float,
    spring_bloom: float,
    summer_dry: float,
    global_sat: float,
    global_contrast: float,
) -> None:
    month_val = label_to_month_value(label)
    copied_files = copy_base_to_label(base_dir, base_month, label)

    for pcx_path in copied_files:
        with Image.open(pcx_path) as im:
            if im.mode != "P":
                im = im.convert("P")
            pal = get_palette(im)

            if do_index_remap:
                im = remap_green_to_magenta_indices(im, pal)
                pal = get_palette(im)

            new_pal = adjust_palette_for_month(
                pal, month_val, reserved_colors,
                season_sigma=season_sigma,
                fall_strength=fall_strength,
                winter_snow=winter_snow,
                spring_bloom=spring_bloom,
                summer_dry=summer_dry,
                global_sat=global_sat,
                global_contrast=global_contrast,
            )
            set_palette(im, new_pal)
            im.save(pcx_path, format="PCX")

# --------------------------- Main ---------------------------

def main():
    p = argparse.ArgumentParser(description="Civ3 PCX seasonal generator (palette-only) with strong foliage mapping.")
    p.add_argument("--data", required=True,
                   help="Parent folder containing the base month folder and sibling month folders (e.g., Seasons).")
    p.add_argument("--base-month", default="06",
                   help='Month folder to use as the unchanged base (default "06" for June).')
    p.add_argument("--only-month", default=None,
                   help='Process only one month label "01".."12" (e.g., 11 for November).')

    p.add_argument("--light-key", action="append", default=[],
                   help="Reserved color that must not change. Repeatable. '#RRGGBB' or 'R,G,B'.")

    p.add_argument("--season-sigma", type=float, default=1.2,
                   help="Gaussian width in months for season blending (higher = broader).")
    p.add_argument("--fall-strength", type=float, default=1.0,
                   help="Strength of foliage shift in fall (increase for punch).")
    p.add_argument("--winter-snow", type=float, default=0.85,
                   help="Strength of snow whitening + coolness in winter.")
    p.add_argument("--spring-bloom", type=float, default=0.80,
                   help="Strength of spring freshening + blossom hint.")
    p.add_argument("--summer-dry", type=float, default=0.35,
                   help="Strength of late-summer dryness (greens to olive).")

    p.add_argument("--global-sat", type=float, default=1.04,
                   help="Global saturation multiplier after seasonal edits.")
    p.add_argument("--global-contrast", type=float, default=1.03,
                   help="Global contrast multiplier around mid 128 (1.0 = none).")

    g = p.add_mutually_exclusive_group()
    g.add_argument("--keep-green-index", action="store_true",
                   help="Do NOT remap green index to magenta; leave pixel indices unchanged.")
    g.add_argument("--map-green-index", dest="map_green_index", action="store_true",
                   help="Force remap green index to magenta (default behavior).")

    args = p.parse_args()

    base_dir = Path(args.data).expanduser().resolve()
    base_src = base_dir / args.base_month
    if not base_src.is_dir():
        raise SystemExit(f"Base month folder not found: {base_src}")

    reserved: List[Tuple[int, int, int]] = [(255, 0, 255), (0, 255, 0)]
    for s in args.light_key:
        reserved.append(parse_rgb(s))

    do_index_remap = not args.keep_green_index or args.map_green_index
    labels = [lbl for lbl in month_labels() if lbl != args.base_month]

    if args.only_month:
        only = args.only_month.zfill(2)
        if only == args.base_month:
            print(f"--only-month {only} is the base source; nothing to do.")
            return
        if only not in labels:
            raise SystemExit(f"--only-month must be one of: {', '.join(labels)}")
        labels = [only]

    print(f"Base: {base_dir}")
    print(f"Base month source: {base_src}")
    print(f"Index remap green→magenta: {'ON' if do_index_remap else 'OFF'}")
    print(f"Generating months: {', '.join(labels)}")

    for lbl in labels:
        print(f"  -> Generating month {lbl} ...")
        process_month_label(
            lbl, base_dir, args.base_month, reserved,
            do_index_remap=do_index_remap,
            season_sigma=args.season_sigma,
            fall_strength=args.fall_strength,
            winter_snow=args.winter_snow,
            spring_bloom=args.spring_bloom,
            summer_dry=args.summer_dry,
            global_sat=args.global_sat,
            global_contrast=args.global_contrast,
        )

    print("Done.")

if __name__ == "__main__":
    main()
