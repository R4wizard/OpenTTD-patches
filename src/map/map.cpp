/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/map.cpp Basic map data. */

#include "../stdafx.h"
#include "../debug.h"
#include "../core/alloc_func.hpp"
#include "../core/bitmath_func.hpp"
#include "../core/math_func.hpp"
#include "map.h"

MapSizeParams map_size;

TileZH *_mth = NULL; ///< Tile zones and heights
Tile   *_mc  = NULL; ///< Tile contents


/**
 * (Re)allocates a map with the given dimension
 * @param size_x the width of the map along the NE/SW edge
 * @param size_y the 'height' of the map along the SE/NW edge
 */
void AllocateMap(uint size_x, uint size_y)
{
	/* Make sure that the map size is within the limits and that
	 * size of both axes is a power of 2. */
	if (!IsInsideMM(size_x, MIN_MAP_SIZE, MAX_MAP_SIZE + 1) ||
			!IsInsideMM(size_y, MIN_MAP_SIZE, MAX_MAP_SIZE + 1) ||
			(size_x & (size_x - 1)) != 0 ||
			(size_y & (size_y - 1)) != 0) {
		error("Invalid map size");
	}

	DEBUG(map, 1, "Allocating map of size %dx%d", size_x, size_y);

	map_size.log_x = FindFirstBit(size_x);
	map_size.log_y = FindFirstBit(size_y);
	map_size.size_x = size_x;
	map_size.size_y = size_y;
	map_size.size = size_x * size_y;

	map_size.diffs[DIR_N ] = -size_x - 1;
	map_size.diffs[DIR_NE] =         - 1;
	map_size.diffs[DIR_E ] =  size_x - 1;
	map_size.diffs[DIR_SE] =  size_x;
	map_size.diffs[DIR_S ] =  size_x + 1;
	map_size.diffs[DIR_SW] =           1;
	map_size.diffs[DIR_W ] = -size_x + 1;
	map_size.diffs[DIR_NW] = -size_x;

	free(_mth);
	free(_mc);

	_mth = CallocT<TileZH>(map_size.size);
	_mc = CallocT<Tile>(map_size.size);
}