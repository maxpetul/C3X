#!/usr/bin/env python3
"""
mp4_to_civ3_sheet.py

Build a Civ3-style sheet from an MP4:
- N columns sampled by time
- 8 rows (only first row contains frames)
- Other rows magenta fill
- SINGLE shared 1px green grid lines (no doubled borders)

Output modes:
- indexed: Civ3-style indexed palette and PCX output
- rgb: RGB output (PNG recommended for editing)

Dependencies:
  pip install pillow opencv-python numpy
"""

from __future__ import annotations

import argparse
import math
import os
from typing import List, Tuple

import cv2
import numpy as np
from PIL import Image

MAGENTA = (255, 0, 255)
GREEN = (0, 255, 0)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)

RESERVED_CIV_COLORS = 64
RESERVED_TAIL = 2  # green + magenta
TOTAL_PALETTE = 256
SAMPLED_COLORS = TOTAL_PALETTE - RESERVED_CIV_COLORS - RESERVED_TAIL  # 190


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--mp4", required=True, help="Path to input mp4")
    p.add_argument("--columns", type=int, required=True, help="Number of columns (sampled frames)")
    p.add_argument("--slot_w", type=int, required=True, help="Slot inner width (excluding border)")
    p.add_argument("--slot_h", type=int, required=True, help="Slot inner height (excluding border)")
    p.add_argument("--out", required=True, help="Output path (pcx for indexed, png for rgb is recommended)")
    p.add_argument(
        "--mode",
        choices=["indexed", "rgb"],
        default="indexed",
        help="Output mode: indexed (Civ3 PCX) or rgb (edit-friendly)",
    )
    p.add_argument(
        "--resample",
        choices=["nearest", "bilinear", "bicubic", "lanczos"],
        default="nearest",
        help="Resampling method for resizing frames (default: nearest / no interpolation)",
    )
    return p.parse_args()


def pil_resample(name: str) -> int:
    if name == "nearest":
        return Image.NEAREST
    if name == "bilinear":
        return Image.BILINEAR
    if name == "bicubic":
        return Image.BICUBIC
    if name == "lanczos":
        return Image.LANCZOS
    raise ValueError(name)


def center_crop_to_aspect(im: Image.Image, target_w: int, target_h: int) -> Image.Image:
    w, h = im.size
    target_aspect = target_w / target_h
    src_aspect = w / h

    if abs(src_aspect - target_aspect) < 1e-9:
        return im

    if src_aspect > target_aspect:
        new_w = int(round(h * target_aspect))
        new_h = h
        left = (w - new_w) // 2
        top = 0
    else:
        new_w = w
        new_h = int(round(w / target_aspect))
        left = 0
        top = (h - new_h) // 2

    return im.crop((left, top, left + new_w, top + new_h))


def get_duration_ms(cap: cv2.VideoCapture) -> float:
    fps = cap.get(cv2.CAP_PROP_FPS) or 0.0
    frame_count = cap.get(cv2.CAP_PROP_FRAME_COUNT) or 0.0
    if fps > 0 and frame_count > 0:
        return max(0.0, (frame_count - 1.0) / fps * 1000.0)

    # Fallback: seek to end and use POS_MSEC
    cur = cap.get(cv2.CAP_PROP_POS_FRAMES)
    cap.set(cv2.CAP_PROP_POS_AVI_RATIO, 1.0)
    _ok, _ = cap.read()
    end_ms = cap.get(cv2.CAP_PROP_POS_MSEC) or 0.0
    cap.set(cv2.CAP_PROP_POS_FRAMES, cur)
    return max(0.0, end_ms)


def linspace_times(duration_ms: float, n: int) -> List[float]:
    if n <= 0:
        return []
    if duration_ms <= 0:
        return [0.0] * n
    return [float(x) for x in np.linspace(0.0, duration_ms, n)]


def read_frame_at_time(cap: cv2.VideoCapture, t_ms: float) -> np.ndarray:
    cap.set(cv2.CAP_PROP_POS_MSEC, float(t_ms))
    ok, frame_bgr = cap.read()
    if not ok or frame_bgr is None:
        raise RuntimeError(f"Failed to read frame at time {t_ms:.2f}ms")
    return cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)


