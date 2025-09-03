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
WARMTH=1.15           # Scale for sunrise/sunset warmth (1.0 = base)
BLUE=1.7              # Scale for night-time blue emphasis (1.0 = base)
DARKNESS=1.0          # Scale for extra night darkening (1.0 = base)
DESAT=1.0             # Scale for dusk/night desaturation toward gray (lower = richer)
SAT=1.1               # Global saturation multiplier after tint (1.0 = none)
CONTRAST=1.08         # Global contrast multiplier around mid 128 (1.0 = none)
SUNRISE_CENTER=6.0    # Hour center for sunrise warmth bump (0-23)
SUNSET_CENTER=18.0    # Hour center for sunset warmth bump (0-23)
TWILIGHT_WIDTH=2.0    # Sigma for sunrise/sunset warmth spread (higher = broader)
NOON_BLEND=0.5        # 0..1 strength to blend toward base palette near 12:00 (0=off)
NOON_SIGMA=1.0        # Gaussian width (hours) around 12:00 (larger = broader)

# ---- City Lights settings ----
CORE_COLOR="#ff8a20"  # Tint color for the bright core (e.g., '#fff87a' for warm yellow)
GLOW_COLOR="#dc6a00"  # Tint color for the outer halo (e.g., '#ff8a20' for orange)
CORE_RADIUS=1.1       # Gaussian blur radius (pixels) of the bright core. Higher = wider/softer core
CORE_GAIN=2.5         # Intensity multiplier for the core alpha (brightness/opacity of the center)
HALO_RADIUS=13        # Gaussian blur radius (pixels) of the outer halo. Higher = glow spreads further outward
HALO_GAIN=20.0        # Intensity multiplier for the halo alpha (strength of the outer glow)
HALO_SEP=0.75         # 0..1: subtracts a fraction of the core from the halo. 0 = seamless merge; 1 = distinct outer ring
HALO_GAMMA=1.4        # Gamma on the halo-only mask. >1 pushes energy outward (softer tail); <1 concentrates near source
HIGHLIGHT_GAIN=0.6    # Extra white highlight added from the core mask (0..~1 typical). Higher = hotter specular highlight
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


# Copy 'Territory.pcx' from '1200' to every other folder
#for dir in "$ART_PATH"/[0-9][0-9][0-9][0-9]; do
#  if [ "$dir" != "$ART_PATH/1200" ]; then
#    echo "Copying '$ART_PATH/1200/Territory.pcx' -> '$dir/Territory.pcx'..."
#    cp "$ART_PATH/1200/Territory.pcx" "$dir"
#  fi
#done
