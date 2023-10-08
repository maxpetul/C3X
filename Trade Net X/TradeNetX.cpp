
#include <stdlib.h>
#include <stdio.h>

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

bool
has_road_open_to (Tile * tile, int civ_id)
{
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

	return has_road &&
		((civ_id == 0) ||
		 (tile->Territory_OwnerID == 0) ||
		 ! leaders[civ_id].At_War[tile->Territory_OwnerID]);
}

// ByteLayer stores one byte of data per map tile in a cache-efficient way, assuming that when you access the data for one tile you'll soon need it
// for the surrounding tiles too.
#define BL_BLOCK_WIDTH  8
#define BL_BLOCK_HEIGHT 8
class ByteLayer
{
	struct ByteBlock {
		byte tiles[BL_BLOCK_WIDTH * BL_BLOCK_HEIGHT];
	};

public:
	ByteBlock * blocks;
	int map_width, map_height;
	int width_in_blocks, height_in_blocks;

	ByteLayer (int _map_width, int _map_height)
	{
		map_width = _map_width, map_height = _map_height;
		width_in_blocks  = map_width  / (2*BL_BLOCK_WIDTH) + (map_width  % (2*BL_BLOCK_WIDTH) != 0 ? 1 : 0);
		height_in_blocks = map_height /    BL_BLOCK_HEIGHT + (map_height %    BL_BLOCK_HEIGHT != 0 ? 1 : 0);
		blocks = (ByteBlock *)calloc (width_in_blocks * height_in_blocks, sizeof *blocks);
	}

	~ByteLayer ()
	{
		free (blocks);
	}