def build_sample_palette_from_frames(frames_rgb: List[Image.Image]) -> List[Tuple[int, int, int]]:
    if not frames_rgb:
        return []

    thumbs = []
    for im in frames_rgb:
        w, h = im.size
        scale = max(1, math.ceil(max(w, h) / 128))
        tw, th = max(1, w // scale), max(1, h // scale)
        thumbs.append(im.resize((tw, th), resample=Image.NEAREST))

    total_w = sum(t.size[0] for t in thumbs)
    max_h = max(t.size[1] for t in thumbs)
    mosaic = Image.new("RGB", (max(1, total_w), max(1, max_h)), color=BLACK)

    x = 0
    for t in thumbs:
        mosaic.paste(t, (x, 0))
        x += t.size[0]

    q = mosaic.quantize(colors=SAMPLED_COLORS, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = q.getpalette()

    colors = []
    for i in range(SAMPLED_COLORS):
        r, g, b = pal[i * 3 : i * 3 + 3]
        colors.append((r, g, b))

    seen = set()
    uniq = []
    for c in colors:
        if c not in seen:
            uniq.append(c)
            seen.add(c)
    return uniq


def make_civ3_palette(sampled: List[Tuple[int, int, int]]) -> List[int]:
    pal: List[int] = []

    for _ in range(RESERVED_CIV_COLORS):
        pal += [*WHITE]

    sampled = sampled[:SAMPLED_COLORS]
    if len(sampled) < SAMPLED_COLORS:
        sampled = sampled + [BLACK] * (SAMPLED_COLORS - len(sampled))
    for (r, g, b) in sampled:
        pal += [r, g, b]

    pal += [*GREEN]    # 254
    pal += [*MAGENTA]  # 255
    assert len(pal) == 768
    return pal


def build_sheet_rgb(frames: List[Image.Image], cols: int, slot_w: int, slot_h: int) -> Image.Image:
    rows = 8
    sheet_w = cols * slot_w + (cols + 1)
    sheet_h = rows * slot_h + (rows + 1)

    sheet = Image.new("RGB", (sheet_w, sheet_h), color=GREEN)  # grid lines
    magenta_inner = Image.new("RGB", (slot_w, slot_h), color=MAGENTA)

    def cell_xy(col: int, row: int) -> Tuple[int, int]:
        x = 1 + col * (slot_w + 1)
        y = 1 + row * (slot_h + 1)
        return x, y

    for c in range(cols):
        # row 0: frame
        x, y = cell_xy(c, 0)
        sheet.paste(frames[c], (x, y))

        # rows 1..7: magenta
        for r in range(1, rows):
            x2, y2 = cell_xy(c, r)
            sheet.paste(magenta_inner, (x2, y2))

    return sheet


def main() -> None:
    args = parse_args()
    if args.columns <= 0:
        raise ValueError("--columns must be > 0")

    resample = pil_resample(args.resample)

    cap = cv2.VideoCapture(args.mp4)
    if not cap.isOpened():
        raise RuntimeError(f"Could not open video: {args.mp4}")

    duration_ms = get_duration_ms(cap)
    times = linspace_times(duration_ms, args.columns)

    frames: List[Image.Image] = []
    for t_ms in times:
        fr = read_frame_at_time(cap, t_ms)
        im = Image.fromarray(fr, mode="RGB")
        im = center_crop_to_aspect(im, args.slot_w, args.slot_h)
        im = im.resize((args.slot_w, args.slot_h), resample=resample)
        frames.append(im)

    cap.release()

    sheet_rgb = build_sheet_rgb(frames, args.columns, args.slot_w, args.slot_h)

    if args.mode == "rgb":
        # Strongly recommend PNG for RGB intermediates.
        # We'll still honor the user's --out extension.
        sheet_rgb.save(args.out)
        print(f"Saved RGB: {args.out}")
        return

    # indexed mode => force Civ3 palette and write PCX
    sampled_colors = build_sample_palette_from_frames(frames)
    palette_list = make_civ3_palette(sampled_colors)

    pal_img = Image.new("P", (16, 16))
    pal_img.putpalette(palette_list)

    indexed = sheet_rgb.quantize(palette=pal_img, dither=Image.NONE)

    ext = os.path.splitext(args.out)[1].lower()
    if ext not in [".pcx"]:
        # Save PCX anyway, but warn via print
        print("Note: --mode indexed is intended for .pcx output.")

    indexed.save(args.out, format="PCX")
    print(f"Saved indexed PCX: {args.out}")


if __name__ == "__main__":
    main()