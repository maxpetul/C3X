**C3X Districts** is a mod which enables fully customizable, interactive "Districts" to appear on the Civ 3 map. C3X Districts is inspired by the [Districts feature in Civ 6](https://civilization.fandom.com/wiki/District_(Civ6)) and is built off of [C3X](https://github.com/maxpetul/C3X), a brilliant mod which makes improvements and allows changes to Civ 3 Conquests EXE.

The goal of C3X Districts is to make the Civ 3 map **feel more alive and interesting**. Districts enable large cities to sprawl, infrastructure to be visualized, and - if you enable them - new gameplay additions which make the map more interactive and interesting without fundamentally altering core Civ 3 mechanics. Districts are also 100% customizable and modular, so you can pick and choose types you like, or simply craft your own with custom art, tech dependencies, buildings, and bonuses.

The core purpose of a District is to enable one or more city improvements (buildings). Barracks, for example, require an Encampment District in a city's work radius before they can be built. Once you do so, the Barracks appear within the Encampment:

<img width="1290" height="307" alt="image" src="https://github.com/user-attachments/assets/663dbb1c-07a6-47d3-b362-e5a9550ab7c4" />

Once districts are built, they become unworkable and the tile can no longer be used by the city unless the District is removed. So placing Districts requires a bit of thought beforehand. Once built, however, Districts can offer certain bonuses - shields, food, gold, science, culture, or defensive boost (all configurable) - to ALL cities within their work radius. So cities with Districts in between them (e.g., Rome and Veii have the same Encampment in their work radius) can share the benefits. If `cities_can_share_buildings_by_districts` is set to true, cities can even share buildings within a District. For example, Veii can automatically "use" the Barracks built by Rome, as they share an Encampment.

<img width="331" height="263" alt="image" src="https://github.com/user-attachments/assets/bdd97212-8b9f-4d22-90c1-8b3350efdaaf" />

Bonuses received from Districts are automatically shown in the City view (with purple shadow around them):
<img width="487" height="205" alt="image" src="https://github.com/user-attachments/assets/e0f7f75a-b548-4b3e-b10a-3f5eae0a465a" />

If a District has food or shield bonuses, you'll also see those in the Production, Food, and Commerce sections:
<img width="612" height="182" alt="image" src="https://github.com/user-attachments/assets/bbe6587c-c9a9-4889-add8-b3b3c90c49c5" />

Districts also have certain implications for combat: destroying a District automatically removes any dependant buildings in surrounding cities.

In terms of art, besides showing which buildings are present, Districts can also show different art by era:

<img width="785" height="497" alt="image" src="https://github.com/user-attachments/assets/345a6318-1da5-4d67-9bc7-b342e062d715" />

or even different art by culture, or both:

<img width="1111" height="293" alt="image" src="https://github.com/user-attachments/assets/87d4b807-edbe-4a36-b4a1-ed7553f78ff8" />


# District Types

There are 4 categories of Districts:

- [Standard Districts](#standard-districts) - fully customizable, configuration-dependant art and building dependencies 
- [Neighborhoods](#neighborhoods) - optional, enable population growth and visual urban sprawl
- [Wonder Districts](#wonder-districts) - optional, enable configurable wonders (both built and under construction) to appear on the map
- [Distribution Hubs](#distribution-hubs) - optional, enable certain food and shields from one area to be "distributed" to all connected cities

## Standard Districts

Standard Districts are, well, standard. They may require a technology to be made available and can have zero or more buildings dependant on them. The default standard Districts are:

- **Encampment**
- **Campus**
- **Holy Site**
- **Commercial Hub**
- **Entertainment Complex**
- **Industrial Zone**

Standard Districts are defined under [`./Districts/Config/Districts.txt`](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/Districts/Config/Districts.txt) in the format:

```
#District
#name                         Encampment
#tooltip                      Build Encampment
#img_paths                    Encampment.pcx
#btn_tile_sheet_row           0
#btn_tile_sheet_column        0
#vary_img_by_era              1
#vary_img_by_culture          0
#advance_prereq               Warrior Code
#dependent_improvs            Barracks, "SAM Missile Battery"
#defense_bonus_multiplier_pct 150
#allow_multiple               0
#culture_bonus                0
#science_bonus                0
#food_bonus                   0
#gold_bonus                   0
#shield_bonus                 0
```

District art (for all Districts, not only standard) is under [`./Art/Districts/1200`](https://github.com/instafluff0/C3X_Districts/tree/districts_v1/Art/Districts/1200). 

Note that if `#vary_img_by_culture` is set to 1 (true), you **must** have 5 PCX images in the order of: American, Euro, Roman, Mideast, Asian. For example:

```
#img_paths HolySite_AMER.pcx, HolySite_EURO.pcx, HolySite_ROMAN.pcx, HolySite_MIDEAST.pcx, HolySite_ASIAN.pcx
```

## Neighborhoods

## Wonder Districts

## Distribution Hubs

## Other Districts

## Special Thanks
1. maxpetul and others whose work made C3X possible
