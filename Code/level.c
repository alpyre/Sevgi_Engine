///defines
#define MAX_GAMEOBJECT_COUNT  (STRPTR)1024  //NOTE: Rename this!
///
///includes
#include <exec/exec.h>
#include <graphics/gfx.h>

#include <proto/exec.h>
#include <proto/graphics.h>

#include <stdio.h>

#include "display_level.h"
#include "display_menu.h"
#include "level.h"
///
///palettes
#include "palettes.h"
///
// globals
// imported globals
extern volatile ULONG loading_gauge_total;       // from loading_display.o
extern volatile ULONG loading_gauge_current;     // from loading_display.o

extern struct BitMap* BOBsBackBuffer;            // from gameobject.o

// exported globals
struct Level current_level = {0}; //current_level is a singleton...
                                  //..it gets updated with every loadLevel()

// private globals
#include "assets.h"

///loadLevel(num)
ULONG ptrArrLength(VOID** arr)
{
  ULONG i = 0;
  if (arr) while (*arr++) i++;
  return i;
}

struct Level* loadLevel(ULONG num)
{
  struct Level* level = &current_level;
  ULONG i = 0;
  ULONG assets = 0;
  UWORD max_bob_width = 0;
  UWORD max_bob_height = 0;
  UWORD max_bob_depth = 0;
  UWORD max_sprite_height = 0;
  UWORD max_image_height = 0;
  UWORD max_gameobject_height = 0;

  //Unload any previously loaded level
  unloadLevel();

  //Count the number of assets to be loaded for this level
  assets += level->num.tilesets         = ptrArrLength((VOID**) levelData[num].tilesets);
  assets += level->num.tilemaps         = ptrArrLength((VOID**) levelData[num].tilemaps);
  assets += level->num.bob_sheets       = ptrArrLength((VOID**) levelData[num].bob_sheets);
  assets += level->num.sprite_banks     = ptrArrLength((VOID**) levelData[num].sprite_banks);
  assets += level->num.music_modules    = ptrArrLength((VOID**) levelData[num].music_modules);
  assets += level->num.sound_samples    = ptrArrLength((VOID**) levelData[num].sound_samples);
            level->num.color_tables     = ptrArrLength((VOID**) levelData[num].palettes);
  assets += level->num.gameobject_banks = ptrArrLength((VOID**) levelData[num].gameobject_banks);
  assets += level->num.bitmaps          = ptrArrLength((VOID**) levelData[num].bitmaps);

  loading_gauge_total = assets;
  loading_gauge_current = 0;

  //load the tilesets of this level
  if (level->num.tilesets) {
    level->tileset = (struct TileSet**)AllocMem(sizeof(struct TileSet*) * level->num.tilesets, MEMF_ANY);
    if (level->tileset) {
      for (i = 0; i < level->num.tilesets; i++) {
        level->tileset[i] = loadTileSet(levelData[num].tilesets[i], NULL);
        if (!level->tileset[i]) {
          printf("Tileset %s could not be loaded!\n", levelData[num].tilesets[i]);
          goto fail;
        }
        loading_gauge_current++;
      }
    }
  }

  //load the tilemaps of this level
  if (level->num.tilemaps) {
    level->tilemap = (struct TileMap**)AllocMem(sizeof(struct TileMap*) * level->num.tilemaps, MEMF_ANY);
    if (level->tilemap) {
      for (i = 0; i < level->num.tilemaps; i++) {
        level->tilemap[i] = loadTileMap(levelData[num].tilemaps[i], levelData[num].screen_width, levelData[num].screen_height);
        if (!level->tilemap[i]) {
          printf("Tilemap %s could not be loaded!\n", levelData[num].tilemaps[i]);
          goto fail;
        }
        loading_gauge_current++;
      }
    }
  }
  else {
    // Every loaded level has to have at least a dummy TileMap to provide mapPos values
    level->num.tilemaps = 1;
    level->tilemap = (struct TileMap**)AllocMem(sizeof(struct TileMap*) * level->num.tilemaps, MEMF_ANY);
    if (level->tilemap) {
      level->tilemap[0] = newTileMap(0, 0, levelData[num].screen_width, levelData[num].screen_height);
      if (!level->tilemap[0]) {
        printf("Not enough memory for dummy tilemap!\n");
        goto fail;
      }
    }
  }

