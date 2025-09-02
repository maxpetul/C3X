ART_PATH="/c/Program Files (x86)/GOG Galaxy/Games/Civilization III Complete/Conquests/C3X_Districts/Art/NightDay"
LIGHT_KEY="#00feff"
HOUR=2400

# Terrain & Cities
#python civ3_daynight_blues.py --data "$ART_PATH" --noon "1200" \
#  --night 0.6 --blue-water 0.3 --blue-land 0.1 --sunset 0.5 --light-key "$LIGHT_KEY" #--only-hour "$HOUR"

python civ3_daynight_grays.py --data "$ART_PATH" --noon "1200" --light-key "$LIGHT_KEY" \
  #--warmth 1.1 --blue 1.12 --darkness 1.08 --desat 0.85 --sat 1.05 --contrast 1.03 # Balanced
  --warmth 1.3 --blue 1.05 --darkness 1.05 --desat 0.8 --sat 1.08 --contrast 1.04 # Warm, cinematic evenings
  #--warmth 1.05 --blue 1.25 --darkness 1.15 --desat 0.9 --sat 1.02 --contrast 1.04 # Moody, cooler nights
  #--warmth 1.2 --blue 1.1 --darkness 1.05 --desat 0.7 --sat 1.1 --contrast 1.0 # Soft, painted look
  #----warmth 1.15 --blue 1.2 --darkness 1.12 --desat 0.85 --sat 1.0 --contrast 1.08 # Crisp, high contrast nights



# Lights
python civ3_city_lights.py --data "$ART_PATH" --noon "1200" \
  --light-key "$LIGHT_KEY" \
  --core-color "#ff8a20" --glow-color "#dc6a00" \
  --core-radius 1.1 --halo-radius 8 \
  --core-gain 2.1  --halo-gain 20.0 \
  --halo-sep 0.75  --halo-gamma 1.4 \
  --highlight-gain 0.6 \
  --size-boost 0.5 --size-radius 3.5 --size-gamma 0.75 \
  --clip-interior yes --clip-erode 0 \
  --blend-mode screen #--only-hour "$HOUR"

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
