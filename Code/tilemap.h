#ifndef TILEMAP_H
#define TILEMAP_H

#include "SDI_headers/SDI_compiler.h"
#include "tiles.h"

typedef UWORD TILEID; // Every tile ID is a 16 bit integer (65535 possible ID's)

// Formula below calculates the tile ID at a given GameObject's map coordinate
// To understand the +1's have look at loadTileMap() function
#define TILEID_AT_COORD(m, x, y) (m->data[((((y) >> TILESIZE_BSMD) + 1) * m->width) + ((x) >> TILESIZE_BSMD) + 1])

/******************************************************************************
 * Number of pixels to scroll the map in the corresponding direction.         *
 * NOTE: Two copies of this struct is used. One kept on the tilemap (see      *
 * below), and one in the level_display_loop. The one on the tilemap struct   *
 * stores the direction and amount of scroll that happened in the current     *
 * frame (valid after the scroll() function call). The other one which is     *
 * declared locally in the level_display_loop is used to initiate the scroll  *
 * routines (scroll() and scrollRemaining() functions) in display_level.c.    *
 ******************************************************************************/
struct ScrollInfo {
  UBYTE up;
  UBYTE down;
  UBYTE left;
  UBYTE right;
};

struct TileMap {
  ULONG width;                // Width of the tilemap (in tiles)
  ULONG height;               // Height of the tilemap (in tiles)
  ULONG size;                 // How many tiles (width*height)
  LONG mapPosX;               // The actual stored values for each map's...
  LONG mapPosY;               // ...position displayed by the level_display...
  LONG mapPosX2;              // ...which will update these values by its...
  LONG mapPosY2;              // ...global pointers with the same name.
  LONG maxMapPosX;            // The maximum values for map positions which...
  LONG maxMapPosY;            // ...will restrict over scrolling.
  struct ScrollInfo si;       // The direction and amount of the last scroll
  TILEID data[FLEXARR];       // An array of UWORDS that holds tile id's
};

struct TileMap *newTileMap(ULONG width, ULONG height, UWORD scr_width, UWORD scr_height);
VOID disposeTileMap(struct TileMap *map);

#endif /* TILEMAP_H */
