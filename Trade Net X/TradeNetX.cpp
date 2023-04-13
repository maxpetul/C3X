
#include <stdlib.h>
#include <stdio.h>
#include <tuple>

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

struct ByteBlock {
	byte tiles[64];
};

// ByteLayer stores one byte of data per map tile in a cache-efficient way, assuming that when you access the data for one tile you'll soon need it
// for the surrounding tiles too.
class ByteLayer
{
public:
	ByteBlock * blocks;
	int map_width, map_height;
	int width_in_blocks, height_in_blocks;

	ByteLayer (int _map_width, int _map_height)
	{
		map_width = _map_width, map_height = _map_height;
		width_in_blocks  = map_width  / 16 + (map_width  % 16 != 0 ? 1 : 0);
		height_in_blocks = map_height / 8  + (map_height % 8  != 0 ? 1 : 0);
		blocks = (ByteBlock *)calloc (width_in_blocks * height_in_blocks, sizeof *blocks);
	}

	~ByteLayer ()
	{
		free (blocks);
	}

	std::tuple<int, int>
	compute_location (int x, int y)
	{
		// Coordinates of the block (x, y) is in and the "remainder" coords inside the block
		int block_x = x / 16, r_x = x % 16,
		    block_y = y / 8 , r_y = y % 16;

		int block_index = block_y * width_in_blocks + block_x,
		    tile_index = r_y * 8 + (r_y%2 == 0 ? r_x : r_x-1) / 2;

		return {block_index, tile_index};
	}

	byte
	get (int x, int y)
	{
		int block_index, tile_index;
		std::tie(block_index, tile_index) = compute_location (x, y);
		return blocks[block_index].tiles[tile_index];
	}

	void
	set (int x, int y, byte val)
	{
		int block_index, tile_index;
		std::tie(block_index, tile_index) = compute_location (x, y);
		blocks[block_index].tiles[tile_index] = val;
	}
};

EXPORT_PROC
void
set_exe_version (int index)
{
	if      (index == 0) bin_addrs = gog_addrs;
	else if (index == 1) bin_addrs = steam_addrs;
	else if (index == 2) bin_addrs = pcg_addrs;
}
