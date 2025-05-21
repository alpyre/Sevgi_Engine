#ifndef TILES_H
#define TILES_H

#include "settings.h"

#define TILESIZE  16 // Tiles are square so this stands for both TILEWIDTH and TILEHEIGHT
//NOTE: This below being define limits the possibility of different depth tilesets within a game.
#define TILEDEPTH  SCREEN_DEPTH // How many bitplanes do the tiles have

#if TILESIZE == 8
  #define TILESIZE_BSMD 3     // Bitshift multiplier/divider
  #define TILESIZE_MASK 0xFFFFFFF8L  // Trims a coordinate to tile boundary
  typedef UBYTE TILEROW_t;
#elif TILESIZE == 16
  #define TILESIZE_BSMD 4     // Bitshift multiplier/divider
  #define TILESIZE_MASK 0xFFFFFFF0L  // Trims a coordinate to tile boundary
  typedef UWORD TILEROW_t;
#elif TILESIZE == 32
  #define TILESIZE_BSMD 5     // Bitshift multiplier/divider
  #define TILESIZE_MASK 0xFFFFFFE0L  // Trims a coordinate to tile boundary
  typedef ULONG TILEROW_t;
#else
  #error Incorrect tilesize!
#endif

/* Non-Interleaved tile structure
struct Tile {
  struct {
    TILEROW_t row[TILESIZE];        // Every plane has TILESIZE rows of 16 bits (pixels) each
  } plane[TILEDEPTH];               // Every tile has TILEDEPTH planes
};*/

// Interleaved tile structure
struct Tile {
  struct {
    TILEROW_t planeline[TILEDEPTH]; // Every row has TILEDEPTH planelines of 16 bits (pixels) each
  } row[TILESIZE];                  // There are TILESIZE pixel rows in a tile
};

struct TileSet {
  ULONG size;
  struct Tile* tiles;               // pointer to tile raster data
};

struct TileSet* newTileSet(ULONG size, struct BitMap *friend);
VOID disposeTileSet(struct TileSet* tileset);

#endif /* TILES_H */
