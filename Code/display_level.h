#ifndef LEVEL_DISPLAY_H
#define LEVEL_DISPLAY_H

#include "missing_hardware_defines.h"
#include "gameobject.h"
#include "anims.h"
#include "settings.h"

// Number of pixels to scroll the map in the corresponding direction.
// After a call to scroll(), will hold the remaining pixels to scroll...
// ...after updateCopperList()
struct ScrollInfo {
  UWORD up;
  UWORD down;
  UWORD left;
  UWORD right;
};

#define SCREEN_COLORS (1 << SCREEN_DEPTH)
#define BITMAP_WIDTH  (SCREEN_WIDTH  + SCR_WIDTH_EXTRA)
#define BITMAP_HEIGHT (((SCREEN_HEIGHT % TILESIZE) ? SCREEN_HEIGHT - (SCREEN_HEIGHT % TILESIZE) + TILESIZE : SCREEN_HEIGHT) + SCR_HEIGHT_EXTRA)

#define DIWSTART_V 0x2981
#define DIWSTOP_V  0x29C1

//Smart sprites only work with dynamic copperlist
#ifndef DYNAMIC_COPPERLIST
  #undef SMART_SPRITES
#endif

//Un-commenting the below define unrolls the bitplane loops in update copperlist functions
#define UNROLL_LOOPS

#if BPL_FMODE == 1

  #define DDFSTART_V 0x0030
  #define DDFSTOP_V  0x00D0
  #define BPLXMOD_V  ((BITMAP_WIDTH / 8) * SCREEN_DEPTH - (SCREEN_WIDTH / 8) - 2)
  #define VSPLTMOD_V ((-((BITMAP_HEIGHT - 1) * (BITMAP_WIDTH / 8) * SCREEN_DEPTH + (SCREEN_WIDTH / 8) + 2)) & 0xFFFF)
  #define DISPLAY_BUFFER_OFFSET       0
  #define DISPLAY_BUFFER_OFFSET_BYTES 0
  #define SCROLL_PIXELS 16
  #define SCROLL_MASK   15
  #define SCROLL_BYTES   2
  #define BPL_FMODE_V 0x0

#elif BPL_FMODE == 2

  #define DDFSTART_V 0x0028
  #define DDFSTOP_V  0x00C8
  #define BPLXMOD_V  ((BITMAP_WIDTH / 8) * SCREEN_DEPTH - (SCREEN_WIDTH / 8) - 4)
  #define VSPLTMOD_V ((-((BITMAP_HEIGHT - 1) * (BITMAP_WIDTH / 8) * SCREEN_DEPTH + (SCREEN_WIDTH / 8) + 4)) & 0xFFFF)
  #define DISPLAY_BUFFER_OFFSET       16
  #define DISPLAY_BUFFER_OFFSET_BYTES  2
  #define SCROLL_PIXELS 32
  #define SCROLL_MASK   31
  #define SCROLL_BYTES   4
  #define BPL_FMODE_V FMODE_BLP32

#elif BPL_FMODE == 4

  #define DDFSTART_V 0x0018
  #define DDFSTOP_V  0x00B8
  #define BPLXMOD_V  ((BITMAP_WIDTH * (SCREEN_DEPTH - 1)) / 8)
  #define VSPLTMOD_V ((-((BITMAP_WIDTH * (BITMAP_HEIGHT - 1) * SCREEN_DEPTH) + BITMAP_WIDTH) / 8) & 0xFFFF)
  #define DISPLAY_BUFFER_OFFSET       48  // 48 for (FMODE_BLP32 | FMODE_BPAGEM)
  #define DISPLAY_BUFFER_OFFSET_BYTES  6  // (DISPLAY_BUFFER_OFFSET / 8)
  #define SCROLL_PIXELS 64
  #define SCROLL_MASK   63
  #define SCROLL_BYTES   8
  #define BPL_FMODE_V FMODE_BLP32 | FMODE_BPAGEM

#endif

#if SPR_FMODE == 1
  #define FMODE_V (BPL_FMODE_V)
#elif SPR_FMODE == 2
  #define FMODE_V (BPL_FMODE_V | FMODE_SPR32)
#elif SPR_FMODE == 4
  #define FMODE_V (BPL_FMODE_V | FMODE_SPR32 | FMODE_SPAGEM)
#endif

VOID LD_blitBOB(struct GameObject* go);
VOID LD_unBlitBOB(struct GameObject* go);

VOID startLevelDisplay(ULONG level_num);

#endif /* LEVEL_DISPLAY_H */
