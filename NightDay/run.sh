ART_PATH="/c/Program Files (x86)/GOG Galaxy/Games/Civilization III Complete/Conquests/C3X_Districts/Art/NightDay"
LIGHT_KEY="#00feff"
HOUR=2400

# Terrain & Cities
python civ3_day_night.py --data "$ART_PATH" --noon "1200" --light-key "$LIGHT_KEY" \
  --warmth 1.05  --blue 1.5 --darkness 1.1 --desat 0.85 --sat 1.2 --contrast 1.08  \
  --sunrise-center 5.0 --sunset-center 18.0 --twilight-width 3.0 \
  --noon-blend 0.7 --noon-sigma 1.0 #--only-hour "$HOUR"
  

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

# Lights
python civ3_city_lights.py --data "$ART_PATH" --noon "1200" \
  --light-key "$LIGHT_KEY" \
  --core-color "#ff8a20" --glow-color "#dc6a00" \
  --core-radius 1.1 --halo-radius 8 \
  --core-gain 2.5  --halo-gain 20.0 \
  --halo-sep 0.75  --halo-gamma 1.4 \
  --highlight-gain 0.6 \
  --size-boost 1.7 --size-radius 6.5 --size-gamma 0.75 \
  --clip-interior yes --clip-erode 0 \
  --blend-mode screen #--only-hour "$HOUR"

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

# Lights
# --size-boost 1.1 --size-radius 6.5 \ # More diffuse, looks good on ancient, not so much others

# Copy 'Territory.pcx' from '1200' to every other folder
#for dir in "$ART_PATH"/[0-9][0-9][0-9][0-9]; do
#  if [ "$dir" != "$ART_PATH/1200" ]; then
#    echo "Copying '$ART_PATH/1200/Territory.pcx' -> '$dir/Territory.pcx'..."
#    cp "$ART_PATH/1200/Territory.pcx" "$dir"
#  fi
#done
