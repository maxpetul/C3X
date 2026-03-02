#!/usr/bin/env python3
"""
mp4_to_civ3_sheet.py

Build a Civ3-style sheet from an MP4:
- N columns sampled by time OR by frame index
- 8 rows (only first row contains frames)
- Other rows magenta fill
- SINGLE shared 1px green grid lines (no doubled borders)

Output modes:
- indexed: Civ3-style indexed palette and PCX output
- rgb: RGB output (PCX supported; PNG also fine)

Optional cutout (Option B: MOG2 background subtraction):
- Learn a background model across the sampled frames
- Foreground mask per frame
- Cleanup + keep only largest blob
- Everything else magenta

Key knobs added:
- --sample_by frame/time, with backoff handling
- --fit crop/contain (contain avoids chopping fish at edges by letterboxing/padding)
- --zoom (scales the frame up/down before fit; helps make subject larger in-slot)

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
    p.add_argument(
        "--out",
        required=True,
        help="Output file path (.pcx recommended for your workflow; png also supported). Must be a file, not a directory.",
    )

    # Fit strategy (fixes “fish bottom cut off”)
    p.add_argument(
        "--fit",
        choices=["crop", "contain"],
        default="contain",
        help="How to adapt frames to slot aspect. crop=center-crop to fill; contain=scale to fit and pad (prevents cutoff).",
    )
    p.add_argument(
        "--pad_color",
        choices=["magenta", "green", "black"],
        default="magenta",
        help="Padding color used when --fit contain.",
    )

    # NEW: zoom
    p.add_argument(
        "--zoom",
        type=float,
        default=1.0,
        help=(
            "Zoom factor applied to each frame BEFORE fit. "
            "1.0=no change, >1.0 zooms in (subject bigger), <1.0 zooms out."
        ),
    )

    # Sampling strategy
    p.add_argument(
        "--sample_by",
        choices=["time", "frame"],
        default="frame",
        help="How to sample frames: 'frame' (robust) or 'time' (uses CAP_PROP_POS_MSEC). Default: frame",
    )

    # Time-based sampling controls
    p.add_argument(
        "--time_endpoint_mode",
        choices=["exclude", "include", "clamp"],
        default="exclude",
        help=(
            "Only used when --sample_by time. "
            "'exclude' never samples the exact end time (recommended). "
            "'include' includes duration endpoint (may fail near end). "
            "'clamp' includes endpoint but clamps to duration-epsilon."
        ),
    )
    p.add_argument(
        "--time_epsilon_ms",
        type=float,
        default=1.0,
        help="Only used when --sample_by time and endpoint mode uses clamping. Default: 1.0ms",
    )
    p.add_argument(
        "--time_seek_backoff_ms",
        type=float,
        nargs="*",
        default=[0.0, 5.0, 10.0, 20.0, 40.0, 80.0, 160.0],
        help="Only used when --sample_by time. Backoff steps (ms) tried if read fails at target time.",
    )

    # Frame-index sampling controls
    p.add_argument(
        "--frame_endpoint_mode",
        choices=["exclude_last", "include_last"],
        default="exclude_last",
        help=(
            "Only used when --sample_by frame. "
            "'exclude_last' avoids sampling the last reported frame index (recommended). "
            "'include_last' may fail on some MP4s."
        ),
    )
    p.add_argument(
        "--frame_seek_backoff",
        type=int,
        nargs="*",
        default=[0, 1, 2, 3, 5, 8, 13],
        help="Only used when --sample_by frame. If a frame read fails, try idx-backoff steps (in frames).",
    )

    # MOG2 tuning
    p.add_argument("--mog2_learning_rate", type=float, default=0.001, help="MOG2 learning rate per frame")
    p.add_argument("--mog2_warmup", type=int, default=0, help="Number of initial sampled frames used for warmup")
    p.add_argument(
        "--mog2_freeze_after_warmup",
        action="store_true",
        help="After warmup, freeze background model (learningRate=0).",
    )

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

    # Cutout (Option B)
    p.add_argument("--cutout", action="store_true", help="Enable background removal (MOG2)")
    p.add_argument("--mog2_history", type=int, default=200, help="MOG2 history length")
    p.add_argument("--mog2_var_threshold", type=float, default=16.0, help="MOG2 varThreshold")
    p.add_argument("--mog2_detect_shadows", action="store_true", help="Enable MOG2 shadow detection")
    p.add_argument("--cutout_center_frac", type=float, default=0.85, help="Center region fraction (0-1)")
    p.add_argument("--cutout_keep_largest", action="store_true", help="Keep only the largest component")
    p.add_argument("--cutout_min_area", type=int, default=200, help="Minimum component area to keep")
    p.add_argument("--cutout_dilate", type=int, default=2, help="Dilate mask pixels")
    p.add_argument("--cutout_blur", type=int, default=3, help="Blur kernel size for mask (odd; 0 disables)")

    # Debug
    p.add_argument("--print_video_info", action="store_true", help="Print FPS/frame_count/duration estimates")

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


def pad_color_rgb(name: str) -> Tuple[int, int, int]:
    if name == "magenta":
        return MAGENTA
    if name == "green":
        return GREEN
    if name == "black":
        return BLACK
    raise ValueError(name)


def apply_zoom_center(im: Image.Image, zoom: float, resample: int) -> Image.Image:
    """
    Zoom around center while keeping the SAME output dimensions as input.
    - zoom > 1: zoom in (subject bigger)
    - zoom < 1: zoom out (subject smaller; adds border replicated by scaling down + padding)
    This is done BEFORE fit/crop/contain.
    """
    if zoom <= 0:
        raise ValueError("--zoom must be > 0")
    if abs(zoom - 1.0) < 1e-9:
        return im

    w, h = im.size
    # Scale image
    nw = max(1, int(round(w * zoom)))
    nh = max(1, int(round(h * zoom)))
    scaled = im.resize((nw, nh), resample=resample)

    if zoom >= 1.0:
        # Center-crop back to original size
        left = (nw - w) // 2
        top = (nh - h) // 2
        return scaled.crop((left, top, left + w, top + h))

    # zoom < 1.0: center-pad back to original size
    out = Image.new(im.mode, (w, h), color=BLACK if im.mode == "RGB" else 0)
    ox = (w - nw) // 2
    oy = (h - nh) // 2
    out.paste(scaled, (ox, oy))
    return out


def center_crop_to_aspect(im: Image.Image, target_w: int, target_h: int) -> Image.Image:
    w, h = im.size
    target_aspect = target_w / target_h
    src_aspect = w / h

    if abs(src_aspect - target_aspect) < 1e-9:
        return im

    if src_aspect > target_aspect:
        new_w = int(round(h * target_aspect))
        left = (w - new_w) // 2
        return im.crop((left, 0, left + new_w, h))
    else:
        new_h = int(round(w / target_aspect))
        top = (h - new_h) // 2
        return im.crop((0, top, w, top + new_h))


def fit_contain(
    im: Image.Image,
    target_w: int,
    target_h: int,
    pad_rgb: Tuple[int, int, int],
    resample: int,
) -> Image.Image:
    """Scale to fit entirely within target, preserve aspect, then pad to exact size."""
    w, h = im.size
    if w <= 0 or h <= 0:
        return Image.new("RGB", (target_w, target_h), color=pad_rgb)

    scale = min(target_w / w, target_h / h)
    nw = max(1, int(round(w * scale)))
    nh = max(1, int(round(h * scale)))

    resized = im.resize((nw, nh), resample=resample)
    out = Image.new("RGB", (target_w, target_h), color=pad_rgb)
    ox = (target_w - nw) // 2
    oy = (target_h - nh) // 2
    out.paste(resized, (ox, oy))
    return out


def get_duration_ms(cap: cv2.VideoCapture) -> float:
    fps = cap.get(cv2.CAP_PROP_FPS) or 0.0
    frame_count = cap.get(cv2.CAP_PROP_FRAME_COUNT) or 0.0
    if fps > 0 and frame_count > 0:
        return max(0.0, (frame_count - 1.0) / fps * 1000.0)

    cur = cap.get(cv2.CAP_PROP_POS_FRAMES)
    cap.set(cv2.CAP_PROP_POS_AVI_RATIO, 1.0)
    _ok, _ = cap.read()
    end_ms = cap.get(cv2.CAP_PROP_POS_MSEC) or 0.0
    cap.set(cv2.CAP_PROP_POS_FRAMES, cur)
    return max(0.0, end_ms)


def linspace_times(duration_ms: float, n: int, endpoint_mode: str, eps_ms: float) -> List[float]:
    if n <= 0:
        return []
    if duration_ms <= 0:
        return [0.0] * n
    if n == 1:
        return [0.0]

    if endpoint_mode == "exclude":
        return [float(x) for x in np.linspace(0.0, float(duration_ms), n, endpoint=False)]
    if endpoint_mode == "include":
        return [float(x) for x in np.linspace(0.0, float(duration_ms), n, endpoint=True)]
    if endpoint_mode == "clamp":
        hi = max(0.0, float(duration_ms) - float(max(0.0, eps_ms)))
        return [float(x) for x in np.linspace(0.0, hi, n, endpoint=True)]
    raise ValueError(endpoint_mode)


def read_frame_at_time_with_backoff(
    cap: cv2.VideoCapture,
    t_ms: float,
    backoffs_ms: List[float],
) -> np.ndarray:
    for back in backoffs_ms:
        t_try = max(0.0, float(t_ms) - float(back))
        cap.set(cv2.CAP_PROP_POS_MSEC, t_try)
        ok, frame_bgr = cap.read()
        if ok and frame_bgr is not None:
            return cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
    raise RuntimeError(f"Failed to read frame near time {t_ms:.2f}ms")


def sample_frames_by_time(
    cap: cv2.VideoCapture,
    cols: int,
    endpoint_mode: str,
    eps_ms: float,
    backoffs_ms: List[float],
    print_info: bool,
) -> List[np.ndarray]:
    duration_ms = get_duration_ms(cap)
    times = linspace_times(duration_ms, cols, endpoint_mode=endpoint_mode, eps_ms=eps_ms)

    if print_info:
        fps = cap.get(cv2.CAP_PROP_FPS) or 0.0
        frame_count = cap.get(cv2.CAP_PROP_FRAME_COUNT) or 0.0
        print(f"[video] fps={fps:.3f} frame_count={frame_count} duration_ms≈{duration_ms:.2f}")
        if times:
            print(f"[sample] time range: {times[0]:.2f}ms .. {times[-1]:.2f}ms (count={len(times)})")

    frames: List[np.ndarray] = []
    for t_ms in times:
        frames.append(read_frame_at_time_with_backoff(cap, t_ms, backoffs_ms=backoffs_ms))
    return frames


def read_frame_at_index_with_backoff(
    cap: cv2.VideoCapture,
    idx: int,
    frame_count: int,
    backoffs: List[int],
) -> np.ndarray:
    for b in backoffs:
        j = int(idx) - int(b)
        if j < 0 or j >= int(frame_count):
            continue
        cap.set(cv2.CAP_PROP_POS_FRAMES, int(j))
        ok, frame_bgr = cap.read()
        if ok and frame_bgr is not None:
            return cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
    raise RuntimeError(f"Failed to read frame index {int(idx)} with backoff (frame_count={int(frame_count)})")


def sample_frames_by_index(
    cap: cv2.VideoCapture,
    cols: int,
    print_info: bool,
    endpoint_mode: str,
    backoffs: List[int],
) -> List[np.ndarray]:
    frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT) or 0)
    fps = float(cap.get(cv2.CAP_PROP_FPS) or 0.0)

    if frame_count <= 0:
        duration_ms = get_duration_ms(cap)
        if print_info:
            print(f"[video] frame_count unavailable; fps={fps:.3f} duration_ms≈{duration_ms:.2f}. Falling back to time sampling.")
        return sample_frames_by_time(
            cap,
            cols=cols,
            endpoint_mode="exclude",
            eps_ms=1.0,
            backoffs_ms=[0.0, 5.0, 10.0, 20.0, 40.0, 80.0, 160.0],
            print_info=print_info,
        )

    duration_ms = (frame_count - 1) / fps * 1000.0 if fps > 0 else float("nan")

    hi = frame_count - 1
    if endpoint_mode == "exclude_last":
        hi = max(0, frame_count - 2)

    if cols == 1:
        idxs = np.array([0], dtype=int)
    else:
        idxs = np.linspace(0, hi, cols, endpoint=True).astype(int)

    if print_info:
        dur_str = f"{duration_ms:.2f}" if duration_ms == duration_ms else "unknown"
        print(f"[video] fps={fps:.3f} frame_count={frame_count} duration_ms≈{dur_str}")
        if len(idxs) > 0:
            print(f"[sample] frame index range: {int(idxs[0])} .. {int(idxs[-1])} (count={len(idxs)})")
            if endpoint_mode == "exclude_last":
                print(f"[sample] exclude_last enabled; max sampled index is {int(hi)} (reported last is {frame_count - 1})")

    frames: List[np.ndarray] = []
    for idx in idxs:
        frames.append(read_frame_at_index_with_backoff(cap, int(idx), frame_count=frame_count, backoffs=backoffs))
    return frames


# --------------------------
# Option B: MOG2 cutout helpers
# --------------------------

def center_gate_mask(h: int, w: int, frac: float) -> np.ndarray:
    frac = float(np.clip(frac, 0.05, 1.0))
    cw = max(1, int(round(w * frac)))
    ch = max(1, int(round(h * frac)))
    x0 = (w - cw) // 2
    y0 = (h - ch) // 2
    mask = np.zeros((h, w), dtype=np.uint8)
    mask[y0:y0 + ch, x0:x0 + cw] = 255
    return mask


def keep_largest_component(mask: np.ndarray) -> np.ndarray:
    num, labels, stats, _ = cv2.connectedComponentsWithStats(mask, connectivity=8)
    if num <= 1:
        return mask
    best_i = 1
    best_area = int(stats[1, cv2.CC_STAT_AREA])
    for i in range(2, num):
        area = int(stats[i, cv2.CC_STAT_AREA])
        if area > best_area:
            best_area = area
            best_i = i
    out = np.zeros_like(mask)
    out[labels == best_i] = 255
    return out


def filter_small_components(mask: np.ndarray, min_area: int) -> np.ndarray:
    num, labels, stats, _ = cv2.connectedComponentsWithStats(mask, connectivity=8)
    out = np.zeros_like(mask)
    for i in range(1, num):
        area = int(stats[i, cv2.CC_STAT_AREA])
        if area >= int(min_area):
            out[labels == i] = 255
    return out


def cleanup_mask(mask: np.ndarray, dilate_iters: int) -> np.ndarray:
    k = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, k, iterations=2)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, k, iterations=1)
    if dilate_iters and int(dilate_iters) > 0:
        mask = cv2.dilate(mask, k, iterations=int(dilate_iters))
    return mask


def mog2_cutout_frames(
    frames_rgb: List[np.ndarray],
    history: int,
    var_threshold: float,
    detect_shadows: bool,
    center_frac: float,
    keep_largest: bool,
    min_area: int,
    dilate_iters: int,
    blur_k: int,
    learning_rate: float,
    warmup: int,
    freeze_after_warmup: bool,
) -> List[np.ndarray]:
    subtractor = cv2.createBackgroundSubtractorMOG2(
        history=int(history),
        varThreshold=float(var_threshold),
        detectShadows=bool(detect_shadows),
    )

    out_frames: List[np.ndarray] = []

    for i, fr in enumerate(frames_rgb):
        if i < int(warmup):
            lr = 0.5
        else:
            if freeze_after_warmup and int(warmup) > 0:
                lr = 0.0
            else:
                lr = float(learning_rate)

        fg = subtractor.apply(fr, learningRate=lr)

        # If shadows enabled, OpenCV uses 127 for shadows; treat as background.
        fg = (fg == 255).astype(np.uint8) * 255

        h, w = fg.shape
        gate = center_gate_mask(h, w, center_frac)
        fg = cv2.bitwise_and(fg, gate)

        fg = cleanup_mask(fg, dilate_iters=dilate_iters)

        if keep_largest:
            fg = keep_largest_component(fg)
        else:
            fg = filter_small_components(fg, min_area=min_area)

        if blur_k and int(blur_k) > 0:
            k = int(blur_k)
            if k % 2 == 0:
                k += 1
            fg = cv2.GaussianBlur(fg, (k, k), 0)
            fg = (fg > 127).astype(np.uint8) * 255

        out = fr.copy()
        out[fg == 0] = MAGENTA
        out_frames.append(out)

    return out_frames


# --------------------------
# Palette + sheet building
# --------------------------

def build_sample_palette_from_frames(frames_rgb: List[Image.Image]) -> List[Tuple[int, int, int]]:
    if not frames_rgb:
        return []

    thumbs: List[Image.Image] = []
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
    pal = q.getpalette() or []

    colors: List[Tuple[int, int, int]] = []
    for i in range(SAMPLED_COLORS):
        base = i * 3
        if base + 2 >= len(pal):
            break
        r, g, b = pal[base: base + 3]
        colors.append((int(r), int(g), int(b)))

    seen = set()
    uniq: List[Tuple[int, int, int]] = []
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
        pal += [int(r), int(g), int(b)]

    pal += [*GREEN]    # 254
    pal += [*MAGENTA]  # 255
    assert len(pal) == 768
    return pal


def build_sheet_rgb(frames: List[Image.Image], cols: int, slot_w: int, slot_h: int) -> Image.Image:
    rows = 8
    sheet_w = cols * slot_w + (cols + 1)
    sheet_h = rows * slot_h + (rows + 1)

    sheet = Image.new("RGB", (sheet_w, sheet_h), color=GREEN)
    magenta_inner = Image.new("RGB", (slot_w, slot_h), color=MAGENTA)

    def cell_xy(col: int, row: int) -> Tuple[int, int]:
        x = 1 + col * (slot_w + 1)
        y = 1 + row * (slot_h + 1)
        return x, y

    for c in range(cols):
        x, y = cell_xy(c, 0)
        sheet.paste(frames[c], (x, y))
        for r in range(1, rows):
            x2, y2 = cell_xy(c, r)
            sheet.paste(magenta_inner, (x2, y2))

    return sheet


def validate_out_path(out_path: str) -> None:
    if os.path.isdir(out_path):
        raise ValueError(f"--out must be a file path, not a directory: {out_path}")
    parent = os.path.dirname(out_path) or "."
    if parent and not os.path.exists(parent):
        os.makedirs(parent, exist_ok=True)


def main() -> None:
    args = parse_args()
    if args.columns <= 0:
        raise ValueError("--columns must be > 0")
    if args.zoom <= 0:
        raise ValueError("--zoom must be > 0")

    validate_out_path(args.out)

    resample = pil_resample(args.resample)
    pad_rgb = pad_color_rgb(args.pad_color)

    cap = cv2.VideoCapture(args.mp4)
    if not cap.isOpened():
        raise RuntimeError(f"Could not open video: {args.mp4}")

    try:
        if args.sample_by == "frame":
            raw_frames_np = sample_frames_by_index(
                cap,
                cols=args.columns,
                print_info=bool(args.print_video_info),
                endpoint_mode=args.frame_endpoint_mode,
                backoffs=[int(x) for x in (args.frame_seek_backoff or [0])],
            )
        else:
            raw_frames_np = sample_frames_by_time(
                cap,
                cols=args.columns,
                endpoint_mode=args.time_endpoint_mode,
                eps_ms=float(args.time_epsilon_ms),
                backoffs_ms=[float(x) for x in (args.time_seek_backoff_ms or [0.0])],
                print_info=bool(args.print_video_info),
            )
    finally:
        cap.release()

    if args.cutout:
        processed_np = mog2_cutout_frames(
            raw_frames_np,
            history=args.mog2_history,
            var_threshold=args.mog2_var_threshold,
            detect_shadows=args.mog2_detect_shadows,
            center_frac=args.cutout_center_frac,
            keep_largest=True if args.cutout_keep_largest else False,
            min_area=args.cutout_min_area,
            dilate_iters=args.cutout_dilate,
            blur_k=args.cutout_blur,
            learning_rate=args.mog2_learning_rate,
            warmup=args.mog2_warmup,
            freeze_after_warmup=args.mog2_freeze_after_warmup,
        )
    else:
        processed_np = raw_frames_np

    frames: List[Image.Image] = []
    for fr_np in processed_np:
        im = Image.fromarray(fr_np, mode="RGB")

        # NEW: apply zoom before fit
        im = apply_zoom_center(im, zoom=float(args.zoom), resample=resample)

        if args.fit == "crop":
            im = center_crop_to_aspect(im, args.slot_w, args.slot_h)
            im = im.resize((args.slot_w, args.slot_h), resample=resample)
        else:
            # contain: NEVER crops; prevents “fish bottom cut off”
            im = fit_contain(im, args.slot_w, args.slot_h, pad_rgb=pad_rgb, resample=resample)

        frames.append(im)

    sheet_rgb = build_sheet_rgb(frames, args.columns, args.slot_w, args.slot_h)

    if args.mode == "rgb":
        sheet_rgb.save(args.out, format="PCX")
        print(f"Saved RGB PCX: {args.out}")
        return

    sampled_colors = build_sample_palette_from_frames(frames)
    palette_list = make_civ3_palette(sampled_colors)

    pal_img = Image.new("P", (16, 16))
    pal_img.putpalette(palette_list)

    indexed = sheet_rgb.quantize(palette=pal_img, dither=Image.NONE)

    ext = os.path.splitext(args.out)[1].lower()
    if ext != ".pcx":
        print("Note: --mode indexed is intended for .pcx output (use .pcx extension).")

    indexed.save(args.out, format="PCX")
    print(f"Saved indexed PCX: {args.out}")


if __name__ == "__main__":
    main()