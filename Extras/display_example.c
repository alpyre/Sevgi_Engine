/******************************************************************************
 * Strings to replace:                                                        *
 * [NAME], [Name], [name], [palette]                                          *
 ******************************************************************************/

///includes
#define ECS_SPECIFIC

#include <stdio.h>
#include <string.h>

#include <exec/exec.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/display.h>
#include <graphics/scale.h>
#include <hardware/custom.h>
#include <hardware/cia.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <libraries/mathffp.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "system.h"
#include "color.h"
#include "cop_inst_macros.h"
#include "input.h"
#include "keyboard.h"
#include "diskio.h"
#include "fonts.h"
#include "palettes.h"

#include "display.h"
#include "display_[name].h"
///
///defines (private)
#define [NAME]_SCREEN_WIDTH  320
#define [NAME]_SCREEN_HEIGHT 256
#define [NAME]_SCREEN_DEPTH  3
#define [NAME]_SCREEN_COLORS (1 << [NAME]_SCREEN_DEPTH)

#define [NAME]_BITMAP_WIDTH  [NAME]_SCREEN_WIDTH
#define [NAME]_BITMAP_HEIGHT [NAME]_SCREEN_HEIGHT
#define [NAME]_BITMAP_DEPTH  [NAME]_SCREEN_DEPTH

#define DDFSTRT_V DEFAULT_DDFSTRT_LORES
#define DDFSTOP_V DEFAULT_DDFSTOP_LORES
#define DIWSTRT_V DEFAULT_DIWSTRT
#define DIWSTOP_V DEFAULT_DIWSTOP

//NON INTERLEAVED / NON INTERLACED
#define BPLXMOD_V (0 & 0xFFFF)
//NON INTERLEAVED / INTERLACED
#define BPLXMOD_V (([NAME]_BITMAP_WIDTH / 8) & 0xFFFF)
//INTERLEAVED / NON INTERLACED
#define BPLXMOD_V (([NAME]_BITMAP_WIDTH / 8 * ([NAME]_BITMAP_DEPTH - 1)) & 0xFFFF)
//INTERLEAVED / INTERLACED
#define BPLXMOD_V (([NAME]_BITMAP_WIDTH / 8 * ([NAME]_BITMAP_DEPTH * 2 - 1)) & 0xFFFF)

#define BPLCON0_V (([NAME]_SCREEN_DEPTH == 8 ? BPLCON0_BPU3 : [NAME]_SCREEN_DEPTH * BPLCON0_BPU0) | BPLCON0_COLOR | USE_BPLCON3)
#define BPLCON0_V (([NAME]_SCREEN_DEPTH * BPLCON0_BPU0) | BPLCON0_COLOR)
#define BPLCON3_V  BPLCON3_BRDNBLNK

#define MAXVECTORS 8
///
///globals
// imported globals
extern struct Custom custom;
extern struct CIA ciaa, ciab;
extern volatile LONG new_frame_flag;
extern UWORD NULL_SPRITE_ADDRESS_H;
extern UWORD NULL_SPRITE_ADDRESS_L;
extern struct TextFont* textFonts[NUM_TEXTFONTS];
extern struct GameFont* gameFonts[NUM_GAMEFONTS];

// private globals
STATIC struct RastPort* rastPort = NULL;
STATIC struct BitMap* bitmap = NULL;
///
///copperlist
STATIC UWORD* CopperList = (UWORD*) 0;

STATIC UWORD* CL_BPL1PTH = (UWORD*) 0;
STATIC UWORD* CL_SPR0PTH = (UWORD*) 0;

