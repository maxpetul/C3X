
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <queue>

#define NOVIRTUALKEYCODES // Keycodes defined in Civ3Conquests.h instead
#include "windows.h"

typedef unsigned char byte;

#include "Civ3Conquests.hpp"

#define EXPORT_PROC extern "C" __declspec(dllexport)

int const * bin_addrs = NULL;

bool
has_outpost (Tile * tile)
{
	return false; // TODO: Implement me
}

// UShortLayer stores two bytes of data per map tile in a cache-efficient way, assuming that when you access the data for one tile you'll soon need it
// for the surrounding tiles too.
#define USL_BLOCK_WIDTH  8
#define USL_BLOCK_HEIGHT 4
class UShortLayer
{
	struct UShortBlock {
		unsigned short tiles[USL_BLOCK_WIDTH * USL_BLOCK_HEIGHT];
	};

public:
	UShortBlock * blocks;
	size_t blocks_size;
	int map_width, map_height;
	int width_in_blocks, height_in_blocks;

	UShortLayer (int _map_width, int _map_height)
	{
		map_width = _map_width, map_height = _map_height;
		width_in_blocks  = map_width  / (2*USL_BLOCK_WIDTH) + (map_width  % (2*USL_BLOCK_WIDTH) != 0 ? 1 : 0);
		height_in_blocks = map_height /    USL_BLOCK_HEIGHT + (map_height %    USL_BLOCK_HEIGHT != 0 ? 1 : 0);
		blocks_size = width_in_blocks * height_in_blocks * (sizeof *blocks);
		blocks = (UShortBlock *)malloc (blocks_size);
		clear ();
	}

	~UShortLayer ()
	{
		free (blocks);
	}

	void
	clear ()
	{
		memset (blocks, 0, blocks_size);
	}

	unsigned short&
	at (int x, int y)
	{
		// Coordinates of the block (x, y) is in and the "remainder" coords inside the block
		int block_x = x / (2*USL_BLOCK_WIDTH), r_x = x % (2*USL_BLOCK_WIDTH),
		    block_y = y /    USL_BLOCK_HEIGHT, r_y = y %    USL_BLOCK_HEIGHT;

		int block_index = block_y * width_in_blocks + block_x,
		    tile_index = r_y * USL_BLOCK_WIDTH + r_x / 2;

		return blocks[block_index].tiles[tile_index];
	}
};

// This is like UShortLayer except it stores only two bits per tile
#define TBL_BLOCK_WIDTH  16
#define TBL_BLOCK_HEIGHT 16
class TwoBitLayer
{
	struct TwoBitBlock {
		unsigned int pieces[16];
	};

public:
	TwoBitBlock * blocks;
	size_t blocks_size;
	int map_width, map_height;
	int width_in_blocks, height_in_blocks;

	TwoBitLayer (int _map_width, int _map_height)
	{
		map_width = _map_width, map_height = _map_height;
		width_in_blocks  = map_width  / (2*TBL_BLOCK_WIDTH) + (map_width  % (2*TBL_BLOCK_WIDTH) != 0 ? 1 : 0);
		height_in_blocks = map_height /    TBL_BLOCK_HEIGHT + (map_height %    TBL_BLOCK_HEIGHT != 0 ? 1 : 0);
		blocks_size = width_in_blocks * height_in_blocks * (sizeof *blocks);
		blocks = (TwoBitBlock *)malloc (blocks_size);
		clear ();
	}

	~TwoBitLayer ()
	{
		free (blocks);
	}

	void
	clear ()
	{
		memset (blocks, 0, blocks_size);
	}

	unsigned int *
	get_piece (int x, int y, int * out_index_within_piece)
	{
		// Coordinates of the block (x, y) is in and the "remainder" coords inside the block
		int block_x = x / (2*TBL_BLOCK_WIDTH), r_x = x % (2*TBL_BLOCK_WIDTH),
		    block_y = y /    TBL_BLOCK_HEIGHT, r_y = y %    TBL_BLOCK_HEIGHT;

		int block_index = block_y * width_in_blocks + block_x,
		    tile_index = r_y * TBL_BLOCK_WIDTH + r_x / 2;

		// There are 16 tiles in each piece (32 bits / 2 bits/tile = 16 tiles)
		*out_index_within_piece = tile_index % 16;
		return &blocks[block_index].pieces[tile_index/16];
	}

