#!/usr/bin/env bash

set -euo pipefail

### === CONFIG ===
# Assumed to be run from DayNight/ directory

DAYNIGHT_ANNOTATION_DIR="../Art/DayNight/Annotations"
DAYNIGHT_DATA_DIR="../Art/DayNight"
DISTRICT_ANNOTATION_DIR="../Art/Districts/Annotations"
DISTRICT_DATA_DIR="../Art/Districts"

ANNOTATION_DIR="../Art/DayNight/Annotations"
DATA_DIR="../Art/DayNight"

NOON_SUBFOLDER="1200"
ONLY_HOUR=""       # set empty "" to process all hours

# ---- Day/Night settings ----
WARMTH=1.7            # Scale for sunrise/sunset warmth (1.0 = base)
BLUE=1.6              # Scale for night-time blue emphasis (1.0 = base)
DARKNESS=3.0          # Scale for extra night darkening (1.0 = base)
DESAT=0.8             # Scale for dusk/night desaturation toward gray (lower = richer)
SAT=1.1               # Global saturation multiplier after tint (1.0 = none)
CONTRAST=1.08         # Global contrast multiplier around mid 128 (1.0 = none)
SUNRISE_CENTER=6.0    # Hour center for sunrise warmth bump (0-23)
SUNSET_CENTER=18.0    # Hour center for sunset warmth bump (0-23)
TWILIGHT_WIDTH=2.6    # Sigma for sunrise/sunset warmth spread (higher = broader)
NOON_BLEND=0.5        # 0..1 strength to blend toward base palette near 12:00 (0=off)
NOON_SIGMA=1.0        # Gaussian width (hours) around 12:00 (larger = broader)

# Examples
#   --warmth 1.05 --blue 1.25 --darkness 1.15 --desat 0.9  --sat 1.02 --contrast 1.04   # Moody, cooler nights
#   --warmth 1.3  --blue 1.05 --darkness 1.05 --desat 0.8  --sat 1.08 --contrast 1.04   # Warm, cinematic evenings
#   --warmth 1.1  --blue 1.12 --darkness 1.08 --desat 0.85 --sat 1.05 --contrast 1.03   # Balanced
#   --warmth 1.2  --blue 1.1  --darkness 1.05 --desat 0.7  --sat 1.1  --contrast 1.0    # Soft, painted look
#   --warmth 1.15 --blue 1.2  --darkness 1.12 --desat 0.85 --sat 1.0  --contrast 1.08   # Crisp, high contrast nights

# Light keys are the annotation colors used to indicate where lights should be at night.
# Each has a different configuration (LIGHT_STYLES, below). The light annotations are in
# /1200/*_light.pcx files.
LIGHT_KEYS=(
  # Main colors
  "#F6915E" # Orange
  "#FEF500" # Yellow

  # Supporting colors, mostly on modern buildings
  "#00feff" # Teal
  "#E4080A" # Red
  "#BD15D0" # Purple
  "#2D9C01" # Green
  "#FF25C8" # Pink
  "#0A02EB" # Blue
  "#8262ED" # Indigo
)

LIGHT_STYLES=(
  # Orange - softer, ambient lighting
  "key=#F6915E; core=#ff8a20; glow=#dc6a00; core_gain=1.0; highlight_gain=0.0; size_radius=1.5; size_boost=0.05; halo_gain=6.0; halo_radius=1.0; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"

  # Yellow - intense, bright, focused lighting
  "key=#FEF500; core=#ff8a20; glow=#dc6a00; core_gain=2.5; highlight_gain=1.0; size_radius=6.5; size_boost=1.5; halo_gain=20.0; halo_radius=0.1; core_radius=1.1; halo_gamma=1.3; size_gamma=0.75;"

  # Red - mostly modern building tops, individual pixels
  "key=#E4080A; core=#E4080A; glow=#E4080A; core_gain=1.0; highlight_gain=0.0; size_radius=0.5; size_boost=0.0; halo_gain=6.0; halo_radius=1.0; core_radius=0.5; halo_gamma=0.9; size_gamma=0.0; blend_mode=add;"
  
  # Teal
  "key=#00feff; core=#00feff; glow=#00feff; core_gain=0.6; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"

  # Purple
  "key=#BD15D0; core=#BD15D0; glow=#BD15D0; core_gain=0.6; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"

  # Green
  "key=#2D9C01; core=#2D9C01; glow=#2D9C01; core_gain=0.6; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"

  # Pink
  "key=#FF25C8; core=#FF25C8; glow=#FF25C8; core_gain=0.6; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"

  # Blue
  "key=#0A02EB; core=#0A02EB; glow=#0A02EB; core_gain=0.2; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"

  # Indigo
  "key=#8262ED; core=#7521DC; glow=#7521DC; core_gain=0.2; highlight_gain=0.0; size_radius=0.9; size_boost=0.3; halo_gain=8.0; halo_radius=0.1; core_radius=0.5; halo_gamma=1.5; size_gamma=0.1;"
)

