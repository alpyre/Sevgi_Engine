#ifndef ASSETS_H
#define ASSETS_H

#ifdef FONTS_H
STATIC struct TextAttr textAttrs[NUM_TEXTFONTS] = {{"Helvetica.font", 9, FS_NORMAL, FPF_DESIGNED}};
STATIC STRPTR gameFontFiles[NUM_GAMEFONTS] = {"orbitron_15.fnt"};
#endif

#ifdef LEVEL_H
// LEVEL 0 (Main Menu)
STATIC STRPTR sprite_banks_0[]  = {"MousePointers2.spr", NULL};
STATIC STRPTR sound_samples_0[] = {"select-granted-05.iff", "select-granted-06.iff", NULL};
STATIC UBYTE* palettes_0[]      = {palette_0_0, NULL};
STATIC STRPTR gameobj_banks_0[] = {(STRPTR)4, NULL};
// LEVEL 1 (Level1)
STATIC STRPTR tilesets_1[]      = {"FreeCuteTileset.tls", NULL};
STATIC STRPTR tilemaps_1[]      = {"FreeCuteTilemap/Tilemap.map", NULL};
STATIC STRPTR sprite_banks_1[]  = {"RedRidingHood.spr", NULL};
STATIC STRPTR music_modules_1[] = {"Background.mod", NULL};
STATIC STRPTR sound_samples_1[] = {"Left_Stomp.iff", "Right_Stomp.iff", "Jump.iff", NULL};
STATIC UBYTE* palettes_1[]      = {palette_1_0, NULL};
STATIC STRPTR gameobj_banks_1[] = {"FreeCuteTilemap/Gameobjects.obj", NULL};
STATIC STRPTR bitmaps_1[]       = {"[NN]Background.ilbm", NULL};

STATIC struct LevelData levelData[NUM_LEVELS] = {{"Main Menu",
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
                                                 {"Level1",
                                                  tilesets_1,
                                                  tilemaps_1,
                                                  NULL,
                                                  sprite_banks_1,
                                                  music_modules_1,
                                                  sound_samples_1,
                                                  palettes_1,
                                                  gameobj_banks_1,
                                                  bitmaps_1,
                                                  LD_blitBOB,
                                                  LD_unBlitBOB,
                                                  SCREEN_WIDTH,
                                                  SCREEN_HEIGHT,
                                                  0,
                                                  200
                                                 }
                                                };
#endif

#endif /* ASSETS_H */