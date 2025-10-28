#!/usr/bin/env bash
set -euo pipefail

### === CONFIG ===
# Assumed to be run from your project root (adjust paths as needed)

DATA_DIR="../Art/Seasons"
BASE_MONTH="06"     # month folder that stays unchanged and is used as the source
ONLY_MONTH=""       # set to "" to process all months; or "11" to process just November

# Optional: keep the special green index as-is (default behavior of the script is to remap green→magenta)
KEEP_GREEN_INDEX=false  # set to true to pass --keep-green-index

# ---- SEASON settings ----
SEASON_SIGMA=0.8      # Gaussian width in months for season overlap (higher = smoother blending)
FALL_STRENGTH=1.0     # Green→orange/brown shift
WINTER_SNOW=1.0       # Snow whitening + coolness
SPRING_BLOOM=1.0      # Fresh green shift + subtle blossom tint
SUMMER_DRY=0.75       # Late-summer olive/desat

GLOBAL_SAT=1.04       # Mild global saturation after seasonal edits
GLOBAL_CONTRAST=1.03  # Mild global contrast around mid 128

# Light keys (annotation colors) that should be preserved (not tinted).
# These match your day/night palette; add/remove as needed.
LIGHT_KEYS=(
  "#F6915E" # Orange
  "#FEF500" # Yellow
  "#00feff" # Teal
  "#E4080A" # Red
  "#BD15D0" # Purple
  "#2D9C01" # Green
  "#FF25C8" # Pink
  "#0A02EB" # Blue
  "#8262ED" # Indigo
)

### === BUILD ARGS ===
LK_ARGS=()
for key in "${LIGHT_KEYS[@]}"; do
  LK_ARGS+=( --light-key "$key" )
done

SEAS_ARGS=(
  --data "$DATA_DIR"
  --base-month "$BASE_MONTH"
  --season-sigma "$SEASON_SIGMA"
  --fall-strength "$FALL_STRENGTH"
  --winter-snow "$WINTER_SNOW"
  --spring-bloom "$SPRING_BLOOM"
  --summer-dry "$SUMMER_DRY"
  --global-sat "$GLOBAL_SAT"
  --global-contrast "$GLOBAL_CONTRAST"
  "${LK_ARGS[@]}"
)

if [[ -n "$ONLY_MONTH" ]]; then
  SEAS_ARGS+=( --only-month "$(printf '%02d' "$ONLY_MONTH")" )
fi

if [[ "$KEEP_GREEN_INDEX" == true ]]; then
  SEAS_ARGS+=( --keep-green-index )
fi

### === RUN ===
python civ3_seasons.py "${SEAS_ARGS[@]}"
