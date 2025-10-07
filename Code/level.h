#ifndef LEVEL_H
#define LEVEL_H

#include "settings.h"

//TO ACCESS CURRENT LEVEL ADD THIS LINE INTO YOUR .c FILE:
//extern struct Level current_level;

#include "system.h"
#include "diskio.h"
#include "tilemap.h"
#include "tiles.h"
#include "audio.h"
#include "color.h"
#include "gameobject.h"

struct LevelData {
  STRPTR name;
  STRPTR* tilesets;
  STRPTR* tilemaps;
  STRPTR* bob_sheets;
  STRPTR* sprite_banks;
  STRPTR* music_modules;
  STRPTR* sound_samples;
  UBYTE** palettes;
  STRPTR* gameobject_banks;
  STRPTR* bitmaps;
  VOID (*blitBOBFunc)(struct GameObject*);
  VOID (*unBlitBOBFunc)(struct GameObject*);
  UWORD screen_width;
  UWORD screen_height;
  LONG initial_mapPosX;
  LONG initial_mapPosY;
};

struct Level {
  struct TileSet** tileset;
  struct TileMap** tilemap;
  struct BOBSheet** bob_sheet;
  struct SpriteBank** sprite_bank;
  struct PT_Module** music_module;
  struct SfxStructure** sound_sample;
  struct ColorTable** color_table;
  struct BOBImage** bobImage;
  struct SpriteImage** spriteImage;
  struct GameObjectBank** gameobject_bank;
  struct BitMap** bitmap;
  LONG initial_mapPosX;
  LONG initial_mapPosY;
  struct {
    UBYTE tilesets;
    UBYTE tilemaps;
    UBYTE bob_sheets;
    UBYTE sprite_banks;
    UBYTE music_modules;
    UBYTE sound_samples;
    UBYTE color_tables;
    UBYTE gameobject_banks;
    UBYTE bitmaps;
    UWORD bobImages;
    UWORD spriteImages;
  }num;
  struct {
    UBYTE tileset;
    UBYTE tilemap;
    UBYTE music_module;
    UBYTE color_table;
    UWORD gameobject_bank;
  }current;
};

struct Level* loadLevel(ULONG num);
VOID unloadLevel(VOID);

#endif /* LEVEL_H */
