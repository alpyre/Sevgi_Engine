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

#include "SDI_headers/SDI_compiler.h"

#include "system.h"
#include "color.h"
#include "cop_inst_macros.h"
#include "input.h"
#include "keyboard.h"
#include "diskio.h"
#include "fonts.h"
#include "audio.h"
#include "level.h"
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

#define BUTTONS_START_Y      128
#define BUTTONS_SEPARATION_Y   5
#define INPUT_DELAY 20

#define BUTTON_ACTIVATE_SOUND 0
#define BUTTON_SELECT_SOUND   1

#define MAXVECTORS 16
///
///globals
// imported globals
extern struct Custom custom;
extern volatile LONG new_frame_flag;
extern UWORD NULL_SPRITE_ADDRESS_H;
extern UWORD NULL_SPRITE_ADDRESS_L;
extern struct TextFont* textFonts[NUM_TEXTFONTS];
extern struct GameFont* gameFonts[NUM_GAMEFONTS];
extern struct PT_VolumeTable volume_table;
extern struct Level current_level;
extern struct BitMap* BOBsBackBuffer;       // from gameobject.o

extern struct GameObject* spriteList[NUM_SPRITES + 1];
extern struct GameObject* bobList[NUM_BOBS + 1];

// private globals
STATIC struct GameObject* gameobjects = NULL;
STATIC struct BitMap* screen_bitmap = NULL; // BitMap for the menu display
STATIC struct RastPort* rastPort;
STATIC struct ColorTable* color_table = NULL;

enum {
  BTN_NONE,
  BTN_START,
  BTN_OPTIONS,
  BTN_QUIT
};
STRPTR buttonText[NUM_BUTTONS] = {"START", "OPTIONS", "QUIT"};
struct BOBImage button_image[NUM_BUTTONS * BUTTON_STATES];
struct BitMap* button_bm = NULL;
struct BitMap* button_mask = NULL;
///
///copperlist
STATIC UWORD* CopperList  = (UWORD*) 0;
STATIC UWORD* CL_BPL1PTH  = (UWORD*) 0;
STATIC UWORD* CL_SPR0PTH  = (UWORD*) 0;

STATIC ULONG copperList_Instructions[] = {
                                              // Access Ptr:  Action:
  MOVE(COLOR00, 0),                           //              Set color 0 to black
  MOVE(FMODE,   MENU_FMODE_V),                //              Set Sprite/Bitplane Fetch Modes
  MOVE(BPLCON0, BPLCON0_V),                   //              Set a lowres display
  MOVE(BPLCON1, 0),                           //              Set h_scroll register
  MOVE(BPLCON2, 0x264),
  MOVE(BPLCON3, BPLCON3_V),
  MOVE(BPLCON4, 0x0),
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
///prototypes (private)
STATIC VOID vblankEvents(VOID);
STATIC BOOL openDisplay(VOID);
STATIC VOID closeDisplay(VOID);
STATIC struct RastPort* openScreen(VOID);
STATIC VOID closeScreen(VOID);
STATIC UWORD* createCopperList(VOID);
STATIC VOID disposeCopperList(VOID);
STATIC VOID switchToMenuCopperList(VOID);
STATIC BOOL prepGameObjects(VOID);
STATIC VOID drawScreen(VOID);
STATIC INLINE VOID setSprite(struct GameObject* go);
STATIC ULONG menuDisplayLoop(VOID);
///

///vblankEvents()
/******************************************************************************
 *
 ******************************************************************************/
STATIC VOID vblankEvents()
{
  setSprite(&gameobjects[0]);
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
                           RPF_BITMAP | RPF_TMPRAS | RPF_AREA, MAXVECTORS);

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
        if (prepGameObjects()) {
          drawScreen();
          return TRUE;
        }
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
  if (button_mask) FreeBitMap(button_mask); button_mask = NULL;
  if (button_bm) FreeBitMap(button_bm); button_bm = NULL;
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
    PT_StopAudio();
  }

  closeDisplay();
  return retVal;
}
///