	byte
	get (int x, int y)
	{
		int i;
		unsigned int * piece = get_piece (x, y, &i);
		return (byte)((*piece >> (i * 2)) & 3);
	}

	void
	set (int x, int y, byte two_bit_val)
	{
		int i;
		unsigned int * piece = get_piece (x, y, &i);
		unsigned int mask = 3U << (i * 2);
		*piece = (*piece & ~mask) | (((unsigned int)two_bit_val << (i * 2)) & mask);
	}
};

// ExplorationLayer stores the "Fog_Of_War" fields from tile objects in a cache-efficient fashion. Those fields record which civs have revealed each
// tile. The fields are stored in 4x4 blocks which are filled in as needed.
#define EL_BLOCK_WIDTH  4
#define EL_BLOCK_HEIGHT 4
class ExplorationLayer
{
	struct ExplorationBlock {
		unsigned int tiles[EL_BLOCK_WIDTH * EL_BLOCK_HEIGHT];
	};

public:
	Map * map;
	ExplorationBlock * blocks;
	byte * block_inits; // bit array storing which blocks have been filled in
	size_t block_inits_size;
	int width_in_blocks, height_in_blocks;

	ExplorationLayer (Map * _map)
	{
		map = _map;
		width_in_blocks  = map->Width  / (2*EL_BLOCK_WIDTH) + (map->Width  % (2*EL_BLOCK_WIDTH) != 0 ? 1 : 0);
		height_in_blocks = map->Height /    EL_BLOCK_HEIGHT + (map->Height %    EL_BLOCK_HEIGHT != 0 ? 1 : 0);
		int block_count = width_in_blocks * height_in_blocks;
		blocks = (ExplorationBlock *)calloc (block_count, sizeof *blocks);
		block_inits_size = (block_count / 8 + (block_count % 8 != 0 ? 1 : 0)) * (sizeof *block_inits);
		block_inits = (byte *)malloc (block_inits_size);
		clear ();
	}

	~ExplorationLayer ()
	{
		free (blocks);
		free (block_inits);
	}

	void
	clear ()
	{
		memset (block_inits, 0, block_inits_size);
	}

	bool
	has_explored (int civ_id, int x, int y)
	{
		int block_x = x / (2*EL_BLOCK_WIDTH), r_x = x % (2*EL_BLOCK_WIDTH),
		    block_y = y /    EL_BLOCK_HEIGHT, r_y = y %    EL_BLOCK_HEIGHT;

		int block_index = block_y * width_in_blocks + block_x,
		    tile_index = r_y * EL_BLOCK_WIDTH + r_x / 2;

		if ((block_inits[block_index/8] & (1 << block_index%8)) == 0) {
			for (int by = 0; by < EL_BLOCK_HEIGHT; by++)
				for (int bx = 0; bx < EL_BLOCK_WIDTH; bx++) {
					int tile_y = EL_BLOCK_HEIGHT * block_y + by;
					int tile_x = 2 * (EL_BLOCK_WIDTH * block_x + bx) + tile_y % 2;
					Tile * tile = tile_at (tile_x, tile_y);
					if (tile != p_null_tile)
						blocks[block_index].tiles[by * EL_BLOCK_WIDTH + bx] = tile->Body.Fog_Of_War;
				}
			block_inits[block_index/8] |= 1 << block_index%8;
		}

		return (blocks[block_index].tiles[tile_index] & (1U << civ_id)) != 0;
	}
};

class TNXCache {
public:
	ExplorationLayer exploration_layer;
	UShortLayer tile_info;

	// These bit fields store which players have the necessary techs/wonders to trade over sea/ocean
	unsigned int sea_trade_player_bits;
	unsigned int ocean_trade_player_bits;

	bool pf_holding_state;
	int pf_civ_id;
	unsigned int pf_flags;
	TwoBitLayer pf_node_states;
	std::vector<CoordPair> pf_open_set;

	TNXCache (Map * map) :
		exploration_layer(map),
		tile_info(map->Width, map->Height),
		pf_holding_state(false),
		pf_civ_id(-1),
		pf_flags(0),
		pf_node_states(map->Width, map->Height)
	{}

