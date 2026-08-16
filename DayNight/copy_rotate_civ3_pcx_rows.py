#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
copy_rotate_civ3_pcx_rows.py

Copies the first animation row (row 0) of a Civ3-style indexed PCX sheet into
rows 1..3, transforming each frame so the motion direction changes:

- Row 1: NW -> SW  (vertical flip)
- Row 2: NW -> NE  (horizontal flip)
- Row 3: NW -> SE  (180° rotate)

Assumes:
- Indexed PCX (mode 'P')
- 64 columns, 8 rows by default
- 1-pixel border grid between slots and around the outer edge
- Background is magenta (#ff00ff), border is (#c000ff) -- used only for sanity checks

This script preserves the palette and keeps everything indexed.

Example:
  python copy_rotate_civ3_pcx_rows.py --in Waves.pcx --out Waves_dirs.pcx
"""

import argparse
import sys
from PIL import Image

MAGENTA = (255, 0, 255)   # background
BORDER  = (192, 0, 255)   # 1px grid/border


def find_palette_index(pal_bytes, rgb):
    """Return palette index (0..255) matching rgb, or None."""
    r_t, g_t, b_t = rgb
    if pal_bytes is None or len(pal_bytes) < 768:
        return None
    for i in range(256):
        r, g, b = pal_bytes[i*3:i*3+3]
        if (r, g, b) == (r_t, g_t, b_t):
            return i
    return None


def compute_grid(img_w, img_h, cols, rows, border):
    """
    Compute slot rectangles assuming:
      total_w = cols*slot_w + (cols+1)*border
      total_h = rows*slot_h + (rows+1)*border
    Returns: (slot_w, slot_h, x0_list, y0_list)
    where x0_list[c] is left edge of slot interior (excluding border),
    and y0_list[r] is top edge of slot interior.
    """
    slot_w = (img_w - (cols + 1) * border) // cols
    slot_h = (img_h - (rows + 1) * border) // rows

    expected_w = cols * slot_w + (cols + 1) * border
    expected_h = rows * slot_h + (rows + 1) * border
    if expected_w != img_w or expected_h != img_h:
        raise ValueError(
            f"Image size {img_w}x{img_h} is not consistent with cols={cols}, rows={rows}, border={border}.\n"
            f"Computed slot {slot_w}x{slot_h} gives expected {expected_w}x{expected_h}."
        )

    x0_list = [border + c * (slot_w + border) for c in range(cols)]
    y0_list = [border + r * (slot_h + border) for r in range(rows)]
    return slot_w, slot_h, x0_list, y0_list


def crop_slot(img, x0, y0, slot_w, slot_h):
    return img.crop((x0, y0, x0 + slot_w, y0 + slot_h))


def paste_slot(dst, src_slot, x0, y0):
    # Direct paste keeps indices unchanged (mode 'P')
    dst.paste(src_slot, (x0, y0))


def fill_slot(dst, x0, y0, slot_w, slot_h, fill_index):
    patch = Image.new("P", (slot_w, slot_h), color=fill_index)
    patch.putpalette(dst.getpalette())
    dst.paste(patch, (x0, y0))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="in_path", required=True, help="Input indexed PCX sheet")
    ap.add_argument("--out", dest="out_path", required=True, help="Output indexed PCX sheet")
    ap.add_argument("--cols", type=int, default=64, help="Number of columns (default 64)")
    ap.add_argument("--rows", type=int, default=8, help="Number of rows (default 8)")
    ap.add_argument("--border", type=int, default=1, help="Border thickness in pixels (default 1)")
    ap.add_argument("--src_row", type=int, default=0, help="Source row index to copy from (default 0)")
    ap.add_argument("--dst_rows", default="1,2,3",
                    help="Comma-separated destination row indices (default '1,2,3')")
    ap.add_argument("--no_clear", action="store_true",
                    help="Do not clear destination slots to magenta before pasting")
    args = ap.parse_args()

    # Load
    im = Image.open(args.in_path)
    if im.mode != "P":
        raise ValueError(f"Input must be indexed (mode 'P'). Got mode={im.mode}")

    pal = im.getpalette()
    pal_bytes = bytes(pal) if pal is not None else None
    bg_idx = find_palette_index(pal_bytes, MAGENTA)
    border_idx = find_palette_index(pal_bytes, BORDER)

    if bg_idx is None:
        print("WARNING: Could not find magenta (#ff00ff) in palette. Clearing (if enabled) may be wrong.", file=sys.stderr)
    if border_idx is None:
        print("WARNING: Could not find border color (#c000ff) in palette. That's OK; it's only used for sanity.", file=sys.stderr)

    cols = args.cols
    rows = args.rows
    border = args.border

    slot_w, slot_h, x0_list, y0_list = compute_grid(im.width, im.height, cols, rows, border)

    # Parse destination rows
    dst_rows = []
    for part in args.dst_rows.split(","):
        part = part.strip()
        if not part:
            continue
        dst_rows.append(int(part))

    if args.src_row < 0 or args.src_row >= rows:
        raise ValueError(f"src_row {args.src_row} out of range 0..{rows-1}")
    for r in dst_rows:
        if r < 0 or r >= rows:
            raise ValueError(f"dst_row {r} out of range 0..{rows-1}")

    out = im.copy()

    # Define transforms relative to NW source
    # Row 1: SW = vertical flip
    # Row 2: NE = horizontal flip
    # Row 3: SE = 180 rotate
    # If user specifies different dst_rows ordering, we map by row number.
    transform_by_row = {
        1: ("SW", lambda s: s.transpose(Image.FLIP_TOP_BOTTOM)),
        2: ("NE", lambda s: s.transpose(Image.FLIP_LEFT_RIGHT)),
        3: ("SE", lambda s: s.transpose(Image.ROTATE_180)),
    }

    # Copy each column slot from src_row to each dst_row with transform
    for c in range(cols):
        sx0 = x0_list[c]
        sy0 = y0_list[args.src_row]
        src_slot = crop_slot(im, sx0, sy0, slot_w, slot_h)

        for r in dst_rows:
            if r == args.src_row:
                continue

            label, xf = transform_by_row.get(r, (f"row{r}", lambda s: s))
            dx0 = x0_list[c]
            dy0 = y0_list[r]

            if (not args.no_clear) and (bg_idx is not None):
                fill_slot(out, dx0, dy0, slot_w, slot_h, bg_idx)

            dst_slot = xf(src_slot)

            # Safety: ensure size matches (it should with flips/180)
            if dst_slot.size != (slot_w, slot_h):
                raise ValueError(
                    f"Transformed slot size {dst_slot.size} does not match slot {slot_w}x{slot_h} "
                    f"(dst row {r} {label}). If you tried to use 90° rotation, it won't fit unless slots are square."
                )

            paste_slot(out, dst_slot, dx0, dy0)

    # Ensure palette preserved
    out.putpalette(im.getpalette())

    # Save as PCX
    out.save(args.out_path, format="PCX")
    print(f"Done. Wrote: {args.out_path}")
    print(f"Detected slot size: {slot_w}x{slot_h}, border={border}, cols={cols}, rows={rows}")


if __name__ == "__main__":
    main()