  //load the bob_sheets of this level
  if (level->num.bob_sheets) {
    level->bob_sheet = (struct BOBSheet**)AllocMem(sizeof(struct BOBSheet*) * level->num.bob_sheets, MEMF_ANY);
    if (level->bob_sheet) {
      for (i = 0; i < level->num.bob_sheets; i++) {
        level->bob_sheet[i] = loadBOBSheet(levelData[num].bob_sheets[i]);
        if (!level->bob_sheet[i]) {
          printf("Bob sheet %s could not be loaded!\n", levelData[num].bob_sheets[i]);
          goto fail;
        }
        loading_gauge_current++;
      }
    }
    for (i = 0; i < level->num.bob_sheets; i++) {
      ULONG l;
      for (l = 0; l < level->bob_sheet[i]->num_images; l++) {
        if (max_bob_width  < level->bob_sheet[i]->image[l].width)  max_bob_width  = level->bob_sheet[i]->image[l].width;
        if (max_bob_height < level->bob_sheet[i]->image[l].height) max_bob_height = level->bob_sheet[i]->image[l].height;
        if (max_bob_depth  < level->bob_sheet[i]->bitmap->Depth)   max_bob_depth  = level->bob_sheet[i]->bitmap->Depth;
      }
    }

    if (NUM_BOBS && level->num.bob_sheets) {
      BOBsBackBuffer = allocBOBBackgroundBuffer(max_bob_width, max_bob_height, max_bob_depth);
      if (!BOBsBackBuffer) {
        printf("Not enough memory for BOBsBackBuffer\n");
        goto fail;
      }
    }
    else BOBsBackBuffer = NULL;
  }

  //load the sprite_banks of this level
  if (level->num.sprite_banks) {
    level->sprite_bank = (struct SpriteBank**)AllocMem(sizeof(struct SpriteBank*) * level->num.sprite_banks, MEMF_ANY);
    if (level->sprite_bank) {
      for (i = 0; i < level->num.sprite_banks; i++) {
        level->sprite_bank[i] = loadSpriteBank(levelData[num].sprite_banks[i]);
        if (!level->sprite_bank[i]) {
          printf("Spritebank %s could not be loaded!\n", levelData[num].sprite_banks[i]);
          goto fail;
        }
        loading_gauge_current++;
      }
    }
    for (i = 0; i < level->num.sprite_banks; i++) {
      ULONG l;
      for (l = 0; l < level->sprite_bank[i]->num_images; l++) {
        if (max_sprite_height < level->sprite_bank[i]->image[l].height) max_sprite_height = level->sprite_bank[i]->image[l].height;
      }
    }
  }

  //load the music_modules of this level
  if (level->num.music_modules) {
    level->music_module = (struct PT_Module**)AllocMem(sizeof(struct PT_Module*) * level->num.music_modules, MEMF_ANY);
    if (level->music_module) {
      for (i = 0; i < level->num.music_modules; i++) {
        level->music_module[i] = PT_LoadModule(levelData[num].music_modules[i]);
        if (!level->music_module[i]) {
          printf("Music module %s could not be loaded!\n", levelData[num].music_modules[i]);
          goto fail;
        }
        loading_gauge_current++;
      }
    }
  }

  //load the sound_samples of this level
  if (level->num.sound_samples) {
    level->sound_sample = (struct SfxStructure**)AllocMem(sizeof(struct SfxStructure*) * level->num.sound_samples, MEMF_ANY);
    if (level->sound_sample) {
      for (i = 0; i < level->num.sound_samples; i++) {
        level->sound_sample[i] = PT_Load8SVX(levelData[num].sound_samples[i]);
        if (!level->sound_sample[i]) {
          printf("Sound sample %s could not be loaded!\n", levelData[num].sound_samples[i]);
          goto fail;
        }
        loading_gauge_current++;
      }
    }
  }

