#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
pcx_palette_unused.py — find unused palette slots in PCX files

Usage:
  python3 pcx_palette_unused.py --folder ./Data/1200 --exclude-255 yes

What it prints:
  • Per-file: which palette indexes are unused (and their RGB in that file)
  • Intersection across all files (indexes unused everywhere)
  • Top candidate RGBs that were unused most often across files
"""

import argparse, os, collections
from PIL import Image

def get_palette(im):
    pal = im.getpalette() or []
    if len(pal) < 256*3: pal += [0]*(256*3 - len(pal))
    return pal[:256*3]

def main():
    ap = argparse.ArgumentParser(description="List unused palette indexes/colors in PCX files")
    ap.add_argument("--folder", required=True, help="Folder to scan (recursively)")
    ap.add_argument("--exclude-255", default="yes", choices=["yes","no"],
                    help="Exclude index 255 (magenta) from 'unused' lists (default: yes)")
    args = ap.parse_args()
    exclude_255 = (args.exclude_255 == "yes")

    per_file_unused = []
    all_files = []

    # Count how often a specific RGB appears in an unused slot across files
    rgb_unused_counter = collections.Counter()

    # Intersection accumulator for indexes (0..254 if exclude_255 else 0..255)
    index_domain = list(range(0, 255 if exclude_255 else 256))
    global_intersection = set(index_domain)

    for root, _, files in os.walk(args.folder):
        for name in files:
            if not name.lower().endswith(".pcx"):
                continue
            path = os.path.join(root, name)
            try:
                im = Image.open(path)
                if im.mode != "P":
                    # Most Civ3 PCX files are 'P'; warn if not.
                    print(f"[warn] {path}: not paletted ('{im.mode}'); attempting to interpret anyway.")
                    im = im.convert("P")
                pal = get_palette(im)

                # Which indexes are used in pixels?
                used = set(im.getdata())
                domain = set(index_domain)
                unused = sorted(domain - used)

                # Aggregate RGBs for unused slots (per-file)
                for idx in unused:
                    r,g,b = pal[3*idx], pal[3*idx+1], pal[3*idx+2]
                    rgb_unused_counter[(r,g,b)] += 1

                per_file_unused.append((path, unused, pal))
                all_files.append(path)
                global_intersection &= set(unused)

            except Exception as e:
                print(f"[error] {path}: {e}")

    # Output
    print("\n=== Per-file unused palette indexes ===")
    for path, unused, pal in per_file_unused:
        print(f"\n{path}")
        if not unused:
            print("  (none)")
        else:
            print(f"  {len(unused)} unused indexes:")
            # show up to 32 entries to keep readable
            preview = unused if len(unused) <= 32 else (unused[:16] + ["..."] + unused[-15:])
            print("   ", preview)
            # show a few RGBs
            rgb_preview = []
            for idx in unused[:12]:
                r,g,b = pal[3*idx], pal[3*idx+1], pal[3*idx+2]
                rgb_preview.append(f"{idx:3d}:#{r:02x}{g:02x}{b:02x}")
            if rgb_preview:
                print("   sample RGBs:", ", ".join(rgb_preview))

    print("\n=== Indexes unused in EVERY scanned file ===")
    if not all_files:
        print("  (no files)")
    else:
        inter_sorted = sorted(global_intersection)
        print(f"  count={len(inter_sorted)}")
        print("  ", inter_sorted[:64] if len(inter_sorted)<=64 else (inter_sorted[:32]+["..."]+inter_sorted[-31:]))

    print("\n=== Top candidate RGBs (unused across many files) ===")
    if not rgb_unused_counter:
        print("  (none)")
    else:
        for (r,g,b), cnt in rgb_unused_counter.most_common(20):
            print(f"  #{r:02x}{g:02x}{b:02x}  unused-in-files={cnt}")

if __name__ == "__main__":
    main()
