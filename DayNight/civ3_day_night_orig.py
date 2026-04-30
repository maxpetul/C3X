#!/usr/bin/env python3
"""
civ3_daynight_pcx.py — v4.3 (stronger nighttime blue + stable noon-neutral zone)

Highlights:
- Half-hour (or any divisor of an hour) time slices via --step-minutes.
- Green→Magenta index remap (on by default).
- Palette-only tinting; indices preserved (except the optional index remap).
- Noon-neutral zone keeps ~10:00–14:00 close to base 1200.
- NEW: Stronger nighttime blue response using the existing --blue knob,
       with an additional night-only hue shift (blue up, red/green down).

Tested knobs (your current favorites work unchanged):
  --warmth 1.05  --blue 2.0 --darkness 1.1 --desat 0.85 --sat 1.2 --contrast 1.08
  --sunrise-center 5.0 --sunset-center 18.0 --twilight-width 3.0
  --noon-blend 0.7 --noon-sigma 1.0
"""

import argparse
import shutil
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

from PIL import Image


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


def time_labels(step_minutes: int) -> List[str]:
    """
    Generate HHMM labels from step to 23:59 stepwise; exclude 0000; add 2400.
    For step=60: 0100..2300 + 2400
    For step=30: 0030, 0100, 0130..2330 + 2400
    """
    if step_minutes <= 0 or 60 % step_minutes != 0:
        raise ValueError("--step-minutes must be a positive divisor of 60 (e.g., 5,10,15,20,30,60).")
    labels: List[str] = []
    total_minutes = 24 * 60
    m = step_minutes
    while m < total_minutes:
        h = m // 60
        mm = m % 60
        labels.append(f"{h:02d}{mm:02d}")
        m += step_minutes
    labels.append("2400")
    return labels


def clamp_byte(x: float) -> int:
    return int(max(0, min(255, round(x))))


# --------------------------- TONEMAP MODEL ---------------------------

def _gauss(x: float, mu: float, sigma: float) -> float:
    from math import exp
    if sigma <= 0.0:
        return 0.0
    return exp(-0.5 * ((x - mu) / sigma) ** 2)


def hour_adjustments(
    hour_value: float,
    *,
    warmth_scale: float,
    blue_scale: float,
    darkness_scale: float,
    desat_scale: float,
    sunrise_center: float,
    sunset_center: float,
    twilight_sigma: float,
) -> dict:
    """
    hour_value can be fractional (e.g., 19.5 for 19:30). Period is 24h.
    Returns a dict with per-channel multipliers, gray blend, brightness,
    and 'night' + 'blue_push' factors for night-only hue shift.
    """
    from math import cos, pi

    h = hour_value % 24.0

    # Daylight (cosine): 1 at noon, ~0 at midnight
    daylight = max(0.0, cos(pi * (h - 12.0) / 12.0))  # 0..1
    night = 1.0 - daylight

    # Warmth around sunrise/sunset
    warmth = 0.85 * (_gauss(h, sunrise_center, twilight_sigma) +
                     _gauss(h, sunset_center, twilight_sigma))
    warmth *= warmth_scale

    # Brightness: darker at night; scaled by darkness_scale (>1 = darker nights)
    base_brightness = 0.60 + 0.40 * daylight
    night_darkening = 1.0 - (0.12 * (darkness_scale - 1.0) * night)
    brightness = base_brightness * night_darkening

    # Channel multipliers
    r_mul = 0.97 + 0.12 * daylight + 0.30 * warmth
    g_mul = 0.97 + 0.12 * daylight + 0.12 * warmth
    b_mul = 0.97 + 0.12 * daylight - 0.18 * warmth + (0.28 * blue_scale) * night

    # Desaturation (toward gray) stronger at night; scaled by desat_scale
    gray_blend = (0.10 + 0.50 * night) * desat_scale
    gray_blend = min(max(gray_blend, 0.0), 0.85)

    # Night-only blue push factor (extra hue shift at night)
    #  - grows with (blue_scale - 1) and with 'night'
    #  - kept separate from b_mul so daytime remains unchanged
    blue_push = max(0.0, blue_scale - 1.0) * night  # 0 at day, up to (blue-1) at night

    # Clamp gentle bounds
    r_mul = min(max(r_mul, 0.65), 1.45)
    g_mul = min(max(g_mul, 0.65), 1.40)
    b_mul = min(max(b_mul, 0.65), 1.55)

    return dict(
        brightness=brightness,
        r_mul=r_mul,
        g_mul=g_mul,
        b_mul=b_mul,
        gray_blend=gray_blend,
        night=night,
        blue_push=blue_push,
    )