  //create the color_tables of this level
  if (level->num.color_tables) {
    level->color_table = (struct ColorTable**) AllocMem(sizeof(struct ColorTable*) * level->num.color_tables, MEMF_ANY);
    if (level->color_table) {
      for (i = 0; i < level->num.color_tables; i++) {
        level->color_table[i] = newColorTable(levelData[num].palettes[i], CT_DEFAULT_STEPS, 0);
        if (!level->color_table[i]) {
          printf("Color table %lu of level %lu could not be created!\n", i, num);
          goto fail;
        }
      }
    }
  }

  //initialize GameObjects for this level
  if (level->num.gameobject_banks) {
    level->gameobject_bank = (struct GameObjectBank**) AllocMem(sizeof(struct GameObjectBank*) * level->num.gameobject_banks, MEMF_ANY);
    if (level->gameobject_bank) {
      for (i = 0; i < level->num.gameobject_banks; i++) {
        if (levelData[num].gameobject_banks[i] < MAX_GAMEOBJECT_COUNT) {
          level->gameobject_bank[i] = newGameObjectBank((ULONG)levelData[num].gameobject_banks[i]);
        }
        else {
          ULONG l;
          level->gameobject_bank[i] = loadGameObjectBank(levelData[num].gameobject_banks[i]);
          //Get max_gameobject_height from gameobjects with set height
          if (level->gameobject_bank[i]) {
            for (l = 0; l < level->gameobject_bank[i]->num_gameobjects; l++) {
              ULONG height = level->gameobject_bank[i]->gameobjects[l].y2 - level->gameobject_bank[i]->gameobjects[l].y1;
              max_gameobject_height = max_gameobject_height > height ? max_gameobject_height : height;
            }
          }
        }
        if (!level->gameobject_bank[i]) goto fail;
        loading_gauge_current++;
      }
    }
  }

  //load the bitmaps for this level
  if (level->num.bitmaps) {
    level->bitmap = (struct BitMap**)AllocMem(sizeof(struct BitMap*) * level->num.bitmaps, MEMF_ANY);
    if (level->bitmap) {
      for (i = 0; i < level->num.bitmaps; i++) {
        ULONG flags = 0;
        STRPTR bitmap_string = levelData[num].bitmaps[i];
        if (bitmap_string[1] == 'D' || bitmap_string[1] == 'd' || bitmap_string[1] == 'C' || bitmap_string[1] == 'c') flags = BM_TYPE_DISPLAYABLE;
        if (bitmap_string[2] == 'I' || bitmap_string[2] == 'i') flags |= BM_TYPE_INTERLEAVED;
        bitmap_string += 4; // skip the [DI] part

        level->bitmap[i] = loadILBMBitMap(bitmap_string, flags);
        if (!level->bitmap[i]) {
          printf("Bitmap %s could not be loaded!\n", bitmap_string);
          goto fail;
        }
        loading_gauge_current++;
      }
    }
  }

  //calculate max_gameobject_height
  max_image_height = max_sprite_height > max_bob_height ? max_sprite_height : max_bob_height;
  max_gameobject_height = max_gameobject_height > max_image_height ? max_gameobject_height : max_image_height;

  initGameObjects(levelData[num].blitBOBFunc, levelData[num].unBlitBOBFunc, max_bob_width, max_gameobject_height);

  level->initial_mapPosX = levelData[num].initial_mapPosX;
  level->initial_mapPosY = levelData[num].initial_mapPosY;

  return level;

