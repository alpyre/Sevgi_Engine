///includes
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include "tiles.h"
#include "display_level.h"
#include "tilemap.h"
///

///newTileMap(width, height)
struct TileMap *newTileMap(ULONG width, ULONG height, UWORD scr_width, UWORD scr_height)
{
  ULONG size = width * height;

  struct TileMap *map = AllocMem(sizeof(struct TileMap) + size * sizeof(TILEID), MEMF_ANY);
  if (map)
  {
    ULONG actualMapWidthInPixels = 0;
    ULONG actualMapHeightInPixels = 0;

    if (size) {
      actualMapWidthInPixels = (width - 1) * TILESIZE;
      actualMapHeightInPixels = (height - 1) * TILESIZE;
    }

    map->width  = width;
    map->height = height;
    map->size   = size;
    map->mapPosX = 0;
    map->mapPosY = 0;
    map->mapPosX2 = scr_width - 1;
    map->mapPosY2 = scr_height - 1;
    map->maxMapPosX = actualMapWidthInPixels > scr_width ? actualMapWidthInPixels - scr_width : 0;
    map->maxMapPosY = actualMapHeightInPixels > scr_height ? actualMapHeightInPixels - scr_height : 0;
  }

  return map;
}
///
///disposeTileMap(map)
VOID disposeTileMap(struct TileMap *map)
{
  if (map) {
    FreeMem(map, sizeof(struct TileMap) + map->size * sizeof(TILEID));
  }
}
///