STATIC ULONG copperList_Instructions[] = {
                                              // Access Ptr:  Action:
  MOVE(COLOR00, 0),                           //              Set color 0 to black
  MOVE(FMODE,   0),                           //              Set Sprite/Bitplane Fetch Modes
  MOVE(BPLCON0, BPLCON0_V),                   //              Set a ECS Lowres display
  MOVE(BPLCON1, 0),                           //              Set h_scroll register
  MOVE(BPLCON2, 0x264),                       //              Set sprite priorities
  MOVE(BPL1MOD, BPLXMOD_V),                   //              Set bitplane mods
  MOVE(BPL2MOD, BPLXMOD_V),                   //               "     "       "
  MOVE(DIWSTRT, DIWSTRT_V),                   //              Set Display Window Start
  MOVE(DIWSTOP, DIWSTOP_V),                   //              Set Display Window Stop
  MOVE(DDFSTRT, DDFSTRT_V),                   //              Set Data Fetch Start to fetch early
  MOVE(DDFSTOP, DDFSTOP_V),                   //              Set Data Fetch Stop
  MOVE_PH(BPL1PTH, 0),                        // CL_BPL1PTH   Set bitplane addresses
  MOVE(BPL1PTL, 0),                           // CL_BPL1PTL    "      "       "
  MOVE(BPL2PTH, 0),                           // CL_BPL2PTH    "      "       "
  MOVE(BPL2PTL, 0),                           // CL_BPL2PTL    "      "       "
  MOVE(BPL3PTH, 0),                           // CL_BPL3PTH    "      "       "
  MOVE(BPL3PTL, 0),                           // CL_BPL3PTL    "      "       "
  MOVE_PH(SPR0PTH, 0),                        // CL_SPR0PTH   Set sprite pointers
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
  END
};
///
///colors
STATIC struct ColorTable* color_table = NULL;
///
///protos (private)
STATIC VOID closeScreen(VOID);
STATIC VOID closeDisplay(VOID);
///

///vblankEvents()
STATIC VOID vblankEvents(VOID)
{
  setColorTable(color_table);
}
///

///openScreen()
/******************************************************************************
 * Albeit the name, this one does not open a screen. It just allocates a      *
 * bitmap and sets up a RastPort for it. A bitmap and a copperlist is enough  *
 * for all our displays (except the null_display of course).                  *
 ******************************************************************************/
STATIC struct RastPort* openScreen(VOID)
{
  rastPort = allocRastPort([NAME]_BITMAP_WIDTH, [NAME]_BITMAP_HEIGHT, [NAME]_BITMAP_DEPTH,
                           BMF_STANDARD | BMF_DISPLAYABLE | BMF_CLEAR, 0,
                           RPF_BITMAP | RPF_TMPRAS | RPF_AREA, MAXVECTORS);

  if (rastPort) {
    bitmap = rastPort->BitMap;
  }

  return rastPort;
}
///
///closeScreen()
STATIC VOID closeScreen(VOID)
{
  freeRastPort(rastPort, RPF_ALL);
  rastPort = NULL;
  bitmap = NULL;
}
///
///drawScreen()
STATIC VOID drawScreen(VOID)
{
  //DRAW THE INITIAL SCREEN FEATURES HERE!

}
///

///createCopperList()
STATIC UWORD* createCopperList(VOID)
{
  if (allocCopperList(copperList_Instructions, CopperList, CL_SINGLE)) {
    UWORD* wp;
    UWORD* sp;
    ULONG i;

    //Set copperlist bitmap instruction point to screen bitmap
    for (wp = CL_BPL1PTH, i = 0; i < [NAME]_SCREEN_DEPTH; i++) {
      *wp = (WORD)((ULONG)bitmap->Planes[i] >> 16); wp += 2;
      *wp = (WORD)((ULONG)bitmap->Planes[i] & 0xFFFF); wp += 2;
    }

    //Set all sprite pointers to null_sprite
    for (sp = CL_SPR0PTH; sp < CL_SPR0PTH + 32; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }
  }
  else
    puts("Couldn't allocate [Name] CopperList!");

  return CopperList;
}
///
///disposeCopperList()
STATIC VOID disposeCopperList(VOID)
{
  if (CopperList) {
    freeCopperList(CopperList);
    CopperList = NULL;
  }
}
///

