///includes
#define ECS_SPECIFIC

#include <stdio.h>
#include <string.h>

#include <exec/exec.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/display.h>
#include <hardware/custom.h>
#include <hardware/cia.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "system.h"
#include "color.h"
#include "cop_inst_macros.h"
#include "input.h"
#include "keyboard.h"
#include "audio.h"
#include "diskio.h"
#include "fonts.h"
#include "level.h"
#include "gameobject.h"
#include "ui.h"

#include "display.h"
#include "display_menu.h"
///
///defines (private)
#define MENU_BITMAP_WIDTH  MENU_SCREEN_WIDTH
#define MENU_BITMAP_HEIGHT MENU_SCREEN_HEIGHT
#define MENU_BITMAP_DEPTH  MENU_SCREEN_DEPTH

#define DDFSTART_V 0x0038
#define DDFSTOP_V  0x00D0
#define DIWSTART_V 0x2C81
#define DIWSTOP_V  0x2CC1

#define BPLXMOD_V (0 & 0xFFFF)

#define BPLCON0_V ((MENU_SCREEN_DEPTH * BPLCON0_BPU0) | BPLCON0_COLOR | BPLCON0_ECSENA)

#define MAXVECTORS 16

#define BLACK 0
#define DARK  1
#define LIGHT 2
#define WHITE 3

#define LOGO_START_X 64
#define LOGO_START_Y 50

#define NUM_BUTTONS   3
#define BUTTON_STATES 2

#define SCREEN_TITLE_TEXT "MAIN MENU"
//#define SCREEN_TITLE_X  Will be calculated from TextExtent.te_Width of text and centered
#define SCREEN_TITLE_Y    50

#define BUTTONS_GROUP_WIDTH  160
#define BUTTONS_START_Y      128
#define BUTTONS_SEPARATION_Y   5
#define INPUT_DELAY           20

#define BUTTON_SELECT_SOUND      0
#define BUTTON_ACKNOWLEDGE_SOUND 1
///
///globals
// imported globals
extern struct Custom custom;
extern struct CIA ciaa, ciab;
extern volatile LONG new_frame_flag;                   // from system.o
extern UWORD NULL_SPRITE_ADDRESS_H;                    // from display.o
extern UWORD NULL_SPRITE_ADDRESS_L;                    // from display.o
extern struct TextFont* textFonts[NUM_TEXTFONTS];      // from fonts.o
extern struct GameFont* gameFonts[NUM_GAMEFONTS];      // from fonts.o
extern struct PT_VolumeTable volume_table;             // from audio.o
extern struct Level current_level;                     // from level.o
extern struct BitMap* BOBsBackBuffer;                  // from gameobject.o
extern struct GameObject* spriteList[NUM_SPRITES + 1]; // from gameobject.o
extern struct GameObject* bobList[NUM_BOBS + 1];       // from gameobject.o

// private globals
STATIC struct GameObject* gameobjects;
STATIC struct GameObject mouse_pointer;
STATIC struct BitMap* screen_bitmap = NULL; // BitMap for the menu display
STATIC struct RastPort* rastPort;
STATIC struct ColorTable* color_table = NULL;

enum {
  BTN_NONE,
  BTN_START,
  BTN_OPTIONS,
  BTN_QUIT
};
///
///copperlist
STATIC UWORD* CopperList  = (UWORD*) 0;
STATIC UWORD* CL_BPL1PTH  = (UWORD*) 0;
STATIC UWORD* CL_SPR0PTH  = (UWORD*) 0;