	byte&
	at (int x, int y)
	{
		// Coordinates of the block (x, y) is in and the "remainder" coords inside the block
		int block_x = x / (2*BL_BLOCK_WIDTH), r_x = x % (2*BL_BLOCK_WIDTH),
		    block_y = y /    BL_BLOCK_HEIGHT, r_y = y %    BL_BLOCK_HEIGHT;

		int block_index = block_y * width_in_blocks + block_x,
		    tile_index = r_y * BL_BLOCK_WIDTH + r_x / 2;

		return blocks[block_index].tiles[tile_index];
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
	ExplorationBlock * blocks;
	byte * block_inits; // bit array storing which blocks have been filled in
	int map_width, map_height;
	int width_in_blocks, height_in_blocks;

	ExplorationLayer (int _map_width, int _map_height)
	{
		map_width = _map_width, map_height = _map_height;
		width_in_blocks  = map_width  / (2*EL_BLOCK_WIDTH) + (map_width  % (2*EL_BLOCK_WIDTH) != 0 ? 1 : 0);
		height_in_blocks = map_height /    EL_BLOCK_HEIGHT + (map_height %    EL_BLOCK_HEIGHT != 0 ? 1 : 0);
		int block_count = width_in_blocks * height_in_blocks;
		blocks = (ExplorationBlock *)calloc (block_count, sizeof *blocks);
		block_inits = (byte *)calloc (block_count / 8 + (block_count % 8 != 0 ? 1 : 0), sizeof *block_inits);
	}

	~ExplorationLayer ()
	{
		free (blocks);
		free (block_inits);
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

struct TNXCache {
	ExplorationLayer exploration_layer;
	ByteLayer tile_info;
};

enum TileInfo : byte {
	TI_INFO_SET          = 0x01,

	TI_LAND              = 0x00,
	TI_COAST             = 0x02,
	TI_SEA               = 0x04,
	TI_OCEAN             = 0x06,

	TI_HAS_CITY          = 0x08,
	TI_HAS_UNIT          = 0x10,
	TI_NEUTRAL_TERRITORY = 0x20,
};

#define TI_TERRAIN_MASK ((byte)0x6)

// This function is a replacement for Trade_Net::get_movement_cost optimized for the case of building a sea trade network. It assumes "flags" is
// either 0x1009 or 0x9 and that "unit" is NULL (so the unit parameter is omitted).
EXPORT_PROC
int
get_move_cost_for_sea_trade (Trade_Net * trade_net, TNXCache * tnx_cache, int from_x, int from_y, int to_x, int to_y, int civ_id, uint flags, int neighbor_index)
{
	Map * map = &p_bic_data->Map;
	ExplorationLayer * el = &tnx_cache->exploration_layer;
	ByteLayer * tile_info = &tnx_cache->tile_info;

	// Check parameters
	if ((to_x   < 0) || (to_x   >= map->Width) || (to_y   < 0) || (to_y   >= map->Height) ||
	    (from_x < 0) || (from_x >= map->Width) || (from_y < 0) || (from_y >= map->Height))
		return -1;
	if ((neighbor_index > 8) || (neighbor_index < 1))
		return -1;

	// Conversion to river code index (? - TODO: Check this)
	int alt_neighbor_index = (neighbor_index != 8) ? neighbor_index : 0;

	// Check that the tiles are adjacent
	if ((Map_get_x_dist (map, from_x, to_x) + Map_get_y_dist (map, from_y, to_y)) / 2 != 1)
		return -1;

	// If either of the from or to tiles have not been explored by the civ, return -1
	if (! (el.has_explored (civ_id, from_x, from_y) && el.has_explored (civ_id, to_x, to_y)))
		return -1;

	// Get info bytes for "to" and "from" tiles, initializing them if necessary
	byte from_info, to_info; {
		for (int n = 0; n < 2; n++) {
			int x = (n == 0) ? from_x : to_x,
			    y = (n == 0) ? from_y : to_y;
			byte info = tile_info.at (x, y);

			// Initialize info
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

				info |= (tile->vtable->m38_Get_Territory_OwnerID (tile) == 0) ? TI_NEUTRAL_TERRITORY : 0;

				tile_info.at (x, y) = info; // Save info for future use
			}

			if (n == 0)
				from_info = info;
			else
				to_info = info;
		}
	}

	// If flag 0x1000 is set and the to tile is occupied by a unit belonging to a civ we're at war with, return -1
	if ((flags & 0x1000) && (to_info & TI_HAS_UNIT)) {
		int occupier_id = get_combat_occupier (to_x, to_y, civ_id, 0);
		if ((occupier_id >= 0) && leaders[civ_id].At_War[occupier_id])
			return -1;
	}


}

EXPORT_PROC
int
test ()
{
	int failure_count = 0;

	struct bl_test {
		int width, height, prime;
	} bl_tests[] = {
		{25,   25, 3571},
		{128, 128, 48611},
		{130,  70, 10061},
	};

	for (int n_test = 0; n_test < (sizeof bl_tests) / (sizeof bl_tests[0]); n_test++) {
		struct bl_test t = bl_tests[n_test];
		ByteLayer bl(t.width, t.height);
		for (int y = 0; y < t.height; y++)
			for (int x = y%2; x < t.width; x += 2)
				bl.at (x, y) = (y * t.width + x) * t.prime;
		for (int y = 0; y < t.height; y++)
			for (int x = y%2; x < t.width; x += 2)
				if (bl.at (x, y) != (byte)((y * t.width + x) * t.prime)) {
					failure_count++;
					goto fail;
				}
	fail:
		;
	}

	{
		int width = 175, height = 175;
		ByteLayer bl(width, height);
		for (int y = 0; y < height; y++)
			for (int x = y%2; x < width; x += 2)
				if ((((y * width) + x) * 132241) % 30 == 0)
					bl.at (x, y) = 1;
		for (int y = 0; y < height; y++)
			for (int x = y%2; x < width; x += 2) {
				bool was_set = (((y * width) + x) * 132241) % 30 == 0;
				if ((   was_set  && (bl.at (x, y) == 0)) ||
				    ((! was_set) && (bl.at (x, y) != 0))) {
					failure_count++;
					goto fail_2;
				}
			}
	}
	fail_2:

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
