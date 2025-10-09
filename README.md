**C3X Districts** is a mod which enables fully customizable, interactive "Districts" to appear on the Civ 3 map. C3X Districts is inspired by the [Districts feature in Civ 6](https://civilization.fandom.com/wiki/District_(Civ6)) and is built off of [C3X](https://github.com/maxpetul/C3X), a brilliant mod which makes improvements and allows changes to Civ 3 Conquests EXE.

The goal of C3X Districts is to make the Civ 3 map **feel more alive and interesting**. Districts enable large cities to sprawl, infrastructure to be visualized, and - if you enable them - new gameplay additions which make the map more interactive and interesting without fundamentally altering core Civ 3 mechanics. Districts are also 100% customizable and modular, so you can pick and choose types you like, or simply craft your own with custom art, tech dependencies, buildings, and bonuses.

The core purpose of a District is to enable one or more city improvements (buildings). Barracks, for example, require an Encampment District in a city's work radius before they can be built. Once you do so, the Barracks appear within the Encampment:

<img width="1290" height="307" alt="image" src="https://github.com/user-attachments/assets/663dbb1c-07a6-47d3-b362-e5a9550ab7c4" />

Once districts are built, they become unworkable and the tile can no longer be used by the city unless the District is removed. So placing Districts requires a bit of thought beforehand. Once built, however, Districts can offer certain bonuses - shields, food, gold, science, culture, or defensive boost (all configurable) - to ALL cities within their work radius. So cities with Districts in between them (e.g., Rome and Veii have the same Encampment in their work radius) can share the benefits. If `cities_can_share_buildings_by_districts` is set to true, cities can even share buildings within a District. For example, Veii can automatically "use" the Barracks built by Rome, as they share an Encampment.

<img width="331" height="263" alt="image" src="https://github.com/user-attachments/assets/bdd97212-8b9f-4d22-90c1-8b3350efdaaf" />

Bonuses received from Districts are automatically shown in the City view with a purple outline:
<img width="651" height="275" alt="image" src="https://github.com/user-attachments/assets/57e169ee-c6eb-4507-866c-320ee0706a01" />

If a District has food or shield bonuses, you'll also see those in the Production, Food, and Commerce sections:
<img width="870" height="290" alt="image" src="https://github.com/user-attachments/assets/fcd0b16c-1dee-4a72-975d-bad9bd57baee" />

Districts also have certain implications for combat: destroying a District automatically removes any dependent buildings in surrounding cities.

In terms of art, besides showing which buildings are present, Districts can also show different art by era:

<img width="785" height="497" alt="image" src="https://github.com/user-attachments/assets/345a6318-1da5-4d67-9bc7-b342e062d715" />

or even different art by culture, or both:

<img width="1113" height="288" alt="image" src="https://github.com/user-attachments/assets/f4861fbf-e72c-4a4b-810b-a9761f2d1a19" />


# District Types

There are 4 categories of Districts:

- [Standard Districts](#standard-districts) - fully customizable, configuration-dependent art and building dependencies 
- [Neighborhoods](#neighborhoods) - optional, enable population growth and visual urban sprawl
- [Wonder Districts](#wonder-districts) - optional, enable configurable wonders (both built and under construction) to appear on the map
- [Distribution Hubs](#distribution-hubs) - optional, enable certain food and shields from one area to be "distributed" to all connected cities

## Standard Districts

Standard Districts are, well, standard. They may require a technology to be made available and can have zero or more buildings dependent on them. The default standard Districts are:

- **Encampment** - allows Barracks, SAM Missile Battery. Enabled by Warrior Code

    <img width="258" height="130" alt="image" src="https://github.com/user-attachments/assets/41ca18ce-576e-458d-9059-1462898a11cc" />

    
- **Campus** - allows Library, University, Research Lab. Enabled by Literature

    <img width="252" height="132" alt="image" src="https://github.com/user-attachments/assets/43a2980c-69a9-453e-a369-5d9d2c35a834" />

- **Holy Site** - allows Temple, Cathedral. Enabled by Ceremonial Burial.

    <img width="256" height="125" alt="image" src="https://github.com/user-attachments/assets/a4697bcd-de62-4d50-8e6c-aeabd7cb3539" />
 
- **Commercial Hub** - allows Marketplace, Bank, Stock Exchange. Enabled by Currency.

    <img width="255" height="129" alt="image" src="https://github.com/user-attachments/assets/d73b4b30-2eaf-4a85-8280-0d727147644d" />

- **Entertainment Complex** - allows Colosseum. Enabled by Construction.

    <img width="255" height="129" alt="image" src="https://github.com/user-attachments/assets/501e6883-6b54-4939-8fc4-62b47041791f" />

- **Industrial Zone** - allows Factory, Manufacturing Plant. Enabled by Industrialization.

    <img width="256" height="132" alt="image" src="https://github.com/user-attachments/assets/70eb7fd2-5026-421d-ab05-e71ee2d405e6" />

> Note that each District above has various art by era/culture/buildings not shown.

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

> Note that if `#vary_img_by_culture` is set to 1 (true), you **must** have 5 PCX images in the order of: American, Euro, Roman, Mideast, Asian. For example:

  ```
  #img_paths HolySite_AMER.pcx, HolySite_EURO.pcx, HolySite_ROMAN.pcx, HolySite_MIDEAST.pcx, HolySite_ASIAN.pcx
  ```

AI workers are triggered to build Standard Districts [when an AI city attempts to build a dependent building](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/injected_code.c#L10900). This is reverted to its 2nd-most valued production item [while the nearest worker builds the District](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/injected_code.c#L4637). After completion, the AI is able to build the dependent building.

## Neighborhoods

Neighborhoods can be enabled via `enable_neighborhood_districts`. Neighborhoods allow a city to expand in population beyond a certain amount set by `cities_can_share_buildings_by_districts`. After that point, a city will need Neighborhoods, each of what allow the city population to grow at `per_neighborhood_pop_growth_enabled` for each Neighborhood. 

Each culture has 4 possible Neighborhood art designs for each era, for visual variety. We use a [semi-random algorithm keyed by tile X and Y coordinates](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/injected_code.c#L18537-L18548)to (mostly) ensure adjancent tiles use different art while keeping the chosen art deterministic (consistent every time it is rendered).

<img width="612" height="306" alt="image" src="https://github.com/user-attachments/assets/e3ed1da2-97e6-4110-bd10-857e80c00db7" />

AI workers are [triggered to build Neighborhoods when they reach their population and food storage cap](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/injected_code.c#L5070-L5072).

## Wonder Districts

Wonder districts can be enabled via `enable_wonder_districts`. Wonder districts enable Wonders (both Great and Small) to be dependent on having a tile reserved for them. Wonder district art will change when you initiate and complete the Wonder:

<img width="1335" height="250" alt="image" src="https://github.com/user-attachments/assets/dde0fcdd-cbf9-42ed-a3c7-2ac5cd973d7c" />

Additionally, if you set `completed_wonder_districts_can_be_destroyed` to true, well, be prepared to defend your Wonders! Setting `destroyed_wonders_can_be_rebuilt` to true puts destroyed Wonders back into play, such that any civ can once again build them.

Like Standard Districts, the Wonders which required a Wonder District can be configured under [`./Districts/Config/Wonders.txt`](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/Districts/Config/Wonders.txt) in the format:

```
#Wonder
#name                     The Pyramids
#img_row                  0
#img_column               0
#img_construct_row        0
#img_construct_column     1
```

Wonder art entries (both under construction and completed, separate slots) can be configured in the file [`./Art/Districts/1200/Wonders.PCX`](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/Art/Districts/1200/Wonders.PCX).

AI workers are triggered to build Wonder Districts in generally the same workflow as Standard Districts, based on when an AI city chooses a Wonder, triggering a worker to build a Wonder District. The only difference is that unlike Standard Districts, Wonders are specifically ["remembered"](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/injected_code.c#L10907) and [city production set to building the Wonder as soon as the Wonder District is completed](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/injected_code.c#L4434-L4448).

With so many tiles taken up by Districts, you may be wondering how to actually feed your cities or gain shields. First, it's probably a good idea to expand your `city_work_radius` (e.g., to 3, adding at least one more ring of workable tiles around your city) and minimum_city_separation to some higher value. Beyond that though, enter Distribution Hubs.

## Distribution Hubs

Distribution Hub districts can be enabled via `enable_distribution_hub_districts`. Building a Distribution Hub makes all of the surrounding tiles unworkable by its surrounding cities and instead makes the food and shields from those tiles available to all cities in your civ connected by trade route, divided by `distribution_hub_food_yield_divisor` and `distribution_hub_shield_yield_divisor`.

For example, say `distribution_hub_food_yield_divisor` = 2 and `distribution_hub_shield_yield_divisor` = 3 and you have one distribution hub. The **raw** yields of the hub are 10 food and 9 shields.

10 / 2 = 5 food and 9 / 3 = 3 shields. Thus all of your connected cities would gain a bonus 5 food and 3 shields. 

<img width="540" height="272" alt="image" src="https://github.com/user-attachments/assets/4528f5c8-e2bc-4fd6-99c4-ddfcf821547f" />

Distribution hub food and shields are shown in the City view with green outline:

<img width="610" height="462" alt="image" src="https://github.com/user-attachments/assets/c34dd610-452a-4427-a9c3-70369c2889cc" />

This effectively enables "breadbaskets" and heavy mining areas far from your urban centers (think: Egypt feeding ancient Rome, the American midwest feeding the coasts, etc.). Creating a Distribution Hub significantly minimizes the growth potential and production output of a given city, so should be built wisely (and likely far from your urban centers). Distribution Hub yields are not subject to corruption.

AI workers are triggered to build Distribution Hubs when their civ's "ideal" number of Distribution Hubs (calculated using `ai_ideal_distribution_hub_count_per_100_cities`) falls below the existing Distribution Hub count, [assessed in the production phase of each turn](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/injected_code.c#L14106-L14158). The AI determines the best potential Distribution Hub tile based on [distance from the capital (farther = better) and aggregate yields of all surrounding tiles](https://github.com/instafluff0/C3X_Districts/blob/districts_v1/injected_code.c#L14117-L14152).

## Other Non-Standard Districts

The Aerodrome District reuses and slightly modifies the base game airfield art. If `air_units_use_aerodrome_districts_not_cities` is set to true, air units can only be built in cities with an Aerodrome in their radius and can only land on Aerodromes (or vanilla game airfields & carriers), not cities.

<img width="778" height="382" alt="image" src="https://github.com/user-attachments/assets/c4b88569-266b-4a1c-aed3-1806e7e8c35c" />


## Special Thanks
1. [@maxpetul](https://github.com/maxpetul/) (I'm standing on the shoulders of giants!) and others whose work made C3X possible