STATIC ULONG copperList_Instructions[] = {
                                              // Access Ptr:  Action:
  MOVE(FMODE,   MENU_FMODE_V),                //              Set Sprite/Bitplane Fetch Modes
  MOVE(BPLCON0, BPLCON0_V),                   //              Set a lowres display
  MOVE(BPLCON1, 0),                           //              Set h_scroll register
  MOVE(BPLCON2, 0x264),
  MOVE(BPLCON3, BPLCON3_V),
  MOVE(BPLCON4, 0x10),
  MOVE(BPL1MOD, BPLXMOD_V),                   //              Set bitplane mods to show same raster line
  MOVE(BPL2MOD, BPLXMOD_V),                   //               "     "       "
  MOVE(DIWSTRT, DIWSTART_V),                  //              Set Display Window Start
  MOVE(DIWSTOP, DIWSTOP_V),                   //              Set Display Window Stop
  MOVE(DDFSTRT, DDFSTART_V),                  //              Set Data Fetch Start to fetch early
  MOVE(DDFSTOP, DDFSTOP_V),                   //              Set Data Fetch Stop
  MOVE_PH(BPL1PTH, 0),                        // CL_BPL1PTH   Set bitplane addresses
  MOVE(BPL1PTL, 0),                           //               "      "       "
  MOVE(BPL2PTH, 0),                           //               "      "       "
  MOVE(BPL2PTL, 0),                           //               "      "       "
  MOVE_PH(SPR0PTH, 0),                        // CL_SPR0PTH   Set sprite pointers
  MOVE(SPR0PTL, 0),                           //               "     "      "
  MOVE(SPR1PTH, 0),                           //               "     "      "
  MOVE(SPR1PTL, 0),                           //               "     "      "
  MOVE(SPR2PTH, 0),                           //               "     "      "
  MOVE(SPR2PTL, 0),                           //               "     "      "
  MOVE(SPR3PTH, 0),                           //               "     "      "
  MOVE(SPR3PTL, 0),                           //               "     "      "
  MOVE(SPR4PTH, 0),                           //               "     "      "
  MOVE(SPR4PTL, 0),                           //               "     "      "
  MOVE(SPR5PTH, 0),                           //               "     "      "
  MOVE(SPR5PTL, 0),                           //               "     "      "
  MOVE(SPR6PTH, 0),                           //               "     "      "
  MOVE(SPR6PTL, 0),                           //               "     "      "
  MOVE(SPR7PTH, 0),                           //               "     "      "
  MOVE(SPR7PTL, 0),                           //               "     "      "
  END
};
///
///protos (private)
STATIC VOID vblankEvents(VOID);
STATIC BOOL openDisplay(VOID);
STATIC VOID closeDisplay(VOID);
STATIC struct RastPort* openScreen(VOID);
STATIC VOID closeScreen(VOID);
STATIC UWORD* createCopperList(VOID);
STATIC VOID disposeCopperList(VOID);
STATIC VOID switchToMenuCopperList(VOID);
STATIC VOID prepMousePointer(VOID);
STATIC VOID drawScreen(VOID);
STATIC INLINE VOID MD_setSprite(struct GameObject* go);
STATIC ULONG menuDisplayLoop(VOID);

VOID onClickMenuButton(struct UIObject* self);
///

///UI_OBJECTS
UI_BUTTON(button_START,   "START",   UIOF_INHERIT_X | UIOF_INHERIT_Y | UIOF_INHERIT_WIDTH, onClickMenuButton);
UI_BUTTON(button_OPTIONS, "OPTIONS", UIOF_INHERIT_X | UIOF_INHERIT_Y | UIOF_INHERIT_WIDTH, onClickMenuButton);
UI_BUTTON(button_QUIT,    "QUIT",    UIOF_INHERIT_X | UIOF_INHERIT_Y | UIOF_INHERIT_WIDTH, onClickMenuButton);

STATIC struct UIObject* root_children[] = {&button_START, &button_OPTIONS, &button_QUIT, NULL};
UI_SIZED_GROUP(group_root, "Root", CENTER(BUTTONS_GROUP_WIDTH, MENU_SCREEN_WIDTH), BUTTONS_START_Y, BUTTONS_GROUP_WIDTH, 0, UIOF_NONE, root_children, BUTTONS_SEPARATION_Y, 1, 0, NULL, NULL);

STATIC ULONG clicked_button = BTN_NONE;
VOID onClickMenuButton(struct UIObject* self)
{
  clicked_button = self->id;
}

