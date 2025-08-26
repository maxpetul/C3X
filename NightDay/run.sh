python3 civ3_daynight_simple.py --data ./Data --noon 1200 \
  --night 0.6 --blue-water 0.8 --blue-land 0.4 --sunset 0.9 --light-key "#00feff" --only-hour 2400

python3 civ3_city_lights.py --data ./Data --noon 1200 --only-hour 2400 \
  --light-key "#00feff" \
  --core-color "#ff8a20" --glow-color "#dc6a00" \
  --core-radius 1.1 --halo-radius 8 \
  --core-gain 2.1  --halo-gain 20.0 \
  --halo-sep 0.75  --halo-gamma 1.4 \
  --highlight-gain 0.6 \
  --size-boost 1.1 --size-radius 4.5 --size-gamma 0.75 \
  --clip-interior yes --clip-erode 0 \
  --blend-mode screen 





#############
# Best so far 
#############

#python3 civ3_daynight_simple.py --data ./Data --noon 1200 \
#  --night 0.6 --blue-water 0.8 --blue-land 0.4 --sunset 0.9 --light-key "#00feff" --only-hour 2400

#python3 civ3_city_lights.py --data ./Data --noon 1200 --only-hour 2400 \
#  --light-key "#00feff" \
#  --core-color "#ff8a20" --glow-color "#dc6a00" \
#  --core-radius 1.1 --halo-radius 8 \
#  --core-gain 2.1  --halo-gain 20.0 \
#  --halo-sep 0.75  --halo-gamma 1.4 \
#  --highlight-gain 0.6 \
#  --size-boost 1.1 --size-radius 4.5 --size-gamma 0.75 \
#  --clip-interior yes --clip-erode 0 \
#  --blend-mode screen 

#############
# Notes 
#############
#- Use --size-radius to control how diffuse vs. intense the glow is
#- Territory.pcx gets messed up, but simply keeping the original seems to work fine.
#- For lights, need to do for all cities & TerrainBuildings.pcx