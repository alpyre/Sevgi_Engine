#ifndef ASSETS_H
#define ASSETS_H

#ifdef FONTS_H
STATIC struct TextAttr textAttrs[NUM_TEXTFONTS] = {{"Helvetica.font", 9, FS_NORMAL, FPF_DESIGNED}};
STATIC STRPTR gameFontFiles[NUM_GAMEFONTS] = {"orbitron_15.fnt"};
#endif

#ifdef LEVEL_H
// LEVEL 0 (Main Menu)
STATIC STRPTR sprite_banks_0[]  = {"MousePointers1.spr", NULL};
STATIC STRPTR music_modules_0[] = {NULL};
STATIC STRPTR sound_samples_0[] = {"select-granted-05.iff", "select-granted-06.iff", NULL};
STATIC UBYTE* palettes_0[]      = {palette_0_0, NULL};
STATIC STRPTR gameobj_banks_0[] = {(STRPTR)4, NULL};

STATIC struct LevelData levelData[NUM_LEVELS] = {{"Main Menu",
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  sprite_banks_0,
                                                  music_modules_0,
                                                  sound_samples_0,
                                                  palettes_0,
                                                  gameobj_banks_0,
                                                  MD_blitBOB,
                                                  MD_unBlitBOB,
                                                  MENU_SCREEN_WIDTH,
                                                  MENU_SCREEN_HEIGHT,
                                                  0,
                                                  0
                                                 }
                                                };
#endif

#endif /* ASSETS_H */
