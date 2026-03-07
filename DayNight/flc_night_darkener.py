#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Darken a Civ3 FLC animation using the same palette tonemapping math as civ3_day_night.py.

Key behavior:
- Reads an input FLC and decodes animation frames (supports COLOR_256, BYTE_RUN, FLI_COPY,
  DELTA_FLC, DELTA_FLI, BLACK).
- Computes which palette indices are actually used by animation frames.
- Applies civ3_day_night palette darkening to only those used indices.
- Preserves reserved Civ3 indices/colors (by default first 63 and last 20 indices, plus #FF00FF).
- Re-emits a Civ3-compatible FLC (BYTE_RUN keyframe + COLOR_256 + LITERAL animation frames).
"""

import argparse
import hashlib
import struct
from dataclasses import dataclass
from typing import Iterable, List, Sequence, Set, Tuple

import civ3_day_night as dn

FLC_MAGIC = 0xAF12
CHUNK_FRAME = 0xF1FA
CHUNK_COLOR_256 = 4
CHUNK_DELTA_FLC = 7
CHUNK_DELTA_FLI = 12
CHUNK_BLACK = 13
CHUNK_BYTE_RUN = 15
CHUNK_FLI_COPY = 16

CIV3_CREATOR_DEFAULT = 0xF1F1F2F2


def u16(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


def s16(data: bytes, off: int) -> int:
    return struct.unpack_from("<h", data, off)[0]


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def s8(b: int) -> int:
    return b - 256 if b > 127 else b


def pack_u16(v: int) -> bytes:
    return struct.pack("<H", v & 0xFFFF)


def pack_i32(v: int) -> bytes:
    return struct.pack("<i", int(v))


def pack_u32(v: int) -> bytes:
    return struct.pack("<I", v & 0xFFFFFFFF)


@dataclass
class Civ3Tail:
    num_anims: int
    anim_length: int
    x_offset: int
    y_offset: int
    xs_orig: int
    ys_orig: int
    anim_time_ms: int
    directions: int

    def pack_40_bytes(self) -> bytes:
        core = (
            pack_u32(28) +
            pack_i32(0) +
            pack_u16(self.num_anims) +
            pack_u16(self.anim_length) +
            pack_u16(self.x_offset) +
            pack_u16(self.y_offset) +
            pack_u16(self.xs_orig) +
            pack_u16(self.ys_orig) +
            pack_u32(self.anim_time_ms) +
            pack_i32(self.directions)
        )
        return core + (b"\x00" * 12)


@dataclass
class FlcDecoded:
    w: int
    h: int
    file_frames: int
    speed_ms: int
    creator: int
    civ3: Civ3Tail
    palette: List[int]
    anim_frames: List[bytes]
    include_ring_frame: bool
    direction_uniques: List[int]


def parse_civ3_tail(hdr: bytes, w: int, h: int, frame_count: int) -> Civ3Tail:
    if len(hdr) < 128:
        return Civ3Tail(1, frame_count, 0, 0, w, h, frame_count * 100, 1)
    try:
        num_anims = u16(hdr, 96)
        anim_length = u16(hdr, 98)
        x_offset = u16(hdr, 100)
        y_offset = u16(hdr, 102)
        xs_orig = u16(hdr, 104)
        ys_orig = u16(hdr, 106)
        anim_time_ms = u32(hdr, 108)
        directions = struct.unpack_from("<i", hdr, 112)[0]
        if num_anims <= 0:
            num_anims = 1
        if anim_length <= 0:
            anim_length = frame_count
        return Civ3Tail(
            num_anims=num_anims,
            anim_length=anim_length,
            x_offset=x_offset,
            y_offset=y_offset,
            xs_orig=xs_orig,
            ys_orig=ys_orig,
            anim_time_ms=anim_time_ms,
            directions=directions,
        )
    except Exception:
        return Civ3Tail(1, frame_count, 0, 0, w, h, frame_count * 100, 1)


def decode_color_256(payload: bytes, pal: List[int]) -> None:
    if len(payload) < 2:
        return
    packets = u16(payload, 0)
    p = 2
    idx = 0
    for _ in range(packets):
        if p + 2 > len(payload):
            break
        skip = payload[p]
        count = payload[p + 1]
        p += 2
        idx += skip
        if count == 0:
            count = 256
        for _ in range(count):
            if p + 3 > len(payload) or idx >= 256:
                break
            pal[3 * idx + 0] = payload[p + 0]
            pal[3 * idx + 1] = payload[p + 1]
            pal[3 * idx + 2] = payload[p + 2]
            p += 3
            idx += 1


def decode_byte_run(payload: bytes, w: int, h: int) -> bytes:
    out = bytearray(w * h)
    p = 0
    for y in range(h):
        if p >= len(payload):
            break
        p += 1
        x = 0
        row_off = y * w
        while x < w and p < len(payload):
            n = s8(payload[p])
            p += 1
            if n >= 0:
                if p >= len(payload):
                    break
                b = payload[p]
                p += 1
                run = min(n, w - x)
                out[row_off + x:row_off + x + run] = bytes([b]) * run
                x += run
            else:
                run = min(-n, w - x, len(payload) - p)
                out[row_off + x:row_off + x + run] = payload[p:p + run]
                p += run
                x += run
    return bytes(out)


def decode_delta_fli(payload: bytes, frame: bytearray, w: int, h: int) -> None:
    if len(payload) < 4:
        return
    y = u16(payload, 0)
    lines = u16(payload, 2)
    p = 4
    while lines > 0 and y < h and p < len(payload):
        packets = payload[p]
        p += 1
        x = 0
        row_off = y * w
        for _ in range(packets):
            if p + 2 > len(payload):
                break
            x += payload[p]
            p += 1
            n = s8(payload[p])
            p += 1
            if n >= 0:
                cnt = n
                avail = min(cnt, len(payload) - p)
                write = min(avail, max(0, w - x))
                if write > 0:
                    frame[row_off + x:row_off + x + write] = payload[p:p + write]
                    x += write
                p += avail
                x += max(0, cnt - write)
            else:
                if p >= len(payload):
                    break
                b = payload[p]
                p += 1
                run = min(-n, w - x)
                frame[row_off + x:row_off + x + run] = bytes([b]) * run
                x += run
        y += 1
        lines -= 1


def decode_delta_flc(payload: bytes, frame: bytearray, w: int, h: int) -> None:
    if len(payload) < 2:
        return
    lines = u16(payload, 0)
    p = 2
    y = 0
    while lines > 0 and y < h and p + 2 <= len(payload):
        op = s16(payload, p)
        p += 2

        if op < 0:
            flag = op & 0xC000
            if flag == 0xC000:
                y += -op
                continue
            if flag == 0x8000:
                if w > 0 and y < h:
                    frame[y * w + (w - 1)] = op & 0xFF
                continue
            continue

        packets = op
        x = 0
        row_off = y * w
        for _ in range(packets):
            if p + 2 > len(payload):
                break
            x += payload[p]
            p += 1
            n = s8(payload[p])
            p += 1
            if n >= 0:
                cnt = n * 2
                avail = min(cnt, len(payload) - p)
                write = min(avail, max(0, w - x))
                if write > 0:
                    frame[row_off + x:row_off + x + write] = payload[p:p + write]
                    x += write
                p += avail
                x += max(0, cnt - write)
            else:
                if p + 2 > len(payload):
                    break
                b0 = payload[p]
                b1 = payload[p + 1]
                p += 2
                reps = -n
                for _ in range(reps):
                    if x + 1 >= w:
                        break
                    frame[row_off + x] = b0
                    frame[row_off + x + 1] = b1
                    x += 2

        y += 1
        lines -= 1


def decode_flc(path: str) -> FlcDecoded:
    data = open(path, "rb").read()
    if len(data) < 128:
        raise SystemExit("Input file is too small to be a valid FLC.")
    magic = u16(data, 4)
    if magic != FLC_MAGIC:
        raise SystemExit(f"Unsupported magic 0x{magic:04X}; expected 0x{FLC_MAGIC:04X}.")

    file_frames = u16(data, 6)
    w = u16(data, 8)
    h = u16(data, 10)
    speed_ms = u32(data, 16)
    creator = u32(data, 26)

    pal = [0] * (256 * 3)
    frame = bytearray(w * h)

    decoded_all: List[bytes] = []
    frame_is_brun: List[bool] = []
    off = 128
    while off + 6 <= len(data):
        csize = u32(data, off)
        ctype = u16(data, off + 4)
        if csize < 6 or off + csize > len(data):
            break
        if ctype == CHUNK_FRAME and csize >= 16:
            nsub = u16(data, off + 6)
            sub_off = off + 16
            frame_end = off + csize
            has_brun = False
            for _ in range(nsub):
                if sub_off + 6 > frame_end:
                    break
                ssize = u32(data, sub_off)
                stype = u16(data, sub_off + 4)
                if ssize < 6 or sub_off + ssize > frame_end:
                    break
                payload = data[sub_off + 6:sub_off + ssize]

                if stype == CHUNK_COLOR_256:
                    decode_color_256(payload, pal)
                elif stype == CHUNK_BYTE_RUN:
                    has_brun = True
                    frame = bytearray(decode_byte_run(payload, w, h))
                elif stype == CHUNK_FLI_COPY:
                    need = w * h
                    if len(payload) >= need:
                        frame = bytearray(payload[:need])
                elif stype == CHUNK_BLACK:
                    frame = bytearray(w * h)
                elif stype == CHUNK_DELTA_FLI:
                    decode_delta_fli(payload, frame, w, h)
                elif stype == CHUNK_DELTA_FLC:
                    decode_delta_flc(payload, frame, w, h)
                sub_off += ssize

            decoded_all.append(bytes(frame))
            frame_is_brun.append(has_brun)
        off += csize

    civ3 = parse_civ3_tail(data[:128], w, h, max(1, file_frames))
    expected_anim = max(1, int(file_frames))

    # Prefer Civ3 multi-direction layout: per direction => keyframe + anim_length frames.
    extracted = False
    anim_frames: List[bytes]
    include_ring = False
    if civ3.num_anims > 0 and civ3.anim_length > 0 and civ3.num_anims * civ3.anim_length == expected_anim:
        per_dir = civ3.anim_length
        dirs = civ3.num_anims
        needed = dirs * (per_dir + 1)
        if len(decoded_all) >= needed:
            temp: List[bytes] = []
            ok = True
            for d in range(dirs):
                k = d * (per_dir + 1)
                if not frame_is_brun[k]:
                    ok = False
                    break
                a0 = k + 1
                a1 = a0 + per_dir
                if a1 > len(decoded_all):
                    ok = False
                    break
                temp.extend(decoded_all[a0:a1])
            if ok and len(temp) == expected_anim:
                anim_frames = temp
                include_ring = len(decoded_all) > needed
                extracted = True

    if not extracted and len(decoded_all) >= expected_anim + 1:
        # Single keyframe layout: one leading BYTE_RUN keyframe, then animated frames.
        anim_frames = decoded_all[1:1 + expected_anim]
        include_ring = len(decoded_all) > (1 + expected_anim)
    elif len(decoded_all) >= expected_anim:
        anim_frames = decoded_all[:expected_anim]
        include_ring = len(decoded_all) > expected_anim
    else:
        anim_frames = decoded_all[:] if decoded_all else [bytes(frame)]
        include_ring = False

    if civ3.num_anims <= 0:
        civ3.num_anims = 1
    if expected_anim % civ3.num_anims == 0:
        civ3.anim_length = expected_anim // civ3.num_anims
    else:
        civ3.num_anims = 1
        civ3.anim_length = expected_anim
    direction_uniques: List[int] = []
    if civ3.num_anims > 0 and len(anim_frames) >= civ3.num_anims and len(anim_frames) % civ3.num_anims == 0:
        per_dir = len(anim_frames) // civ3.num_anims
        for d in range(civ3.num_anims):
            seg = anim_frames[d * per_dir:(d + 1) * per_dir]
            direction_uniques.append(len(set(seg)))

    return FlcDecoded(
        w=w,
        h=h,
        file_frames=expected_anim,
        speed_ms=speed_ms,
        creator=creator if creator else CIV3_CREATOR_DEFAULT,
        civ3=civ3,
        palette=pal,
        anim_frames=anim_frames,
        include_ring_frame=include_ring,
        direction_uniques=direction_uniques,
    )


def iter_color_chunk_payload_spans(data: bytes) -> List[Tuple[int, int]]:
    spans: List[Tuple[int, int]] = []
    off = 128
    n = len(data)
    while off + 6 <= n:
        csize = u32(data, off)
        ctype = u16(data, off + 4)
        if csize < 6 or off + csize > n:
            break
        if ctype == CHUNK_FRAME and csize >= 16:
            frame_end = off + csize
            nsub = u16(data, off + 6)
            so = off + 16
            for _ in range(nsub):
                if so + 6 > frame_end:
                    break
                ssize = u32(data, so)
                stype = u16(data, so + 4)
                if ssize < 6 or so + ssize > frame_end:
                    break
                if stype == CHUNK_COLOR_256:
                    spans.append((so + 6, so + ssize))
                so += ssize
        off += csize
    return spans


def patch_color_256_payload(payload: bytearray, new_pal: Sequence[int]) -> None:
    if len(payload) < 2:
        return
    packets = u16(payload, 0)
    p = 2
    idx = 0
    for _ in range(packets):
        if p + 2 > len(payload):
            break
        skip = payload[p]
        count = payload[p + 1]
        p += 2
        idx += skip
        cnt = 256 if count == 0 else count
        for _ in range(cnt):
            if p + 3 > len(payload) or idx >= 256:
                break
            payload[p + 0] = new_pal[3 * idx + 0]
            payload[p + 1] = new_pal[3 * idx + 1]
            payload[p + 2] = new_pal[3 * idx + 2]
            p += 3
            idx += 1


def patch_flc_palette_only(inp: str, out: str, new_pal: Sequence[int]) -> int:
    data = bytearray(open(inp, "rb").read())
    spans = iter_color_chunk_payload_spans(data)
    for a, b in spans:
        payload = bytearray(data[a:b])
        patch_color_256_payload(payload, new_pal)
        data[a:b] = payload
    with open(out, "wb") as f:
        f.write(data)
    return len(spans)


def parse_ranges(spec: str) -> Set[int]:
    out: Set[int] = set()
    for raw in (spec or "").split(","):
        s = raw.strip()
        if not s:
            continue
        if "-" in s:
            a_str, b_str = s.split("-", 1)
            a = int(a_str)
            b = int(b_str)
            if a > b:
                a, b = b, a
            for i in range(max(0, a), min(255, b) + 1):
                out.add(i)
        else:
            i = int(s)
            if 0 <= i <= 255:
                out.add(i)
    return out


def parse_rgb(s: str) -> Tuple[int, int, int]:
    return dn.parse_rgb(s)


def is_magenta_like(rgb: Tuple[int, int, int]) -> bool:
    return rgb[0] == 255 and rgb[2] == 255


def used_indices(frames: Sequence[bytes]) -> Set[int]:
    used: Set[int] = set()
    for fr in frames:
        used.update(fr)
    return used


def apply_darkening_to_used_indices(
    pal: List[int],
    used: Set[int],
    reserved_indices: Set[int],
    reserved_colors: Set[Tuple[int, int, int]],
    preserve_magenta_like: bool,
    *,
    hour: float,
    warmth: float,
    blue: float,
    darkness: float,
    desat: float,
    sat: float,
    contrast: float,
    sunrise_center: float,
    sunset_center: float,
    twilight_width: float,
    noon_blend: float,
    noon_sigma: float,
    noon_window_start: float,
    noon_window_end: float,
    noon_window_soft: float,
) -> Tuple[List[int], int]:
    adjusted = dn.adjust_palette_for_time(
        pal,
        hour,
        reserved_colors,
        reserved_indices=reserved_indices,
        warmth_scale=warmth,
        blue_scale=blue,
        darkness_scale=darkness,
        desat_scale=desat,
        sat_boost=sat,
        contrast=contrast,
        sunrise_center=sunrise_center,
        sunset_center=sunset_center,
        twilight_sigma=twilight_width,
        noon_blend=noon_blend,
        noon_sigma=noon_sigma,
        noon_window_start=noon_window_start,
        noon_window_end=noon_window_end,
        noon_window_soft=noon_window_soft,
    )

    out = pal[:]
    changed = 0
    for i in sorted(used):
        if i in reserved_indices:
            continue
        r, g, b = pal[3 * i:3 * i + 3]
        if (r, g, b) in reserved_colors:
            continue
        if preserve_magenta_like and is_magenta_like((r, g, b)):
            continue

        nr, ng, nb = adjusted[3 * i:3 * i + 3]
        if (nr, ng, nb) != (r, g, b):
            changed += 1
        out[3 * i + 0] = nr
        out[3 * i + 1] = ng
        out[3 * i + 2] = nb
    return out, changed


def make_color_256_payload(pal768: bytes) -> bytes:
    return struct.pack("<HBB", 1, 0, 0) + pal768


def make_subchunk(chunk_type: int, payload: bytes) -> bytes:
    size = 6 + len(payload)
    return pack_u32(size) + pack_u16(chunk_type) + payload


def make_frame_chunk(subchunks: List[bytes]) -> bytes:
    payload = b"".join(subchunks)
    size = 16 + len(payload)
    return pack_u32(size) + pack_u16(CHUNK_FRAME) + pack_u16(len(subchunks)) + (b"\x00" * 8) + payload


def make_byte_run_payload(pix: bytes, w: int, h: int) -> bytes:
    out = bytearray()
    for y in range(h):
        row = pix[y * w:(y + 1) * w]
        # Civ3FlcEdit writes this as 0xCD and ignores it on read.
        out.append(0xCD)
        x = 0
        while x < w:
            run = 1
            while x + run < w and run < 127 and row[x + run] == row[x]:
                run += 1
            if run >= 2:
                out.append(run & 0xFF)
                out.append(row[x])
                x += run
                continue

            start = x
            x += 1
            while x < w and (x - start) < 127:
                look = 1
                while x + look < w and look < 127 and row[x + look] == row[x]:
                    look += 1
                if look >= 2:
                    break
                x += 1
            n = x - start
            out.append((256 - n) & 0xFF)
            out.extend(row[start:x])
    return bytes(out)


def build_flc_header_128(
    total_file_size: int,
    frames_without_ring: int,
    w: int,
    h: int,
    speed_ms: int,
    creator: int,
    oframe1: int,
    oframe2: int,
    civ3_tail_40: bytes,
) -> bytes:
    hdr = bytearray(b"\x00" * 128)
    hdr[0:4] = pack_u32(total_file_size)
    hdr[4:6] = pack_u16(FLC_MAGIC)
    hdr[6:8] = pack_u16(frames_without_ring)
    hdr[8:10] = pack_u16(w)
    hdr[10:12] = pack_u16(h)
    hdr[12:14] = pack_u16(8)
    hdr[14:16] = pack_u16(0x0003)
    hdr[16:20] = pack_u32(speed_ms)
    hdr[26:30] = pack_u32(creator)
    hdr[38:40] = pack_u16(1)
    hdr[40:42] = pack_u16(1)
    hdr[80:84] = pack_u32(oframe1)
    hdr[84:88] = pack_u32(oframe2)
    hdr[88:128] = civ3_tail_40
    return bytes(hdr)


def write_flc(path: str, decoded: FlcDecoded, palette: Sequence[int]) -> None:
    if len(palette) != 768:
        raise ValueError("Palette must be 768 bytes.")
    w, h = decoded.w, decoded.h
    frames = decoded.anim_frames
    if not frames:
        raise SystemExit("No animation frames decoded from input FLC.")

    civ = decoded.civ3
    target_frames = max(1, decoded.file_frames)
    if len(frames) > target_frames:
        frames = frames[:target_frames]
    elif len(frames) < target_frames:
        raise SystemExit(
            f"Decoded only {len(frames)} animation frames, but header expects {target_frames}. "
            "Refusing to pad/repeat frames."
        )

    if civ.num_anims <= 0:
        civ.num_anims = 1
    if target_frames % civ.num_anims == 0:
        civ.anim_length = target_frames // civ.num_anims
    else:
        civ.num_anims = 1
        civ.anim_length = target_frames
    civ_tail = civ.pack_40_bytes()

    chunks: List[bytes] = []
    anim_len = max(1, civ.anim_length)
    dir_count = max(1, civ.num_anims)

    if dir_count * anim_len != len(frames):
        dir_count = 1
        anim_len = len(frames)
        civ.num_anims = 1
        civ.anim_length = anim_len
        civ_tail = civ.pack_40_bytes()

    for d in range(dir_count):
        base = d * anim_len
        key_fr = frames[base]
        if d == 0:
            key_sub = [
                make_subchunk(CHUNK_BYTE_RUN, make_byte_run_payload(key_fr, w, h)),
                make_subchunk(CHUNK_COLOR_256, make_color_256_payload(bytes(palette))),
            ]
        else:
            key_sub = [make_subchunk(CHUNK_BYTE_RUN, make_byte_run_payload(key_fr, w, h))]
        chunks.append(make_frame_chunk(key_sub))

        for i in range(anim_len):
            fr = frames[base + i]
            chunks.append(make_frame_chunk([make_subchunk(CHUNK_FLI_COPY, fr)]))

    if decoded.include_ring_frame:
        chunks.append(make_frame_chunk([make_subchunk(CHUNK_BYTE_RUN, make_byte_run_payload(frames[0], w, h))]))

    body = b"".join(chunks)
    total = 128 + len(body)
    frame_count = target_frames

    header = build_flc_header_128(
        total_file_size=total,
        frames_without_ring=frame_count,
        w=w,
        h=h,
        speed_ms=decoded.speed_ms,
        creator=decoded.creator,
        oframe1=128,
        oframe2=0,
        civ3_tail_40=civ_tail,
    )

    with open(path, "wb") as f:
        f.write(header)
        f.write(body)


def main() -> None:
    p = argparse.ArgumentParser(
        description="Darken a Civ3 FLC palette for night mode using civ3_day_night tonemapping."
    )
    p.add_argument("--in", dest="inp", required=True, help="Input FLC path.")
    p.add_argument("--out", required=True, help="Output FLC path.")

    p.add_argument("--hour", type=float, default=22.0,
                   help="Target hour used for tonemapping (default 22.0 = ~10pm).")

    p.add_argument("--warmth", type=float, default=1.10)
    p.add_argument("--blue", type=float, default=1.12)
    p.add_argument("--darkness", type=float, default=1.08)
    p.add_argument("--desat", type=float, default=0.85)
    p.add_argument("--sat", type=float, default=1.05)
    p.add_argument("--contrast", type=float, default=1.03)
    p.add_argument("--sunrise-center", type=float, default=6.0)
    p.add_argument("--sunset-center", type=float, default=18.0)
    p.add_argument("--twilight-width", type=float, default=1.8)

    p.add_argument("--noon-blend", type=float, default=0.85)
    p.add_argument("--noon-sigma", type=float, default=1.1)
    p.add_argument("--noon-window-start", type=float, default=10.0)
    p.add_argument("--noon-window-end", type=float, default=14.0)
    p.add_argument("--noon-window-soft", type=float, default=0.7)

    p.add_argument("--reserve-index-ranges", default="0-62,236-255",
                   help="Comma-separated index ranges to preserve (default first 63 and last 20).")
    p.add_argument("--preserve-rgb", action="append", default=["#ff00ff"],
                   help="Exact RGB colors to preserve. Repeatable. Default includes #ff00ff.")
    p.add_argument("--no-preserve-magenta-like", action="store_true",
                   help="Allow changing colors with R=255 and B=255 (disabled by default).")

    args = p.parse_args()

    dec = decode_flc(args.inp)
    used = used_indices(dec.anim_frames)

    reserved_indices = parse_ranges(args.reserve_index_ranges)
    reserved_colors = set(parse_rgb(s) for s in args.preserve_rgb)

    new_pal, changed = apply_darkening_to_used_indices(
        dec.palette,
        used=used,
        reserved_indices=reserved_indices,
        reserved_colors=reserved_colors,
        preserve_magenta_like=not args.no_preserve_magenta_like,
        hour=args.hour,
        warmth=args.warmth,
        blue=args.blue,
        darkness=args.darkness,
        desat=args.desat,
        sat=args.sat,
        contrast=args.contrast,
        sunrise_center=args.sunrise_center,
        sunset_center=args.sunset_center,
        twilight_width=args.twilight_width,
        noon_blend=args.noon_blend,
        noon_sigma=args.noon_sigma,
        noon_window_start=args.noon_window_start,
        noon_window_end=args.noon_window_end,
        noon_window_soft=args.noon_window_soft,
    )

    patched_chunks = patch_flc_palette_only(args.inp, args.out, new_pal)
    frame_hashes = [hashlib.md5(fr).hexdigest() for fr in dec.anim_frames]
    unique_frames = len(set(frame_hashes))
    print(f"Input header frames: {dec.file_frames}")
    print(f"Output animation frames: {dec.file_frames}")
    print(f"Unique decoded animation frames: {unique_frames}")
    if dec.direction_uniques:
        print(f"Per-direction unique decoded frames: {dec.direction_uniques}")
    print(f"Patched COLOR_256 chunks: {patched_chunks}")
    print(f"Used palette indices in animation: {len(used)}")
    print(f"Changed used indices: {changed}")
    print(f"Wrote: {args.out}")


if __name__ == "__main__":
    main()