///prepGameObjects()
STATIC BOOL prepGameObjects()
{
  struct TextExtent te = {0};
  struct RastPort* button_rastport;
  struct {
    ULONG width;
    ULONG height;
    ULONG row;
  }buttons[NUM_BUTTONS];
  ULONG row = 0;
  ULONG max_width  = 0;
  ULONG max_height = 0;
  ULONG max_go_height = 0;
  ULONG i;

  SetFont(rastPort, textFonts[TF_DEFAULT]);

  for (i = 0; i < NUM_BUTTONS; i++) {
    row += te.te_Height;
    buttons[i].row = row;

    TextExtent(rastPort, buttonText[i], strlen(buttonText[i]), &te);

    buttons[i].width  = te.te_Width;
    buttons[i].height = te.te_Height;

    if (max_width < te.te_Width) max_width = te.te_Width;
    max_height = row + te.te_Height;
  }

  button_rastport = allocRastPort(max_width, max_height * BUTTON_STATES, MENU_SCREEN_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR, NULL,
                                  RPF_BITMAP | RPF_TMPRAS, 0);
  if (button_rastport) {
    ULONG go;

    button_bm = button_rastport->BitMap;
    button_rastport->DrawMode = JAM1;
    SetFont(button_rastport, textFonts[TF_DEFAULT]);
    SetAPen(button_rastport, DARK);
    SetBPen(button_rastport, BLACK);

    for (i = 0; i < NUM_BUTTONS; i++) {
      button_rastport->cp_x = 0;
      button_rastport->cp_y = buttons[i].row + textFonts[TF_DEFAULT]->tf_Baseline;
      Text(button_rastport, buttonText[i], strlen(buttonText[i]));
    }
    SetAPen(button_rastport, WHITE);
    for (i = 0; i < NUM_BUTTONS; i++) {
      button_rastport->cp_x = 0;
      button_rastport->cp_y = max_height + buttons[i].row + textFonts[TF_DEFAULT]->tf_Baseline;
      Text(button_rastport, buttonText[i], strlen(buttonText[i]));
    }
    freeRastPort(button_rastport, RPF_TMPRAS);

    button_mask = createBltMasks(button_bm);
    if (button_mask) {
      struct SpriteImage* mouse_image = &current_level.sprite_bank[0]->image[0];
      ULONG y = 0;

      //prep the images and game objects for buttons
      for (i = 0, go = 1; i < NUM_BUTTONS; i++, go++) {
        button_image[i].width  = buttons[i].width;
        button_image[i].height = buttons[i].height;
        button_image[i].h_offs = 0;
        button_image[i].v_offs = 0;
        button_image[i].bytesPerRow = buttons[i].row; // WARNING: we will utilize this member as row!
        button_image[i].bob_sheet = button_bm;
        button_image[i].mask = button_mask->Planes[0];

        button_image[i + NUM_BUTTONS].width  = buttons[i].width;
        button_image[i + NUM_BUTTONS].height = buttons[i].height;
        button_image[i + NUM_BUTTONS].h_offs = 0;
        button_image[i + NUM_BUTTONS].v_offs = 0;
        button_image[i + NUM_BUTTONS].bytesPerRow = buttons[i].row + max_height; // WARNING: we will utilize this member as row!
        button_image[i + NUM_BUTTONS].bob_sheet = button_bm;
        button_image[i + NUM_BUTTONS].mask = button_mask->Planes[0];

        gameobjects[go].x = (MENU_SCREEN_WIDTH - button_image[i].width) / 2;
        gameobjects[go].y = BUTTONS_START_Y + y;
        gameobjects[go].x1 = gameobjects[go].x + button_image[i].h_offs;
        gameobjects[go].y1 = gameobjects[go].y + button_image[i].v_offs;
        gameobjects[go].x2 = gameobjects[go].x1 + button_image[i].width;
        gameobjects[go].y2 = gameobjects[go].y1 + button_image[i].height;
        gameobjects[go].type = BOB_OBJECT;
        gameobjects[go].state = GOB_DEAD;
        gameobjects[go].me_mask = 0x01;
        gameobjects[go].hit_mask = 0x00;
        gameobjects[go].image = (struct ImageCommon*)&button_image[i];
        gameobjects[go].medium = NULL;
        gameobjects[go].priority = 0;
        gameobjects[go].anim.func = NULL;

        y += button_image[i].height + BUTTONS_SEPARATION_Y;
        if (max_go_height < button_image[i].height) max_go_height = button_image[i].height;
      }

      //prep the mouse pointer
      gameobjects[0].x = 0; //NOTE: Maybe we can memorize mouse positions globally?
      gameobjects[0].y = 0;
      gameobjects[0].x1 = gameobjects[0].x + mouse_image->h_offs;
      gameobjects[0].y1 = gameobjects[0].y + mouse_image->v_offs;
      gameobjects[0].x2 = gameobjects[0].x1 + mouse_image->width;
      gameobjects[0].y2 = gameobjects[0].y1 + mouse_image->height;
      gameobjects[0].type = SPRITE_OBJECT;
      gameobjects[0].state = 0;
      gameobjects[0].me_mask = 0x00;
      gameobjects[0].hit_mask = 0x01;
      gameobjects[0].image = (struct ImageCommon*)mouse_image;
      gameobjects[0].medium = NULL;
      gameobjects[0].priority = 0;

      if (NUM_BOBS) {
        BOBsBackBuffer = allocBOBBackgroundBuffer(max_width, max_go_height, MENU_BITMAP_DEPTH);
        if (!BOBsBackBuffer) return FALSE;
      }

      max_go_height = max_go_height > mouse_image->height ? max_go_height : mouse_image->height;

      initGameObjects(MD_blitBOB, MD_unBlitBOB, max_width, max_go_height);
    }
    else return FALSE;
  }
  else return FALSE;

  return TRUE;
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

///activateButton()
STATIC VOID activateButton(ULONG goIndex)
{
  #define BOB_DEAD 0x20
  struct GameObject* go = &gameobjects[goIndex];
  struct BOB* bob = (struct BOB*)go->medium;
  bob->flags &= ~BOB_DEAD;

  go->image = (struct ImageCommon*)&button_image[goIndex - 1 + NUM_BUTTONS];
}
///
///deActivateButton()
STATIC VOID deActivateButton(ULONG goIndex)
{
  struct GameObject* go = &gameobjects[goIndex];
  struct BOB* bob = (struct BOB*)go->medium;
  bob->flags &= ~BOB_DEAD;

  go->image = (struct ImageCommon*)&button_image[goIndex - 1];
}
///

///menuDisplayLoop()
STATIC ULONG menuDisplayLoop()
{
  BOOL exiting = FALSE;
  ULONG retVal = 0;
  LONG activeButton = BTN_NONE;
  LONG clickedButton = BTN_NONE;
  LONG inputDelay = 0;
  ULONG i;

  color_table->state = CT_FADE_IN;

  while (TRUE) {
    struct MouseState ms;
    new_frame_flag = 1;

    updateColorTable(color_table);
    updateVolume();

    doKeyboardIO();
    UL_VALUE(ms) = readMouse(0);

    if (UL_VALUE(ms)) {
      gameobjects[0].state &= ~GOB_INACTIVE; // activate mouse
      moveGameObjectClamped(&gameobjects[0], ms.deltaX, ms.deltaY, 0, 0, MENU_SCREEN_WIDTH - 2, MENU_SCREEN_HEIGHT - 2);
    }

    if (exiting == TRUE && color_table->state == CT_IDLE && volume_table.state == PTVT_IDLE) {
      spriteList[0] = NULL;
      bobList[0] = NULL;
      break;
    }

    if (!exiting) {
      // Handle key presses
      if (keyState(RAW_ESC) && !exiting) {
        retVal  = MENU_RV_QUIT;
        goto init_exit;
      };

        if (keyState(RAW_UP) || JOY_UP(1)) {
          if (!inputDelay) {
            gameobjects[0].state |= GOB_INACTIVE; // deactivate mouse
            inputDelay = INPUT_DELAY;
            if (activeButton) {
              if (activeButton > BTN_START) {
                deActivateButton(activeButton);
                activeButton--;
                activateButton(activeButton);
                PT_PlaySFX(current_level.sound_sample[BUTTON_ACTIVATE_SOUND]);
              }
            }
            else {
              activeButton = 1;
              activateButton(1);
              PT_PlaySFX(current_level.sound_sample[BUTTON_ACTIVATE_SOUND]);
            }
          }
        }
        else if (keyState(RAW_DOWN) || JOY_DOWN(1)) {
          if (!inputDelay) {
            gameobjects[0].state |= GOB_INACTIVE; // deactivate mouse
            inputDelay = INPUT_DELAY;
            if (activeButton) {
              if (activeButton < BTN_QUIT) {
                deActivateButton(activeButton);
                activeButton++;
                activateButton(activeButton);
                PT_PlaySFX(current_level.sound_sample[BUTTON_ACTIVATE_SOUND]);
              }
            }
            else {
              activeButton = 1;
              activateButton(1);
              PT_PlaySFX(current_level.sound_sample[BUTTON_ACTIVATE_SOUND]);
            }
          }
        }
        else inputDelay = 0;

      if (keyState(RAW_RETURN) || keyState(RAW_SPACE)) {
        if (activeButton) {
          PT_PlaySFX(current_level.sound_sample[BUTTON_SELECT_SOUND]);
          retVal = activeButton;
          goto init_exit;
        }
      }

      // Highlight and select the button under the mouse pointer!
      if (!(gameobjects[0].state & GOB_INACTIVE)) {
        for (i = 1; i <= NUM_BUTTONS; i++) {
          if (gameobjects[0].x > gameobjects[i].x1 && gameobjects[0].x < gameobjects[i].x2 &&
              gameobjects[0].y > gameobjects[i].y1 && gameobjects[0].y < gameobjects[i].y2) {
            if (i != activeButton) {
              if (activeButton) deActivateButton(activeButton);
              activeButton = i;
              activateButton(i);
              PT_PlaySFX(current_level.sound_sample[BUTTON_ACTIVATE_SOUND]);
            }
            break;
          }
        }
        if (i > NUM_BUTTONS && activeButton) {
          deActivateButton(activeButton);
          activeButton = BTN_NONE;
        }

        if (ms.buttons & LEFT_MOUSE_BUTTON) { //LMB is pressed
          if (!clickedButton && activeButton) {
            clickedButton = activeButton;
          }
        }
        else { //LMB is released after a press
          if (clickedButton) {
            if (activeButton) {
              PT_PlaySFX(current_level.sound_sample[BUTTON_SELECT_SOUND]);
              retVal  = activeButton;
              goto init_exit;
            }
            else clickedButton = BTN_NONE;
          }
        }
      }
    }

    if (inputDelay) inputDelay--;

    updateGameObjects();
    updateBOBs();
    waitTOF();
    continue;

    init_exit:
    exiting = TRUE;
    color_table->state = CT_FADE_OUT;
    volume_table.state = PTVT_FADE_OUT;
  }

  return retVal;
}
///

///setSprite(gameobject)
//NOTE: This can be highly simplified because we know the mouse pointer is a
//      16x16 single hardware sprite!
STATIC INLINE VOID setSprite(struct GameObject* go)
{
  ULONG hsn = ((struct Sprite*)go->medium)->hsn;
  if (hsn < 8) {
    struct SpriteImage* image = (struct SpriteImage*)go->image;
    struct SpriteTable* entry = &image->sprite_bank->table[image->image_num];

    UWORD offset = entry->offset;
    UBYTE type = entry->type;

    UWORD offsetOfNext = *((UWORD*)((UBYTE*)entry + sizeof(struct SpriteTable)));
    UWORD numSprites = type & 0xF;

    UWORD ssize = (offsetOfNext - offset) / numSprites;
    UWORD height = image->height; // (ssize / (4 * MENU_SPR_FMODE)) - 2;

    ULONG x = go->x1 + (DIWSTART_V & 0xFF);
    ULONG y = go->y1 + (DIWSTART_V >> 8);
    ULONG s = y + height;

    UBYTE* saddr = image->sprite_bank->data + offset;
    WORD* haddr = CL_SPR0PTH + hsn * 4;

    //Prepare the two sprite control words (in one long) for the static vertical coords.
    ULONG ctl_l0 = (y << 24) | ((s << 8) & 0xFF00) | ((y >> 8) << 2) | ((s >> 8) << 1);
    ULONG ctl_l1;
    ULONG ctl_l2;
    if (type & 0x10) {
      while (numSprites) {
        //finalize the control words with the current x value for this glued sprite
        ctl_l1 = ctl_l0 | ((x >> 1) << 16) | 0x80 | (x & 0x1);
        ctl_l2 = ctl_l1 << 16;

        //Handle the first attached sprite:
        //set pos & ctl words on the sprite
        switch (MENU_SPR_FMODE) {
          case 4: *((ULONG*)saddr + 2) = ctl_l2;
          case 2: *((ULONG*)saddr + 1) = ctl_l2;
          case 1: *((ULONG*)saddr) = ctl_l1;
        }

        //set the sprite address on CopperList
        *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
        *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;

        //get to the attached sprite
        saddr += ssize;

        //set pos & ctl words on this sprite structure as well
        switch (MENU_SPR_FMODE) {
          case 4: *((ULONG*)saddr + 2) = ctl_l2;
          case 2: *((ULONG*)saddr + 1) = ctl_l2;
          case 1: *((ULONG*)saddr) = ctl_l1;
        }

        //set this sprite address on CopperList as well
        *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
        *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;

        //get to the next glued sprite (if there is any)
        saddr += ssize;
        x += (16 * MENU_SPR_FMODE);
        numSprites -= 2;
      }
    }
    else
    {
      while (numSprites) {
        //finalize the control words with the current x value for this glued sprite
        ctl_l1 = ctl_l0 | ((x >> 1) << 16) | (x & 0x1);
        ctl_l2 = ctl_l1 << 16;

        //set pos & ctl words on the sprite
        switch (MENU_SPR_FMODE) {
          case 4: *((ULONG*)saddr + 2) = ctl_l2;
          case 2: *((ULONG*)saddr + 1) = ctl_l2;
          case 1: *((ULONG*)saddr) = ctl_l1;
        }

        //set the sprite address on CopperList
        *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
        *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;

        //get to the next glued sprite (if there is any)
        saddr += ssize;
        x += (16 * MENU_SPR_FMODE);
        numSprites--;
      }
    }
  }
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
