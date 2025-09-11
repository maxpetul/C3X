# TODOs
## Day/Night Cycle
- [x] Art
  - [x] Asian
  - [x] Roman
  - [x] European
  - [x] American
  - [x] Mideast
  - [x] Walled cities
  - [x] Terrain buildings
  - [x] Volcanos
  - [x] Railroads
- [x] Mechanics

## Districts
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

# Known bugs/issues
- [ ] Can't build on floodplains (as it's a mine, behind the scenes)


# Districts stats
- Campus
  - Bonus: Science (?)
  - Enables
    - Library
    - University
    - Research Lab
- Holy Site
  - Bonus: Culture (?)
  - Enables
    - Temple
    - Cathedral
- Encampment
  - Enables
    - Barracks
    - SAM Missile Battery
  - Bonus: 50% tile defense
- Commercial Hub
  - Bonus: +1 gold surrounding tiles?
  - Enables
    - Marketplace
    - Bank
    - Stock Exchange
- Port
  - Enables
    - Harbor
    - Commercial Dock
    - Offshore Platform
    - Coastal Fortress
- Industrial Zone
  - Bonus: +1 shield surrounding tiles?
  - Enables
    - Factory
    - Manufacturing Plant
    - Power plants
    - Recycling Center
- Entertainment Complex
  - Enables
    - Colosseum
- Aerodrome
  - Enables
    - Airport
- Borough

- Water Park (?)
- Aquaduct (?)
- Canal (?)
- Dam (?)
- Spaceport (?)
- Waterfront (?)
- Bridge (?)