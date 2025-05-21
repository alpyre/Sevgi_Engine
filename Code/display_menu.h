#ifndef MENU_DISPLAY_H
#define MENU_DISPLAY_H

#define MENU_RV_ERROR   0
#define MENU_RV_START   1
#define MENU_RV_OPTIONS 2
#define MENU_RV_QUIT    3

#include "missing_hardware_defines.h"

/******************************************************************************
 * OPTIONS                                                                    *
 ******************************************************************************/
#define MENU_SCREEN_WIDTH  320
#define MENU_SCREEN_HEIGHT 256
#define MENU_SCREEN_DEPTH  2

#define MENU_SPR_FMODE 1   // Sprite fetch mode (1, 2 or 4)
/******************************************************************************
 * END OF OPTIONS                                                             *
 ******************************************************************************/

#define MENU_SCREEN_COLORS (1 << MENU_SCREEN_DEPTH)

#if MENU_SPR_FMODE == 1
  #define MENU_FMODE_V 0x0
#elif MENU_SPR_FMODE == 2
  #define MENU_FMODE_V (FMODE_SPR32)
#elif MENU_SPR_FMODE == 4
  #define MENU_FMODE_V (FMODE_SPR32 | FMODE_SPAGEM)
#endif

VOID MD_blitBOB(struct GameObject* go);
VOID MD_unBlitBOB(struct GameObject* go);
ULONG startMenuDisplay(VOID);

#endif /* MENU_DISPLAY_H */