VOID onHoverMenuButton(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  if (hovered) {
    if (self->flags & UIOF_HOVERED) {
      // Already hovered do nothing
    }
    else {
      self->flags |= UIOF_HOVERED;
      PT_PlaySFX(current_level.sound_sample[BUTTON_SELECT_SOUND]);
      if (self->draw) self->draw(self);
    }
  }
  else {
    if (self->flags & UIOF_HOVERED) {
      self->flags &= ~UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
}
///

///vblankEvents()
/******************************************************************************
 *
 ******************************************************************************/
STATIC VOID vblankEvents()
{
  MD_setSprite(&mouse_pointer);
  setColorTable(color_table);
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
  rastPort = allocRastPort(MENU_BITMAP_WIDTH, MENU_BITMAP_HEIGHT, MENU_BITMAP_DEPTH,
                           BMF_STANDARD | BMF_DISPLAYABLE | BMF_CLEAR, 0,
                           RPF_BITMAP | RPF_LAYER | RPF_TMPRAS | RPF_AREA, MAXVECTORS);

  if (rastPort) {
    screen_bitmap = rastPort->BitMap;
  }

  return rastPort;
}
///
///closeScreen()
STATIC VOID closeScreen()
{
  freeRastPort(rastPort, RPF_ALL);
  rastPort = NULL;
  screen_bitmap = NULL;
}
///

///createCopperList()
STATIC UWORD* createCopperList()
{
  if (allocCopperList(copperList_Instructions, CopperList, CL_SINGLE)) {
    UWORD* wp;
    UWORD* sp;
    ULONG i;

    //Set bitplane pointers on the copperlist
    for (wp = CL_BPL1PTH, i = 0; i < MENU_SCREEN_DEPTH; i++) {
      *wp = (WORD)((ULONG)screen_bitmap->Planes[i] >> 16); wp += 2;
      *wp = (WORD)((ULONG)screen_bitmap->Planes[i] & 0xFFFF); wp += 2;
    }

    for (sp = CL_SPR0PTH; sp < CL_SPR0PTH + 32; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }
  }
  else {
    puts("Couldn't allocate Menu CopperList!");
    return NULL;
  }

  return CopperList;
}
///
///disposeCopperList()
STATIC VOID disposeCopperList()
{
  if (CopperList) {
    freeCopperList(CopperList);
    CopperList = NULL;
  }
}
///

///openDisplay()
STATIC BOOL openDisplay()
{
  if (openScreen()) {
    if (createCopperList()) {
      if (loadLevel(0)) {
        color_table = current_level.color_table[0];
        gameobjects = current_level.gameobject_bank[0]->gameobjects;

        prepMousePointer();
        drawScreen();
        return TRUE;
      }
    }
  }

  closeDisplay();
  return FALSE;
}
///
///closeDisplay()
STATIC VOID closeDisplay()
{
  unloadLevel();
  disposeCopperList();
  closeScreen();
}
///

///switchToMenuCopperList()
STATIC VOID switchToMenuCopperList()
{
  custom.cop2lc = (ULONG)CopperList;
  new_frame_flag = 1;
  waitTOF();
  custom.dmacon = 0x8020; // Enable Sprite DMA
  WaitVBeam((DIWSTART_V >> 8));
  setVBlankEvents(vblankEvents);
}
///

///startMenuDisplay()
ULONG startMenuDisplay()
{
  ULONG retVal = 0;

  if (openDisplay()) {
    switchToMenuCopperList();
    retVal = menuDisplayLoop();
    switchToNullCopperList();
    //PT_StopAudio();
  }

  closeDisplay();
  return retVal;
}
///

///prepMousePointer
STATIC VOID prepMousePointer()
{
  struct SpriteImage* mouse_image = &current_level.sprite_bank[0]->image[0];

  //prep the mouse pointer
  mouse_pointer.x = 0; //NOTE: Maybe we can memorize mouse positions globally?
  mouse_pointer.y = 0;
  mouse_pointer.x1 = mouse_pointer.x + mouse_image->h_offs;
  mouse_pointer.y1 = mouse_pointer.y + mouse_image->v_offs;
  mouse_pointer.x2 = mouse_pointer.x1 + mouse_image->width;
  mouse_pointer.y2 = mouse_pointer.y1 + mouse_image->height;
  mouse_pointer.type = SPRITE_OBJECT;
  mouse_pointer.state = 0;
  mouse_pointer.me_mask = 0x00;
  mouse_pointer.hit_mask = 0x01;
  mouse_pointer.image = (struct ImageCommon*)mouse_image;
  mouse_pointer.medium = NULL;
  mouse_pointer.priority = 0;

  //Set mouse pointer colors directly to hardware registers:
  //NOTE: This should be better on a palette so colours would fade
  setColor(17, 224,   4,  64);
  setColor(18,   0,   0,   0);
  setColor(19, 224, 224, 192);
}
///
///drawScreen()
STATIC VOID drawScreen()
{
  //Draw Menu Screen Title
  rastPort->cp_x = (MENU_SCREEN_WIDTH - GF_TextLength(gameFonts[GF_DEFAULT], SCREEN_TITLE_TEXT, strlen(SCREEN_TITLE_TEXT))) / 2;
  rastPort->cp_y = SCREEN_TITLE_Y;
  GF_Text(rastPort, gameFonts[GF_DEFAULT], SCREEN_TITLE_TEXT, strlen(SCREEN_TITLE_TEXT));
}
///

///menuDisplayLoop()
STATIC ULONG menuDisplayLoop()
{
  BOOL exiting = FALSE;
  ULONG retVal = 0;
  LONG inputDelay = 0;
  clicked_button = NULL;

  color_table->state = CT_FADE_IN;

  button_START.id = BTN_START;
  button_OPTIONS.id = BTN_OPTIONS;
  button_QUIT.id = BTN_QUIT;

  button_START.onHover = onHoverMenuButton;
  button_OPTIONS.onHover = onHoverMenuButton;
  button_QUIT.onHover = onHoverMenuButton;

  SetFont(rastPort, textFonts[1]);
  initUI(DIWSTART_V >> 8, rastPort, &group_root, root_children);

  while (TRUE) {
    struct MouseState ms;
    new_frame_flag = 1;

    updateColorTable(color_table);
    updateVolume();

    doKeyboardIO();
    UL_VALUE(ms) = readMouse(0);

    if (UL_VALUE(ms)) {
      moveGameObjectClamped(&mouse_pointer, ms.deltaX, ms.deltaY, 0, 0, MENU_SCREEN_WIDTH - 2, MENU_SCREEN_HEIGHT - 2);
    }

    if (exiting == TRUE && color_table->state == CT_IDLE && volume_table.state == PTVT_IDLE) {
      spriteList[0] = NULL;
      bobList[0] = NULL;
      break;
    }


    if (!exiting) {
      // Check if a button was clicked by the ui
      if (clicked_button) {
        retVal = clicked_button;
        PT_PlaySFX(current_level.sound_sample[BUTTON_ACKNOWLEDGE_SOUND]);
        goto init_exit;
      }

      // Handle key presses
      if (keyState(RAW_ESC) && !exiting) {
        retVal  = MENU_RV_QUIT;
        goto init_exit;
      };

      if (keyState(RAW_UP) || JOY_UP(1)) {
        if (!inputDelay) {
          inputDelay = INPUT_DELAY;
          PT_PlaySFX(current_level.sound_sample[BUTTON_SELECT_SOUND]);
          prevObject();
        }
      }
      else if (keyState(RAW_DOWN) || JOY_DOWN(1)) {
        if (!inputDelay) {
          inputDelay = INPUT_DELAY;
          PT_PlaySFX(current_level.sound_sample[BUTTON_SELECT_SOUND]);
          nextObject();
        }
      }
      else inputDelay = 0;

      if (keyState(RAW_RETURN) || keyState(RAW_SPACE) || JOY_BUTTON1(1)) {
        struct UIObject* selected;
        if ((selected = getSelectedObject())) {
          retVal = selected->id;
          PT_PlaySFX(current_level.sound_sample[BUTTON_ACKNOWLEDGE_SOUND]);
          goto init_exit;
        }
      }

      updateUI(&group_root, (WORD)mouse_pointer.x, (WORD)mouse_pointer.y, ms.buttons & LEFT_MOUSE_BUTTON);
    }

    if (inputDelay) inputDelay--;

    //updateGameObjects();
    updateBOBs();
    waitTOF();
    continue;

    init_exit:
    exiting = TRUE;
    color_table->state = CT_FADE_OUT;
    volume_table.state = PTVT_FADE_OUT;
  }

  resetUI();

  return retVal;
}
///

///MD_setSprite(gameobject)
STATIC INLINE VOID MD_setSprite(struct GameObject* go)
{
  setSprite((struct SpriteImage*)go->image, go->x, go->y, CL_SPR0PTH, DIWSTART_V, 0, SPR_FMODE);
}
///
///MD_blitBOB(gameobject)
/******************************************************************************
 * Every display which wants to get use of the algorithms on gameobject.o has *
 * to implement its own blitBOB() and unBlitBOB() functions. But since those  *
 * algorithms are optimized for level_display.o and this display is very      *
 * different, and we also want to use blit functions from the API, some       *
 * members on image and bob structs are utilized out of their purposes here.  *
 ******************************************************************************/
VOID MD_blitBOB(struct GameObject* go)
{
  struct BOB* bob = (struct BOB*)go->medium;
  struct BOBImage* image = (struct BOBImage*)go->image;
  UWORD row = image->bytesPerRow; // WARNING: we've utilized this member as row here!

  //Clip blit into bitmap boundaries
  LONG xSrc  = go->x1 < 0 ? -go->x1 : 0;
  LONG ySrc  = go->y1 < 0 ? -go->y1 : 0;
  LONG xDest = go->x1 + xSrc;
  LONG yDest = go->y1 + ySrc;
  LONG xSize = image->width  - xSrc - (go->x2 > MENU_BITMAP_WIDTH ? go->x2 - MENU_BITMAP_WIDTH : 0);
  LONG ySize = image->height - ySrc - (go->y2 > MENU_BITMAP_HEIGHT ? go->y2 - MENU_BITMAP_HEIGHT : 0);

  // Preserve bob background
  bob->lastBlt.x = xDest;
  bob->lastBlt.y = yDest;
  bob->lastBlt.words = xSize; //WARNING: we've utilized this member as pixel size here!
  bob->lastBlt.rows  = ySize; //WARNING: we've utilized this member as pixel size here!

  busyWaitBlit();
  BltBitMap(rastPort->BitMap, xDest, yDest, BOBsBackBuffer, (bob->background - BOBsBackBuffer->Planes[0]) * 8, 0, xSize, ySize, 0x0C0, 0xFF, NULL);
  // Paste bob into screen bitmap
  busyWaitBlit();
  BltMaskBitMapRastPort(image->bob_sheet, xSrc, row + ySrc, rastPort, xDest, yDest, xSize, ySize, (ABC|ABNC|ANBC), image->mask);
}
///
///MD_unBlitBOB(gameobject)
/******************************************************************************
 * Every display which wants to get use of the algorithms on gameobject.o has *
 * to implement its own blitBOB() and unBlitBOB() functions.                  *
 ******************************************************************************/
VOID MD_unBlitBOB(struct GameObject* go)
{
  struct BOB* bob = (struct BOB*)go->medium;

  busyWaitBlit();
  BltBitMapRastPort(BOBsBackBuffer, (bob->background - BOBsBackBuffer->Planes[0]) * 8, 0, rastPort, bob->lastBlt.x, bob->lastBlt.y, bob->lastBlt.words, bob->lastBlt.rows, 0x0C0);
}
///
