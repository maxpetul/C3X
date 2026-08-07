#!/usr/bin/env python3
"""
pcx_rgb_to_civ3_indexed_pcx.py

Input:  PCX file (intended to be RGB / truecolor)
Output: PCX file (indexed / paletted) with Civ3 palette rules:

Palette indices (256 entries):
  0..63   reserved civ colors (set to white)
  64..253 sampled from the image (190 colors)
  254     green  (#00ff00)
  255     magenta(#ff00ff)

Dependencies:
  pip install pillow numpy

Usage:
  python pcx_rgb_to_civ3_indexed_pcx.py --in input_rgb.pcx --out output_indexed.pcx
"""

from __future__ import annotations

import argparse
import math
from typing import List, Tuple

import numpy as np
from PIL import Image

MAGENTA = (255, 0, 255)
GREEN = (0, 255, 0)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)

RESERVED_CIV_COLORS = 64
RESERVED_TAIL = 2
TOTAL_PALETTE = 256
SAMPLED_COLORS = TOTAL_PALETTE - RESERVED_CIV_COLORS - RESERVED_TAIL  # 190


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--in", dest="inp", required=True, help="Input PCX path (RGB PCX)")
    p.add_argument("--out", required=True, help="Output PCX path (indexed Civ3 style)")
    p.add_argument(
        "--sample_thumb",
        type=int,
        default=256,
        help="Max thumb dimension used for palette sampling (default 256)",
    )
    p.add_argument(
        "--dither",
        action="store_true",
        help="Enable dithering (usually OFF for Civ3 assets)",
    )
    p.add_argument(
        "--snap_reserved_tol",
        type=int,
        default=0,
        help=(
            "If >0, snap pixels within this per-channel tolerance to exact reserved colors "
            "(helps if editing introduced near-magenta/near-green)."
        ),
    )
    return p.parse_args()


def is_pcx_path(path: str) -> bool:
    return path.lower().endswith(".pcx")


def snap_reserved_colors(arr: np.ndarray, tol: int) -> np.ndarray:
    """
    Snap near-magenta and near-green pixels back to exact MAGENTA / GREEN.
    tol is per-channel tolerance (0 disables).
    """
    if tol <= 0:
        return arr

    r = arr[:, :, 0].astype(np.int16)
    g = arr[:, :, 1].astype(np.int16)
    b = arr[:, :, 2].astype(np.int16)

    def near(color: Tuple[int, int, int]) -> np.ndarray:
        cr, cg, cb = color
        return (
            (np.abs(r - cr) <= tol)
            & (np.abs(g - cg) <= tol)
            & (np.abs(b - cb) <= tol)
        )

    near_magenta = near(MAGENTA)
    near_green = near(GREEN)

    arr[near_magenta] = MAGENTA
    arr[near_green] = GREEN
    return arr


def build_sample_palette(im_rgb: Image.Image, thumb_max: int) -> List[Tuple[int, int, int]]:
    """
    Build up to 190 representative colors from the image safely.
    """

    w, h = im_rgb.size
    scale = max(1, math.ceil(max(w, h) / max(1, int(thumb_max))))
    tw, th = max(1, w // scale), max(1, h // scale)
    thumb = im_rgb.resize((tw, th), resample=Image.NEAREST)

    arr = np.array(thumb, dtype=np.uint8)

    # Remove reserved colors from sampling influence
    is_magenta = (arr[:, :, 0] == 255) & (arr[:, :, 1] == 0) & (arr[:, :, 2] == 255)
    is_green = (arr[:, :, 0] == 0) & (arr[:, :, 1] == 255) & (arr[:, :, 2] == 0)
    arr[is_magenta | is_green] = BLACK

    thumb2 = Image.fromarray(arr, mode="RGB")

    q = thumb2.quantize(colors=SAMPLED_COLORS, method=Image.MEDIANCUT, dither=Image.NONE)

    pal = q.getpalette()
    if pal is None:
        return []

    # Determine how many palette entries actually exist
    actual_entries = len(pal) // 3

    colors = []
    for i in range(min(actual_entries, SAMPLED_COLORS)):
        base = i * 3
        r, g, b = pal[base], pal[base + 1], pal[base + 2]
        colors.append((r, g, b))

    # Remove reserved colors + dedupe
    seen = set()
    uniq = []
    for c in colors:
        if c in (MAGENTA, GREEN):
            continue
        if c not in seen:
            uniq.append(c)
            seen.add(c)

    return uniq


def make_civ3_palette(sampled: List[Tuple[int, int, int]]) -> List[int]:
    """
    Produce a 256*3 palette list.
    """
    pal: List[int] = []

    # 0..63 = white placeholders
    for _ in range(RESERVED_CIV_COLORS):
        pal += [*WHITE]

    sampled = [c for c in sampled if c not in (GREEN, MAGENTA)]
    sampled = sampled[:SAMPLED_COLORS]
    if len(sampled) < SAMPLED_COLORS:
        sampled = sampled + [BLACK] * (SAMPLED_COLORS - len(sampled))

    for (r, g, b) in sampled:
        pal += [r, g, b]

    # 254 green, 255 magenta
    pal += [*GREEN]
    pal += [*MAGENTA]

    assert len(pal) == 768
    return pal


def main() -> None:
    args = parse_args()

    if not is_pcx_path(args.inp):
        raise SystemExit("Input must be a .pcx file")
    if not is_pcx_path(args.out):
        raise SystemExit("Output must be a .pcx file")

    # Read PCX
    im = Image.open(args.inp)  # may fail if PCX is saved in an odd truecolor variant
    im_rgb = im.convert("RGB")

    # Optional snapping of near-reserved colors back to exact reserved values
    arr = np.array(im_rgb, dtype=np.uint8)
    arr = snap_reserved_colors(arr, tol=int(args.snap_reserved_tol))
    im_rgb = Image.fromarray(arr, mode="RGB")

    # Build fixed Civ3 palette and quantize to it
    sampled = build_sample_palette(im_rgb, thumb_max=int(args.sample_thumb))
    palette_list = make_civ3_palette(sampled)

    pal_img = Image.new("P", (16, 16))
    pal_img.putpalette(palette_list)

    dither_flag = Image.FLOYDSTEINBERG if args.dither else Image.NONE
    indexed = im_rgb.quantize(palette=pal_img, dither=dither_flag)

    # Save indexed PCX
    indexed.save(args.out, format="PCX")
    print(f"Saved indexed Civ3 PCX: {args.out}")


if __name__ == "__main__":
    main()