def _apply_saturation(rgb: Tuple[float, float, float], sat: float) -> Tuple[float, float, float]:
    r, g, b = rgb
    gray = (r + g + b) / 3.0
    r = gray + (r - gray) * sat
    g = gray + (g - gray) * sat
    b = gray + (b - gray) * sat
    return r, g, b


def _apply_contrast(rgb: Tuple[float, float, float], contrast: float) -> Tuple[float, float, float]:
    r, g, b = rgb
    r = 128 + (r - 128) * contrast
    g = 128 + (g - 128) * contrast
    b = 128 + (b - 128) * contrast
    return r, g, b


def _apply_night_blue_push(
    rgb: Tuple[float, float, float],
    blue_push: float,
) -> Tuple[float, float, float]:
    """
    Extra night-only hue shift:
      - Slightly reduces R and G
      - Increases B
    'blue_push' is ~ (blue_scale - 1) * night, so this is zero in daytime.
    Coefficients tuned to be visible but not cartoonish; adjust if needed.
    """
    if blue_push <= 0.0:
        return rgb
    r, g, b = rgb
    c = blue_push  # c in [0..(blue-1)] scaled by night
    # Gentle but noticeable: at c=1 (e.g., --blue 2.0 at full night)
    r *= (1.0 - 0.15 * c)
    g *= (1.0 - 0.10 * c)
    b *= (1.0 + 0.35 * c)
    return r, g, b


def tint_rgb(
    rgb: Tuple[int, int, int],
    params: dict,
    *,
    sat_boost: float,
    contrast: float,
) -> Tuple[int, int, int]:
    r, g, b = rgb
    r_f = r * params["r_mul"] * params["brightness"]
    g_f = g * params["g_mul"] * params["brightness"]
    b_f = b * params["b_mul"] * params["brightness"]

    gray = (r_f + g_f + b_f) / 3.0
    t = params["gray_blend"]
    r_f = (1 - t) * r_f + t * gray
    g_f = (1 - t) * g_f + t * gray
    b_f = (1 - t) * b_f + t * gray

    # Global saturation, then extra night-only blue hue push, then contrast
    r_f, g_f, b_f = _apply_saturation((r_f, g_f, b_f), sat_boost)
    r_f, g_f, b_f = _apply_night_blue_push((r_f, g_f, b_f), params.get("blue_push", 0.0))
    r_f, g_f, b_f = _apply_contrast((r_f, g_f, b_f), contrast)

    return (clamp_byte(r_f), clamp_byte(g_f), clamp_byte(b_f))


# --------------------------- Noon-neutral weighting ---------------------------

def _smoothstep01(t: float) -> float:
    """Cubic smoothstep on [0,1]."""
    if t <= 0.0:
        return 0.0
    if t >= 1.0:
        return 1.0
    return t * t * (3.0 - 2.0 * t)


def _interval_membership(x: float, a: float, b: float, soft: float) -> float:
    """
    Smooth membership of time-of-day x (0..24) inside interval [a,b] (hours),
    with soft edge width 'soft' (hours). Supports wrapped intervals (a>b).
    Returns 0..1.
    """
    x = x % 24.0
    a = a % 24.0
    b = b % 24.0
    soft = max(0.0, soft)

    def segment_membership(x: float, s: float, e: float, soft: float) -> float:
        # Non-wrapped segment s <= e in [0,24]
        if soft <= 0.0:
            return 1.0 if (s <= x <= e) else 0.0

        # Left ramp [s-soft, s]
        if s - soft <= x < s:
            t = (x - (s - soft)) / soft  # 0..1
            return _smoothstep01(t)

        # Core [s, e]
        if s <= x <= e:
            return 1.0

        # Right ramp [e, e+soft]
        if e < x <= e + soft:
            t = (x - e) / soft  # 0..1
            return 1.0 - _smoothstep01(t)

        return 0.0

    if a <= b:
        return segment_membership(x, a, b, soft)
    else:
        # Wrapped interval: union of [a,24) and [0,b]
        m1 = segment_membership(x, a, 24.0, soft)
        m2 = segment_membership(x, 0.0, b, soft)
        return max(m1, m2)


