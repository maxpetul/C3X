ART_PATH="/c/Program Files (x86)/GOG Galaxy/Games/Civilization III Complete/Conquests/C3X_Districts/Art/NightDay"
LIGHT_KEY="#00feff"
HOUR=2400

# Terrain
python civ3_daynight_simple.py --data "$ART_PATH" --noon "1200" \
  --night 0.75 --blue-water 0.4 --blue-land 0.4 --sunset 1.0 --light-key "$LIGHT_KEY" #--only-hour "$HOUR"

# Cities
#python civ3_daynight_simple.py --data "$ART_PATH" --noon "1200" \
#  --night 0.75 --blue-water 0.4 --blue-land 0.4 --sunset 1.0 --light-key "$LIGHT_KEY" #--only-hour "$HOUR"

python civ3_city_lights.py --data "$ART_PATH" --noon "1200" #--only-hour "$HOUR" \
  --light-key "$LIGHT_KEY" \
  --core-color "#ff8a20" --glow-color "#dc6a00" \
  --core-radius 1.1 --halo-radius 8 \
  --core-gain 2.1  --halo-gain 20.0 \
  --halo-sep 0.75  --halo-gamma 1.4 \
  --highlight-gain 0.6 \
  --size-boost 0.5 --size-radius 3.5 --size-gamma 0.75 \
  --clip-interior yes --clip-erode 0 \
  --blend-mode screen 

###############
# Best settings
###############

# Lights
# --size-boost 1.1 --size-radius 6.5 \ # More diffuse, looks good on ancient, not so much others

# Remove current paths if exist
#rm -rf "$CURRENT_TERRAIN_PATH"
#rm -rf "$CURRENT_CITIES_PATH"
#
## Move $HOUR to current path, and print to let user know
#echo "Moving '$ORIG_TERRAIN_PATH/$HOUR' -> '$CURRENT_TERRAIN_PATH'..."
#mv "$ORIG_TERRAIN_PATH/$HOUR" "$CURRENT_TERRAIN_PATH"
#mv "$ORIG_CITIES_PATH/$HOUR" "$CURRENT_CITIES_PATH"
#
## Special case: need to copy and replace $ORIG_TERRAIN_PATH/Territory.pcx to $CURRENT_TERRAIN_PATH
#echo "Copying '$ORIG_TERRAIN_PATH/Territory.pcx' -> '$CURRENT_TERRAIN_PATH/Territory.pcx'..."
#cp "$ORIG_TERRAIN_PATH/Territory.pcx" "$CURRENT_TERRAIN_PATH"

# Copy 'Territory.pcx' from '1200' to every other folder
#for dir in "$ART_PATH"/[0-9][0-9][0-9][0-9]; do
#  if [ "$dir" != "$ART_PATH/1200" ]; then
#    echo "Copying '$ART_PATH/1200/Territory.pcx' -> '$dir/Territory.pcx'..."
#    cp "$ART_PATH/1200/Territory.pcx" "$dir"
#  fi
#done
