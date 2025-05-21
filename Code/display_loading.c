///includes
#define ECS_SPECIFIC

#include <stdio.h>
#include <string.h>

#include <exec/exec.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/display.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <libraries/mathffp.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <clib/mathffp_protos.h>
#include <clib/mathtrans_protos.h>

#include "system.h"
#include "color.h"
#include "cop_inst_macros.h"
#include "input.h"
#include "diskio.h"
#include "fonts.h"

#include "display.h"
#include "display_loading.h"
///
///defines (private)
#define LOADING_SCREEN_WIDTH  320
#define LOADING_SCREEN_HEIGHT 256
#define LOADING_SCREEN_DEPTH  2
#define LOADING_SCREEN_COLORS (1 << LOADING_SCREEN_DEPTH)

#define LOADING_BITMAP_WIDTH  LOADING_SCREEN_WIDTH
#define LOADING_BITMAP_HEIGHT 64
#define LOADING_BITMAP_DEPTH  LOADING_SCREEN_DEPTH

#define LOADING_SCREEN_START  ((LOADING_SCREEN_HEIGHT - LOADING_BITMAP_HEIGHT) / 2)
#define LOADING_SCREEN_END    (LOADING_SCREEN_START + LOADING_BITMAP_HEIGHT - 1)

#define COPPERLIST_INSTRUCTIONS 38
#define COPPERLIST_SIZE (COPPERLIST_INSTRUCTIONS * 4)     // A copper instruction is 4 bytes
#define COPPERLIST_ALLOC_SIZE COPPERLIST_SIZE

#define DDFSTART_V 0x0038
#define DDFSTOP_V  0x00D0
#define DIWSTART_V 0x2C81
#define DIWSTOP_V  0x2CC1

#define BPLXMOD_V1 ((-LOADING_BITMAP_WIDTH / 8) & 0xFFFF)
#define BPLXMOD_V2 (0 & 0xFFFF)

#define BPLCON0_V ((LOADING_SCREEN_DEPTH * BPLCON0_BPU0) | BPLCON0_COLOR | BPLCON0_ECSENA)

#define CT_FAST_STEPS 20

#define BLACK 0
#define DARK  1
#define LIGHT 2
#define WHITE 3

#define LOADING_TEXT_X 20
#define LOADING_TEXT_Y  5
#define LOADING_TEXT "LOADING..."

#define GAUGE_PEN     3
#define GAUGE_BAR_PEN 3
#define GAUGE_X1 20
#define GAUGE_Y1 22
#define GAUGE_X2 300
#define GAUGE_Y2 32
#define GAUGE_BAR_X1 (GAUGE_X1 + 2)
#define GAUGE_BAR_Y1 (GAUGE_Y1 + 2)
#define GAUGE_BAR_X2 (GAUGE_X2 - 2)
#define GAUGE_BAR_Y2 (GAUGE_Y2 - 2)
#define GAUGE_BAR_MAX (GAUGE_BAR_X2 - GAUGE_BAR_X1)
#define RESET  TRUE
#define UPDATE FALSE

#define MAXVECTORS 8
///
///globals
// imported globals
extern struct Custom custom;
extern volatile LONG new_frame_flag;
extern UWORD NULL_SPRITE_ADDRESS_H;
extern UWORD NULL_SPRITE_ADDRESS_L;
extern struct TextFont* textFonts[NUM_TEXTFONTS];
extern struct GameFont* gameFonts[NUM_GAMEFONTS];

// exported globals
volatile ULONG loading_gauge_total = 1;   // number of totalupdateColorTable assets to be loaded
volatile ULONG loading_gauge_current;     // assets loaded so far

// private globals
static struct BitMap* bitmap = NULL; // BitMap for the loading display
static struct RastPort* rastPort = NULL;
static WORD* CopperList  = (WORD*) 0;