def _noon_weight(hour_value: float, blend: float, sigma: float,
                 w_start: float, w_end: float, w_soft: float) -> float:
    """
    Combined noon weight in 0..1:
      - Gaussian around 12:00 with width 'sigma' (hours), scaled by 'blend'.
      - Smooth interval window [w_start, w_end] with soft edges 'w_soft',
        also scaled by 'blend'.
      - The final weight is max of both components, clamped 0..1.
    """
    try:
        h = hour_value % 24.0
    except Exception:
        h = 0.0
    blend = float(blend) if blend is not None else 0.0
    sigma = float(sigma) if sigma is not None else 0.0
    w_start = float(w_start) if w_start is not None else 10.0
    w_end = float(w_end) if w_end is not None else 14.0
    w_soft = max(0.0, float(w_soft) if w_soft is not None else 0.7)

    if blend <= 0.0:
        return 0.0

    # Gaussian around noon
    from math import exp
    d = abs(h - 12.0)
    d = min(d, 24.0 - d)
    g = exp(-0.5 * (d / sigma) ** 2) if sigma > 0.0 else 0.0
    g *= blend

    # Smooth window
    window_m = _interval_membership(h, w_start, w_end, w_soft) * blend

    w = max(g, window_m)
    if w < 0.0:
        return 0.0
    if w > 1.0:
        return 1.0
    return w


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


# --------------------------- Core operations ---------------------------

def adjust_palette_for_time(
    pal: List[int],
    hour_value: float,
    reserved_colors: Iterable[Tuple[int, int, int]],
    *,
    # look controls
    warmth_scale: float,
    blue_scale: float,
    darkness_scale: float,
    desat_scale: float,
    sat_boost: float,
    contrast: float,
    sunrise_center: float,
    sunset_center: float,
    twilight_sigma: float,
    # noon neutral zone controls
    noon_blend: float,
    noon_sigma: float,
    noon_window_start: float,
    noon_window_end: float,
    noon_window_soft: float,
) -> List[int]:
    params = hour_adjustments(
        hour_value,
        warmth_scale=warmth_scale,
        blue_scale=blue_scale,
        darkness_scale=darkness_scale,
        desat_scale=desat_scale,
        sunrise_center=sunrise_center,
        sunset_center=sunset_center,
        twilight_sigma=twilight_sigma,
    )
    reserved = set(reserved_colors)

    # Noon weight (0..1): stronger near 10:00–14:00, peak at 12:00
    noon_w = _noon_weight(
        hour_value, noon_blend, noon_sigma,
        noon_window_start, noon_window_end, noon_window_soft
    )

    # Damp sat/contrast near noon to avoid “pop”
    sat_eff = 1.0 + (sat_boost - 1.0) * (1.0 - noon_w)
    contrast_eff = 1.0 + (contrast - 1.0) * (1.0 - noon_w)

    out = pal[:]  # copy
    for i in range(256):
        r, g, b = pal[3 * i:3 * i + 3]
        rgb = (r, g, b)

        if rgb in reserved:
            out[3 * i + 0] = r
            out[3 * i + 1] = g
            out[3 * i + 2] = b
            continue

        nr, ng, nb = tint_rgb(rgb, params, sat_boost=sat_eff, contrast=contrast_eff)

        if noon_w > 0.0:
            # Blend back toward the original (noon) palette color at the SAME index
            nr = int(round((1.0 - noon_w) * nr + noon_w * r))
            ng = int(round((1.0 - noon_w) * ng + noon_w * g))
            nb = int(round((1.0 - noon_w) * nb + noon_w * b))

        out[3 * i + 0] = nr
        out[3 * i + 1] = ng
        out[3 * i + 2] = nb

    return out