	void
	set_up_before_building_network ()
	{
		exploration_layer.clear ();
		tile_info.clear ();

		sea_trade_player_bits = ocean_trade_player_bits = 0;
		for (int n = 0; n < 32; n++)
			if (*p_player_bits & (1 << n)) {
				if (Leader_has_tech_with_flag (&leaders[n], ATF_Enables_Trade_Over_Sea_Tiles) ||
				    (Leader_count_wonders_with_flag (&leaders[n], ITW_Safe_Sea_Travel, NULL) > 0))
					sea_trade_player_bits |= (1U << n);
				if (Leader_has_tech_with_flag (&leaders[n], ATF_Enables_Trade_Over_Ocean_Tiles))
					ocean_trade_player_bits |= (1U << n);
			}

		clear_pathfinder_state ();
	}

	void
	clear_pathfinder_state ()
	{
		pf_holding_state = false;
		pf_civ_id = -1;
		pf_flags = 0;
		pf_node_states.clear ();
		pf_open_set.clear ();
	}
};

EXPORT_PROC
void *
create_tnx_cache (Map * map)
{
	return new TNXCache (map);
}

EXPORT_PROC
void
destroy_tnx_cache (void * tnx_cache)
{
	delete tnx_cache;
}

EXPORT_PROC
void
set_up_before_building_network (void * tnx_cache)
{
	((TNXCache *)tnx_cache)->set_up_before_building_network ();
}

enum TileInfo : unsigned short {
	TI_INFO_SET = 0x01,

	TI_LAND     = 0x00,
	TI_COAST    = 0x02,
	TI_SEA      = 0x04,
	TI_OCEAN    = 0x06,

	TI_HAS_CITY = 0x08,
	TI_HAS_UNIT = 0x10,

	// 0x20, 0x40, 0x80, 0x100, 0x200 bits contain territory owner
	// 0x400 is reserved

	TI_HAS_ROAD = 0x800,
};

#define TI_TERRAIN_MASK ((unsigned short)0x6)

#define TI_SET_OWNER(ti, id) (((ti)&~(31<<5)) | (((id)&31)<<5))
#define TI_GET_OWNER(ti) (((ti)>>5)&31)

bool
has_road_open_to (unsigned short tile_info, int civ_id)
{
	int territory_owner_id = TI_GET_OWNER (tile_info);
	return (tile_info & TI_HAS_ROAD)
		&& ((civ_id == 0)
		    || (territory_owner_id == 0)
		    || ! leaders[civ_id].At_War[territory_owner_id]);
}

unsigned short
get_tile_info (UShortLayer * tile_info, int x, int y)
{
	unsigned short info = tile_info->at (x, y);

	// Initialize info if necessary
	if ((info & TI_INFO_SET) == 0) {
		Tile * tile = tile_at (x, y);
		info = TI_INFO_SET;

		if (tile->vtable->m35_Check_Is_Water (tile)) {
			SquareTypes terrain_type = tile->vtable->m50_Get_Square_BaseType (tile);
			if      (terrain_type == SQ_Coast) info |= TI_COAST;
			else if (terrain_type == SQ_Sea)   info |= TI_SEA;
			else if (terrain_type == SQ_Ocean) info |= TI_OCEAN;
		}

		info |= Tile_has_city (tile) ? TI_HAS_CITY : 0;

		bool any_units_on_tile = false; {
			if (p_units->Units != NULL) {
				int tile_unit_id = tile->vtable->m40_get_TileUnit_ID (tile);
				int unit_id = TileUnits_TileUnitID_to_UnitID (p_tile_units, tile_unit_id, NULL);
				any_units_on_tile = (unit_id >= 0) && (unit_id <= p_units->LastIndex) &&
					((int)p_units->Units[unit_id].Unit > offsetof (Unit, Body));
			}
		}
		info |= any_units_on_tile ? TI_HAS_UNIT : 0;

		info = TI_SET_OWNER (info, tile->vtable->m38_Get_Territory_OwnerID (tile));

		bool has_road; {
			int tech_req_civ_id = -1;
			if (tile->CityID != -1)
				tech_req_civ_id = tile->vtable->m69_get_Tile_City_CivID (tile);
			else if (has_outpost (tile))
				tech_req_civ_id = tile->vtable->m70_Get_Tile_Building_OwnerID (tile);

			if ((tech_req_civ_id < 0) ||
			    Leader_has_tech (&leaders[tech_req_civ_id], p_bic_data->WorkerJobs[WJ_Build_Road].RequireID))
				has_road = tile->Overlays & 1;
			else
				has_road = false;
		}
		info |= has_road ? TI_HAS_ROAD : 0;

		tile_info->at (x, y) = info; // Save info for future use
	}

	return info;
}

