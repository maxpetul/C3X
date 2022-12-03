
#include <stdio.h>

#define NOVIRTUALKEYCODES // Keycodes defined in Civ3Conquests.h instead
#include "windows.h"

typedef unsigned char byte;

#include "Civ3Conquests.hpp"

int
print_the_thing ()
{
	printf ("the thing\n");
	return 0;
}