///createCopperList()
STATIC UWORD* createCopperList()
{
  if (allocCopperList(copperList_Instructions, CopperList, CL_DOUBLE)) {
    UWORD* wp;
    UWORD* sp;
    ULONG i;

    //Set copperlist bitmap instructions point to screen bitmap
    for (wp = CL_BPL1PTH_1, i = 0; i < [NAME]_SCREEN_DEPTH; i++) {
      *wp = (WORD)((ULONG)bitmap->Planes[i] >> 16); wp += 2;
      *wp = (WORD)((ULONG)bitmap->Planes[i] & 0xFFFF); wp += 2;
    }

    for (wp = CL_BPL1PTH_2, i = 0; i < [NAME]_SCREEN_DEPTH; i++) {
      *wp = (WORD)(((ULONG)bitmap->Planes[i] + [NAME]_BITMAP_WIDTH / 8) >> 16); wp += 2;
      *wp = (WORD)(((ULONG)bitmap->Planes[i] + [NAME]_BITMAP_WIDTH / 8) & 0xFFFF); wp += 2;
    }

    //Set all sprite pointers to null_sprite
    for (sp = CL_SPR0PTH_1; sp < CL_SPR0PTH_1 + 32; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }

    for (sp = CL_SPR0PTH_2; sp < CL_SPR0PTH_2 + 32; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }

    //Set both copperlists to point to each other (interlaced copper)
    *CL_COP2LCH_1 = (WORD)((ULONG)CopperList2 >> 16);
    *CL_COP2LCL_1 = (WORD)((ULONG)CopperList2 & 0xFFFF);
    *CL_COP2LCH_2 = (WORD)((ULONG)CopperList1 >> 16);
    *CL_COP2LCL_2 = (WORD)((ULONG)CopperList1 & 0xFFFF);
  }
  else
    puts("Couldn't allocate [Name] CopperList!");

  return CopperList;
}
///
///disposeCopperList()
STATIC VOID disposeCopperList()
{
  if (CopperList) {
    freeCopperList(CopperList1);
    CopperList = NULL; CopperList1 = NULL; CopperList2 = NULL;
  }
}
///

///openDisplay()
STATIC BOOL openDisplay(VOID)
{
  if ((color_table = newColorTable([palette], CT_DEFAULT_STEPS, 0))) {
    color_table->state = CT_FADE_IN;
    if (openScreen()) {
      if (createCopperList()) {
        if (createChipData()) return TRUE;
        else closeDisplay();
      }
      else closeDisplay();
    }
    else closeDisplay();
  }

  return FALSE;
}
///
///closeDisplay()
STATIC VOID closeDisplay(VOID)
{
  disposeCopperList();
  closeScreen();
  if (color_table) {
    freeColorTable(color_table); color_table = NULL;
  }
}
///

///displayLoop()
STATIC VOID displayLoop(VOID)
{
  struct MouseState ms;
  UWORD exiting = FALSE;

  while(TRUE) {
    new_frame_flag = 1;
    doKeyboardIO();

    if (keyState(RAW_ESC) && !exiting) {
      exiting = TRUE;
      color_table->state = CT_FADE_OUT;
      volume_table.state = PTVT_FADE_OUT;
    }
    if (exiting == TRUE && color_table->state == CT_IDLE) {
      break;
    }

    updateColorTable(color_table);
    waitTOF();
  }
}
///

///switchTo[Name]CopperList()
STATIC VOID switchTo[Name]CopperList(VOID)
{
  custom.cop2lc = (ULONG)CopperList;
  setVBlankEvents(vblankEvents);
  new_frame_flag = 1;
  waitTOF();
}
///
///show[Name]Display()
VOID show[Name]Display(VOID)
{
  if(openDisplay()) {
    //DRAW INITIAL SCREEN HERE!
    drawScreen();
    switchTo[Name]CopperList();
    displayLoop();
    switchToNullCopperList();
    closeDisplay();
  }
}
///

///switchTo[Name]CopperList()
STATIC VOID switchTo[Name]CopperList(VOID)
{
  WaitVBeam(299);                 // We have to wait Copper to set cop2lc on an interlaced display!
  custom.dmacon = DMAF_COPPER;    // Disable copperlist DMA so it does not reset our lace bit below!
  custom.bplcon0 = BPLCON0_LACE;  // Set LACE bit
  waitTOF();                      // Wait until you get a vertical blank
  new_frame_flag = 1;

  if (custom.vposr & VPOS_LOF) {  // Check if this frame is a long frame or a short frame!
    CopperList = CopperList1;
    custom.cop2lc = (ULONG)CopperList1;
  }
  else {
    CopperList = CopperList2;
    custom.cop2lc = (ULONG)CopperList2;
  }

  custom.dmacon = DMAF_SETCLR | DMAF_COPPER; // Reactivate copperlist DMA!

  setVBlankEvents(vblankEvents);
  waitTOF();
}
///
///show[Name]Display()
VOID show[Name]Display(VOID)
{
  if(openDisplay()) {
    drawScreen();
    switchTo[Name]CopperList();
    displayLoop();
    switchToNullCopperList();
    closeDisplay();
  }
}
///