enum FloodFillNodeState {
	FFNS_NONE = 0, // Hasn't been processed yet
	FFNS_OPEN, // Pending processing
	FFNS_REACHABLE, // Processed & concluded this tile is reachable
	FFNS_UNREACHABLE // Processed & concluded this tile is NOT reachable
};

EXPORT_PROC
void
flood_fill_road_network (void * tnx_cache, int from_x, int from_y, int civ_id)
{
	struct CoordPair {
		short x, y;
	};

	UShortLayer * tile_info = &((TNXCache *)tnx_cache)->tile_info;

	if (! has_road_open_to (get_tile_info (tile_info, from_x, from_y), civ_id))
		return;

	int from_tile_city_id = city_at (from_x, from_y)->Body.ID;

	TwoBitLayer node_states(p_bic_data->Map.Width, p_bic_data->Map.Height);
	std::vector<CoordPair> open; // Stores tile indices

	// The original code iterates over neighbors in this non-standard order. I've replicated that here although I doubt it matters. Computing the
	// dx's & dy's ahead of time speeds up the flood fill operation modestly.
	int const neighbor_dirs[8] = {DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_E, DIR_S, DIR_W, DIR_N};
	int8_t dxs[8];
	int8_t dys[8];
	for (int n = 0; n < 8; n++) {
		int dx, dy;
		neighbor_index_to_diff (neighbor_dirs[n], &dx, &dy);
		dxs[n] = dx;
		dys[n] = dy;
	}

	// Add (from_x, from_y) to open set
	node_states.set (from_x, from_y, FFNS_OPEN);
	open.push_back (CoordPair{(short)from_x, (short)from_y});

	while (! open.empty ()) {
		// Remove node from open set
		struct CoordPair coords = open.back ();
		open.pop_back ();
		int x = coords.x, y = coords.y;

		if (has_road_open_to (get_tile_info (tile_info, x, y), civ_id)) {
			// Add this tile to the road network
			tile_at (x, y)->Body.connected_city_ids[civ_id] = from_tile_city_id;
			node_states.set (x, y, FFNS_REACHABLE);

			// Loop over all tiles neighboring (x, y)
			for (int n = 0; n < 8; n++) {
				int nx = Map_wrap_horiz (&p_bic_data->Map, x + dxs[n]),
				    ny = Map_wrap_vert  (&p_bic_data->Map, y + dys[n]);

				// If this neighbor is not already in a set, add it to the open set
				if ((nx >= 0) && (nx < p_bic_data->Map.Width) &&
				    (ny >= 0) && (ny < p_bic_data->Map.Height) &&
				    (node_states.get (nx, ny) == FFNS_NONE)) {
					open.push_back (CoordPair{(short)nx, (short)ny});
					node_states.set (nx, ny, FFNS_OPEN);
				}
			}
		} else
			node_states.set (x, y, FFNS_UNREACHABLE);
	}
}

bool
can_trade_via_water_at (TNXCache * tnx_cache, int civ_id, int x, int y, bool ignore_enemy_units, bool ignore_enemy_territory)
{
	int occupier_id;
	unsigned short tile_info = get_tile_info (&tnx_cache->tile_info, x, y);
	return  // We must have explored the tile
		   tnx_cache->exploration_layer.has_explored (civ_id, x, y)
		// AND (we can ignore enemy units OR there's no combat unit on the tile OR there is but we're not at war)
		&& (   ignore_enemy_units
		    || ! (tile_info & TI_HAS_UNIT)
		    || (0 > (occupier_id = get_combat_occupier (x, y, civ_id, 0)))
		    || ! leaders[civ_id].At_War[occupier_id])
		// AND (we can ignore enemy territory OR the tile doesn't belong to any civ OR it does but we're not at war)
		&& (   ignore_enemy_territory
		    || (0 == TI_GET_OWNER (tile_info))
		    || ! leaders[civ_id].At_War[TI_GET_OWNER (tile_info)])
		// AND (the tile is not land OR it is but has a city)
		&& (((tile_info & TI_TERRAIN_MASK) != TI_LAND) || (tile_info & TI_HAS_CITY))
		// AND (the tile is not sea OR we can trade over sea)
		&& (((tile_info & TI_TERRAIN_MASK) != TI_SEA) || (tnx_cache->sea_trade_player_bits & (1 << civ_id)))
		// AND (tile tile is not ocean OR we can trade over ocean)
		&& (((tile_info & TI_TERRAIN_MASK) != TI_OCEAN) || (tnx_cache->ocean_trade_player_bits & (1 << civ_id)));
}