# ---- City Lights settings ----
CORE_COLOR="#ff8a20"  # Tint color for the bright core (e.g., '#fff87a' for warm yellow)
GLOW_COLOR="#dc6a00"  # Tint color for the outer halo (e.g., '#ff8a20' for orange)
CORE_RADIUS=1.1       # Gaussian blur radius (pixels) of the bright core. Higher = wider/softer core
CORE_GAIN=2.5         # Intensity multiplier for the core alpha (brightness/opacity of the center)
HALO_RADIUS=13        # Gaussian blur radius (pixels) of the outer halo. Higher = glow spreads further outward
HALO_GAIN=20.0        # Intensity multiplier for the halo alpha (strength of the outer glow)
HALO_SEP=0.75         # 0..1: subtracts a fraction of the core from the halo. 0 = seamless merge; 1 = distinct outer ring
HALO_GAMMA=1.3        # Gamma on the halo-only mask. >1 pushes energy outward (softer tail); <1 concentrates near source
HIGHLIGHT_GAIN=0.5    # Extra white highlight added from the core mask (0..~1 typical). Higher = hotter specular highlight
SIZE_BOOST=1.7        # How much cluster size increases intensity (0 disables). Higher = large groups pop more
SIZE_RADIUS=6.5       # Neighborhood radius (pixels) when measuring cluster size. Higher = smoother/broader size effect
SIZE_GAMMA=0.75       # Gamma on the size map. <1 emphasizes medium clusters; >1 favors only the largest clusters
CLIP_INTERIOR="yes"   # If 'yes', lights only affect non-transparent interior (avoids glow on MAGENTA/GREEN). Recommended: yes.
CLIP_ERODE=0          # Erode the interior mask by N pixels. Higher = more conservative (keeps glow off edges)
BLEND_MODE="screen"   # How layers combine. 'screen' (default) is safer; 'add' is punchier but can clip highlights

# Notes:
#   • Increase --halo-radius and --halo-gain for a bigger, softer glow.
#   • If you see a dark ring between core/halo, reduce --halo-sep or increase --halo-gamma.
#   • 'screen' blend is safer for colors; 'add' is punchier but can clip highlights."

### === BUILD ARGS ===
LK_ARGS=()
for c in "${LIGHT_KEYS[@]}"; do LK_ARGS+=( --light-key "$c" ); done

STYLE_ARGS=()
for s in "${LIGHT_STYLES[@]}"; do STYLE_ARGS+=( --light-style "$s" ); done

process_art_set() {
  local data_dir="$1"
  local annotation_dir="$2"

  if [[ ! -d "$data_dir" ]]; then
    echo "Skipping missing data directory: $data_dir" >&2
    return
  fi
  if [[ ! -d "$annotation_dir" ]]; then
    echo "Skipping missing annotation directory: $annotation_dir" >&2
    return
  fi

  echo "Processing art set in $data_dir"

  mkdir -p "$data_dir/$NOON_SUBFOLDER"
  shopt -s nullglob
  local -a annotation_files=("$annotation_dir"/*)
  shopt -u nullglob
  if (( ${#annotation_files[@]} == 0 )); then
    echo "No annotation files found in $annotation_dir" >&2
  else
    cp "${annotation_files[@]}" "$data_dir/$NOON_SUBFOLDER"
  fi

  local -a dn_args=(
    --data "$data_dir"
    --noon "$NOON_SUBFOLDER"
    --warmth "$WARMTH"
    --blue "$BLUE"
    --darkness "$DARKNESS"
    --desat "$DESAT"
    --sat "$SAT"
    --contrast "$CONTRAST"
    --sunrise-center "$SUNRISE_CENTER"
    --sunset-center "$SUNSET_CENTER"
    --twilight-width "$TWILIGHT_WIDTH"
    --noon-blend "$NOON_BLEND"
    --noon-sigma "$NOON_SIGMA"
    "${LK_ARGS[@]}"
  )

  local -a cl_args=(
    --data "$data_dir" --noon "$NOON_SUBFOLDER"
    "${LK_ARGS[@]}" "${STYLE_ARGS[@]}"
    --core-color "$CORE_COLOR" --glow-color "$GLOW_COLOR"
    --core-radius "$CORE_RADIUS" --halo-radius "$HALO_RADIUS"
    --core-gain "$CORE_GAIN" --halo-gain "$HALO_GAIN"
    --highlight-gain "$HIGHLIGHT_GAIN"
    --size-boost "$SIZE_BOOST" --size-radius "$SIZE_RADIUS" --size-gamma "$SIZE_GAMMA"
    --clip-interior "$CLIP_INTERIOR" --clip-erode "$CLIP_ERODE"
    --halo-sep "$HALO_SEP" --halo-gamma "$HALO_GAMMA"
    --blend-mode "$BLEND_MODE"
  )

  local -a pp_args=(
    --data "$data_dir"
    --noon "$NOON_SUBFOLDER"
    --verbose
  )

  if [[ -n "${ONLY_HOUR}" ]]; then
    dn_args+=( --only-hour "$ONLY_HOUR" )
    cl_args+=( --only-hour "$ONLY_HOUR" )
    pp_args+=( --only-hour "$ONLY_HOUR" )
  fi

  python civ3_day_night.py "${dn_args[@]}"
  python civ3_city_lights.py "${cl_args[@]}"
  python civ3_postprocess_pixels.py "${pp_args[@]}"
}

process_art_set "$DAYNIGHT_DATA_DIR" "$DAYNIGHT_ANNOTATION_DIR"
process_art_set "$DISTRICT_DATA_DIR" "$DISTRICT_ANNOTATION_DIR"