def remap_green_to_magenta_indices(img: Image.Image, pal: Sequence[int]) -> Image.Image:
    green_idx = find_color_index(pal, (0, 255, 0))
    magenta_idx = find_color_index(pal, (255, 0, 255))
    if green_idx < 0 or magenta_idx < 0 or green_idx == magenta_idx:
        return img
    lut = [magenta_idx if i == green_idx else i for i in range(256)]
    return img.point(lut, mode="P")


# --------------------------- File ops ---------------------------

def copy_noon_to_label(noon_dir: Path, label_dir: Path) -> List[Path]:
    label_dir.mkdir(parents=True, exist_ok=True)
    copied: List[Path] = []
    for src in noon_dir.glob("*.pcx"):
        dst = label_dir / src.name
        shutil.copy2(src, dst)
        copied.append(dst)
    return copied


def label_to_hour_value(label: str) -> float:
    """Convert 'HHMM' or '2400' to hour value (fractional hours)."""
    if label == "2400":
        return 0.0
    h = int(label[:2])
    m = int(label[2:])
    return (h % 24) + (m / 60.0)


def process_time_label(
    label: str,
    base_dir: Path,
    noon_label: str,
    reserved_colors: Iterable[Tuple[int, int, int]],
    *,
    do_index_remap: bool,
    # look controls
    warmth_scale: float,
    blue_scale: float,
    darkness_scale: float,
    desat_scale: float,
    sat_boost: float,
    contrast: float,
    sunrise_center: float,
    sunset_center: float,
    twilight_sigma: float,
    # noon neutral controls
    noon_blend: float,
    noon_sigma: float,
    noon_window_start: float,
    noon_window_end: float,
    noon_window_soft: float,
) -> None:
    noon_dir = base_dir / noon_label
    out_dir = base_dir / label
    hour_value = label_to_hour_value(label)

    copied_files = copy_noon_to_label(noon_dir, out_dir)

    for pcx_path in copied_files:
        with Image.open(pcx_path) as im:
            if im.mode != "P":
                im = im.convert("P")
            pal = get_palette(im)

            if do_index_remap:
                im = remap_green_to_magenta_indices(im, pal)

            new_pal = adjust_palette_for_time(
                pal, hour_value, reserved_colors,
                warmth_scale=warmth_scale,
                blue_scale=blue_scale,
                darkness_scale=darkness_scale,
                desat_scale=desat_scale,
                sat_boost=sat_boost,
                contrast=contrast,
                sunrise_center=sunrise_center,
                sunset_center=sunset_center,
                twilight_sigma=twilight_sigma,
                noon_blend=noon_blend,
                noon_sigma=noon_sigma,
                noon_window_start=noon_window_start,
                noon_window_end=noon_window_end,
                noon_window_soft=noon_window_soft,
            )
            set_palette(im, new_pal)
            im.save(pcx_path, format="PCX")


# --------------------------- Main ---------------------------