// This function is a replacement for Trade_Net::get_movement_cost optimized for the case of building a sea trade network. It assumes "flags" is
// either 0x1009 or 0x9 and that "unit" is NULL (so the unit parameter is omitted).
// For efficiency, it doesn't compute the actual movement cost and instead returns 1 if the move is possible and -1 otherwise.
EXPORT_PROC
int
get_move_cost_for_sea_trade (Trade_Net * trade_net, void * tnx_cache, int from_x, int from_y, int to_x, int to_y, int civ_id, unsigned int flags, int neighbor_index, Trade_Net_Distance_Info * dist_info)
{
	Map * map = &p_bic_data->Map;
	ExplorationLayer * el = &((TNXCache *)tnx_cache)->exploration_layer;
	UShortLayer * tile_info = &((TNXCache *)tnx_cache)->tile_info;

	// Check parameters
	if ((to_x   < 0) || (to_x   >= map->Width) || (to_y   < 0) || (to_y   >= map->Height) ||
	    (from_x < 0) || (from_x >= map->Width) || (from_y < 0) || (from_y >= map->Height))
		return -1;
	if ((neighbor_index > 8) || (neighbor_index < 1))
		return -1;

	// Check that the tiles are adjacent
	if ((Map_get_x_dist (map, from_x, to_x) + Map_get_y_dist (map, from_y, to_y)) / 2 != 1)
		return -1;

	return (can_trade_via_water_at ((TNXCache *)tnx_cache, civ_id, from_x, from_y, true              , true) &&
		can_trade_via_water_at ((TNXCache *)tnx_cache, civ_id, to_x  , to_y  , ! (flags & 0x1000), false)) ?
		1 : -1;
}

// This function replaces Trade_Net::set_unit_path when searching for a water trade route.
EXPORT_PROC
bool
try_drawing_sea_trade_route (Trade_Net * trade_net, void * vp_tnx_cache, int from_x, int from_y, int to_x, int to_y, int civ_id, unsigned int flags)
{
	/*
	struct CoordPair {
		short x, y;

		int
		squared_dist (CoordPair const& other) const
		{
			int dx = (int)x - other.x, dy = (int)y - other.y; // TODO: Use Map_get_x/y_dist instead
			return dx*dx + dy*dy;
		}
	};

	struct CloserTo {
		CoordPair target;

		CloserTo(int x, int y) : target{(short)x, (short)y} {}

		bool
		operator() (CoordPair const& a, CoordPair const& b) const
		{
			return a.squared_dist (target) > b.squared_dist (target);
		}
	};
	*/

	TNXCache * tnx_cache = (TNXCache *)vp_tnx_cache;
	UShortLayer * tile_info = &tnx_cache->tile_info;
	TwoBitLayer& node_states = tnx_cache->pf_node_states;
	std::vector<CoordPair>& open = tnx_cache->pf_open_set;

	// TwoBitLayer node_states(p_bic_data->Map.Width, p_bic_data->Map.Height);
	// std::priority_queue<CoordPair, std::vector<CoordPair>, CloserTo> open(CloserTo(to_x, to_y));
	// std::vector<CoordPair> open;

	// Check if we can reuse intermediate results from a previous pathfinding operation
	if (tnx_cache->pf_holding_state && (tnx_cache->pf_civ_id == civ_id) && (tnx_cache->pf_flags == flags) && (node_states.get (from_x, from_y) == FFNS_REACHABLE)) {
		// If we've already determined the reachability of the "to" tile, return that. Otherwise re-enter the main loop to continue searching
		// for a path from the previously accessible set.
		byte to_node_state = node_states.get (to_x, to_y);
		if (to_node_state == FFNS_REACHABLE)
			return true;
		else if (to_node_state == FFNS_UNREACHABLE)
			return false;

	// Set up new pathfinding state
	} else {
		tnx_cache->clear_pathfinder_state ();

		// Add "from" tile to the open set
		node_states.set (from_x, from_y, FFNS_OPEN);
		// open.push (CoordPair{(short)from_x, (short)from_y});
		open.push_back (CoordPair{(short)from_x, (short)from_y});

		tnx_cache->pf_civ_id = civ_id;
		tnx_cache->pf_flags = flags;
		tnx_cache->pf_holding_state = true;
	}

	// neighbor_dirs same as flood fill
	int const neighbor_dirs[8] = {DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_E, DIR_S, DIR_W, DIR_N};
	int8_t dxs[8];
	int8_t dys[8];
	for (int n = 0; n < 8; n++) {
		int dx, dy;
		neighbor_index_to_diff (neighbor_dirs[n], &dx, &dy);
		dxs[n] = dx;
		dys[n] = dy;
	}

	while (! open.empty ()) {
		// Remove node from open set
		// CoordPair coords = open.top ();
		CoordPair coords = open.back ();
		// open.pop ();
		open.pop_back ();
		int x = coords.x, y = coords.y;

		if (can_trade_via_water_at (tnx_cache, civ_id, x, y, ! (flags & 0x1000), false)) {
			node_states.set (x, y, FFNS_REACHABLE);
			if ((x == to_x) && (y == to_y))
				return true;

			// Loop over all tiles neighboring (x, y)
			for (int n = 0; n < 8; n++) {
				int nx = Map_wrap_horiz (&p_bic_data->Map, x + dxs[n]),
				    ny = Map_wrap_vert  (&p_bic_data->Map, y + dys[n]);

				// If this neighbor is not already in a set, add it to the open set
				if ((nx >= 0) && (nx < p_bic_data->Map.Width) &&
				    (ny >= 0) && (ny < p_bic_data->Map.Height) &&
				    (node_states.get (nx, ny) == FFNS_NONE)) {
					// open.push (CoordPair{(short)nx, (short)ny});
					open.push_back (CoordPair{(short)nx, (short)ny});
					node_states.set (nx, ny, FFNS_OPEN);
				}
			}
		} else
			node_states.set (x, y, FFNS_UNREACHABLE);
	}

	return false;
}

