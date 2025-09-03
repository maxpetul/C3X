set -euo pipefail

### === CONFIG ===
DATA_DIR="/c/Program Files (x86)/GOG Galaxy/Games/Civilization III Complete/Conquests/C3X_Districts/Art/NightDay"
NOON_SUBFOLDER="1200"
ONLY_HOUR=""        # set empty "" to process all hours

# Multiple light keys
LIGHT_KEYS=( "#00feff")

LIGHT_STYLES=(
  "key=#00feff; core=#ff8a20; glow=#dc6a00;"
)

# ---- Day/Night settings ----
WARMTH=1.05
BLUE=1.5
DARKNESS=1.1
DESAT=0.85
SAT=1.2
CONTRAST=1.08
SUNRISE_CENTER=5.0
SUNSET_CENTER=18.0
TWILIGHT_WIDTH=3.0
NOON_BLEND=0.7
NOON_SIGMA=1.0

# ---- City Lights settings ----
CORE_COLOR="#ff8a20"
GLOW_COLOR="#dc6a00"
CORE_RADIUS=1.1
CORE_GAIN=2.5
HALO_RADIUS=13
HALO_GAIN=20.0
HALO_SEP=0.75
HALO_GAMMA=1.4
HIGHLIGHT_GAIN=0.6
SIZE_BOOST=1.7
SIZE_RADIUS=6.5
SIZE_GAMMA=0.75
CLIP_INTERIOR="yes"   # yes|no
CLIP_ERODE=0
BLEND_MODE="screen"   # screen|add

### === BUILD ARGS ===
LK_ARGS=()
for c in "${LIGHT_KEYS[@]}"; do LK_ARGS+=( --light-key "$c" ); done

STYLE_ARGS=()
for s in "${LIGHT_STYLES[@]}"; do STYLE_ARGS+=( --light-style "$s" ); done

DN_ARGS=(
  --data "$DATA_DIR"
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

CL_ARGS=(
  --data "$DATA_DIR" --noon "$NOON_SUBFOLDER"
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

if [[ -n "${ONLY_HOUR}" ]]; then DN_ARGS+=( --only-hour "$ONLY_HOUR" ); CL_ARGS+=( --only-hour "$ONLY_HOUR" ); fi


python civ3_day_night.py "${DN_ARGS[@]}"
python civ3_city_lights.py "${CL_ARGS[@]}"

# ==========================
# === Day/Night settings ===
# ==========================

#--warmth 1.05 --blue 1.25 --darkness 1.15 --desat 0.9  --sat 1.02 --contrast 1.04   # Moody, cooler nights
#--warmth 1.3  --blue 1.05 --darkness 1.05 --desat 0.8  --sat 1.08 --contrast 1.04   # Warm, cinematic evenings
#--warmth 1.1  --blue 1.12 --darkness 1.08 --desat 0.85 --sat 1.05 --contrast 1.03   # Balanced
#--warmth 1.2  --blue 1.1  --darkness 1.05 --desat 0.7  --sat 1.1  --contrast 1.0    # Soft, painted look
#--warmth 1.15 --blue 1.2  --darkness 1.12 --desat 0.85 --sat 1.0  --contrast 1.08   # Crisp, high contrast nights

#  --warmth", type=float, default=1.10, help="Scale for sunrise/sunset warmth (1.0 = base)."
#  --blue", type=float, default=1.12, help="Scale for night-time blue emphasis (1.0 = base)."
#  --darkness", type=float, default=1.08, help="Scale for extra night darkening (1.0 = base)."
#  --desat", type=float, default=0.85, help="Scale for dusk/night desaturation toward gray (lower = richer)."
#  --sat", type=float, default=1.05, help="Global saturation multiplier after tint (1.0 = none)."
#  --contrast", type=float, default=1.03, help="Global contrast multiplier around mid 128 (1.0 = none)."

# Curve placement/width (optional fine-tuning)
#  --sunrise-center", type=float, default=6.0, help="Hour center for sunrise warmth bump (0-23)."
#  --sunset-center", type=float, default=18.0, help="Hour center for sunset warmth bump (0-23)."
#  --twilight-width", type=float, default=1.8, help="Sigma for sunrise/sunset warmth spread (higher = broader)."
#  --step-minutes", type=int, default=60, help="Time step in minutes; must divide 60 (e.g., 5,10,15,20,30,60). Default: 60."

# Noon-neutral zone controls (defaults designed to keep ~10:00–14:00 close to noon)
#  --noon-blend", type=float, default=0.85, help="0..1 strength to blend toward base palette near 12:00 (0=off)."
#  --noon-sigma", type=float, default=1.1, help="Gaussian width (hours) around 12:00 (larger = broader)."
#  --noon-window-start", type=float, default=10.0, help="Start hour of the noon-like window (default 10.0)."
#  --noon-window-end", type=float, default=14.0, help="End hour of the noon-like window (default 14.0)."
#  --noon-window-soft", type=float, default=0.7, help="Soft edge (hours) for window ramps (0=hard)."


# ==========================
# === City Light settings ===
# ==========================

#  --core-radius", type=float, default=1.1)
#  --halo-radius", type=float, default=13.0)
#  --core-gain",   type=float, default=2.1)
#  --halo-gain",   type=float, default=20.0)
#  --core-color", type=str, default="#ff8a20")
#  --glow-color", type=str, default="#dc6a00")
#  --highlight-gain", type=float, default=0.6)

#  --size-boost",  type=float, default=1.1)
#  --size-radius", type=float, default=3.5)
#  --size-gamma",  type=float, default=0.75)

#  --clip-interior", type=str, default="yes", choices=["yes","no"])
#  --clip-erode", type=int, default=0)

#  --halo-sep", type=float, default=0.75)
#  --halo-gamma", type=float, default=1.4)
#  --blend-mode", type=str, default="screen", choices=["screen","add"])

###############
# Best settings
###############

# Copy 'Territory.pcx' from '1200' to every other folder
#for dir in "$ART_PATH"/[0-9][0-9][0-9][0-9]; do
#  if [ "$dir" != "$ART_PATH/1200" ]; then
#    echo "Copying '$ART_PATH/1200/Territory.pcx' -> '$dir/Territory.pcx'..."
#    cp "$ART_PATH/1200/Territory.pcx" "$dir"
#  fi
#done
