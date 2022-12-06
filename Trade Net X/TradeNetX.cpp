
#include <stdio.h>

#define NOVIRTUALKEYCODES // Keycodes defined in Civ3Conquests.h instead
#include "windows.h"

typedef unsigned char byte;

#include "Civ3Conquests.hpp"

#define EXPORT_PROC extern "C" __declspec(dllexport)

EXPORT_PROC
void
tnx_test (Tile * tile)
{
	printf ("testing... you probably won't see this anyway\n");
}

int
print_the_thing ()
{
	printf ("the thing\n");
	return 0;
}