static ULONG copperList_Instructions[COPPERLIST_INSTRUCTIONS] = {
                                              // Access Ptr:  Action:
  MOVE(COLOR00, 0),                           //              Set color 0 to black
  MOVE(FMODE,   0),                           //              Set Sprite/Bitplane Fetch Modes
  MOVE(BPLCON0, BPLCON0_V),                   //              Set a lowres display
  MOVE(BPLCON1, 0),                           // CL_BPLCON1   Set h_scroll register
  MOVE(BPLCON2, 0x264),
  MOVE(BPL1MOD, BPLXMOD_V1),                  //              Set bitplane mods to show same raster line
  MOVE(BPL2MOD, BPLXMOD_V1),                  //               "     "       "
  MOVE(DIWSTRT, DIWSTART_V),                  //              Set Display Window Start
  MOVE(DIWSTOP, DIWSTOP_V),                   //              Set Display Window Stop
  MOVE(DDFSTRT, DDFSTART_V),                  //              Set Data Fetch Start to fetch early
  MOVE(DDFSTOP, DDFSTOP_V),                   //              Set Data Fetch Stop
  MOVE(BPL1PTH, 0),                           // CL_BPL1PTH   Set bitplane addresses
  MOVE(BPL1PTL, 0),                           // CL_BPL1PTL    "      "       "
  MOVE(BPL2PTH, 0),                           // CL_BPL2PTH    "      "       "
  MOVE(BPL2PTL, 0),                           // CL_BPL2PTL    "      "       "
  MOVE(SPR0PTH, 0),                           // CL_SPR0PTH   Set sprite pointers
  MOVE(SPR0PTL, 0),                           // CL_SPT0PTL    "     "      "
  MOVE(SPR1PTH, 0),                           // CL_SPT1PTH    "     "      "
  MOVE(SPR1PTL, 0),                           // CL_SPT1PTL    "     "      "
  MOVE(SPR2PTH, 0),                           // CL_SPT2PTH    "     "      "
  MOVE(SPR2PTL, 0),                           // CL_SPT2PTL    "     "      "
  MOVE(SPR3PTH, 0),                           // CL_SPT3PTH    "     "      "
  MOVE(SPR3PTL, 0),                           // CL_SPT3PTL    "     "      "
  MOVE(SPR4PTH, 0),                           // CL_SPT4PTH    "     "      "
  MOVE(SPR4PTL, 0),                           // CL_SPT4PTL    "     "      "
  MOVE(SPR5PTH, 0),                           // CL_SPT5PTH    "     "      "
  MOVE(SPR5PTL, 0),                           // CL_SPT5PTL    "     "      "
  MOVE(SPR6PTH, 0),                           // CL_SPT6PTH    "     "      "
  MOVE(SPR6PTL, 0),                           // CL_SPT6PTL    "     "      "
  MOVE(SPR7PTH, 0),                           // CL_SPT7PTH    "     "      "
  MOVE(SPR7PTL, 0),                           // CL_SPT7PTL    "     "      "
  WAIT(0, ((DIWSTART_V >> 8) + LOADING_SCREEN_START)),
  MOVE(BPL1MOD, BPLXMOD_V2),                  //              Set bitplane mods to regular
  MOVE(BPL2MOD, BPLXMOD_V2),                  //               "     "       "
  WAIT(0, ((DIWSTART_V >> 8) + LOADING_SCREEN_END)),
  MOVE(BPL1MOD, BPLXMOD_V1),                  //              Set bitplane mods to show same raster line
  MOVE(BPL2MOD, BPLXMOD_V1),                  //               "     "       "
  END
};
///
///colors
static struct ColorTable* color_table = NULL;
static UBYTE colors[] = {(UBYTE)(LOADING_SCREEN_COLORS - 1),
  0,   0,   0,
121, 121, 121,
180, 180, 180,
255, 255, 255};
///
///prototypes (private)
STATIC VOID vblankEvents(VOID);
STATIC struct RastPort* openScreen(VOID);
STATIC VOID closeScreen(VOID);
STATIC WORD* createCopperList(VOID);
STATIC VOID disposeCopperList(VOID);
STATIC VOID switchToLoadingCopperList(VOID);
STATIC VOID drawBox(UWORD x1, UWORD y1, UWORD x2, UWORD y2);
STATIC VOID updateLoadingGauge(BOOL reset);
STATIC VOID drawLoadingGauge(VOID);
STATIC VOID drawLoadingScreen(VOID);
///

///vblankEvents()
/******************************************************************************
 * We can't have a display loop in loading screen so we do every graphics     *
 * update in the vertical blanking interrupt!                                 *
 ******************************************************************************/
STATIC VOID vblankEvents()
{
  updateColorTable(color_table);
  setColorTable(color_table);
  updateLoadingGauge(UPDATE);
}
///

///openScreen()
/******************************************************************************
 * Albeit the name, this one does not open a screen. It just allocates a      *
 * bitmap and sets up a RastPort for it. A bitmap and a copperlist is enough  *
 * for all our displays (except the null_display of course).                  *
 ******************************************************************************/
STATIC struct RastPort* openScreen()
{
  rastPort = allocRastPort(LOADING_BITMAP_WIDTH, LOADING_BITMAP_HEIGHT, LOADING_BITMAP_DEPTH,
                          BMF_STANDARD | BMF_DISPLAYABLE | BMF_CLEAR, 0,
                          RPF_BITMAP | RPF_TMPRAS | RPF_AREA, MAXVECTORS);

  if (rastPort) bitmap = rastPort->BitMap;

  return rastPort;
}
///
///closeScreen()
STATIC VOID closeScreen()
{
  freeRastPort(rastPort, RPF_ALL);
}
///

