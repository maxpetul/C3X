#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Scale the visible pixels in a Civ3 FLC down inside the original canvas.

The output keeps the first input's Civ3 FLC dimensions, speed, palette bytes,
and most custom header tail values.  When multiple inputs are provided, their
frames are concatenated within each Civ3 direction, and the output animation
length/header frame count are updated accordingly.  It re-emits the animation
using the stock Civ3 resource FLC rhythm: per-direction BYTE_RUN keyframes and
DELTA_FLC animation frames.

Pixels are resampled as palette indexes, not RGB, so the palette is unchanged.
Nearest-neighbor scaling is deliberate: it cannot introduce non-palette colors.
By default, combined animations are capped at 64 frames per direction.
"""

import argparse
import os
import struct
from typing import List, Optional, Sequence, Tuple

from flc_night_darkener import (
    CHUNK_BYTE_RUN,
    CHUNK_COLOR_256,
    CHUNK_DELTA_FLC,
    Civ3Tail,
    FlcDecoded,
    build_flc_header_128,
    decode_flc,
    make_byte_run_payload,
    make_color_256_payload,
    make_frame_chunk,
    make_subchunk,
    pack_u16,
)


def flatten_inputs(raw_inputs: Sequence[Sequence[str]]) -> List[str]:
    return [path for group in raw_inputs for path in group]


def parse_anchor(spec: str) -> Tuple[float, float]:
    s = (spec or "").strip().lower().replace("_", "-")
    named = {
        "center": (0.5, 0.5),
        "middle": (0.5, 0.5),
        "top-left": (0.0, 0.0),
        "top": (0.5, 0.0),
        "top-right": (1.0, 0.0),
        "left": (0.0, 0.5),
        "right": (1.0, 0.5),
        "bottom-left": (0.0, 1.0),
        "bottom": (0.5, 1.0),
        "bottom-right": (1.0, 1.0),
    }
    if s in named:
        return named[s]
    if "," in s:
        a, b = s.split(",", 1)
        ax = float(a)
        ay = float(b)
        if not (0.0 <= ax <= 1.0 and 0.0 <= ay <= 1.0):
            raise argparse.ArgumentTypeError("anchor coordinates must be in the range 0..1")
        return ax, ay
    raise argparse.ArgumentTypeError(
        "anchor must be a name like center/bottom or coordinates like 0.5,1.0"
    )


def frame_dimensions(frame: bytes, w: int, h: int) -> None:
    if len(frame) != w * h:
        raise ValueError(f"Decoded frame has {len(frame)} bytes, expected {w * h}.")


def choose_background_index(frames: Sequence[bytes], w: int, h: int) -> int:
    """
    Pick a conservative fill index for the newly exposed border.

    Civ3 transparent/magenta backgrounds are normally around the frame edges, so
    edge pixels across all decoded frames are a better signal than the full-frame
    mode for sprites that occupy much of the canvas.
    """
    if not frames:
        return 0

    counts = [0] * 256
    for frame in frames:
        frame_dimensions(frame, w, h)
        if w <= 0 or h <= 0:
            continue

        top = frame[:w]
        bottom = frame[(h - 1) * w:h * w]
        for b in top:
            counts[b] += 1
        for b in bottom:
            counts[b] += 1

        for y in range(1, max(1, h - 1)):
            row = y * w
            counts[frame[row]] += 1
            counts[frame[row + w - 1]] += 1

    return max(range(256), key=lambda i: counts[i])


def content_bbox(frame: bytes, w: int, h: int, background_index: int) -> Optional[Tuple[int, int, int, int]]:
    frame_dimensions(frame, w, h)
    min_x = w
    min_y = h
    max_x = -1
    max_y = -1
    for y in range(h):
        row = y * w
        for x in range(w):
            if frame[row + x] == background_index:
                continue
            if x < min_x:
                min_x = x
            if y < min_y:
                min_y = y
            if x > max_x:
                max_x = x
            if y > max_y:
                max_y = y
    if max_x < min_x or max_y < min_y:
        return None
    return min_x, min_y, max_x + 1, max_y + 1


def union_bbox(frames: Sequence[bytes], w: int, h: int, background_index: int) -> Optional[Tuple[int, int, int, int]]:
    boxes = [content_bbox(fr, w, h, background_index) for fr in frames]
    boxes = [box for box in boxes if box is not None]
    if not boxes:
        return None
    return (
        min(box[0] for box in boxes),
        min(box[1] for box in boxes),
        max(box[2] for box in boxes),
        max(box[3] for box in boxes),
    )


def scale_frame_about_anchor(
    frame: bytes,
    w: int,
    h: int,
    scale: float,
    background_index: int,
    bbox: Tuple[int, int, int, int],
    anchor: Tuple[float, float],
    shift_x: int,
    shift_y: int,
) -> bytes:
    frame_dimensions(frame, w, h)
    x0, y0, x1, y1 = bbox
    src_w = x1 - x0
    src_h = y1 - y0
    if src_w <= 0 or src_h <= 0:
        return bytes([background_index]) * (w * h)

    dst_w = max(1, int(round(src_w * scale)))
    dst_h = max(1, int(round(src_h * scale)))

    ax, ay = anchor
    src_anchor_x = x0 + ax * src_w
    src_anchor_y = y0 + ay * src_h
    dst_x0 = int(round(src_anchor_x - ax * dst_w)) + shift_x
    dst_y0 = int(round(src_anchor_y - ay * dst_h)) + shift_y

    out = bytearray([background_index]) * (w * h)
    for dy in range(dst_h):
        oy = dst_y0 + dy
        if oy < 0 or oy >= h:
            continue
        sy = y0 + min(src_h - 1, int(dy * src_h / dst_h))
        src_row = sy * w
        out_row = oy * w
        for dx in range(dst_w):
            ox = dst_x0 + dx
            if ox < 0 or ox >= w:
                continue
            sx = x0 + min(src_w - 1, int(dx * src_w / dst_w))
            out[out_row + ox] = frame[src_row + sx]
    return bytes(out)


def scale_frames(
    frames: Sequence[bytes],
    w: int,
    h: int,
    scale: float,
    background_index: int,
    per_frame_bbox: bool,
    anchor: Tuple[float, float],
    shift_x: int,
    shift_y: int,
) -> List[bytes]:
    if not (0.0 < scale <= 1.0):
        raise ValueError("--scale must be greater than 0 and no greater than 1.")
    if not frames:
        raise ValueError("No decoded animation frames found.")

    shared_bbox = None if per_frame_bbox else union_bbox(frames, w, h, background_index)
    out: List[bytes] = []
    for frame in frames:
        bbox = content_bbox(frame, w, h, background_index) if per_frame_bbox else shared_bbox
        if bbox is None:
            out.append(bytes([background_index]) * (w * h))
            continue
        out.append(
            scale_frame_about_anchor(
                frame=frame,
                w=w,
                h=h,
                scale=scale,
                background_index=background_index,
                bbox=bbox,
                anchor=anchor,
                shift_x=shift_x,
                shift_y=shift_y,
            )
        )
    return out


def combined_anim_time_ms(
    decoded: Sequence[FlcDecoded],
    per_input_lengths: Sequence[int],
    kept_frames_per_direction: int,
    fallback_speed_ms: int,
) -> int:
    remaining = kept_frames_per_direction
    total = 0.0
    for dec, per_dir in zip(decoded, per_input_lengths):
        if remaining <= 0:
            break
        take = min(remaining, per_dir)
        if per_dir > 0 and dec.civ3.anim_time_ms > 0:
            total += (dec.civ3.anim_time_ms / float(per_dir)) * take
        else:
            total += fallback_speed_ms * take
        remaining -= take
    return max(1, int(round(total)))


def direction_blocks(dec: FlcDecoded) -> List[List[bytes]]:
    dir_count = max(1, dec.civ3.num_anims)
    if len(dec.anim_frames) % dir_count != 0:
        raise ValueError(
            f"Decoded frame count {len(dec.anim_frames)} is not divisible by {dir_count} directions."
        )
    per_dir = len(dec.anim_frames) // dir_count
    return [
        list(dec.anim_frames[d * per_dir:(d + 1) * per_dir])
        for d in range(dir_count)
    ]


def combine_decoded_inputs(
    decoded: Sequence[FlcDecoded],
    max_frames_per_direction: int,
    *,
    audit_directions: bool = False,
) -> FlcDecoded:
    if not decoded:
        raise ValueError("No input FLCs were decoded.")
    if max_frames_per_direction < 0:
        raise ValueError("--max-frames-per-direction must be 0 or greater.")

    first = decoded[0]
    dir_count = max(1, first.civ3.num_anims)
    first_directions = first.civ3.directions
    decoded_blocks = [direction_blocks(dec) for dec in decoded]
    per_input_lengths: List[int] = []

    for i, dec in enumerate(decoded, start=1):
        if dec.w != first.w or dec.h != first.h:
            raise ValueError(
                f"Input {i} has canvas {dec.w}x{dec.h}; expected {first.w}x{first.h}."
            )
        if max(1, dec.civ3.num_anims) != dir_count:
            raise ValueError(
                f"Input {i} has {max(1, dec.civ3.num_anims)} directions; expected {dir_count}."
            )
        if dec.civ3.directions != first_directions:
            raise ValueError(
                f"Input {i} has direction mask {dec.civ3.directions}; expected {first_directions}."
            )
        if len(decoded_blocks[i - 1]) != dir_count:
            raise ValueError(
                f"Input {i} decoded into {len(decoded_blocks[i - 1])} direction blocks; expected {dir_count}."
            )
        per_input_lengths.append(len(decoded_blocks[i - 1][0]))

    total_anim_length = sum(per_input_lengths)
    if max_frames_per_direction > 0:
        kept_anim_length = min(total_anim_length, max_frames_per_direction)
    else:
        kept_anim_length = total_anim_length

    combined: List[bytes] = []
    for direction in range(dir_count):
        kept_for_direction = 0
        if audit_directions:
            print(f"Direction {direction}:")
        for input_index, (blocks, per_dir) in enumerate(zip(decoded_blocks, per_input_lengths), start=1):
            if kept_for_direction >= kept_anim_length:
                break
            take = min(per_dir, kept_anim_length - kept_for_direction)
            combined.extend(blocks[direction][:take])
            if audit_directions:
                out_a = kept_for_direction
                out_b = kept_for_direction + take - 1
                print(f"  output frames {out_a}-{out_b}: input {input_index} direction {direction} frames 0-{take - 1}")
            kept_for_direction += take

    total_frames = dir_count * kept_anim_length
    total_anim_time_ms = combined_anim_time_ms(
        decoded=decoded,
        per_input_lengths=per_input_lengths,
        kept_frames_per_direction=kept_anim_length,
        fallback_speed_ms=max(1, first.speed_ms),
    )

    civ = Civ3Tail(
        num_anims=dir_count,
        anim_length=kept_anim_length,
        x_offset=first.civ3.x_offset,
        y_offset=first.civ3.y_offset,
        xs_orig=first.civ3.xs_orig,
        ys_orig=first.civ3.ys_orig,
        anim_time_ms=total_anim_time_ms,
        directions=first.civ3.directions,
    )

    return FlcDecoded(
        w=first.w,
        h=first.h,
        file_frames=total_frames,
        speed_ms=first.speed_ms,
        creator=first.creator,
        civ3=civ,
        palette=first.palette,
        anim_frames=combined,
        include_ring_frame=False,
        direction_uniques=[],
    )


def make_delta_flc_payload(prev: bytes, cur: bytes, w: int, h: int) -> bytes:
    frame_dimensions(prev, w, h)
    frame_dimensions(cur, w, h)

    changed_rows = []
    for y in range(h):
        a = y * w
        b = a + w
        if prev[a:b] != cur[a:b]:
            changed_rows.append(y)

    out = bytearray(pack_u16(len(changed_rows)))
    cursor_y = 0
    for y in changed_rows:
        skip = y - cursor_y
        while skip > 0:
            step = min(skip, 32767)
            out.extend(struct.pack("<h", -step))
            cursor_y += step
            skip -= step

        row = cur[y * w:(y + 1) * w]
        packets = []
        x = 0
        while x < w:
            chunk_len = min(254, w - x)
            payload = row[x:x + chunk_len]
            if len(payload) & 1:
                payload += bytes([0])
            packets.append((0, len(payload) // 2, payload))
            x += chunk_len

        out.extend(struct.pack("<h", len(packets)))
        for skip_x, word_count, payload in packets:
            out.append(skip_x)
            out.append(word_count)
            out.extend(payload)
        cursor_y = y + 1

    return bytes(out)


def write_zoom_flc(path: str, decoded: FlcDecoded, frames: Sequence[bytes], palette: Sequence[int]) -> None:
    if len(palette) != 768:
        raise ValueError("Palette must be 768 bytes.")
    if not frames:
        raise ValueError("No frames to write.")

    w, h = decoded.w, decoded.h
    for frame in frames:
        frame_dimensions(frame, w, h)

    civ = decoded.civ3
    target_frames = max(1, decoded.file_frames)
    if len(frames) != target_frames:
        raise ValueError(f"Expected {target_frames} frames, got {len(frames)}.")

    dir_count = max(1, civ.num_anims)
    anim_len = max(1, civ.anim_length)
    if dir_count * anim_len != len(frames):
        dir_count = 1
        anim_len = len(frames)
        civ.num_anims = 1
        civ.anim_length = anim_len

    chunks: List[bytes] = []
    for d in range(dir_count):
        base = d * anim_len
        key_frame = frames[base]
        key_subchunks = [make_subchunk(CHUNK_BYTE_RUN, make_byte_run_payload(key_frame, w, h))]
        if d == 0:
            key_subchunks.append(make_subchunk(CHUNK_COLOR_256, make_color_256_payload(bytes(palette))))
        chunks.append(make_frame_chunk(key_subchunks))

        prev = key_frame
        for i in range(anim_len):
            cur = frames[base + i]
            delta = make_delta_flc_payload(prev, cur, w, h)
            chunks.append(make_frame_chunk([make_subchunk(CHUNK_DELTA_FLC, delta)]))
            prev = cur

    if decoded.include_ring_frame and dir_count == 1:
        delta = make_delta_flc_payload(frames[-1], frames[0], w, h)
        chunks.append(make_frame_chunk([make_subchunk(CHUNK_DELTA_FLC, delta)]))

    body = b"".join(chunks)
    header = build_flc_header_128(
        total_file_size=128 + len(body),
        frames_without_ring=target_frames,
        w=w,
        h=h,
        speed_ms=decoded.speed_ms,
        creator=decoded.creator,
        oframe1=128,
        oframe2=0,
        civ3_tail_40=civ.pack_40_bytes(),
    )

    with open(path, "wb") as f:
        f.write(header)
        f.write(body)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Make a Civ3 FLC sprite look zoomed out while keeping the original FLC canvas and palette."
    )
    parser.add_argument(
        "--in",
        dest="inputs",
        nargs="+",
        action="append",
        required=True,
        help="Input Civ3 FLC path(s). Repeat --in or pass multiple paths to concatenate animations.",
    )
    parser.add_argument("--out", required=True, help="Output Civ3 FLC path.")
    parser.add_argument(
        "--scale",
        type=float,
        default=0.75,
        help="Visible content scale, where 0.75 means 75%% of original size. Default: 0.75.",
    )
    parser.add_argument(
        "--background-index",
        type=int,
        default=None,
        help="Palette index used to fill newly exposed border pixels. Default: auto-detect from frame edges.",
    )
    parser.add_argument(
        "--anchor",
        type=parse_anchor,
        default=(0.5, 0.5),
        help="Anchor for scaling: center, bottom, top-left, etc., or x,y in 0..1. Default: center.",
    )
    parser.add_argument(
        "--per-frame-bbox",
        action="store_true",
        help="Scale each frame's own non-background bounding box instead of the shared animation bounds.",
    )
    parser.add_argument("--shift-x", type=int, default=0, help="Move scaled content horizontally after scaling.")
    parser.add_argument("--shift-y", type=int, default=0, help="Move scaled content vertically after scaling.")
    parser.add_argument(
        "--max-frames-per-direction",
        type=int,
        default=64,
        help="Truncate each Civ3 direction to this many frames after concatenation. Use 0 for no cap. Default: 64.",
    )
    parser.add_argument(
        "--audit-directions",
        action="store_true",
        help="Print the per-direction input ranges used in the combined output.",
    )

    args = parser.parse_args()

    input_paths = flatten_inputs(args.inputs)
    decoded_inputs = [decode_flc(path) for path in input_paths]
    dec = combine_decoded_inputs(
        decoded_inputs,
        args.max_frames_per_direction,
        audit_directions=args.audit_directions,
    )
    bg = args.background_index
    if bg is None:
        bg = choose_background_index(dec.anim_frames, dec.w, dec.h)
    if bg < 0 or bg > 255:
        raise SystemExit("--background-index must be in the range 0..255.")

    scaled = scale_frames(
        frames=dec.anim_frames,
        w=dec.w,
        h=dec.h,
        scale=args.scale,
        background_index=bg,
        per_frame_bbox=args.per_frame_bbox,
        anchor=args.anchor,
        shift_x=args.shift_x,
        shift_y=args.shift_y,
    )

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    write_zoom_flc(args.out, dec, scaled, dec.palette)

    print(f"Inputs: {len(input_paths)}")
    for path in input_paths:
        print(f"  {path}")
    print(f"Output: {args.out}")
    print(f"Canvas: {dec.w}x{dec.h}")
    print(f"Header animation frames: {dec.file_frames}")
    print(f"Decoded/scaled animation frames: {len(scaled)}")
    print(f"Palette source: first input ({len(dec.palette)} bytes)")
    print(f"Background index: {bg}")
    print(f"Scale: {args.scale:g}")
    print(f"Civ3 directions: {dec.civ3.num_anims}")
    print(f"Frames per direction: {dec.civ3.anim_length}")
    print(f"Max frames per direction: {args.max_frames_per_direction or 'none'}")
    print("Animation chunks: DELTA_FLC")


if __name__ == "__main__":
    main()