  fail:
  unloadLevel();
  return NULL;
}
///
///unloadLevel()
VOID unloadLevel()
{
  LONG i;
  struct Level* level = &current_level;

  level->initial_mapPosX = 0;
  level->initial_mapPosY = 0;

  //free all bitmaps loaded for this level
  if (level->bitmap) {
    for (i = level->num.bitmaps - 1; i >= 0; i--) {
      struct BitMap* bm = level->bitmap[i];
      if (bm) FreeBitMap(bm);
    }
    FreeMem(level->bitmap, sizeof(struct BitMap*) * level->num.bitmaps);
    level->bitmap = NULL;
    level->num.bitmaps = 0;
  }

  //free all GameObjectBanks in this level
  if (level->gameobject_bank) {
    for (i = level->num.gameobject_banks - 1; i >= 0; i--) {
      struct GameObjectBank* gob = level->gameobject_bank[i];
      freeGameObjectBank(gob);
    }
    FreeMem(level->gameobject_bank, sizeof(struct GameObjectBank*) * level->num.gameobject_banks);
    level->gameobject_bank = NULL;
    level->num.gameobject_banks = 0;
    level->current.gameobject_bank = 0;
  }

  //free all color_tables in this level
  if (level->color_table) {
    for (i = level->num.color_tables - 1; i >= 0; i--) {
      struct ColorTable* ct = level->color_table[i];
      if (ct) freeColorTable(ct);
    }
    FreeMem(level->color_table, sizeof(struct ColorTable*) * level->num.color_tables);
    level->color_table = NULL;
    level->num.color_tables = 0;
    level->current.color_table = 0;
  }

  //free all sound_samples in this level
  if (level->sound_sample) {
    for (i = level->num.sound_samples - 1; i >= 0; i--) {
      struct SfxStructure* ss = level->sound_sample[i];
      if (ss) PT_FreeSFX(ss);
    }
    FreeMem(level->sound_sample, sizeof(struct SfxStructure*) * level->num.sound_samples);
    level->sound_sample = NULL;
    level->num.sound_samples = 0;
  }

  //free all music_modules in this level
  if (level->music_module) {
    for (i = level->num.music_modules - 1; i >= 0; i--) {
      struct PT_Module* mm = level->music_module[i];
      if (mm) PT_FreeModule(mm);
    }
    FreeMem(level->music_module, sizeof(struct PT_Module*) * level->num.music_modules);
    level->music_module = NULL;
    level->num.music_modules = 0;
    level->current.music_module = 0;
  }

  //free all sprite_banks in this level
  if (level->sprite_bank) {
    for (i = level->num.sprite_banks - 1; i >= 0; i--) {
      struct SpriteBank* sb = level->sprite_bank[i];
      if (sb) freeSpriteBank(sb);
    }
    FreeMem(level->sprite_bank, sizeof(struct SpriteBank*) * level->num.sprite_banks);
    level->sprite_bank = NULL;
    level->num.sprite_banks = 0;
  }

  //free all bob_sheets in this level
  if (level->bob_sheet) {
    for (i = level->num.bob_sheets - 1; i >= 0; i--) {
      struct BOBSheet* bs = level->bob_sheet[i];
      if (bs) freeBOBSheet(bs);
    }
    FreeMem(level->bob_sheet, sizeof(struct BOBSheet*) * level->num.bob_sheets);
    level->bob_sheet = NULL;
    level->num.bob_sheets = 0;
  }

  if (BOBsBackBuffer) {
    FreeBitMap(BOBsBackBuffer);
    BOBsBackBuffer = NULL;
  }

  //free all tilemaps in this level
  if (level->tilemap) {
    for (i = level->num.tilemaps - 1; i >= 0; i--) {
      struct TileMap* tm = level->tilemap[i];
      if (tm) disposeTileMap(tm);
    }
    FreeMem(level->tilemap, sizeof(struct TileMap*) * level->num.tilemaps);
    level->tilemap = NULL;
    level->num.tilemaps = 0;
    level->current.tilemap = 0;
  }

  //free all tilesets in this level
  if (level->tileset) {
    for (i = level->num.tilesets - 1; i >= 0; i--) {
      struct TileSet* ts = level->tileset[i];
      if (ts) disposeTileSet(ts);
    }
    FreeMem(level->tileset, sizeof(struct TileSet*) * level->num.tilesets);
    level->tileset = NULL;
    level->num.tilesets = 0;
    level->current.tileset = 0;
  }
}
///
