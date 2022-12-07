
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
		    Leader_has_tech (&leaders[tech_req_civ_id], __, p_bic_data->WorkerJobs[WJ_Build_Road].RequireID))
			has_road = tile->Overlays & 1;
		else
			has_road = false;
	}

	return has_road &&
		((civ_id == 0) ||
		 (tile->Territory_OwnerID == 0) ||
		 ! leaders[civ_id].At_War[tile->Territory_OwnerID]);
}

EXPORT_PROC
void
set_exe_version (int index)
{
	if      (index == 0) bin_addrs = gog_addrs;
	else if (index == 1) bin_addrs = steam_addrs;
	else if (index == 2) bin_addrs = pcg_addrs;
}
