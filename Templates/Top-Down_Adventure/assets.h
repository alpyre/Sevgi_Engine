#ifndef ASSETS_H
#define ASSETS_H

#ifdef FONTS_H
STATIC struct TextAttr textAttrs[NUM_TEXTFONTS] = {{"helvetica.font", 9, FS_NORMAL, FPF_DESIGNED},
                                                   {"diamond.font", 12, FS_NORMAL, FPF_DESIGNED}};
STATIC STRPTR gameFontFiles[NUM_GAMEFONTS] = {"orbitron_15.fnt"};
#endif

#ifdef LEVEL_H
// LEVEL 0 (Main Menu)
STATIC STRPTR sprite_banks_0[]  = {"MousePointers1.spr", NULL};
STATIC STRPTR sound_samples_0[] = {"select-granted-05.iff", "select-granted-06.iff", NULL};
STATIC UBYTE* palettes_0[]      = {palette_0_0, NULL};
STATIC STRPTR gameobj_banks_0[] = {(STRPTR)4, NULL};
// LEVEL 1 (Level2)
STATIC STRPTR tilesets_1[]      = {"Pixel16.tls", NULL};
STATIC STRPTR tilemaps_1[]      = {"Pixel16/Tilemap.map", NULL};
STATIC STRPTR bob_sheets_1[]    = {"waterfall.sht", NULL};
STATIC STRPTR sprite_banks_1[]  = {"Character.spr", NULL};
STATIC STRPTR music_modules_1[] = {"walls-beautiful_world.mod", NULL};
STATIC UBYTE* palettes_1[]      = {Pixel16_Palette, NULL};
STATIC STRPTR gameobj_banks_1[] = {"Pixel16/Gameobjects.obj", NULL};

static struct LevelData levelData[NUM_LEVELS] = {{"Main Menu",
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  sprite_banks_0,
                                                  NULL,
                                                  sound_samples_0,
                                                  palettes_0,
                                                  gameobj_banks_0,
                                                  NULL,
                                                  MD_blitBOB,
                                                  MD_unBlitBOB,
                                                  MENU_SCREEN_WIDTH,
                                                  MENU_SCREEN_HEIGHT,
                                                  0,
                                                  0
                                                 },
                                                 {"Level2",
                                                  tilesets_1,
                                                  tilemaps_1,
                                                  bob_sheets_1,
                                                  sprite_banks_1,
                                                  music_modules_1,
                                                  NULL,
                                                  palettes_1,
                                                  gameobj_banks_1,
                                                  NULL,
                                                  LD_blitBOB,
                                                  LD_unBlitBOB,
                                                  SCREEN_WIDTH,
                                                  SCREEN_HEIGHT,
                                                  500,
                                                  500
                                                 }
                                                };
#endif

#endif /* ASSETS_H */
