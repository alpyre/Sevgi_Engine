#ifndef SETTINGS_H
#define SETTINGS_H

/******************************************************************************
 * VIDEO                                                                      *
 ******************************************************************************/

//Un-comment if you wrote a custom display_level.c which is not compatible with Sevgi_Editor
//#define CUSTOM_LEVEL_DISPLAY

#ifdef CUSTOM_LEVEL_DISPLAY
#include "custom_display_settings.h"
#else
//Un-comment to activate 24bit AGA color features.
//#define CT_AGA
//Set the default fade in/out speed. Smaller is faster. (2-256)
#define CT_DEFAULT_STEPS 64

//Un-comment to activate per-frame dynamic copperlist generation.
//#define DYNAMIC_COPPERLIST
//Un-comment to activate smart sprite algorithms.
//#define SMART_SPRITES
//Un-comment to activate dual-playfield mode.
//#define DUALPLAYFIELD
//Select bitplane and sprite fetch modes
#define BPL_FMODE 1   // Bitplane fetch mode (1, 2 or 4)
#define SPR_FMODE 1   // Sprite fetch mode (1, 2 or 4)

#define TOP_PANEL_HEIGHT 0
#define BOTTOM_PANEL_HEIGHT 0

#define SCREEN_WIDTH    320  // Visible resolution of the level screen
#define SCREEN_HEIGHT   256  // "       "          "  "   "     "
#define SCR_WIDTH_EXTRA  64  // Additional horizontal pixels for scroll
#define SCR_HEIGHT_EXTRA 32  // Additional vertical pixels for scroll (2 extra tiles)
#define SCREEN_DEPTH      6  // How many bitplanes (1-6 on OCS/ECS, 1-8 on AGA)

//Set these to valid values in DUALPLAYFIELD mode
#define BITMAP_DEPTH_PF2 5
#define BITMAP_HEIGHT_PF2 256

//Activate double buffering on display_level.c and gameobjects.c
//#define DOUBLE_BUFFER
//#define FRAME_SKIP 0

//Do color register updates on copperlist
#define USE_CLP 5

#endif //CUSTOM_LEVEL_DISPLAY

/******************************************************************************
 * AUDIO                                                                      *
 ******************************************************************************/

#define PAULA_PAL_CYCLES  3546895
#define PAULA_NTSC_CYCLES 3579545
#define PAULA_MIN_PERIOD  124

//Set PAL/NTSC
#define PAULA_CYCLES PAULA_PAL_CYCLES
//Set the default module playback volume. (0-64)
#define PTVT_DEFAULT_VOLUME 64
//Set the default audio fade in/out speed. Smaller is faster. (1-256)
#define PTVT_DEFAULT_STEPS  64
//Default playback volume for sound samples (0-64)
#define SFX_DEFAULT_VOLUME  64
//Default playback channel for sound samples (0-3 / -1 for best avail. channel)
#define SFX_DEFAULT_CHANNEL -1
//Default playback priority for sound samples (1-127)
#define SFX_DEFAULT_PRIORITY 1

/******************************************************************************
 * FONTS                                                                      *
 ******************************************************************************/

#define NUM_TEXTFONTS  2  // Number of Amiga TextFonts to load
#define TF_DEFAULT 0      // Index of the first loaded TextFont

#define NUM_GAMEFONTS  1  // Number of GameFonts to load
#define GF_DEFAULT 0      // Index of the first loaded GameFont

/******************************************************************************
 * GAMEOBJECTS                                                                *
 ******************************************************************************/

#define NUM_LEVELS 2 // Number of levels in the game

#define NUM_SPRITES 8
#define NUM_BOBS    10
#define NUM_GAMEOBJECTS 0

#define SMALL_IMAGE_SIZES
//#define BIG_IMAGE_SIZES
//#define SMALL_HITBOX_SIZES

#endif /* SETTINGS_H */
