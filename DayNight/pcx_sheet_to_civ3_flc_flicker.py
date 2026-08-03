#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
pcx_lights_sheet_to_civ3_flc_overlay.py

Generate Civ3-compatible-ish FLC overlay animations from a Civ3-style *_lights.pcx annotation sheet.
This script IMPORTS civ3_city_lights.py to reuse the exact glow math / palette rules.

Input:  PCX sheet (ideally indexed P mode) where specific "light key" colors mark light sources.
Output: one .flc per (row, col) cell, containing:
        - one BYTE_RUN keyframe (not counted in header frame count; FLICster-friendly)
        - N animation frames (LITERAL) with flicker
        - optional ring frame

Defaults: cell size 128x64

Notes:
- We render a TRANSPARENT overlay: background becomes MAGENTA (255,0,255) at palette index 255.
- We do NOT need a base city image; this is just the glow overlay from the annotation mask.
"""

import argparse
import math
import os
import struct
from dataclasses import dataclass
from typing import List, Tuple, Dict, Optional

from PIL import Image, ImageChops

# Import your compositor module (must be accessible)
import civ3_city_lights as c3


# -----------------------------
# FLC constants
# -----------------------------
FLC_MAGIC = 0xAF12
CHUNK_FRAME = 0xF1FA
CHUNK_COLOR_256 = 4
CHUNK_BYTE_RUN = 15
CHUNK_LITERAL = 16

CIV3_CREATOR = 0xF1F1F2F2
CIV3_SPEED = 4  # Civ3 convention


# -----------------------------
# Packing helpers
# -----------------------------
def clamp8(v: float) -> int:
    if v <= 0:
        return 0
    if v >= 255:
        return 255
    return int(v + 0.5)


def pack_u16(v: int) -> bytes:
    return struct.pack("<H", v & 0xFFFF)

def pack_i32(v: int) -> bytes:
    return struct.pack("<i", int(v))

def pack_u32(v: int) -> bytes:
    return struct.pack("<I", v & 0xFFFFFFFF)


def palette_bytes_768(im_p: Image.Image) -> bytes:
    pal = im_p.getpalette() or []
    if len(pal) < 768:
        pal = pal + [0] * (768 - len(pal))
    return bytes(pal[:768])


# -----------------------------
# FLC chunk builders
# -----------------------------
def make_color_256_payload(pal768: bytes) -> bytes:
    """
    COLOR_256 payload packetized:
      u16 packet_count
      packet: u8 skip, u8 count(0=>256), RGB*count
    """
    if len(pal768) != 768:
        raise ValueError("Palette must be 768 bytes")
    return struct.pack("<HBB", 1, 0, 0) + pal768

def make_subchunk(chunk_type: int, payload: bytes) -> bytes:
    size = 6 + len(payload)
    return pack_u32(size) + pack_u16(chunk_type) + payload

def make_frame_chunk(subchunks: List[bytes]) -> bytes:
    payload = b"".join(subchunks)
    size = 16 + len(payload)
    return pack_u32(size) + pack_u16(CHUNK_FRAME) + pack_u16(len(subchunks)) + (b"\x00" * 8) + payload

def make_byte_run_payload(pix: bytes, w: int, h: int) -> bytes:
    """
    FLI_BRUN (type 15) payload.
    Per scanline:
      u8 packets (often ignored)
      signed size packets until line is full:
        size > 0 : repeat next byte size times
        size < 0 : copy next abs(size) literal bytes
    """
    if len(pix) != w * h:
        raise ValueError("BYTE_RUN payload size mismatch")

    out = bytearray()
    for y in range(h):
        row = pix[y * w:(y + 1) * w]
        out.append(0)  # legacy packet count
        x = 0
        while x < w:
            # RLE run
            run = 1
            while x + run < w and run < 127 and row[x + run] == row[x]:
                run += 1
            if run >= 2:
                out.append(run & 0xFF)
                out.append(row[x])
                x += run
                continue

            # Literal run
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
            out.append((256 - n) & 0xFF)  # negative signed byte as u8
            out.extend(row[start:x])

    return bytes(out)


# -----------------------------
# Civ3 custom header (FlicAnimHeader) tail
# -----------------------------
@dataclass
class Civ3FlicAnimHeader:
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
        if len(core) != 28:
            raise AssertionError("Civ3FlicAnimHeader core must be 28 bytes")
        return core + (b"\x00" * 12)


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
    if len(civ3_tail_40) != 40:
        raise ValueError("civ3_tail_40 must be 40 bytes")

    hdr = bytearray(b"\x00" * 128)
    hdr[0:4] = pack_u32(total_file_size)
    hdr[4:6] = pack_u16(FLC_MAGIC)
    hdr[6:8] = pack_u16(frames_without_ring)
    hdr[8:10] = pack_u16(w)
    hdr[10:12] = pack_u16(h)
    hdr[12:14] = pack_u16(8)          # depth
    hdr[14:16] = pack_u16(0x0003)     # flags
    hdr[16:20] = pack_u32(speed_ms)
    hdr[20:22] = pack_u16(0)

    hdr[26:30] = pack_u32(creator)
    hdr[38:40] = pack_u16(1)          # aspectx
    hdr[40:42] = pack_u16(1)          # aspecty

    hdr[80:84] = pack_u32(oframe1)
    hdr[84:88] = pack_u32(oframe2)

    hdr[88:128] = civ3_tail_40
    return bytes(hdr)


def write_flc(
    out_path: str,
    frames_p: List[Image.Image],  # P-mode frames (indexed), same size
    fps: float,
    civ_num_anims: int,
    civ_directions_bitmask: int,
    civ_x_offset: int,
    civ_y_offset: int,
    civ_xs_orig: int,
    civ_ys_orig: int,
    include_ring_frame: bool,
    flc_speed_ms: int,
) -> None:
    if not frames_p:
        raise ValueError("No frames")
    w, h = frames_p[0].size
    for im in frames_p:
        if im.mode != "P":
            raise ValueError("All frames must be 'P' mode")
        if im.size != (w, h):
            raise ValueError("All frames must have identical size")

    anim_length = len(frames_p)
    anim_time_ms = int(round(anim_length * 1000.0 / max(0.001, fps)))

    civ_hdr = Civ3FlicAnimHeader(
        num_anims=civ_num_anims,
        anim_length=anim_length,
        x_offset=civ_x_offset,
        y_offset=civ_y_offset,
        xs_orig=civ_xs_orig,
        ys_orig=civ_ys_orig,
        anim_time_ms=anim_time_ms,
        directions=civ_directions_bitmask,
    ).pack_40_bytes()

    pal768 = palette_bytes_768(frames_p[0])

    # Build chunks:
    # - one BRUN keyframe with palette (not counted in header frames)
    # - then LITERAL frames (counted)
    frame_chunks: List[bytes] = []

    key_pix = frames_p[0].tobytes()
    key_subchunks = [
        make_subchunk(CHUNK_BYTE_RUN, make_byte_run_payload(key_pix, w, h)),
        make_subchunk(CHUNK_COLOR_256, make_color_256_payload(pal768)),
    ]
    frame_chunks.append(make_frame_chunk(key_subchunks))

    for im in frames_p:
        frame_chunks.append(make_frame_chunk([make_subchunk(CHUNK_LITERAL, im.tobytes())]))

    body = b"".join(frame_chunks)

    if include_ring_frame:
        ring_pix = frames_p[0].tobytes()
        ring = make_frame_chunk([make_subchunk(CHUNK_BYTE_RUN, make_byte_run_payload(ring_pix, w, h))])
        body += ring

    total_size = 128 + len(body)

    # Civ3 expects header frames = num_anims * anim_length (excluding keyframe and ring)
    frames_without_ring = civ_num_anims * anim_length

    oframe1 = 128
    oframe2 = 0  # many Civ3-related tools tolerate/expect 0 here

    header = build_flc_header_128(
        total_file_size=total_size,
        frames_without_ring=frames_without_ring,
        w=w,
        h=h,
        speed_ms=flc_speed_ms,
        creator=CIV3_CREATOR,
        oframe1=oframe1,
        oframe2=oframe2,
        civ3_tail_40=civ_hdr,
    )

    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(header)
        f.write(body)


# -----------------------------
# Overlay glow rendering (reusing civ3_city_lights functions)
# -----------------------------
def _flicker_multiplier(frame_i: int, frames: int, fps: float, phase: float, amp: float, hz1: float, hz2: float) -> float:
    t = frame_i / max(1.0, fps)
    a = math.sin(2.0 * math.pi * hz1 * t + phase)
    b = math.sin(2.0 * math.pi * hz2 * t + phase * 1.37 + 0.9)
    v = math.tanh((0.65 * a + 0.35 * b) * 1.2)
    # Keep weight in [0, 1] for build_glow_maps(wtime=...).
    # Center at 0.5 so both dimming and brightening are represented.
    return max(0.0, min(1.0, 0.5 + amp * v))

def _phase_for_cell(r: int, c: int, seed: int) -> float:
    # deterministic cell-level phase
    v = (r * 10007 + c * 10009 + seed * 700001) & 0xFFFFFFFF
    v ^= (v >> 16)
    frac = (v & 0x00FFFFFF) / float(0x01000000)
    return frac * 2.0 * math.pi


def _coord_hash_u32(x: int, y: int, seed: int) -> int:
    v = (x * 374761393 + y * 668265263 + seed * 700001) & 0xFFFFFFFF
    v ^= (v >> 13) & 0xFFFFFFFF
    v = (v * 1274126177) & 0xFFFFFFFF
    v ^= (v >> 16) & 0xFFFFFFFF
    return v


def apply_per_pixel_flicker(
    overlay_rgba: Image.Image,
    frame_i: int,
    fps: float,
    amp: float,
    hz1: float,
    hz2: float,
    seed: int,
) -> Image.Image:
    """
    Subtle pixel-level modulation so lights don't all move in lockstep.
    Background magenta is preserved exactly.
    """
    w, h = overlay_rgba.size
    out = overlay_rgba.copy()
    px = out.load()
    t = frame_i / max(1.0, fps)

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if (r, g, b) == c3.MAGENTA:
                continue
            ph = (_coord_hash_u32(x, y, seed) & 0x00FFFFFF) / float(0x01000000) * (2.0 * math.pi)
            s1 = math.sin(2.0 * math.pi * hz1 * t + ph)
            s2 = math.sin(2.0 * math.pi * hz2 * t + ph * 1.37 + 0.9)
            v = math.tanh((0.65 * s1 + 0.35 * s2) * 1.2)
            mult = max(0.0, 1.0 + amp * v)
            px[x, y] = (clamp8(r * mult), clamp8(g * mult), clamp8(b * mult), a)

    return out


def apply_global_flicker_gain(
    overlay_rgba: Image.Image,
    weight_0_1: float,
    strength: float,
) -> Image.Image:
    """
    Apply a frame-wide gain to non-magenta pixels.
    This guarantees visible frame-to-frame movement even after quantization.
    """
    w, h = overlay_rgba.size
    out = overlay_rgba.copy()
    px = out.load()

    # Map weight [0,1] to gain around 1.0 with asymmetric swing:
    # stronger dimming than brightening avoids clipping bright cores to 255
    # across all frames, which can flatten visible flicker after quantization.
    delta = (weight_0_1 * 2.0 - 1.0)
    s = max(0.0, strength)
    if delta >= 0.0:
        gain = 1.0 + (0.18 * s * delta)
    else:
        gain = 1.0 + (0.70 * s * delta)

    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if (r, g, b) == c3.MAGENTA:
                continue
            px[x, y] = (clamp8(r * gain), clamp8(g * gain), clamp8(b * gain), a)

    return out


def ensure_adjacent_frames_differ(frames_p: List[Image.Image], transparent_index: int) -> int:
    """
    Prevent accidental frame collapse after quantization by nudging several
    non-transparent pixels if two adjacent indexed frames are byte-identical.
    """
    if len(frames_p) < 2:
        return 0

    adjusted = 0

    for i in range(1, len(frames_p)):
        if frames_p[i].tobytes() != frames_p[i - 1].tobytes():
            continue

        fr = frames_p[i].copy()
        px = fr.load()
        w, h = fr.size
        seed = (i * 1103515245 + w * 12345 + h * 2654435761) & 0xFFFFFFFF

        changed = 0
        total = w * h
        target_changes = max(8, min(64, total // 512))
        for n in range(total):
            p = (seed + n * 2654435761) % total
            x = p % w
            y = p // w
            idx = int(px[x, y])
            if idx == transparent_index:
                continue

            # Keep index 255 reserved for transparency.
            px[x, y] = (idx + 1) % 255
            changed += 1
            if changed >= target_changes:
                break

        if changed > 0:
            frames_p[i] = fr
            adjusted += 1

    return adjusted

def render_overlay_frame_rgba(
    mask_source_rgb: Image.Image,
    keys: List[Tuple[int, int, int]],
    styles: Dict[Tuple[int, int, int], Dict[str, object]],
    intensity: float,
    # global defaults (same semantics as civ3_city_lights):
    core_radius: float,
    halo_radius: float,
    core_gain: float,
    halo_gain: float,
    core_color: Tuple[int, int, int],
    glow_color: Tuple[int, int, int],
    size_boost: float,
    size_radius: float,
    size_gamma: float,
    highlight_gain: float,
    blend_mode: str,
    halo_sep: float,
    halo_gamma: float,
) -> Image.Image:
    """
    Produce an RGBA overlay where background is MAGENTA (will become transparent).
    We reuse civ3_city_lights primitives to match your existing glow appearance.
    """
    w, h = mask_source_rgb.size

    # Start with black comp for accurate glow blending, plus an alpha union mask.
    comp = Image.new("RGB", (w, h), (0, 0, 0))
    union_alpha = Image.new("L", (w, h), 0)

    # Ensure styled keys included
    keys_set = set(keys)
    keys_set.update(styles.keys())
    keys_order = list(keys_set)

    for key_rgb in keys_order:
        mask_bin = c3.color_equal(mask_source_rgb, key_rgb)
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
        k_blend_mode = str(st.get("blend_mode", blend_mode)).lower()

        # For overlays, interior mask is full 255 (no clipping to city silhouette)
        interior = Image.new("L", (w, h), 255)

        core_alpha, halo_alpha = c3.build_glow_maps(
            mask_bin=mask_bin,
            interior_mask=interior,
            wtime=max(0.0, min(1.0, intensity)),  # intensity acts like time weight
            core_radius=k_core_rad,
            halo_radius=k_halo_rad,
            core_gain=k_core_gain,
            halo_gain=k_halo_gain,
            size_boost=k_size_boost,
            size_radius=k_size_radius,
            size_gamma=k_size_gamma,
            halo_sep=k_halo_sep,
            halo_gamma=k_halo_gamma,
        )

        core_layer = c3.layer_from_alpha(k_core_color, core_alpha)
        halo_layer = c3.layer_from_alpha(k_glow_color, halo_alpha)

        if k_blend_mode == "add":
            comp = ImageChops.add(comp, core_layer, scale=1.0)
            comp = ImageChops.add(comp, halo_layer, scale=1.0)
        else:
            comp = c3.screen_blend(comp, core_layer)
            comp = c3.screen_blend(comp, halo_layer)

        # Union alpha for transparency decision
        union_alpha = ImageChops.lighter(union_alpha, core_alpha)
        union_alpha = ImageChops.lighter(union_alpha, halo_alpha)

        if k_highlight > 0.0:
            hl = c3.scale_L(core_alpha, k_highlight)
            # add highlight to comp
            comp = ImageChops.add(
                comp,
                Image.composite(
                    Image.new("RGB", (w, h), (255, 255, 255)),
                    Image.new("RGB", (w, h), (0, 0, 0)),
                    hl
                ),
                scale=1.0
            )
            union_alpha = ImageChops.lighter(union_alpha, hl)

    # Convert black background to MAGENTA where union_alpha == 0
    mag = Image.new("RGB", (w, h), c3.MAGENTA)
    comp_rgb = comp
    # mask where alpha==0 => choose magenta else comp
    bgmask = union_alpha.point(lambda v: 255 if v == 0 else 0, mode="L")
    final_rgb = Image.composite(mag, comp_rgb, bgmask)  # bgmask selects magenta where 255

    # Make RGBA (opaque; transparency will be via palette index 255 later)
    return final_rgb.convert("RGBA")


def quantize_overlay_to_civ3_p(
    overlay_rgba: Image.Image,
    transparent_index: int = 255,
    fixed_palette: Optional[Image.Image] = None,
) -> Image.Image:
    """
    Quantize RGBA overlay to P/256 and enforce Civ3 palette rules:
    - MAGENTA at palette index 255
    - any magenta pixels -> index 255
    - remove GREEN if present in palette (like your compositor does)
    """
    # Use a single shared palette for all frames when provided.
    if fixed_palette is None:
        imP = c3.quantize_to_p_256(overlay_rgba)
    else:
        try:
            dnone = Image.Dither.NONE
        except Exception:
            dnone = getattr(Image, "NONE", 0)
        imP = overlay_rgba.convert("RGB").quantize(palette=fixed_palette, dither=dnone)

    # Ensure MAGENTA exists, then force it to 255
    pal = c3.ensure_palette_has_colors(imP, [c3.MAGENTA])
    c3.put_palette(imP, pal)
    replacement_idx = c3.force_magenta_at_255(imP)

    # Force magenta pixels to index 255, and avoid leaving replacement_idx as 255.
    px = imP.load()
    w, h = imP.size
    # Use source RGB to detect magenta precisely
    src_rgb = overlay_rgba.convert("RGB").load()
    for y in range(h):
        for x in range(w):
            r, g, b = src_rgb[x, y]
            cur = px[x, y]
            if (r, g, b) == c3.MAGENTA:
                px[x, y] = transparent_index
            elif cur == transparent_index and replacement_idx != transparent_index:
                # if quantizer mapped something to 255 that is not magenta, remap
                px[x, y] = replacement_idx

    # Remove GREEN from palette (optional; matches your compositor behavior)
    pal_after = c3.get_palette(imP)
    for i in range(256):
        if (pal_after[3*i], pal_after[3*i+1], pal_after[3*i+2]) == c3.GREEN:
            pal_after[3*i:3*i+3] = [0, 0, 0]
    c3.put_palette(imP, pal_after)

    return imP


def count_light_key_pixels(
    mask_source_rgb: Image.Image,
    keys: List[Tuple[int, int, int]],
    styles: Dict[Tuple[int, int, int], Dict[str, object]],
) -> Dict[Tuple[int, int, int], int]:
    counts: Dict[Tuple[int, int, int], int] = {}
    keys_set = set(keys)
    keys_set.update(styles.keys())
    for key_rgb in keys_set:
        m = c3.color_equal(mask_source_rgb, key_rgb)
        counts[key_rgb] = int(m.histogram()[255])
    return counts


def parse_light_styles_local(values: List[str]) -> Dict[Tuple[int, int, int], Dict[str, object]]:
    """
    Local style parser for flicker generation.
    Keeps compatibility with civ3_city_lights style syntax, and also accepts:
    - highlight_gain (alias for highlight)
    - blend_mode (per-key: add|screen)
    """
    styles: Dict[Tuple[int, int, int], Dict[str, object]] = {}
    for raw in values or []:
        parts = [p.strip() for p in raw.replace(",", ";").split(";") if p.strip()]
        kv: Dict[str, str] = {}
        for p in parts:
            if "=" in p:
                k, v = p.split("=", 1)
                kv[k.strip().lower()] = v.strip()
        if "key" not in kv:
            raise SystemExit("Each --light-style must include key=<color>")

        key_rgb = c3.parse_rgb_one(kv["key"])
        entry: Dict[str, object] = {}

        if "core" in kv:
            entry["core_color"] = c3.parse_rgb_one(kv["core"])
        if "glow" in kv:
            entry["glow_color"] = c3.parse_rgb_one(kv["glow"])

        for numk in [
            "core_gain", "halo_gain", "core_radius", "halo_radius",
            "halo_sep", "halo_gamma", "highlight", "size_boost",
            "size_radius", "size_gamma"
        ]:
            if numk in kv:
                entry[numk] = float(kv[numk])

        if "highlight_gain" in kv:
            entry["highlight"] = float(kv["highlight_gain"])

        if "blend_mode" in kv:
            bm = kv["blend_mode"].strip().lower()
            if bm in ("screen", "add"):
                entry["blend_mode"] = bm

        styles[key_rgb] = entry
    return styles


def build_shared_palette_source(overlays_rgba: List[Image.Image]) -> Image.Image:
    if not overlays_rgba:
        raise ValueError("No overlays to build shared palette")
    # Build palette from all frame pixels (not a max-light merge), so darker shades remain available.
    w, h = overlays_rgba[0].size
    atlas = Image.new("RGB", (w, h * len(overlays_rgba)))
    for i, ov in enumerate(overlays_rgba):
        atlas.paste(ov.convert("RGB"), (0, i * h))
    return c3.quantize_to_p_256(atlas)


# -----------------------------
# Sheet slicing
# -----------------------------
def get_cell_box(sheet_w: int, sheet_h: int, rows: int, cols: int, r: int, c: int,
                 cell_w: int, cell_h: int) -> Tuple[int, int, int, int]:
    left = c * cell_w
    top = r * cell_h
    return (left, top, left + cell_w, top + cell_h)


# -----------------------------
# Main
# -----------------------------
def main() -> None:
    ap = argparse.ArgumentParser(
        description="Generate Civ3 overlay FLC flicker animations from a Civ3-style *_lights.pcx annotation sheet."
    )
    ap.add_argument("--in", dest="inp", required=True, help="Input annotation PCX sheet (lights markers).")
    ap.add_argument("--out-dir", required=True, help="Output directory for per-cell .flc files.")

    ap.add_argument("--rows", type=int, required=True, help="Rows (eras).")
    ap.add_argument("--cols", type=int, required=True, help="Cols (variants).")

    ap.add_argument("--cell-w", type=int, default=128, help="Cell width (default 128).")
    ap.add_argument("--cell-h", type=int, default=64,  help="Cell height (default 64).")

    ap.add_argument("--frames", type=int, default=12, help="Animation frames (excluding keyframe/ring).")
    ap.add_argument("--fps", type=float, default=12.0, help="FPS used for Civ3 anim_time metadata.")

    ap.add_argument("--transparent-index", type=int, default=255, help="Transparent palette index (default 255).")

    # Flicker tuning
    ap.add_argument("--amp", type=float, default=0.12, help="Flicker amplitude (0.05..0.25 typical).")
    ap.add_argument("--hz1", type=float, default=2.2, help="Primary flicker frequency (Hz).")
    ap.add_argument("--hz2", type=float, default=5.1, help="Secondary flicker frequency (Hz).")
    ap.add_argument("--seed", type=int, default=1337, help="Deterministic seed.")
    ap.add_argument("--frame-change-rate", type=float, default=0.75,
                    help="0..1. Lower = more gradual frame-to-frame change, higher = snappier.")

    # Light keys/styles (same syntax as your compositor)
    ap.add_argument("--light-key", action="append", default=["#00feff"],
                    help="Marker color(s) in the annotation PCX, repeatable. '#rrggbb' or 'R,G,B'.")
    ap.add_argument("--light-style", action="append", default=[],
                    help="Per-key overrides. Example: \"key=#00feff; core=#fff87a; glow=#ff8a20; halo_gain=18; halo_radius=14\"")

    # Global glow defaults (matching civ3_city_lights)
    ap.add_argument("--core-radius", type=float, default=1.1)
    ap.add_argument("--halo-radius", type=float, default=13.0)
    ap.add_argument("--core-gain", type=float, default=2.1)
    ap.add_argument("--halo-gain", type=float, default=20.0)
    ap.add_argument("--core-color", type=str, default="#ff8a20")
    ap.add_argument("--glow-color", type=str, default="#dc6a00")
    ap.add_argument("--highlight-gain", type=float, default=0.6)

    ap.add_argument("--size-boost", type=float, default=1.1)
    ap.add_argument("--size-radius", type=float, default=3.5)
    ap.add_argument("--size-gamma", type=float, default=0.75)

    ap.add_argument("--halo-sep", type=float, default=0.75)
    ap.add_argument("--halo-gamma", type=float, default=1.4)
    ap.add_argument("--blend-mode", type=str, default="screen", choices=["screen", "add"])

    # Civ3 header knobs
    ap.add_argument("--civ-num-anims", type=int, default=1, help="Directions count in Civ3 header (usually 1).")
    ap.add_argument("--civ-directions-mask", type=lambda s: int(s, 0), default=0x0001,
                    help="Directions bitmask in Civ3 header (default 0x0001).")
    ap.add_argument("--civ-xs-orig", type=int, default=240, help="xs_orig in Civ3 FlicAnimHeader (default 240).")
    ap.add_argument("--civ-ys-orig", type=int, default=240, help="ys_orig in Civ3 FlicAnimHeader (default 240).")
    ap.add_argument("--with-ring-frame", action="store_true", help="Append explicit ring frame (off by default).")
    ap.add_argument("--flc-speed", type=int, default=170,
                    help="FLC header speed/delay in milliseconds for viewer playback (default 170).")

    ap.add_argument("--name-prefix", default="Lights", help="Output naming prefix.")

    args = ap.parse_args()
    args.frame_change_rate = max(0.0, min(1.0, args.frame_change_rate))

    os.makedirs(args.out_dir, exist_ok=True)

    sheet = Image.open(args.inp)
    # mask source must be RGB to compare marker colors exactly
    sheet_rgb = sheet.convert("RGB")

    sw, sh = sheet.size
    expected_w = args.cols * args.cell_w
    expected_h = args.rows * args.cell_h
    if sw < expected_w or sh < expected_h:
        raise SystemExit(f"Sheet is {sw}x{sh}, but rows*cell is {expected_w}x{expected_h}. Check --rows/--cols/--cell-w/--cell-h.")

    # Parse keys with compositor parser; parse styles locally to support
    # flicker-specific compatibility aliases like highlight_gain.
    light_keys = c3.parse_rgb_list(args.light_key)
    styles = parse_light_styles_local(args.light_style)
    core_color = c3.parse_rgb_one(args.core_color)
    glow_color = c3.parse_rgb_one(args.glow_color)

    for r in range(args.rows):
        for c in range(args.cols):
            box = get_cell_box(sw, sh, args.rows, args.cols, r, c, args.cell_w, args.cell_h)
            cell_mask_rgb = sheet_rgb.crop(box)

            key_counts = count_light_key_pixels(cell_mask_rgb, light_keys, styles)
            total_key_pixels = sum(key_counts.values())
            if total_key_pixels == 0:
                print(
                    f"WARNING: Cell r{r:02d}c{c:02d} has 0 marker pixels for keys "
                    f"{[f'#{k[0]:02x}{k[1]:02x}{k[2]:02x}' for k in key_counts.keys()]}; "
                    "output will be fully transparent (magenta in editor previews)."
                )

            # Build frame overlays first, then quantize all frames with a shared palette.
            cell_phase = _phase_for_cell(r, c, args.seed)
            overlays_rgba: List[Image.Image] = []
            prev_overlay: Optional[Image.Image] = None
            for i in range(args.frames):
                mult = _flicker_multiplier(i, args.frames, args.fps, cell_phase, args.amp, args.hz1, args.hz2)

                overlay_rgba = render_overlay_frame_rgba(
                    mask_source_rgb=cell_mask_rgb,
                    keys=light_keys,
                    styles=styles,
                    intensity=mult,
                    core_radius=args.core_radius,
                    halo_radius=args.halo_radius,
                    core_gain=args.core_gain,
                    halo_gain=args.halo_gain,
                    core_color=core_color,
                    glow_color=glow_color,
                    size_boost=args.size_boost,
                    size_radius=args.size_radius,
                    size_gamma=args.size_gamma,
                    highlight_gain=args.highlight_gain,
                    blend_mode=args.blend_mode,
                    halo_sep=args.halo_sep,
                    halo_gamma=args.halo_gamma,
                )

                # Add pixel-level shimmer variation.
                overlay_rgba = apply_per_pixel_flicker(
                    overlay_rgba=overlay_rgba,
                    frame_i=i,
                    fps=args.fps,
                    amp=args.amp * 1.25,
                    hz1=args.hz1,
                    hz2=args.hz2,
                    seed=args.seed ^ (r * 10007 + c * 10009),
                )
                overlay_rgba = apply_global_flicker_gain(
                    overlay_rgba=overlay_rgba,
                    weight_0_1=mult,
                    strength=1.0,
                )

                # Smooth temporal changes: lower rate => more gradual transitions.
                if prev_overlay is not None and args.frame_change_rate < 1.0:
                    overlay_rgba = Image.blend(prev_overlay, overlay_rgba, args.frame_change_rate)
                prev_overlay = overlay_rgba
                overlays_rgba.append(overlay_rgba)

            palette_source = build_shared_palette_source(overlays_rgba)
            frames_p = [
                quantize_overlay_to_civ3_p(
                    overlay_rgba,
                    transparent_index=args.transparent_index,
                    fixed_palette=palette_source,
                )
                for overlay_rgba in overlays_rgba
            ]
            adjusted = ensure_adjacent_frames_differ(frames_p, args.transparent_index)
            if adjusted > 0:
                print(f"NOTE: Cell r{r:02d}c{c:02d} had {adjusted} quantized frame collapse(s); applied anti-collapse nudges.")

            out_name = f"{args.name_prefix}_r{r:02d}c{c:02d}.flc"
            out_path = os.path.join(args.out_dir, out_name)

            write_flc(
                out_path=out_path,
                frames_p=frames_p,
                fps=args.fps,
                civ_num_anims=args.civ_num_anims,
                civ_directions_bitmask=args.civ_directions_mask,
                civ_x_offset=0,
                civ_y_offset=0,
                civ_xs_orig=args.civ_xs_orig,
                civ_ys_orig=args.civ_ys_orig,
                include_ring_frame=args.with_ring_frame,
                flc_speed_ms=args.flc_speed,
            )

            ring_note = "+ring" if args.with_ring_frame else ""
            print(f"Wrote {out_path} ({args.cell_w}x{args.cell_h}, frames={args.frames}{ring_note})")


if __name__ == "__main__":
    main()
