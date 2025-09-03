#!/usr/bin/env python3
"""
least_used_pcx_colors.py

Reads a paletted (8-bit, mode 'P') PCX file and prints the top 5 least used
colors among indices [0..254] by default.

Output columns: index, HTML color (#RRGGBB), count

Usage:
  python least_used_pcx_colors.py path/to/image.pcx
  # include colors that never appear:
  python least_used_pcx_colors.py image.pcx --include-zeros
  # change the highest considered index:
  python least_used_pcx_colors.py image.pcx --max-index 255
"""

import argparse
import sys
from PIL import Image

def idx_to_hex(rgb_tuple):
    r, g, b = rgb_tuple
    return f"#{r:02X}{g:02X}{b:02X}"

def main():
    parser = argparse.ArgumentParser(description="Find least-used colors in a paletted PCX.")
    parser.add_argument("pcx_path", help="Path to the PCX file (8-bit paletted).")
    parser.add_argument("--include-zeros", action="store_true", default=True,
                        help="Include palette indices that occur 0 times.")
    parser.add_argument("--max-index", type=int, default=254,
                        help="Highest palette index to consider (default: 254).")
    parser.add_argument("--top", type=int, default=5,
                        help="How many least-used colors to return (default: 5).")
    args = parser.parse_args()

    # Open image
    try:
        im = Image.open(args.pcx_path)
    except Exception as e:
        print(f"Error: failed to open '{args.pcx_path}': {e}", file=sys.stderr)
        sys.exit(1)

    # Require paletted image to avoid quantization changing counts
    if im.mode != "P":
        print(f"Error: '{args.pcx_path}' is mode '{im.mode}', not an 8-bit paletted image ('P').", file=sys.stderr)
        sys.exit(2)

    # Get palette (list of [R, G, B, R, G, B, ...], up to 256*3 entries)
    pal = im.getpalette()
    if not pal:
        print("Error: No palette data found.", file=sys.stderr)
        sys.exit(3)

    # Build palette as list of (R,G,B)
    palette_colors = [(pal[i], pal[i+1], pal[i+2]) for i in range(0, len(pal), 3)]
    max_considered = min(args.max_index, len(palette_colors) - 1)

    # Histogram: for 'P' images, 256 bins (0..255)
    hist = im.histogram()
    if len(hist) < 256:
        # Shouldn't happen for 'P', but guard anyway
        hist += [0] * (256 - len(hist))

    # Build (count, index, html) entries for chosen range
    entries = []
    for idx in range(0, max_considered + 1):
        count = hist[idx] if idx < len(hist) else 0
        # Some palettes might be shorter than 256 entries; guard for safety
        if idx < len(palette_colors):
            html = idx_to_hex(palette_colors[idx])
        else:
            # If palette entry missing, treat as black (or skip); choose to skip
            html = None

        if html is None:
            continue  # skip indices without a palette color

        if args.include_zeros or count > 0:
            entries.append((count, idx, html))

    if not entries:
        print("No colors to report in the specified index range.")
        sys.exit(0)

    # Sort by count ascending, then by index to stabilize ties
    entries.sort(key=lambda t: (t[0], t[1]))

    # Take top N least-used
    top_n = entries[:args.top]

    # Print header and results
    print("index\thtml\tcount")
    for count, idx, html in top_n:
        print(f"{idx}\t{html}\t{count}")

if __name__ == "__main__":
    main()
