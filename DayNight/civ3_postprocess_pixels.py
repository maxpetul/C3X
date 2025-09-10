#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Civ3 post-process: remove *isolated* GREEN (#00ff00) pixels from PCX outputs.

Rules
-----
• Recursively scan under Data/*hour* folders (0100..2400 etc.), skipping the noon folder.
• Only touch files that have a corresponding "*_lights.pcx" in the same directory:
    - For "NAME.pcx" → process only if "NAME_lights.pcx" exists.
    - For "NAME_lights.pcx" → always process (it is the corresponding lights file).
• Only replace *isolated* green pixels: a green pixel whose neighbors within
  --isolation-radius (default 1, i.e. the 8-connected ring) contain no other green pixels.
• Replacement uses *neighbor palette indices* so the palette stays unchanged.
  If no non-green neighbors exist, fall back to nearest palette color to local RGB average.

Usage
-----
  python civ3_fix_green.py --data ./Data --noon 1200 --verbose
  # knobs:
  #   --isolation-radius 1   (1 = 8-neighborhood)
  #   --search-radius 3
  #   --max-radius 5
"""

import argparse, os, collections
from typing import Tuple, List, Set
from PIL import Image

GREEN   = (0, 255, 0)
MAGENTA = (255, 0, 255)

# ---------- palette helpers ----------

def get_palette(im: Image.Image) -> List[int]:
    pal = im.getpalette() or []
    if len(pal) < 256*3:
        pal += [0]*(256*3 - len(pal))
    return pal[:256*3]

def find_color_index(pal: List[int], rgb: Tuple[int,int,int]) -> int:
    r,g,b = rgb
    for i in range(256):
        if pal[3*i] == r and pal[3*i+1] == g and pal[3*i+2] == b:
            return i
    return -1

def nearest_palette_index(target_rgb: Tuple[int,int,int], pal: List[int], banned: Set[int]) -> int:
    """Choose the palette index (not in banned) whose RGB is nearest (Euclidean)."""
    tr,tg,tb = target_rgb
    best_i, best_d = None, 10**9
    for i in range(256):
        if i in banned: continue
        r,g,b = pal[3*i], pal[3*i+1], pal[3*i+2]
        dr, dg, db = tr-r, tg-g, tb-b
        d = dr*dr + dg*dg + db*db
        if d < best_d:
            best_d, best_i = d, i
    return best_i if best_i is not None else 0

# ---------- neighborhood ops ----------

def has_green_neighbor(rgb_img: Image.Image, x: int, y: int, radius: int) -> bool:
    """Return True if any neighbor within Chebyshev radius is GREEN (#00ff00)."""
    w, h = rgb_img.size
    px = rgb_img.load()
    r = max(1, int(radius))
    for yy in range(max(0, y-r), min(h, y+r+1)):
        for xx in range(max(0, x-r), min(w, x+r+1)):
            if xx == x and yy == y:
                continue
            if px[xx,yy] == GREEN:
                return True
    return False

def gather_neighbor_indices(idx_img: Image.Image, rgb_img: Image.Image,
                            x: int, y: int, radius: int,
                            green_idx: int, mag_idx: int) -> List[int]:
    """Collect neighbor indices around (x,y) within Chebyshev radius; exclude green/magenta."""
    w, h = idx_img.size
    px_idx = idx_img.load()
    px_rgb = rgb_img.load()
    out = []
    r = max(1, int(radius))
    for yy in range(max(0, y-r), min(h, y+r+1)):
        for xx in range(max(0, x-r), min(w, x+r+1)):
            if xx == x and yy == y: continue
            i = px_idx[xx, yy]
            r0,g0,b0 = px_rgb[xx, yy]
            if (r0,g0,b0) == GREEN or (r0,g0,b0) == MAGENTA:
                continue
            if i == green_idx or i == mag_idx:
                continue
            out.append(i)
    return out

def local_average_rgb(rgb_img: Image.Image, x: int, y: int, radius: int,
                      skip_colors: Set[Tuple[int,int,int]]) -> Tuple[int,int,int]:
    """Average RGB in a square window, skipping listed colors; falls back to (0,0,0)."""
    w, h = rgb_img.size
    px = rgb_img.load()
    total = [0,0,0]
    n = 0
    r = max(1, int(radius))
    for yy in range(max(0, y-r), min(h, y+r+1)):
        for xx in range(max(0, x-r), min(w, x+r+1)):
            c = px[xx, yy]
            if c in skip_colors:
                continue
            total[0] += c[0]; total[1] += c[1]; total[2] += c[2]
            n += 1
    if n == 0:
        return (0,0,0)
    return (total[0]//n, total[1]//n, total[2]//n)

# ---------- core fixer ----------

def fix_green_in_image(path: str, isolation_radius: int, search_radius: int, max_radius: int, verbose: bool=False) -> int:
    """
    Returns number of pixels changed (only isolated greens are touched).
    """
    try:
        imP = Image.open(path)
    except Exception:
        return 0
    if imP.mode != 'P':
        return 0

    pal = get_palette(imP)
    green_idx = find_color_index(pal, GREEN)
    mag_idx   = find_color_index(pal, MAGENTA)

    imRGB = imP.convert('RGB')
    px = imRGB.load()
    w, h = imRGB.size

    # Collect isolated green pixels
    isolated_coords = []
    for y in range(h):
        for x in range(w):
            if px[x,y] == GREEN and not has_green_neighbor(imRGB, x, y, isolation_radius):
                isolated_coords.append((x,y))

    if not isolated_coords:
        return 0

    outP = imP.copy()
    out_px = outP.load()
    banned_indices = set()
    if green_idx != -1: banned_indices.add(green_idx)
    if mag_idx   != -1: banned_indices.add(mag_idx)

    changed = 0
    for (x,y) in isolated_coords:
        picked_index = None

        # 1) Most frequent neighbor index (expand search out to max_radius if needed)
        r = max(1, int(search_radius))
        m = max(r, int(max_radius))
        while r <= m and picked_index is None:
            neigh = gather_neighbor_indices(imP, imRGB, x, y, r, green_idx, mag_idx)
            if neigh:
                cnt = collections.Counter(neigh)
                picked_index = cnt.most_common(1)[0][0]
                break
            r += 1

        # 2) Fallback: nearest palette color to local average (excluding banned)
        if picked_index is None:
            avg_rgb = local_average_rgb(imRGB, x, y, radius=m, skip_colors={GREEN, MAGENTA})
            picked_index = nearest_palette_index(avg_rgb, pal, banned_indices)

        if picked_index is not None:
            out_px[x,y] = picked_index
            changed += 1

    if changed > 0:
        outP.putpalette(pal)   # keep palette exactly the same
        outP.save(path, format='PCX')
        if verbose:
            print(f"[fixed] {path}: {changed} isolated green px")
    else:
        if verbose:
            print(f"[ok]    {path}: no isolated green px")

    return changed

# ---------- file selection logic ----------

def has_corresponding_lights(dirpath: str, fname: str) -> bool:
    """
    Return True iff this file should be processed according to the rule:
      • If fname ends with '_lights.pcx' → True.
      • Else if a sibling '<stem>_lights.pcx' exists → True.
      • Otherwise False.
    """
    name_lower = fname.lower()
    if name_lower.endswith("_lights.pcx"):
        return True
    if not name_lower.endswith(".pcx"):
        return False
    stem = fname[:-4]  # remove '.pcx'
    lights_name = stem + "_lights.pcx"
    return os.path.exists(os.path.join(dirpath, lights_name))

def walk_and_fix(data_dir: str, noon_folder: str,
                 isolation_radius: int, search_radius: int, max_radius: int,
                 only_hour: int=None, verbose: bool=False) -> None:
    noon_abs = os.path.normpath(os.path.join(data_dir, noon_folder))
    targets = []

    if only_hour is not None:
        hour_name = f"{only_hour:04d}"
        hour_abs = os.path.normpath(os.path.join(data_dir, hour_name))
        if os.path.isdir(hour_abs):
            targets.append(hour_abs)
        else:
            print(f"[warn] Hour folder not found: {hour_abs}")
    else:
        for name in os.listdir(data_dir):
            sub = os.path.normpath(os.path.join(data_dir, name))
            if not os.path.isdir(sub): continue
            if os.path.normcase(sub) == os.path.normcase(noon_abs): continue
            targets.append(sub)

    total_changed = 0
    for root in targets:
        for dirpath, _, filenames in os.walk(root):
            for fname in filenames:
                if not fname.lower().endswith(".pcx"):
                    continue
                if not has_corresponding_lights(dirpath, fname):
                    continue
                path = os.path.join(dirpath, fname)
                total_changed += fix_green_in_image(path, isolation_radius, search_radius, max_radius, verbose=verbose)

    print(f"Done. Total isolated green pixels fixed: {total_changed}")

def main():
    ap = argparse.ArgumentParser(description="Remove *isolated* #00ff00 pixels from PCX outputs (only files with a matching _lights.pcx).")
    ap.add_argument("--data", required=True, help="Path to Data root (contains noon + hour folders).")
    ap.add_argument("--noon", default="1200", help="Name of noon folder to SKIP. Default: 1200")
    ap.add_argument("--only-hour", type=int, help="Restrict to a single hour folder (e.g., 2400)")
    ap.add_argument("--isolation-radius", type=int, default=1, help="Green is 'isolated' if no other green is found within this Chebyshev radius. Default: 1 (8-neighborhood)")
    ap.add_argument("--search-radius", type=int, default=3, help="Neighbor radius (pixels) for index voting. Default: 3")
    ap.add_argument("--max-radius", type=int, default=5, help="Max expansion radius if no neighbor found. Default: 5")
    ap.add_argument("--verbose", action="store_true", help="Print per-file changes.")
    args = ap.parse_args()

    walk_and_fix(args.data, args.noon, args.isolation_radius, args.search_radius, args.max_radius, args.only_hour, verbose=args.verbose)

if __name__ == "__main__":
    main()
