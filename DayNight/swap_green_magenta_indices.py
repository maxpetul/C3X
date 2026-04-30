#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
from pathlib import Path
from typing import List, Tuple, Optional

from PIL import Image

GREEN = (0, 255, 0)       # #00ff00
MAGENTA = (255, 0, 255)   # #ff00ff
GREEN_TARGET_IDX = 254
MAGENTA_TARGET_IDX = 255

def chunk_palette(pal: List[int]) -> List[Tuple[int, int, int]]:
    """Convert flat [r,g,b,...] list to list of 256 (r,g,b) tuples."""
    return [(pal[i], pal[i+1], pal[i+2]) for i in range(0, 256*3, 3)]

def flatten_palette(pal_tuples: List[Tuple[int, int, int]]) -> List[int]:
    """Convert list of (r,g,b) tuples back to flat list."""
    flat: List[int] = []
    for r, g, b in pal_tuples:
        flat.extend([r, g, b])
    return flat

def find_color_index(pal_tuples: List[Tuple[int, int, int]], color: Tuple[int, int, int]) -> Optional[int]:
    """Return first index of exact color match, else None."""
    try:
        return pal_tuples.index(color)
    except ValueError:
        return None

def compute_mapping(idx_green: int, idx_magenta: int) -> List[int]:
    """
    Build a 256-length LUT mapping old index -> new index so that:
      - old idx_green goes to GREEN_TARGET_IDX
      - old GREEN_TARGET_IDX goes to idx_green
      - old idx_magenta goes to MAGENTA_TARGET_IDX
      - old MAGENTA_TARGET_IDX goes to idx_magenta
    Other indices map to themselves.
    Handles overlap cases gracefully (e.g., green already at 254, magenta already at 255,
    or the two are swapped).
    """
    lut = list(range(256))
    # Swap pair for green <-> 254
    lut[idx_green] = GREEN_TARGET_IDX
    lut[GREEN_TARGET_IDX] = idx_green

    # Swap pair for magenta <-> 255
    lut[idx_magenta] = MAGENTA_TARGET_IDX
    lut[MAGENTA_TARGET_IDX] = idx_magenta

    return lut

def remap_palette(old_pal: List[Tuple[int, int, int]], lut: List[int]) -> List[Tuple[int, int, int]]:
    """
    Produce a new palette consistent with the index remap.
    If pixels with old index i become new index lut[i],
    then new_palette[lut[i]] should equal old_palette[i].
    """
    new_pal = [(0, 0, 0)] * 256
    for old_idx, rgb in enumerate(old_pal):
        new_idx = lut[old_idx]
        new_pal[new_idx] = rgb
    return new_pal

def process(path: Path) -> None:
    with Image.open(path) as im:
        if im.mode != "P":
            raise ValueError(f"{path} is mode '{im.mode}', not paletted 'P'. Aborting to avoid altering colors.")
        pal = im.getpalette()
        if pal is None:
            raise ValueError("Image has no palette.")
        if len(pal) < 256 * 3:
            raise ValueError(f"Palette has {len(pal)//3} colors, but 256 are required.")
        pal_tuples = chunk_palette(pal)

        idx_green = find_color_index(pal_tuples, GREEN)
        idx_magenta = find_color_index(pal_tuples, MAGENTA)
        if idx_green is None:
            raise ValueError("Exact green #00ff00 not found in palette.")
        if idx_magenta is None:
            raise ValueError("Exact magenta #ff00ff not found in palette.")

        # Build pixel index remap LUT (length 256)
        lut = compute_mapping(idx_green, idx_magenta)

        # Remap pixel indices using Image.point(LUT)
        # point() expects a 256-length sequence for 'P' images.
        im_remapped = im.point(lut)

        # Reorder the palette to match the remapped indices
        new_pal_tuples = remap_palette(pal_tuples, lut)
        new_pal_flat = flatten_palette(new_pal_tuples)
        im_remapped.putpalette(new_pal_flat)

        # Sanity check: confirm targets are now correct
        final_pal = chunk_palette(im_remapped.getpalette())
        assert final_pal[GREEN_TARGET_IDX] == GREEN, "Post-remap sanity check failed for green at index 254."
        assert final_pal[MAGENTA_TARGET_IDX] == MAGENTA, "Post-remap sanity check failed for magenta at index 255."

        # Overwrite the original file
        im_remapped.save(path, format="PCX")

def main():
    parser = argparse.ArgumentParser(description="Swap #00ff00 and #ff00ff into palette indices 254 and 255 in a 256-color PCX, remapping pixels and palette accordingly.")
    parser.add_argument("pcx_path", type=Path, help="Path to the .pcx file")
    args = parser.parse_args()

    process(args.pcx_path)

if __name__ == "__main__":
    main()