def main():
    p = argparse.ArgumentParser(description="Civ3 PCX day/night generator with variable time steps and noon-neutral zone.")
    p.add_argument("--data", required=True,
                   help="Parent folder containing the noon folder and sibling time folders (e.g., NightDay).")
    p.add_argument("--noon", default="1200",
                   help="Name of the noon folder (default: 1200).")
    p.add_argument("--only-hour", default=None,
                   help="Process only a single time label (e.g., 1900, 1930, or 2400).")
    p.add_argument("--step-minutes", type=int, default=60,
                   help="Time step in minutes; must divide 60 (e.g., 5,10,15,20,30,60). Default: 60.")

    p.add_argument("--light-key", action="append", default=[],
                   help="Reserved color that must not change. Repeatable. '#RRGGBB' or 'R,G,B'.")

    # Look/feel controls
    p.add_argument("--warmth", type=float, default=1.10,
                   help="Scale for sunrise/sunset warmth (1.0 = base).")
    p.add_argument("--blue", type=float, default=1.12,
                   help="Scale for night-time blue emphasis (1.0 = base).")
    p.add_argument("--darkness", type=float, default=1.08,
                   help="Scale for extra night darkening (1.0 = base).")
    p.add_argument("--desat", type=float, default=0.85,
                   help="Scale for dusk/night desaturation toward gray (lower = richer).")
    p.add_argument("--sat", type=float, default=1.05,
                   help="Global saturation multiplier after tint (1.0 = none).")
    p.add_argument("--contrast", type=float, default=1.03,
                   help="Global contrast multiplier around mid 128 (1.0 = none).")

    # Curve placement/width
    p.add_argument("--sunrise-center", type=float, default=6.0,
                   help="Hour center for sunrise warmth bump (0-23).")
    p.add_argument("--sunset-center", type=float, default=18.0,
                   help="Hour center for sunset warmth bump (0-23).")
    p.add_argument("--twilight-width", type=float, default=1.8,
                   help="Sigma for sunrise/sunset warmth spread (higher = broader).")

    # Noon-neutral zone controls (defaults keep ~10:00–14:00 close to noon)
    p.add_argument("--noon-blend", type=float, default=0.85,
                   help="0..1 strength to blend toward base palette near 12:00 (0=off).")
    p.add_argument("--noon-sigma", type=float, default=1.1,
                   help="Gaussian width (hours) around 12:00 (larger = broader).")
    p.add_argument("--noon-window-start", type=float, default=10.0,
                   help="Start hour of the noon-like window (default 10.0).")
    p.add_argument("--noon-window-end", type=float, default=14.0,
                   help="End hour of the noon-like window (default 14.0).")
    p.add_argument("--noon-window-soft", type=float, default=0.7,
                   help="Soft edge (hours) for window ramps (0=hard).")

    # Index remap control
    g = p.add_mutually_exclusive_group()
    g.add_argument("--keep-green-index", action="store_true",
                   help="Do NOT remap green index to magenta; leave pixel indices unchanged.")
    g.add_argument("--map-green-index", dest="map_green_index", action="store_true",
                   help="Force remap green index to magenta (default behavior).")

    args = p.parse_args()

    base_dir = Path(args.data).expanduser().resolve()
    noon_dir = base_dir / args.noon
    if not noon_dir.is_dir():
        raise SystemExit(f"Noon folder not found: {noon_dir}")

    # Reserved colors (Magenta + Green + user light-keys)
    reserved: List[Tuple[int, int, int]] = [(255, 0, 255), (0, 255, 0)]
    for s in args.light_key:
        reserved.append(parse_rgb(s))

    # Determine remap behavior (default ON)
    do_index_remap = not args.keep_green_index or args.map_green_index

    # Build time labels for the given step, then exclude noon folder itself
    labels = [lbl for lbl in time_labels(args.step_minutes) if lbl != args.noon]

    # only-hour (accept 0000 -> 2400)
    if args.only_hour:
        only = "2400" if args.only_hour == "0000" else args.only_hour
        if only == args.noon:
            print(f"--only-hour {only} is the noon source; nothing to do.")
            return
        if only not in labels:
            raise SystemExit(f"--only-hour must be one of: {', '.join(labels)}")
        labels = [only]

    print(f"Base: {base_dir}")
    print(f"Noon source: {noon_dir}")
    print(f"Time step: {args.step_minutes} minutes")
    print(f"Index remap green→magenta: {'ON' if do_index_remap else 'OFF'}")
    print(f"Generating labels: {', '.join(labels)}")

    for lbl in labels:
        print(f"  -> Generating {lbl} ...")
        process_time_label(
            lbl, base_dir, args.noon, reserved,
            do_index_remap=do_index_remap,
            warmth_scale=args.warmth,
            blue_scale=args.blue,
            darkness_scale=args.darkness,
            desat_scale=args.desat,
            sat_boost=args.sat,
            contrast=args.contrast,
            sunrise_center=args.sunrise_center,
            sunset_center=args.sunset_center,
            twilight_sigma=args.twilight_width,
            noon_blend=args.noon_blend,
            noon_sigma=args.noon_sigma,
            noon_window_start=args.noon_window_start,
            noon_window_end=args.noon_window_end,
            noon_window_soft=args.noon_window_soft,
        )

    print("Done.")


if __name__ == "__main__":
    main()