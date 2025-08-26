## TODO
- [x] Workers see district buttons
- [x] District built by workers
- [x] Districts drawn on map
- [ ] Districts drawn vary by era / culture / buildings present
- [?] Districts limited by tech
- [ ] Districts limited by distance to cities
  - Can probably use `unsigned char const workable_tile_counts[8] = {1, 9, 21, 37, 61, 89, 137, 193};` and `config.city_work_radius`
- [ ] City buildings limited by Districts
- [ ] Districts give yields to cities
- [ ] Districts can be pillaged & bombed, buildings lost, ruins shown
- [ ] Workers can enter coastal sea tiles
- [ ] Naval districts built by workers
- [ ] AI builds Districts
- [ ] (maybe?) show Wonders on tile (as small, in corner part) nearby city, though creation process doesn't change, and tile not consumed

## Known bugs/issues
- [ ] Can't build on floodplains (as it's a mine, behind the scenes)


## Districts
- Campus
  - Library
  - University
  - Research Lab
- Holy Site
  - Temple
  - Cathedral
- Encampment
  - Barracks
  - SAM Missile Battery
- Commercial Hub
  - Marketplace
  - Bank
  - Stock Exchange
- Port
  - Harbor
  - Commercial Dock
  - Offshort Platform
  - Coastal Fortress
- Industrial Zone
  - Factory
  - Manufacturing Plant
  - Power plants
  - Recycling Center
- Entertainment Complex
  - Colosseum
- Aerodrome
  - Airport
- Borough

- Water Park (?)
- Aquaduct (?)
- Canal (?)
- Dam (?)
- Spaceport (?)
- Waterfront (?)
- Arsenal (?)
- Theater Square (?)


## Future projects?
- Updated art to be more like Civ 6 - test, but probably wouldn't render well
- Day/night cycle art? - way too much work
- Prominent vs. non-prominent roads? Weird when every tile has a road, could find shortest distance between cities like trade network, then render using same algo railroads use to visually emphasize main roads, but functionality remains the same