EXPORT_PROC
int
test ()
{
	int failure_count = 0;

	struct usl_test {
		int width, height, prime;
	} usl_tests[] = {
		{25,   25, 3571},
		{128, 128, 48611},
		{130,  70, 10061},
	};

	for (int n_test = 0; n_test < (sizeof usl_tests) / (sizeof usl_tests[0]); n_test++) {
		struct usl_test t = usl_tests[n_test];
		UShortLayer usl(t.width, t.height);
		for (int y = 0; y < t.height; y++)
			for (int x = y%2; x < t.width; x += 2)
				usl.at (x, y) = (y * t.width + x) * t.prime;
		for (int y = 0; y < t.height; y++)
			for (int x = y%2; x < t.width; x += 2)
				if (usl.at (x, y) != (unsigned short)((y * t.width + x) * t.prime)) {
					failure_count++;
					goto fail;
				}
	fail:
		;
	}

	{
		int width = 175, height = 175;
		UShortLayer usl(width, height);
		for (int y = 0; y < height; y++)
			for (int x = y%2; x < width; x += 2)
				if ((((y * width) + x) * 132241) % 30 == 0)
					usl.at (x, y) = 1;
		for (int y = 0; y < height; y++)
			for (int x = y%2; x < width; x += 2) {
				bool was_set = (((y * width) + x) * 132241) % 30 == 0;
				if ((   was_set  && (usl.at (x, y) == 0)) ||
				    ((! was_set) && (usl.at (x, y) != 0))) {
					failure_count++;
					goto fail_2;
				}
			}
	}
	fail_2:

	struct tbl_test {
		int width, height, prime;
	} tbl_tests[] = {
		{80,  80,  103},
		{100, 50,  109},
		{202, 101, 139},
		{59,  73,  167},
		{362, 362, 3571},
		{10,  10,  48611},
	};
	for (int n_test = 0; n_test < (sizeof tbl_tests) / (sizeof tbl_tests[0]); n_test++) {
		struct tbl_test t = tbl_tests[n_test];
		TwoBitLayer tbl(t.width, t.height);
		for (int y = 0; y < t.height; y++)
			for (int x = y%2; x < t.width; x += 2)
				tbl.set (x, y, (y * t.width + x) * t.prime);
		for (int y = 0; y < t.height; y++)
			for (int x = y%2; x < t.width; x += 2)
				if (tbl.get (x, y) != (byte)(((y * t.width + x) * t.prime) & 3)) {
					failure_count++;
					goto fail_3;
				}
	}
	fail_3:

	return failure_count;
}

EXPORT_PROC
void
set_exe_version (int index)
{
	if      (index == 0) bin_addrs = gog_addrs;
	else if (index == 1) bin_addrs = steam_addrs;
	else if (index == 2) bin_addrs = pcg_addrs;
}
