#ifndef TILEMAP_H
#define TILEMAP_H

#include "tiles.h"

typedef UWORD TILEID; // Every tile ID is a 16 bit integer (65535 possible ID's)

// Formula below calculates the tile ID at a given GameObject's map coordinate
// To understand the +1's have look at loadTileMap() function
#define TILEID_AT_COORD(m, x, y) (m->data[((((y) >> TILESIZE_BSMD) + 1) * m->width) + ((x) >> TILESIZE_BSMD) + 1])

struct TileMap {
  ULONG width;
  ULONG height;
  ULONG size;                       // how many tiles (width*heigth)
  LONG mapPosX;                     // The actual stored values for each map's...
  LONG mapPosY;                     // ...position displayed by the level_display...
  LONG mapPosX2;                    // ...which will update these values by its...
  LONG mapPosY2;                    // ...global pointers with the same name.
  LONG maxMapPosX;                  // The maximum values for map positions which...
  LONG maxMapPosY;                  // ...will restrict over scrolling.
  //struct TileSet *tileset;        // tileset for this map //NOTE: Not yet implemented
  //struct GameObject *gameobjects; //NOTE: Not yet implemented
  TILEID data[0];                   // an array of UWORDS that holds tile id's
};

struct TileMap *newTileMap(ULONG width, ULONG height, UWORD scr_width, UWORD scr_height);
VOID disposeTileMap(struct TileMap *map);

#endif /* TILEMAP_H */