///createCopperList()
STATIC WORD* createCopperList()
{
  WORD* buffer = (WORD*)AllocMem(COPPERLIST_ALLOC_SIZE, MEMF_CHIP);
  if (buffer) {
    UWORD* CL_BPL1PTH;
    UWORD* CL_SPR0PTH;
    UWORD* wp;
    UWORD* sp;
    ULONG i;

    CopperList = buffer;
    CopyMem(copperList_Instructions, CopperList, COPPERLIST_SIZE);

    //Set bitmap to point to the Loading_bitmap
    CL_BPL1PTH = CopperList + (11 * 2) + 1;

    for (wp = CL_BPL1PTH, i = 0; i < LOADING_SCREEN_DEPTH; i++) {
      *wp = (WORD)((ULONG)bitmap->Planes[i] >> 16); wp += 2;
      *wp = (WORD)((ULONG)bitmap->Planes[i] & 0xFFFF); wp += 2;
    }

    //Set all sprite pointers to null_sprite
    CL_SPR0PTH = CopperList + (15 * 2) + 1;

    for (sp = CL_SPR0PTH; sp < CL_SPR0PTH + 32; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }
  }
  else puts("Couldn't allocate Loading CopperList!");

  return buffer;
}
///
///disposeCopperList()
STATIC VOID disposeCopperList()
{
  if (CopperList) {
    FreeMem(CopperList, COPPERLIST_ALLOC_SIZE);
    CopperList = NULL;
  }
}
///

///openLoadingDisplay()
BOOL openLoadingDisplay()
{
  if ((color_table = newColorTable(colors, CT_FAST_STEPS, 0))) {
    color_table->state = CT_FADE_IN;
    if (openScreen()) {
      if (createCopperList()) {
        drawLoadingScreen();
        return TRUE;
      }
      else closeLoadingDisplay();
    }
    else closeLoadingDisplay();
  }

  return FALSE;
}
///
///closeLoadingDisplay()
VOID closeLoadingDisplay()
{
  disposeCopperList();
  closeScreen();
  if (color_table) {
    freeColorTable(color_table); color_table = NULL;
  }
}
///

///switchToLoadingCopperList()
STATIC VOID switchToLoadingCopperList()
{
  custom.cop2lc = (ULONG)CopperList;
  setVBlankEvents(vblankEvents);
  new_frame_flag = 1;
  waitTOF();
}
///

///updateLoadingGauge(reset)
STATIC VOID drawBox(UWORD x1, UWORD y1, UWORD x2, UWORD y2)
{
  AreaMove(rastPort, x1, y1);
  AreaDraw(rastPort, x2, y1);
  AreaDraw(rastPort, x2, y2);
  AreaDraw(rastPort, x1, y2);
  AreaEnd(rastPort);
}

STATIC VOID updateLoadingGauge(BOOL reset)
{
  static ULONG gauge_bar_old_X2 = GAUGE_BAR_X1;

  if (reset) {
    loading_gauge_current = 0;
    gauge_bar_old_X2 = GAUGE_BAR_X1;
    SetAPen(rastPort, BLACK);
    drawBox(GAUGE_BAR_X1, GAUGE_BAR_Y1, GAUGE_BAR_X2, GAUGE_BAR_Y2);
  }

  if (loading_gauge_total) {
    ULONG gauge_bar_width = (loading_gauge_current * GAUGE_BAR_MAX) / loading_gauge_total;
    ULONG gauge_bar_X2 = GAUGE_BAR_X1 + gauge_bar_width;

    if (gauge_bar_X2 != gauge_bar_old_X2) {
      SetAPen(rastPort, GAUGE_BAR_PEN);
      drawBox(gauge_bar_old_X2, GAUGE_BAR_Y1, gauge_bar_X2, GAUGE_BAR_Y2);

      gauge_bar_old_X2 = gauge_bar_X2;
    }
  }
}
///
///drawLoadingGauge()
STATIC VOID drawLoadingGauge()
{
  // draw frame
  Move(rastPort, GAUGE_X1, GAUGE_Y1);
  Draw(rastPort, GAUGE_X2, GAUGE_Y1);
  Draw(rastPort, GAUGE_X2, GAUGE_Y2);
  Draw(rastPort, GAUGE_X1, GAUGE_Y2);
  Draw(rastPort, GAUGE_X1, GAUGE_Y1);

  // reset gauge bar
  updateLoadingGauge(RESET);
}
///
///drawLoadingScreen()
STATIC VOID drawLoadingScreen()
{
  // Write loading text
  Move(rastPort, LOADING_TEXT_X, LOADING_TEXT_Y);
  GF_Text(rastPort, gameFonts[GF_DEFAULT], LOADING_TEXT, strlen(LOADING_TEXT));
  // Draw the loading gauge
  drawLoadingGauge();
}
///

///startLoadingDisplay()
VOID startLoadingDisplay()
{
  // reset gauge bar
  updateLoadingGauge(RESET);
  color_table->state = CT_FADE_IN;
  switchToLoadingCopperList();
}
///
///endLoadingDisplay()
VOID endLoadingDisplay()
{
  // Ensure displaying at least one frame of empty gauge
  new_frame_flag = 1;
  waitTOF();
  // Ensure displaying at least one frame of full gauge
  new_frame_flag = 1;
  waitTOF();
  // Wait until fade out completes
  color_table->state = CT_FADE_OUT;
  while (color_table->state == CT_FADE_OUT) {
    new_frame_flag = 1;
    waitTOF();
  }
  switchToNullCopperList();
}
///
