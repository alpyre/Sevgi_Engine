///includes
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include "tiles.h"
///

///newTileSet(size, friend)
struct TileSet* newTileSet(ULONG size, struct BitMap *friend)
{
  struct TileSet* tileset = (struct TileSet*)AllocMem(sizeof(struct TileSet), MEMF_ANY);

  if (tileset) {
    UBYTE* mem = AllocMem(sizeof(struct Tile) * size, MEMF_CHIP);
    if (mem) {
      tileset->size = size;
      tileset->tiles = (struct Tile*)mem;
    }
    else {
      FreeMem(tileset, sizeof(struct TileSet));
      tileset = NULL;
    }
  }

  return tileset;
}
///
///disposeTileSet(tileset)
VOID disposeTileSet(struct TileSet* tileset)
{
  if (tileset) {
    FreeMem((UBYTE*)tileset->tiles, sizeof(struct Tile) * tileset->size);
    FreeMem(tileset, sizeof(struct TileSet));
  }
}
///
