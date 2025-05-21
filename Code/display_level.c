///includes
#define ECS_SPECIFIC

#include <stdio.h>

#include <exec/exec.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/display.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <hardware/cia.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "SDI_headers/SDI_compiler.h"

#include "system.h"
#include "tiles.h"
#include "tilemap.h"
#include "gameobject.h"
#include "color.h"
#include "keyboard.h"
#include "input.h"
#include "audio.h"
#include "rainbow.h"
#include "cop_inst_macros.h"
#include "level.h"
#include "display.h"
#include "display_loading.h"

#include "display_level.h"
///
///defines (private)
#define MAX_SCROLL_SPEED 16  // pixels per frame
#define ROUND2TILESIZE(a) ((a) & ~(TILESIZE - 1))

#define SCREEN_BYTES_PER_ROW (SCREEN_WIDTH / 8)
#define BITMAP_BYTES_PER_ROW (BITMAP_WIDTH / 8)

#define TILEWIDTH TILESIZE
#define TILEHEIGHT TILESIZE

#define NUMSTEPS_X TILEWIDTH
#define NUMSTEPS_Y TILEHEIGHT

#define BITMAP_TILES_PER_ROW (BITMAP_WIDTH  / TILESIZE)
#define BITMAP_TILES_PER_COL (BITMAP_HEIGHT / TILESIZE)
#define BITMAP_PLANE_LINES (BITMAP_HEIGHT * TILEDEPTH)
#define TILE_PLANE_LINES  (TILEHEIGHT * TILEDEPTH)

#define PLANE_PIXEL_BITMAP_WIDTH (BITMAP_WIDTH * TILEDEPTH)

#define DISPLAY_START (DIWSTART_V >> 8)
#define SCREEN_START (DISPLAY_START + TOP_PANEL_HEIGHT)
#define SCREEN_END (SCREEN_START + SCREEN_HEIGHT)
#if SCREEN_END < 256
  #define SCREEN_END_WAIT SCREEN_END
#else
  #define SCREEN_END_WAIT (SCREEN_END - 256)
#endif

#ifdef DUALPLAYFIELD
  //Uncomment below to have a non-interleaved bitmap for level_bitmap_pf2
  #define INTERLEAVED_PF2_BITMAP

  //WARNING: Below values only work for BPL_FMODE == 1
  #define SCREEN_PLANES (SCREEN_DEPTH + BITMAP_DEPTH_PF2)
  #define BPLCON0_V ((SCREEN_PLANES == 8 ? BPLCON0_BPU3 : (SCREEN_PLANES * BPLCON0_BPU0)) | BPLCON0_COLOR | USE_BPLCON3 | BPLCON0_DBLPF)
  #define BITMAP_WIDTH_PF2 (SCREEN_WIDTH * 2 + SCROLL_PIXELS)
  #define BITMAP_BYTES_PER_ROW_PF2 (ROUND_TO_64(BITMAP_WIDTH_PF2) / 8)

  #ifdef INTERLEAVED_PF2_BITMAP
    #define BPLXMOD_V_PF2 (BITMAP_BYTES_PER_ROW_PF2 * BITMAP_DEPTH_PF2 - (SCREEN_WIDTH / 8) - SCROLL_BYTES)
    #define BMF_INTERLEAVED_PF2 BMF_INTERLEAVED
  #else //INTERLEAVED_PF2_BITMAP
    #define BPLXMOD_V_PF2 (BITMAP_BYTES_PER_ROW_PF2 - (SCREEN_WIDTH / 8) - SCROLL_BYTES)
    #define BMF_INTERLEAVED_PF2 0
  #endif

#else //DUALPLAYFIELD
  #define SCREEN_PLANES SCREEN_DEPTH
  #define BPLCON0_V ((SCREEN_PLANES == 8 ? BPLCON0_BPU3 : (SCREEN_PLANES * BPLCON0_BPU0)) | BPLCON0_COLOR | USE_BPLCON3)
  #define BPLXMOD_V_PF2 BPLXMOD_V
#endif

#if TOP_PANEL_HEIGHT > 0
  #define TOP_PANEL_DEPTH SCREEN_PLANES
  #define BPLXMOD_TP ((SCREEN_WIDTH * (TOP_PANEL_DEPTH - 1)) / 8)
  #define BPLCON0_V_TP ((TOP_PANEL_DEPTH == 8 ? BPLCON0_BPU3 : (TOP_PANEL_DEPTH * BPLCON0_BPU0)) | BPLCON0_COLOR | USE_BPLCON3)
  #define TOP_PANEL_END (DISPLAY_START + TOP_PANEL_HEIGHT - 1)
#endif

#if BOTTOM_PANEL_HEIGHT > 0
  #define BOTTOM_PANEL_DEPTH SCREEN_PLANES
  #define BPLXMOD_BP ((SCREEN_WIDTH * (BOTTOM_PANEL_DEPTH - 1)) / 8)
  #define BPLCON0_V_BP ((BOTTOM_PANEL_DEPTH == 8 ? BPLCON0_BPU3 : (BOTTOM_PANEL_DEPTH * BPLCON0_BPU0)) | BPLCON0_COLOR | USE_BPLCON3)
  #define BOTTOM_PANEL_START (SCREEN_END + 1)
  #if BOTTOM_PANEL_START < 256
    #define BTM_PNL_STRT_W_X 0
    #define BTM_PNL_STRT_W_Y BOTTOM_PANEL_START
  #elif BOTTOM_PANEL_START == 256
    #define BTM_PNL_STRT_W_X 225
    #define BTM_PNL_STRT_W_Y 255
  #else
    #define BTM_PNL_STRT_W_X 0
    #define BTM_PNL_STRT_W_Y (BOTTOM_PANEL_START - 256)
  #endif
#endif
///
///prototypes (private)
//NOTE: take care of these forward declarations below!
STATIC struct BitMap* openScreen(VOID);
STATIC VOID closeScreen(VOID);
STATIC VOID blitTile(ULONG, ULONG, ULONG, ULONG);
STATIC VOID blitHScrollLine(ULONG);

STATIC BOOL openDisplay(ULONG level_num);
STATIC VOID closeDisplay(VOID);
STATIC VOID initLevelDisplay(VOID);
STATIC VOID fillDisplay(UWORD, UWORD);
STATIC UWORD scrollRight(UWORD);
STATIC VOID scrollRightPixels(UWORD);
STATIC UWORD scrollLeft(UWORD);
STATIC VOID scrollLeftPixels(UWORD);
STATIC UWORD scrollUp(UWORD);
STATIC VOID scrollUpPixels(UWORD);
STATIC UWORD scrollDown(UWORD);
STATIC VOID scrollDownPixels(UWORD);
STATIC VOID initTileBlit(VOID);
STATIC VOID levelDisplayLoop(VOID);

STATIC UWORD *createCopperList(VOID);
STATIC VOID disposeCopperList(VOID);
STATIC VOID updateCopperList(VOID);
STATIC VOID switchToLevelCopperList(VOID);
#ifdef DYNAMIC_COPPERLIST
STATIC VOID updateDynamicCopperList(VOID);
STATIC INLINE VOID initCopperListBlit(VOID);
STATIC BOOL prepGradients(ULONG level_num);
STATIC VOID freeGradients(VOID);
#ifdef SMART_SPRITES
STATIC VOID sortSpriteCopOps(VOID);
#endif /*SMART_SPRITES*/
#endif /*DYNAMIC_COPPERLIST*/

STATIC VOID setSprites(VOID);
STATIC INLINE VOID setSprite(struct GameObject* go);
///
///globals
// imported globals
extern struct Custom custom;
extern struct CIA ciaa, ciab;
extern volatile LONG new_frame_flag;
extern struct PT_VolumeTable volume_table;
extern struct Level current_level;

extern UWORD NULL_SPRITE_ADDRESS_H;
extern UWORD NULL_SPRITE_ADDRESS_L;

extern struct GameObject* spriteList[NUM_SPRITES + 1];
extern struct GameObject* bobList[NUM_BOBS + 1];
extern struct BitMap* BOBsBackBuffer;

extern LONG* mapPosX;  // map's horizontal position in pixels
extern LONG* mapPosY;  // map's vertical position in pixels
extern LONG* mapPosX2; // defines the square that displayed on
extern LONG* mapPosY2; // ...the screen.

STATIC LONG mapPosX_s; // The map pos values to be used in the scroll routines. They'll
STATIC LONG mapPosY_s; // match the above counterparts when a scroll is complete.

// private globals
STATIC ULONG* maxMapPosX;
STATIC ULONG* maxMapPosY;
STATIC WORD vidPosX = 0;   // X position of the bitmap area currently visible (in pixels).
STATIC WORD vidPosY = 0;   // Y position of the bitmap area currently visible (in pixels).
STATIC WORD vidPosX_s = 0; // Same with vidPosX, though this one is used only in tile blit
                           // calculations, while the other is used in updateCopperList()
                           // and they may occasionally hold different values during VBL.
                           // NOTE: Logically there should also be a vidPosY2 but because
                           // tile blit calculations do not depend on vidPosY it is
                           // optimized out.

STATIC UBYTE *bitmapTop    = NULL; // Address to first visible pixel of the bitmap
STATIC UBYTE *bitmapBottom = NULL; // Address to the horizontal scroll buffer line
STATIC WORD fillUpRowPos = 0; // Y position of the vertical scroll fill-up row (in pixels).
STATIC WORD mapTileX;
STATIC WORD mapTileY;
STATIC WORD stepX = 0;    // step for horizontal scrolling. (*mapPosX % TILESIZE)
STATIC WORD stepY = 0;    // step for vertical scrolling.   (*mapPosY % TILESIZE)

STATIC WORD *saveWordPtr;     // address of the last planeline backup.
STATIC WORD  saveWord;        // the planeline backup storage.
STATIC ULONG lastHDir = NONE; // last horizontal scroll direction

//Number of tiles to blit at each horizontal and vertical scroll steps
//The values below are the defaults in ScrollingTrick code for a 320x256 display
//Actual proper values are recalculated in initLevelDisplay() function
UBYTE tilesPerStepX[NUMSTEPS_X] = {2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
UBYTE posPerStepX[NUMSTEPS_X] = {1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};
UBYTE tilesPerStepY[NUMSTEPS_Y] = {2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1};
UBYTE posPerStepY[NUMSTEPS_Y] = {0,2,4,6,8,10,12,14,16,17,18,19,20,21,22,23};

#if TOP_PANEL_HEIGHT > 0
STATIC struct RastPort* top_panel_rastport = NULL;
#endif
#if BOTTOM_PANEL_HEIGHT > 0
STATIC struct RastPort* bottom_panel_rastport = NULL;
#endif
STATIC struct BitMap* level_bitmap = NULL;   // Main game screen opened by openDisplay()
#ifdef DUALPLAYFIELD
STATIC struct BitMap* level_bitmap_pf2 = NULL; // Parallaxing background image
#endif
STATIC struct ColorTable* color_table = NULL;
STATIC struct TileMap* map = NULL;           // The loaded tile map for this display
STATIC struct TileSet* tileset = NULL;       // The loaded tile set for this map

STATIC PLANEPTR *planes = NULL;  //quick access pointer for level_bitmap->Planes
#ifdef DUALPLAYFIELD
STATIC PLANEPTR *planes_pf2 = NULL; //access pointer for playfield 2's bitmap
#endif
STATIC ULONG bobs_back_buffer_mod;

#ifdef DYNAMIC_COPPERLIST
  ULONG num_cl_header_instructions = 0;
  STATIC UWORD* CopperList1 = NULL;
  STATIC UWORD* CopperList2 = NULL;
  STATIC struct Rainbow* rainbow = NULL;
  STATIC struct Rainbow* empty_rainbow = NULL;
  STATIC UWORD* CL_COP2LCH_1;
  STATIC UWORD* CL_COP2LCH_2;
  STATIC ULONG* vsplit_list = NULL;
  struct CopOp vsplit_CopOps[3];
  #ifdef SMART_SPRITES
    STATIC struct CopOp sprite_CopOps[NUM_SPRITES * 8 + 1];
    STATIC struct CopOp* sprite_CopOps_list[NUM_SPRITES * 8 + 1]; // a sortable list for CopOps
    STATIC UWORD hardSpriteUsage[8] = {0};
    STATIC ULONG* sprite_list = NULL;  //NOTE: Rename this too confusing!
    STATIC ULONG* sprite_list_i = NULL;
    STATIC UBYTE sprite_CopOp_Index = 0;
  #else
    STATIC struct CopOp sprite_CopOps[1];
    STATIC struct CopOp* sprite_CopOps_list[1];
  #endif
#endif
///
///copperlist
STATIC UWORD* CopperList    = (UWORD*)0;

#ifndef SMART_SPRITES
STATIC UWORD* CL_SPR0PTH    = (UWORD*)0;
#else
#if TOP_PANEL_HEIGHT > 0
STATIC UWORD* TP_SPR0PTH    = (UWORD*)0;
#endif
#endif

#if TOP_PANEL_HEIGHT > 0
STATIC UWORD* TP_BPL1PTH    = (UWORD*)0;
#endif

STATIC UWORD* CL_BPLCON1    = (UWORD*)0;
STATIC UWORD* CL_BPL1PTH    = (UWORD*)0;
STATIC UWORD* CL_BPL1PTL    = (UWORD*)0;

#ifdef DYNAMIC_COPPERLIST
STATIC UWORD* CL_COP2LCH    = (UWORD*)0;
#ifdef SMART_SPRITES
STATIC UWORD* CL_SPR0POS    = (UWORD*)0;
STATIC UWORD* CL_SPR0PTH    = (UWORD*)0;
#endif
#endif

STATIC UWORD* CL_VIDSPLT    = (UWORD*)0;
STATIC UWORD* CL_VIDSPLT2   = (UWORD*)0;

STATIC UWORD* CL_BPL1PTH_2  = (UWORD*)0;

STATIC UWORD* CL_ENDWAIT    = (UWORD*)0;

#if BOTTOM_PANEL_HEIGHT > 0
STATIC UWORD* BP_COP2LCH    = (UWORD*)0;
STATIC UWORD* BP_BPL1PTH    = (UWORD*)0;
#endif

STATIC UWORD* CL_END        = (UWORD*)0;

STATIC ULONG copperList_Instructions[] = {
                                              // Access Ptr:  Action:
  MOVE(FMODE,   FMODE_V),                     //              Set Sprite/Bitplane Fetch Modes
  MOVE(DIWSTRT, DIWSTART_V),                  //              Set Display Window Start
  MOVE(DIWSTOP, DIWSTOP_V),                   //              Set Display Window Stop
  MOVE(DMACON,  0x8020),                      //              Enable sprite DMA
#ifndef SMART_SPRITES
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
#else
#if TOP_PANEL_HEIGHT > 0
  MOVE_PH(SPR0PTH, 0),                        // TP_SPR0PTH   Set OS's mouse sprite off
  MOVE(SPR0PTL, 0),                           //               "    "    "     "     "
#endif //TOP_PANEL_HEIGHT
#endif //NOT SMART_SPRITES
  //TOP PANEL INSTRUCTIONS
#if TOP_PANEL_HEIGHT > 0
  MOVE(DDFSTRT, DEFAULT_DDFSTRT_LORES),       //              Set Data Fetch Start
  MOVE(DDFSTOP, DEFAULT_DDFSTOP_LORES),       //              Set Data Fetch Stop
  MOVE(BPLCON0, BPLCON0_V_TP),                //              Set display properties for top panel
  MOVE(BPLCON1, 0),                           //              No scroll
  MOVE(BPLCON2, 0x23F),                       //              Default plane priorities
  MOVE(BPLCON3, BPLCON3_BRDNBLNK),            //              Border blank only
  MOVE(BPLCON4, 0x0),                         //              Default sprite colors
  MOVE(BPL1MOD, BPLXMOD_TP),                  //              Set bitplane mods for top panel
  MOVE(BPL2MOD, BPLXMOD_TP),                  //               "     "       "   "   "    "
  MOVE_PH(BPL1PTH, 0),                        // TP_BPL1PTH   Set bitplane addresses
  MOVE(BPL1PTL, 0),                           //               "      "       "
  MOVE(BPL2PTH, 0),                           //               "      "       "
  MOVE(BPL2PTL, 0),                           //               "      "       "
  MOVE(BPL3PTH, 0),                           //               "      "       "
  MOVE(BPL3PTL, 0),                           //               "      "       "
  MOVE(BPL4PTH, 0),                           //               "      "       "
  MOVE(BPL4PTL, 0),                           //               "      "       "
  MOVE(BPL5PTH, 0),                           //               "      "       "
  MOVE(BPL5PTL, 0),                           //               "      "       "
  MOVE(BPL6PTH, 0),                           //               "      "       "
  MOVE(BPL6PTL, 0),                           //               "      "       "
  MOVE(BPL7PTH, 0),                           //               "      "       "
  MOVE(BPL7PTL, 0),                           //               "      "       "
  MOVE(BPL8PTH, 0),                           //               "      "       "
  MOVE(BPL8PTL, 0),                           //               "      "       "
  WAIT(0, TOP_PANEL_END),                     //              Last row of top panel switches to level display
  MOVE(DMACON, 0x100),                        //              Disable Bitplane DMA
#endif
  //END OF TOP PANEL INSTRUCTIONS
  MOVE(DDFSTRT, DDFSTART_V),                  //              Set Data Fetch Start to fetch early
  MOVE(DDFSTOP, DDFSTOP_V),                   //              Set Data Fetch Stop
  MOVE(BPLCON0, BPLCON0_V),                   //              Set display properties
  MOVE_PH(BPLCON1, 0),                        // CL_BPLCON1   Set h_scroll register
  MOVE(BPLCON2, 0x23F),
  MOVE(BPLCON3, BPLCON3_V),
  MOVE(BPLCON4, 0x11),
  MOVE(BPL1MOD, BPLXMOD_V),                   //              Set bitplane mods
  MOVE(BPL2MOD, BPLXMOD_V_PF2),               //               "     "       "
  MOVE_PH(BPL1PTH, 0),                        // CL_BPL1PTH   Set bitplane addresses
  MOVE_PH(BPL1PTL, 0),                        // CL_BPL1PTL    "      "       "
  MOVE(BPL2PTH, 0),                           //               "      "       "
  MOVE(BPL2PTL, 0),                           //               "      "       "
  MOVE(BPL3PTH, 0),                           //               "      "       "
  MOVE(BPL3PTL, 0),                           //               "      "       "
  MOVE(BPL4PTH, 0),                           //               "      "       "
  MOVE(BPL4PTL, 0),                           //               "      "       "
  MOVE(BPL5PTH, 0),                           //               "      "       "
  MOVE(BPL5PTL, 0),                           //               "      "       "
  MOVE(BPL6PTH, 0),                           //               "      "       "
  MOVE(BPL6PTL, 0),                           //               "      "       "
  MOVE(BPL7PTH, 0),                           //               "      "       "
  MOVE(BPL7PTL, 0),                           //               "      "       "
  MOVE(BPL8PTH, 0),                           //               "      "       "
  MOVE(BPL8PTL, 0),                           //               "      "       "
#ifdef DYNAMIC_COPPERLIST
  MOVE_PH(COP2LCH, 0),                        // CL_COP2LCH   Address to switch to the other double
  MOVE(COP2LCL, 0),                           //              buffered copper list at VBL
#ifdef SMART_SPRITES
#if TOP_PANEL_HEIGHT == 0
  WAIT(0, DISPLAY_START - 1),                 //              Wait until sprite DMA fetches pointers (min 26)
#endif
  MOVE_PH(SPR0POS, 0),                        // CL_SPR0POS   Set sprite controls
  MOVE(SPR0CTL, 0),                           //               "     "      "
  MOVE(SPR1POS, 0),                           //               "     "      "
  MOVE(SPR1CTL, 0),                           //               "     "      "
  MOVE(SPR2POS, 0),                           //               "     "      "
  MOVE(SPR2CTL, 0),                           //               "     "      "
  MOVE(SPR3POS, 0),                           //               "     "      "
  MOVE(SPR3CTL, 0),                           //               "     "      "
  MOVE(SPR4POS, 0),                           //               "     "      "
  MOVE(SPR4CTL, 0),                           //               "     "      "
  MOVE(SPR5POS, 0),                           //               "     "      "
  MOVE(SPR5CTL, 0),                           //               "     "      "
  MOVE(SPR6POS, 0),                           //               "     "      "
  MOVE(SPR6CTL, 0),                           //               "     "      "
  MOVE(SPR7POS, 0),                           //               "     "      "
  MOVE(SPR7CTL, 0),                           //               "     "      "
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
#endif // SMART_SPRITES
#endif // DYNAMIC_COPPERLIST
#if TOP_PANEL_HEIGHT > 0
  WAIT(0, SCREEN_START),                      //              Enable Bitplane DMA back
  MOVE(DMACON, 0x8100),                       //              Enable Bitplane DMA back
#endif
  // END OF HEADER INSTRUCTIONS IN DYNAMIC_COPPERLIST MODE
  WAIT_PH(0, 0),                              // CL_VIDSPLT   Wait for the scan line...
  WAIT(0, 0),                                 //              ...before video split.
#ifdef DUALPLAYFIELD
  MOVE(BPL1MOD, VSPLTMOD_V),                  //              Set video split mods
  WAIT_PH(0, 0),                              // CL_VIDSPLT2  Wait for the video split
  WAIT(0, 0),                                 //                "   "   "    "     "
  MOVE_PH(BPL1PTH, 0),                        // CL_BPL1PTH_2 Set the high words of...
  MOVE(BPL3PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL5PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL7PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL1MOD, BPLXMOD_V),                   //              Reset video split mod
#else
  MOVE(BPL1MOD, VSPLTMOD_V),                  //              Set video split mods
  MOVE(BPL2MOD, VSPLTMOD_V),                  //               "    "     "    "
  WAIT_PH(0, 0),                              // CL_VIDSPLT2  Wait for the video split
  WAIT(0, 0),                                 //                "   "   "    "     "
  MOVE_PH(BPL1PTH, 0),                        // CL_BPL1PTH_2 Set the high words of...
  MOVE(BPL2PTH, 0),                           //              ...bitplane addresses
  MOVE(BPL3PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL4PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL5PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL6PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL7PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL8PTH, 0),                           //               "   "   "     "   "
  MOVE(BPL1MOD, BPLXMOD_V),                   //              Reset video split mod
  MOVE(BPL2MOD, BPLXMOD_V),                   //                "     "     "    "
#endif
  WAIT_PH(0, 0),                              // CL_ENDWAIT   Wait for the end of screen
  WAIT(0, SCREEN_END_WAIT),                   //                "   "   "   "  "    "
  // A BOTTOM PANEL CAN BE IMPLEMENTED HERE IN NON-DYNAMIC_COPPERLIST MODE
#if BOTTOM_PANEL_HEIGHT > 0
  MOVE_PH(COP2LCH, 0),                        // BP_COP2LCH   Reset COP2LCH
  MOVE(COP2LCL, 0),                           //                "      "
  MOVE(DMACON,  0x100),                       //              Disable Bitplane DMA
  MOVE(DDFSTRT, DEFAULT_DDFSTRT_LORES),       //              Set Data Fetch Start
  MOVE(DDFSTOP, DEFAULT_DDFSTOP_LORES),       //              Set Data Fetch Stop
  MOVE(BPLCON0, BPLCON0_V_BP),                //              Set display properties for bottom panel
  MOVE(BPLCON1, 0),                           //              No scroll
  MOVE(BPLCON2, 0x23F),                       //              Default plane priorities
  MOVE(BPLCON3, BPLCON3_BRDNBLNK),            //              Border blank only
  MOVE(BPLCON4, 0x0),                         //              Default sprite colors
  MOVE(BPL1MOD, BPLXMOD_BP),                  //              Set bitplane mods for bottom panel
  MOVE(BPL2MOD, BPLXMOD_BP),                  //               "     "       "   "   "    "
  MOVE_PH(BPL1PTH, 0),                        // BP_BPL1PTH   Set bitplane addresses
  MOVE(BPL1PTL, 0),                           //               "      "       "
  MOVE(BPL2PTH, 0),                           //               "      "       "
  MOVE(BPL2PTL, 0),                           //               "      "       "
  MOVE(BPL3PTH, 0),                           //               "      "       "
  MOVE(BPL3PTL, 0),                           //               "      "       "
  MOVE(BPL4PTH, 0),                           //               "      "       "
  MOVE(BPL4PTL, 0),                           //               "      "       "
  MOVE(BPL5PTH, 0),                           //               "      "       "
  MOVE(BPL5PTL, 0),                           //               "      "       "
  MOVE(BPL6PTH, 0),                           //               "      "       "
  MOVE(BPL6PTL, 0),                           //               "      "       "
  MOVE(BPL7PTH, 0),                           //               "      "       "
  MOVE(BPL7PTL, 0),                           //               "      "       "
  MOVE(BPL8PTH, 0),                           //               "      "       "
  MOVE(BPL8PTL, 0),                           //               "      "       "
  WAIT(BTM_PNL_STRT_W_X, BTM_PNL_STRT_W_Y),   //              Enable Bitplane DMA back
  MOVE(DMACON, 0x8100),                       //              Enable Bitplane DMA back
#endif
  END_PH                                      // CL_END       Terminate copperlist
};

#if BOTTOM_PANEL_HEIGHT > 0
STATIC ULONG end_Instructions[] = {
  WAIT(0, SCREEN_END_WAIT),                   //              Wait for the panel start
  MOVE(DMACON,  0x120),                       //              Disable Bitplane
  MOVE(DDFSTRT, DEFAULT_DDFSTRT_LORES),       //              Set Data Fetch Start
  MOVE(DDFSTOP, DEFAULT_DDFSTOP_LORES),       //              Set Data Fetch Stop
  MOVE(BPLCON0, BPLCON0_V_BP),                //              Set display properties for bottom panel
  MOVE(BPLCON1, 0),                           //              No scroll
  MOVE(BPLCON2, 0x23F),                       //              Default plane priorities
  MOVE(BPLCON3, BPLCON3_BRDNBLNK),            //              Border blank only
  MOVE(BPLCON4, 0x0),                         //              Default sprite colors
  MOVE(BPL1MOD, BPLXMOD_BP),                  //              Set bitplane mods for bottom panel
  MOVE(BPL2MOD, BPLXMOD_BP),                  //               "     "       "   "   "    "
  MOVE(SPR0CTL, 0),                           //               "     "      "
  MOVE(SPR1CTL, 0),                           //               "     "      "
  MOVE(SPR2CTL, 0),                           //               "     "      "
  MOVE(SPR3CTL, 0),                           //               "     "      "
  MOVE(SPR4CTL, 0),                           //               "     "      "
  MOVE(SPR5CTL, 0),                           //               "     "      "
  MOVE(SPR6CTL, 0),                           //               "     "      "
  MOVE(SPR7CTL, 0),                           //               "     "      "
  MOVE(BPL1PTH, 0),                           //              Set bitplane addresses
  MOVE(BPL1PTL, 0),                           //               "      "       "
  MOVE(BPL2PTH, 0),                           //               "      "       "
  MOVE(BPL2PTL, 0),                           //               "      "       "
  MOVE(BPL3PTH, 0),                           //               "      "       "
  MOVE(BPL3PTL, 0),                           //               "      "       "
  MOVE(BPL4PTH, 0),                           //               "      "       "
  MOVE(BPL4PTL, 0),                           //               "      "       "
  MOVE(BPL5PTH, 0),                           //               "      "       "
  MOVE(BPL5PTL, 0),                           //               "      "       "
  MOVE(BPL6PTH, 0),                           //               "      "       "
  MOVE(BPL6PTL, 0),                           //               "      "       "
  MOVE(BPL7PTH, 0),                           //               "      "       "
  MOVE(BPL7PTL, 0),                           //               "      "       "
  MOVE(BPL8PTH, 0),                           //               "      "       "
  MOVE(BPL8PTL, 0),                           //               "      "       "
  WAIT(BTM_PNL_STRT_W_X, BTM_PNL_STRT_W_Y),   //              Enable Bitplane DMA back
  MOVE(DMACON, 0x8100),                       //              Enable Bitplane DMA back
  END
};
#endif
///

///openScreen()
STATIC struct BitMap* openScreen()
{
  struct BitMap* bm = NULL;

  #ifdef DUALPLAYFIELD
  level_bitmap_pf2 = allocBitMap(BITMAP_WIDTH_PF2,
                                 BITMAP_HEIGHT_PF2,
                                 BITMAP_DEPTH_PF2,
                                 BMF_STANDARD | BMF_INTERLEAVED_PF2 | BMF_DISPLAYABLE | BMF_CLEAR,
                                 0);

  if (level_bitmap_pf2) {
  #endif

    bm = allocBitMap(BITMAP_WIDTH,
                     BITMAP_HEIGHT + 1,  // +1 pixelline for horizontal scroll buffer
                     SCREEN_DEPTH,
                     BMF_STANDARD | BMF_INTERLEAVED | BMF_DISPLAYABLE | BMF_CLEAR,
                     0);

    if (bm) {
      // check if the bitmap really is in chip memory and is really interleaved
      if ((TypeOfMem(bm->Planes[0]) & MEMF_CHIP) && (GetBitMapAttr(bm, BMA_FLAGS) & BMF_INTERLEAVED)) {
        level_bitmap = bm;
        bitmapTop = bm->Planes[0] + DISPLAY_BUFFER_OFFSET_BYTES;
        bitmapBottom = bitmapTop + (BITMAP_HEIGHT * SCREEN_DEPTH) * BITMAP_BYTES_PER_ROW;

        #if TOP_PANEL_HEIGHT > 0
        if ((top_panel_rastport = allocRastPort(SCREEN_WIDTH, TOP_PANEL_HEIGHT, TOP_PANEL_DEPTH,
                                                BMF_STANDARD | BMF_INTERLEAVED | BMF_DISPLAYABLE | BMF_CLEAR,
                                                bm, RPF_BITMAP, 0))) {
          //All allocations OK
          SetRast(top_panel_rastport, 2);
        }
        else {
          FreeBitMap(bm); bm = NULL;
          #ifdef DUALPLAYFIELD
          FreeBitMap(level_bitmap_pf2); level_bitmap_pf2 = NULL;
          #endif
          puts("Couldn't allocate top_panel_rastport!");
        }
        #endif

        #if BOTTOM_PANEL_HEIGHT > 0
          #if TOP_PANEL_HEIGHT > 0
          if (top_panel_rastport) {
          #endif
            if ((bottom_panel_rastport = allocRastPort(SCREEN_WIDTH, BOTTOM_PANEL_HEIGHT, BOTTOM_PANEL_DEPTH,
                                                       BMF_STANDARD | BMF_INTERLEAVED | BMF_DISPLAYABLE | BMF_CLEAR,
                                                       bm, RPF_BITMAP, 0))) {
              //All allocations OK
              SetRast(bottom_panel_rastport, 3);

              #ifdef DYNAMIC_COPPERLIST
              {
                UWORD* wp;
                ULONG i;

                //locate the BPL1PTH instruction
                for (wp = (UWORD*)end_Instructions; *wp != (UWORD)(MOVE(BPL1PTH, 0) >> 16); wp += 2);
                wp++;

                //set bitplane addresses on end_Instructions
                for (i = 0; i < BOTTOM_PANEL_DEPTH; i++)
                {
                  ULONG plane = ((ULONG)bottom_panel_rastport->BitMap->Planes[i]);

                  *wp = (UWORD)(plane >> 16);     wp += 2;
                  *wp = (UWORD)(plane & 0xFFFF);  wp += 2;
                }
              }

              #endif
            }
            else {
              #if TOP_PANEL_HEIGHT > 0
              freeRastPort(top_panel_rastport, RPF_BITMAP); top_panel_rastport = NULL;
              #endif
              FreeBitMap(bm); bm = NULL;
              #ifdef DUALPLAYFIELD
              FreeBitMap(level_bitmap_pf2); level_bitmap_pf2 = NULL;
              #endif
              puts("Couldn't allocate bottom_panel_rastport!");
            }
          #if TOP_PANEL_HEIGHT > 0
          }
          #endif
        #endif
      }
      else {
        puts("Couldn't allocate an interleaved screen bitmap!");
        FreeBitMap(bm); bm = NULL;
        #ifdef DUALPLAYFIELD
        FreeBitMap(level_bitmap_pf2); level_bitmap_pf2 = NULL;
        #endif
      }
    }
    else {
      puts("Not enough memory for screen bitmap!");
      #ifdef DUALPLAYFIELD
      FreeBitMap(level_bitmap_pf2); level_bitmap_pf2 = NULL;
      #endif
    }
#ifdef DUALPLAYFIELD
  }
  else puts("Not enough memory for background bitmap!");
#endif

  return bm;
}
///
///closeScreen()
STATIC VOID closeScreen()
{
  if (level_bitmap) {
    WaitBlit();

    #if BOTTOM_PANEL_HEIGHT > 0
    #ifdef DYNAMIC_COPPERLIST
      freeRainbow(rainbow); rainbow = NULL;
    #endif
    freeRastPort(bottom_panel_rastport, RPF_BITMAP);
    #endif

    #if TOP_PANEL_HEIGHT > 0
    freeRastPort(top_panel_rastport, RPF_BITMAP);
    #endif

    FreeBitMap(level_bitmap); level_bitmap = NULL;
    #ifdef DUALPLAYFIELD
    FreeBitMap(level_bitmap_pf2); level_bitmap_pf2 = NULL;
    #endif
  }
}
///
///prepGradients(level_num)
#ifdef DYNAMIC_COPPERLIST
#include "level_display_gradients.c"
#endif
///
///openDisplay(level_num)
STATIC BOOL openDisplay(ULONG level_num)
{
  if (openScreen()) {
#ifdef DYNAMIC_COPPERLIST
    if (prepGradients(level_num)) {
  #if BOTTOM_PANEL_HEIGHT > 0
      // if no rainbow is created for the display open one because bottom panel requires one
      if (!rainbow) rainbow = createRainbow(NULL, end_Instructions);
  #endif
#endif
      if (createCopperList()) {
        return TRUE;
      }
#ifdef DYNAMIC_COPPERLIST
    }
#endif
  }

  closeDisplay();
  return FALSE;
}
///
///closeDisplay()
STATIC VOID closeDisplay()
{
  disposeCopperList();
#ifdef DYNAMIC_COPPERLIST
  freeGradients();
#endif
  closeScreen();
  color_table = NULL; // actual color_tables are freed by unloadLevel()
}
///
///fillDisplay()
STATIC VOID fillDisplay(UWORD posX, UWORD posY)
{
  WORD x;
  WORD y;
  WORD mapX;
  WORD mapY;
  WORD xStep;
  WORD vpx;
  WORD vpy;

  // Clamp the coordinates down to not exceed map limits!
  if (posX > *maxMapPosX)
    posX = *maxMapPosX;
  if (posY > *maxMapPosY)
    posY = *maxMapPosY;

  // Precalculate the state variables which are calculated incrementally to a
  // position that is a multiple of TILESIZE
  mapTileX = posX >> TILESIZE_BSMD;
  mapTileY = posY >> TILESIZE_BSMD;
  *mapPosX = mapTileX << TILESIZE_BSMD;
  *mapPosY = mapTileY << TILESIZE_BSMD;
  mapPosX_s = *mapPosX;
  mapPosY_s = *mapPosY;
  *mapPosX2 = *mapPosX + SCREEN_WIDTH - 1;
  *mapPosY2 = *mapPosY + SCREEN_HEIGHT - 1;
  vidPosX = *mapPosX % (BITMAP_WIDTH * SCREEN_DEPTH);
  vidPosX_s = vidPosX;
  xStep = *mapPosX / (BITMAP_WIDTH * SCREEN_DEPTH);
  vidPosY = (*mapPosY + xStep) % BITMAP_HEIGHT;
  fillUpRowPos = vidPosY;

  // Fill the display with tiles at this display position
  vpx = vidPosX % BITMAP_WIDTH;
  vpy = vidPosX / BITMAP_WIDTH;

  initTileBlit();

  for (x = vpx, mapX = mapTileX; x < BITMAP_WIDTH; x += TILESIZE, mapX++) {
    for (y = vidPosY * SCREEN_DEPTH + vpy, mapY = mapTileY; y < BITMAP_HEIGHT * SCREEN_DEPTH; y += (SCREEN_DEPTH * TILESIZE), mapY++) {
      blitTile(x, y, mapX, mapY);
    }
    for (y = vpy + (xStep * SCREEN_DEPTH); y < (vidPosY - xStep) * SCREEN_DEPTH + vpy; y += (SCREEN_DEPTH * TILESIZE), mapY++) {
      blitTile(x, y, mapX, mapY);
    }
  }
  for (x = 0; x < vpx; x += TILESIZE, mapX++) {
    for (y = vidPosY * SCREEN_DEPTH + vpy + 1, mapY = mapTileY; y < BITMAP_HEIGHT * SCREEN_DEPTH + 1; y += (SCREEN_DEPTH * TILESIZE), mapY++) {
      blitTile(x, y, mapX, mapY);
    }
    for (y = vpy + (xStep * SCREEN_DEPTH) + 1; y < (vidPosY - xStep) * SCREEN_DEPTH + vpy + 1; y += (SCREEN_DEPTH * TILESIZE), mapY++) {
      blitTile(x, y, mapX, mapY);
    }
  }

  if (vpy) blitHScrollLine(DOWN);

  //Now scroll the remaining pixels
  stepX = 0; // NOTE: resetting these here is essential if we design to recall
  stepY = 0; // this function during game (ie. to teleport to some location).
  lastHDir = NONE;
  scrollDown(posY % TILESIZE);
  scrollRight(posX % TILESIZE);
}
///
///initLevelDisplay()
/******************************************************************************
 * Bresenham discrete distributor                                             *
 * This is to used for setting the number tiles to blit per scroll steps.     *
 ******************************************************************************/
VOID distributeItems(UWORD num_items, UBYTE* containers, UWORD num_containers)
{
  // Base amount every container gets
  UWORD base_amount = num_items / num_containers;
  UWORD remainder = num_items % num_containers; // Leftover items to distribute
  UWORD i;

  // Assign the base amount to all containers
  for (i = 0; i < num_containers; i++) containers[i] = base_amount;

  // Spread out the remainder items as evenly as possible
  if (remainder) {
    UWORD step = (num_containers * 2) / remainder; // Use fixed-point step
    UWORD pos = 0;

    for (i = 0; i < remainder; i++) {
      containers[pos / 2]++; // Convert fixed-point to index and add extra item
      pos += step;           // Increment fixed-point position
    }
  }
}

VOID calcTileOffsets(UBYTE* tilesPerStep, UBYTE* posPerStep, UWORD num_steps, UWORD offs)
{
  UWORD i;

  posPerStep[0] = offs;

  for (i = 1; i < num_steps; i++) {
    posPerStep[i] = tilesPerStep[i - 1] + posPerStep[i - 1];
  }
}

STATIC VOID initLevelDisplay()
{
  if (BOBsBackBuffer) bobs_back_buffer_mod = BOBsBackBuffer->BytesPerRow / BOBsBackBuffer->Depth;
  color_table = current_level.color_table[current_level.current.color_table];
  color_table->state = CT_FADE_IN;
  tileset = current_level.tileset[current_level.current.tileset];
  map = current_level.tilemap[current_level.current.tilemap];

  mapPosX = &map->mapPosX;
  mapPosY = &map->mapPosY;
  mapPosX2 = &map->mapPosX2;
  mapPosY2 = &map->mapPosY2;
  maxMapPosX = &map->maxMapPosX;
  maxMapPosY = &map->maxMapPosY;

  //Set the number of tiles to blit per scroll step
  distributeItems(BITMAP_TILES_PER_COL - 2, tilesPerStepX, NUMSTEPS_X - 1);
  tilesPerStepX[NUMSTEPS_X - 1] = 1; //last step must always have a tile blit
  distributeItems(BITMAP_TILES_PER_ROW, tilesPerStepY, NUMSTEPS_Y);

  //Now set the offsets for the next tile blit at each scroll step
  calcTileOffsets(tilesPerStepX, posPerStepX, NUMSTEPS_X, 1);
  calcTileOffsets(tilesPerStepY, posPerStepY, NUMSTEPS_Y, 0);
}
///
///initTileBlit()
STATIC VOID initTileBlit()
{
  busyWaitBlit();
  custom.bltcon0 = 0x9F0; // use A and D. Op: D = A (direct copy)
  custom.bltcon1 = 0;
  custom.bltafwm = 0xFFFF;
  custom.bltalwm = 0xFFFF;
  custom.bltamod = 0;
  custom.bltdmod = BITMAP_BYTES_PER_ROW - (TILESIZE / 8);
}
///
///blitTile(x, y, mapX, mapY)
STATIC VOID blitTile(ULONG x, ULONG y, ULONG mapX, ULONG mapY)
{
  // get the tile at this map position
  TILEID tileID = map->data[mapY * map->width + mapX];
  struct Tile *tile = &tileset->tiles[tileID];

  x = (x >> 3) & 0xFFFE; // convert x coord to byte offset

  if (y <= (BITMAP_HEIGHT - TILESIZE) * SCREEN_DEPTH) {
    // blit does NOT collide with video split (bottom of bitmap)
    busyWaitBlit();

    custom.bltapt  = tile;
    custom.bltdpt  = bitmapTop + y * BITMAP_BYTES_PER_ROW + x;

    custom.bltsize = ((TILESIZE * TILEDEPTH) << 6) + (TILESIZE / 16);
  }
  else {
    // blit DOES collide with video split
    // OPTIMIZE: Theoretically tile blits never collide with video split
    //           when TILESIZE is 16 or 8. But this is only true if the mapsize
    //           in pixels does not exceed BITMAP_WIDTH * SCREEN_DEPTH.
    //           So beware!
    busyWaitBlit();

    custom.bltapt = tile;
    custom.bltdpt = bitmapTop + y * BITMAP_BYTES_PER_ROW + x;

    y = (BITMAP_HEIGHT * SCREEN_DEPTH) - y;
    custom.bltsize = (y << 6) + (TILESIZE / 16);

    busyWaitBlit();

    custom.bltdpt  = bitmapTop + x;
    custom.bltsize = (((TILESIZE * TILEDEPTH) - y) << 6) + (TILESIZE / 16);
  }
}
///
///blitHScrollLine(pos)
STATIC VOID blitHScrollLine(ULONG pos)
{
  busyWaitBlit();

  custom.bltdmod = 0;
  custom.bltapt  = (pos == UP) ? bitmapBottom : bitmapTop;
  custom.bltdpt  = (pos == UP) ? bitmapTop : bitmapBottom;

  custom.bltsize = ((1 * SCREEN_DEPTH) << 6) + (BITMAP_BYTES_PER_ROW / 2);
  busyWaitBlit();
  custom.bltdmod = BITMAP_BYTES_PER_ROW - (TILESIZE / 8);
}
///

///scrollUp()
STATIC UWORD scrollUp(UWORD pixels)
{
  UWORD boundary = stepY; //*mapPosY & (TILESIZE - 1);
  UWORD firstPartPixels;
  UWORD secondPartPixels;

  if (pixels > *mapPosY) pixels = *mapPosY;
  *mapPosY  -= pixels;
  *mapPosY2 -= pixels;

  vidPosY -= pixels;
  if (vidPosY < 0) vidPosY += BITMAP_HEIGHT;

  if (pixels > boundary) {
    firstPartPixels = boundary;
    secondPartPixels = pixels - boundary;
  }
  else {
    firstPartPixels = pixels;
    secondPartPixels = 0;
  }

  if (firstPartPixels) scrollUpPixels(firstPartPixels);
  return secondPartPixels;
}

STATIC VOID scrollUpPixels(UWORD pixels)
{
	while (pixels--) {
		WORD mapx;
    WORD mapy;
    WORD x;
    WORD y;
    UWORD num_blits;

		mapPosY_s--;
		mapTileY = mapPosY_s >> TILESIZE_BSMD; // mapPosY_s / TILEHEIGHT
		stepY = mapPosY_s & (NUMSTEPS_Y - 1);

		if (stepY == (NUMSTEPS_Y - 1)) {
			// a complete row is filled up
			// : the next fill up row will be TILEHEIGHT (16)
			// pixels at the top, so we have to adjust
			// the fillup column (for x scrolling), but
			// only if the fillup column (x) contains some
			// fill up blocks

      fillUpRowPos -= TILEHEIGHT;
      if (fillUpRowPos < 0) fillUpRowPos += BITMAP_HEIGHT;

			if (stepX) {
				// step 1: blit the 1st block in the fillup
				//         col (x). There can only be 0 or
				//         [2 or more] fill up blocks in the
				//         actual implementation, so we do
				//         not need to check lastHDir
				//         for this blit

				mapx = mapTileX + BITMAP_TILES_PER_ROW;
				mapy = mapTileY + 1;

				x = ROUND2TILESIZE(vidPosX_s);
				y = (fillUpRowPos + TILEHEIGHT) % BITMAP_HEIGHT;
				y *= TILEDEPTH;

				blitTile(x + BITMAP_WIDTH, y, mapx, mapy);

				// step 2: remove the (former) bottommost fill up
				// block

				if (lastHDir == RIGHT) {
					*saveWordPtr = saveWord;
				}

        mapy = posPerStepX[stepX];

				// we blit a 'left' block

				y = (fillUpRowPos + (mapy * TILEHEIGHT)) % BITMAP_HEIGHT;
				y *= TILEDEPTH;

				saveWordPtr = (WORD *)(bitmapTop + (y * BITMAP_BYTES_PER_ROW) + (x / 8));
				saveWord = *saveWordPtr;

				mapx -= BITMAP_TILES_PER_ROW;
				mapy += mapTileY;

				blitTile(x, y, mapx, mapy);

				lastHDir = LEFT;

			} /* if (stepX) */

		} /* if (stepY == NUMSTEPS_Y - 1) */

		mapy = mapTileY;

		y = fillUpRowPos * TILEDEPTH;

    if ((num_blits = tilesPerStepY[stepY])) {

      mapx = posPerStepY[stepY];

      x = mapx * TILEWIDTH + ROUND2TILESIZE(vidPosX_s);

      mapx += mapTileX;

      blitTile(x, y, mapx, mapy);

      while (--num_blits) {
        x += TILEWIDTH;
        mapx++;

        blitTile(x, y, mapx, mapy);
      }
    }
	}
}
///
///scrollDown()
STATIC UWORD scrollDown(UWORD pixels)
{
  UWORD boundary = TILESIZE - stepY - 1;
  UWORD firstPartPixels;
  UWORD secondPartPixels;

  if (pixels > (*maxMapPosY - *mapPosY)) pixels = *maxMapPosY - *mapPosY;
  *mapPosY  += pixels;
  *mapPosY2 += pixels;

  vidPosY += pixels;
  if (vidPosY >= BITMAP_HEIGHT) vidPosY -= BITMAP_HEIGHT;

  if (pixels > boundary) {
    firstPartPixels = boundary;
    secondPartPixels = pixels - boundary;
  }
  else {
    firstPartPixels = pixels;
    secondPartPixels = 0;
  }

  if (firstPartPixels) scrollDownPixels(firstPartPixels);
  return secondPartPixels;
}

STATIC VOID scrollDownPixels(UWORD pixels)
{
	while (pixels--) {
    WORD mapx;
    WORD mapy;
    WORD x;
    WORD y;
    WORD y2;
    UWORD num_blits;

		mapy = mapTileY + BITMAP_TILES_PER_COL;

		y = fillUpRowPos * TILEDEPTH;

    if ((num_blits = tilesPerStepY[stepY])) {

      mapx = posPerStepY[stepY];

      x = mapx * TILEWIDTH + ROUND2TILESIZE(vidPosX_s);

      mapx += mapTileX;

      blitTile(x, y, mapx, mapy);

      while (--num_blits) {
        x += TILEWIDTH;
        mapx++;

        blitTile(x, y, mapx, mapy);
      }
    }

		mapPosY_s++;
    mapTileY = mapPosY_s >> TILESIZE_BSMD; // mapPosY_s / TILEHEIGHT
		stepY = mapPosY_s & (NUMSTEPS_Y - 1);

		if (!stepY) {
			// a complete row is filled up
			// : the next fill up row will be TILEHEIGHT (16)
			// pixels at the bottom, so we have to adjust
			// the fillup column (for x scrolling), but
			// only if the fillup column (x) contains some
			// fill up blocks

      fillUpRowPos += TILEHEIGHT;
      if (fillUpRowPos >= BITMAP_HEIGHT) fillUpRowPos -= BITMAP_HEIGHT;

			if (stepX) {
				// step 1: blit the 1st block in the fillup
				//         row (y) because this block must
				//         not be part of the fillup col (x)
				//         instead it is for exclusive use
				//         by the fillup row

				mapx = mapTileX;
				mapy = mapTileY;

				x = ROUND2TILESIZE(vidPosX_s);
				y = fillUpRowPos * TILEDEPTH;

				blitTile(x, y, mapx, mapy);

				// step 2: blit the (new) bottommost fill up
				// block

				if (lastHDir == LEFT) {
					*saveWordPtr = saveWord;
				}

        mapy = posPerStepX[stepX] - 1;

				// we blit a 'right-block'

				x += BITMAP_WIDTH;

				y = (fillUpRowPos + (mapy * TILEHEIGHT)) % BITMAP_HEIGHT;
				y *= TILEDEPTH;

				y2 = (y + TILE_PLANE_LINES - 1) % BITMAP_PLANE_LINES;

				saveWordPtr = (WORD *)(bitmapTop + (y2 * BITMAP_BYTES_PER_ROW) + (x / 8));
				saveWord = *saveWordPtr;

				mapx += BITMAP_TILES_PER_ROW;
				mapy += mapTileY;

				blitTile(x, y, mapx, mapy);

				lastHDir = RIGHT;
			} /* if (stepX) */

		} /* if (stepY == 0) */
	}
}
///
///scrollLeft()
STATIC UWORD scrollLeft(UWORD pixels)
{
  UWORD boundary = stepX + 1; // *mapPosX & (TILESIZE - 1) + 1 NOTE: +1 is to keep blitHScrollLine() in firstPartPixels
  UWORD firstPartPixels;
  UWORD secondPartPixels;

  if (pixels > *mapPosX) pixels = *mapPosX;
  *mapPosX  -= pixels;
  *mapPosX2 -= pixels;

  vidPosX -= pixels;
  if (vidPosX < 0)
  {
    vidPosX += (BITMAP_WIDTH * SCREEN_DEPTH);

    vidPosY--;
    if (vidPosY < 0) vidPosY = BITMAP_HEIGHT - 1;
  }

  if (pixels > boundary) {
    firstPartPixels = boundary;
    secondPartPixels = pixels - boundary;

    if (firstPartPixels) scrollLeftPixels(firstPartPixels);
  }
  else {
    firstPartPixels = pixels;
    secondPartPixels = 0;

    if (firstPartPixels) scrollLeftPixels(firstPartPixels);
  }

  return secondPartPixels;
}

STATIC VOID scrollLeftPixels(UWORD pixels)
{
	while (pixels--) {
    WORD mapx;
    WORD mapy;
    WORD x;
    WORD y;
    UWORD num_blits;

		mapPosX_s--;
		mapTileX = mapPosX_s >> TILESIZE_BSMD; // mapPosX_s / TILEWIDTH;
		stepX = mapPosX_s & (NUMSTEPS_X - 1);

		vidPosX_s--;
		if (vidPosX_s < 0) {
			blitHScrollLine(DOWN);

			vidPosX_s = PLANE_PIXEL_BITMAP_WIDTH - 1;

			fillUpRowPos--;
			if (fillUpRowPos < 0) fillUpRowPos = BITMAP_HEIGHT - 1;
		}

		if (stepX == (NUMSTEPS_X - 1)) {
			// a complete column is filled up
			// : the next fill up column will be TILEWIDTH (16)
			// pixels at the left, so we have to adjust
			// the fillup row (for y scrolling)

			// step 1: blit the block which came in at
			//         the left side and which might or
			//         might not be a fill up block

			mapx = mapTileX;
			mapy = mapTileY;

			if (stepY) {
				// there is a fill up block
				// so block which comes in left is
				// a fillup block

				mapy += BITMAP_TILES_PER_COL;
			}

			x = ROUND2TILESIZE(vidPosX_s);
			y = fillUpRowPos * TILEDEPTH;

			blitTile(x, y, mapx, mapy);

			// step 2: remove the (former) rightmost fillup-block

			if (stepY) {
				// there is a fill up block;
        mapx = posPerStepY[stepY];

				x = ROUND2TILESIZE(vidPosX_s) + (mapx * TILEWIDTH);
				y = fillUpRowPos * TILEDEPTH;

				mapx += mapTileX;
				mapy -= BITMAP_TILES_PER_COL;
				blitTile(x, y, mapx, mapy);
			}
		}

		mapx = mapTileX;
		mapy = posPerStepX[stepX];

		x = ROUND2TILESIZE(vidPosX_s);

		if (lastHDir == RIGHT) {
			busyWaitBlit();
			*saveWordPtr = saveWord;
		}

    if ((num_blits = tilesPerStepX[stepX])) {

      y = (fillUpRowPos + (mapy * TILEHEIGHT)) % BITMAP_HEIGHT;
      y *= TILEDEPTH;

      saveWordPtr = (WORD *)(bitmapTop + (y * BITMAP_BYTES_PER_ROW) + (x / 8));
      saveWord = *saveWordPtr;

      mapy += mapTileY;

      blitTile(x, y, mapx, mapy);

      while (--num_blits) {
        y = (y + TILE_PLANE_LINES) % BITMAP_PLANE_LINES;
        mapy++;

        blitTile(x, y, mapx, mapy);
      }
    }

    lastHDir = stepX ? LEFT : NONE;
	}
}
///
///scrollRight()
STATIC UWORD scrollRight(UWORD pixels)
{
  UWORD boundary = TILESIZE - stepX;
  UWORD firstPartPixels;
  UWORD secondPartPixels;

  if (pixels > (*maxMapPosX - *mapPosX)) pixels = *maxMapPosX - *mapPosX;
  *mapPosX  += pixels;
  *mapPosX2 += pixels;

  vidPosX += pixels;
  if (vidPosX >= (BITMAP_WIDTH * SCREEN_DEPTH)) {
    vidPosX -= (BITMAP_WIDTH * SCREEN_DEPTH);

    vidPosY++;
    if (vidPosY == BITMAP_HEIGHT) vidPosY = 0;
  }

  if (pixels > boundary) {
    firstPartPixels = boundary;
    secondPartPixels = pixels - boundary;

    if (firstPartPixels) scrollRightPixels(firstPartPixels);
  }
  else {
    firstPartPixels = pixels;
    secondPartPixels = 0;

    if (firstPartPixels) scrollRightPixels(firstPartPixels);
  }

  return secondPartPixels;
}

STATIC VOID scrollRightPixels(UWORD pixels)
{
	while (pixels--) {
    WORD mapx;
    WORD mapy;
    WORD x;
    WORD y;
    WORD y2;
    UWORD num_blits;

		mapx = mapTileX + BITMAP_TILES_PER_ROW;
		mapy = posPerStepX[stepX];

		x = ROUND2TILESIZE(vidPosX_s);

		if (lastHDir == LEFT) {
			busyWaitBlit();
			*saveWordPtr = saveWord;
		}

    if ((num_blits = tilesPerStepX[stepX])) {

      y = (fillUpRowPos + (mapy * TILEHEIGHT)) % BITMAP_HEIGHT;
      y *= TILEDEPTH;

      if (num_blits == 1) {
        y2 = (y + TILE_PLANE_LINES - 1) % BITMAP_PLANE_LINES;
        saveWordPtr = (WORD *)(bitmapTop + (y2 * BITMAP_BYTES_PER_ROW) + ((x + BITMAP_WIDTH) / 8));
        saveWord = *saveWordPtr;
      }

      mapy += mapTileY;
      blitTile(x + BITMAP_WIDTH, y, mapx, mapy);

      while (--num_blits) {
        y = (y + TILE_PLANE_LINES) % BITMAP_PLANE_LINES;
        mapy++;

        if (num_blits == 1) {
          y2 = (y + TILE_PLANE_LINES - 1) % BITMAP_PLANE_LINES;
          saveWordPtr = (WORD *)(bitmapTop + (y2 * BITMAP_BYTES_PER_ROW) + ((x + BITMAP_WIDTH) / 8));
          saveWord = *saveWordPtr;
        }

        blitTile(x + BITMAP_WIDTH, y, mapx, mapy);
      }
    }

		mapPosX_s++;
		mapTileX = mapPosX_s >> TILESIZE_BSMD; // mapPosX_s / TILEWIDTH
		stepX = mapPosX_s & (NUMSTEPS_X - 1);

		vidPosX_s++;
		if (vidPosX_s == PLANE_PIXEL_BITMAP_WIDTH) {
			blitHScrollLine(UP);

			vidPosX_s = 0;

			fillUpRowPos++;
			if (fillUpRowPos == BITMAP_HEIGHT) fillUpRowPos = 0;
		}

		if (!stepX) {
			// a complete column is filled up
			// : the next fill up column will be TILEWIDTH (16)
			// pixels at the right, so we have to adjust
			// the fillup row (for y scrolling)

			// step 1: blit the block which came in at
			//         the right side and which is never
			//         a fill up block

			mapx = mapTileX + BITMAP_TILES_PER_ROW - 1;
			mapy = mapTileY;

			x = ROUND2TILESIZE(vidPosX_s) + (BITMAP_TILES_PER_ROW - 1) * TILEWIDTH;
			y = fillUpRowPos * TILEDEPTH;

			blitTile(x, y, mapx, mapy);

			// step 2: blit the (new) rightmost fillup-block

			if (stepY) {
				// there is a fill up block;

        mapx = posPerStepY[stepY] - 1;

				x = ROUND2TILESIZE(vidPosX_s) + (mapx * TILEWIDTH);
				y = fillUpRowPos * TILEDEPTH;

				mapx += mapTileX;

				blitTile(x, y, mapx, mapy + BITMAP_TILES_PER_COL);
			}
		}

    lastHDir = stepX ? RIGHT : NONE;
	}
}
///
///scroll(scrollInfo)
STATIC INLINE VOID scroll(struct ScrollInfo* si)
{
  if (si->up    > MAX_SCROLL_SPEED) si->up    = MAX_SCROLL_SPEED;
  if (si->down  > MAX_SCROLL_SPEED) si->down  = MAX_SCROLL_SPEED;
  if (si->left  > MAX_SCROLL_SPEED) si->left  = MAX_SCROLL_SPEED;
  if (si->right > MAX_SCROLL_SPEED) si->right = MAX_SCROLL_SPEED;

  initTileBlit();
  si->up    = scrollUp(si->up);
  si->down  = scrollDown(si->down);
  si->left  = scrollLeft(si->left);
  si->right = scrollRight(si->right);
}
///
///scrollRemaining(scrollInfo)
STATIC INLINE VOID scrollRemaining(struct ScrollInfo* si)
{
  initTileBlit();
  if (si->up)    scrollUpPixels(si->up);
  if (si->down)  scrollDownPixels(si->down);
  if (si->left)  scrollLeftPixels(si->left);
  if (si->right) scrollRightPixels(si->right);

  si->up    = 0;
  si->down  = 0;
  si->left  = 0;
  si->right = 0;
}
///

///LD_blitBOB(gameobject)
VOID LD_blitBOB(struct GameObject* go)
{
  struct BOBImage* image = (struct BOBImage*)go->image;
  struct BOB* bob = (struct BOB*)go->medium;
  UBYTE* mask = (UBYTE*)image->mask;

  // Screen Coordinates
  LONG x  = go->x1 - *mapPosX;
  LONG y  = go->y1 - *mapPosY;
  LONG x2 = go->x2 - *mapPosX;
  LONG y2 = go->y2 - *mapPosY;

  UWORD x_sw  = 0; // horizontal skip words (the words to clip from the left side of the bob image: unit is in bytes)
  UWORD y_sr  = 0; // vertical skip rows (the rows to clip form to top of the bob image: unit is in pixellines)
  UWORD fwm = 0xFFFF;
  UWORD lwm = 0x0;

  UWORD words;  // how many words to blit (unit is in bytes)
  UWORD wordsB; // same as above but for background preservation
  UWORD rows;   // how many rows to blit (blit height)
  UWORD x_s;    // how many pixels to shift right the source on blitter (value should be shifted left 12 to go into BLTCONx)
  UWORD x_sB;   // same as above but for background preservation
  UWORD bx;     // bitmap coordinates
  UWORD by;     //   "         "
  UWORD amod;   // moduli for bob image (to go in to bltamod and bltbmod)
  UWORD dmod;   // moduli for the bitmap(to go in to bltcmod and bltdmod)
  UBYTE* bltbpt;

  words  = image->bytesPerRow + 2;
  wordsB = words;
  rows   = image->height;
  x_s = go->x1 & 0xF;
  x_sB = x_s ? 16 - x_s : 0;

  // Clip the bob image to screen boundaries
  if (x < 0) {
    x_sw = ((UWORD)(-x) >> 3) & 0xFFFE;
    fwm >>= (UWORD)(-x) & 0xF;
    x += x_sw << 3;
    words -= x_sw;
    wordsB = words;
  }
  if (y < 0) {
    y_sr = -y;
    rows -= y_sr;               // clip pixellines from the top of the bob image
    y = 0;
  }
  if (x2 > SCREEN_WIDTH) { //NOTE: re-consider this calculation
    UWORD wordsC = ((((*mapPosX2 - 1) & 0xFFFFFFF0) - (go->x1 & 0xFFFFFFF0)) >> 3) + 2 - x_sw;

    if (wordsC < words) {
      lwm = 0xFFFF << x_s;
      wordsB = wordsC + 2;
    }
    words = wordsC;
  }
  if (y2 > SCREEN_HEIGHT) {
    rows -= y2 - SCREEN_HEIGHT; // clip pixellines from the bottom of the bob image
    y2 = SCREEN_HEIGHT;
  }

  amod  = image->bob_sheet->BytesPerRow / image->bob_sheet->Depth - words; //OPTIMIZE: division should be precalculated into BOBImage struct
  dmod  = BITMAP_BYTES_PER_ROW - words;

  //Find bitmap coords
  bx = vidPosX + TILESIZE + x;
  by = vidPosY + TILESIZE + y;

  //preserve underlying background to bob's background buffer
  busyWaitBlit();
  {
    UBYTE* bltdpt = (UBYTE*)bob->background - 2 + y_sr * bobs_back_buffer_mod * image->bob_sheet->Depth + x_sw;
    UBYTE* bltapt = (UBYTE*)mask - 2 + y_sr * image->bob_sheet->BytesPerRow + x_sw;

    custom.bltcon0 = 0x0FCA; // use A = Mask, B = Source, C, D = Destination (cookie-cut)
    custom.bltcon1 = x_sB << 12;
    custom.bltafwm = 0x0;
    custom.bltalwm = 0xFFFF;
    custom.bltamod = image->bob_sheet->BytesPerRow / image->bob_sheet->Depth - wordsB; // OPTIMIZE: precalculate
    custom.bltbmod = BITMAP_BYTES_PER_ROW - wordsB;
    custom.bltcmod = bobs_back_buffer_mod - wordsB;
    custom.bltdmod = bobs_back_buffer_mod - wordsB;
    custom.bltapt  = bltapt;
    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;

    wordsB >>= 1;
    if ((by + rows) <= BITMAP_HEIGHT) {
      custom.bltbpt  = bitmapTop + by * (BITMAP_BYTES_PER_ROW * SCREEN_DEPTH) + (((bx - 1) >> 3) & 0xFFFE);
      custom.bltsize = ((rows * image->bob_sheet->Depth) << 6) | wordsB;
    }
    else if (by >= BITMAP_HEIGHT) {
      custom.bltbpt  = bitmapTop + (by - BITMAP_HEIGHT) * BITMAP_BYTES_PER_ROW * SCREEN_DEPTH + (((bx - 1) >> 3) & 0xFFFE);
      custom.bltsize = ((rows * image->bob_sheet->Depth) << 6) | wordsB;
    }
    else { // blit it in two parts because of the videosplit!
      UWORD firstPartRows = BITMAP_HEIGHT - by;
      UWORD skipRows = firstPartRows * bobs_back_buffer_mod * image->bob_sheet->Depth; // OPTIMIZE: precalculate

      custom.bltbpt  = bitmapTop + by * (BITMAP_BYTES_PER_ROW * SCREEN_DEPTH) + (((bx - 1) >> 3) & 0xFFFE);
      custom.bltsize = ((firstPartRows * image->bob_sheet->Depth) << 6) | wordsB;

      busyWaitBlit();

      custom.bltapt  = bltapt + firstPartRows * image->bob_sheet->BytesPerRow;
      custom.bltcpt  = bltdpt + skipRows;
      custom.bltdpt  = bltdpt + skipRows;
      custom.bltbpt  = bitmapTop + (((bx - 1) >> 3) & 0xFFFE);
      custom.bltsize = (((rows - firstPartRows) * image->bob_sheet->Depth) << 6) | wordsB;
    }
  }

  x_s <<= 12;
  bob->lastBlt.x = x + *mapPosX;
  bob->lastBlt.y = y + *mapPosY;
  bob->lastBlt.y2 = y2;
  bob->lastBlt.bob_sheet = image->bob_sheet;
  bob->lastBlt.x_s = x_s;
  bob->lastBlt.bltafwm = fwm;
  bob->lastBlt.bltalwm = lwm;
  bob->lastBlt.bltamod = amod;
  bob->lastBlt.bltbmod = bobs_back_buffer_mod - words;
  bob->lastBlt.bltdmod = dmod;
  bob->lastBlt.bltapt  = (UBYTE*)mask + y_sr * image->bob_sheet->BytesPerRow + x_sw;
  bob->lastBlt.bltbpt  = (UBYTE*)bob->background + y_sr * bobs_back_buffer_mod * image->bob_sheet->Depth + x_sw;

  words >>= 1;
  bob->lastBlt.words = words;
  bob->lastBlt.rows = rows;

  bltbpt = (UBYTE*)image->pointer + y_sr * image->bob_sheet->BytesPerRow + x_sw;

  waitVBeam(y2 + SCREEN_START);
  busyWaitBlit();

  custom.bltcon0 = 0x0FCA | x_s; // use A = Mask, B = Source, C, D = Destination (cookie-cut)
  custom.bltcon1 = x_s;
  custom.bltafwm = fwm;
  custom.bltalwm = lwm;
  custom.bltamod = amod;
  custom.bltbmod = amod;
  custom.bltcmod = dmod;
  custom.bltdmod = dmod;
  custom.bltapt  = bob->lastBlt.bltapt;
  custom.bltbpt  = bltbpt;

  // We are now ready to actually blit the image to the bitmap
  if ((by + rows) <= BITMAP_HEIGHT) {
    UBYTE* bltdpt = bitmapTop + by * (BITMAP_BYTES_PER_ROW * SCREEN_DEPTH) + ((bx >> 3) & 0xFFFE);

    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;
    custom.bltsize = ((rows * image->bob_sheet->Depth) << 6) | words;
  }
  else if (by >= BITMAP_HEIGHT) {
    UBYTE* bltdpt = bitmapTop + (by - BITMAP_HEIGHT) * BITMAP_BYTES_PER_ROW * SCREEN_DEPTH + ((bx >> 3) & 0xFFFE);

    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;
    custom.bltsize = ((rows * image->bob_sheet->Depth) << 6) | words;
  }
  else { // blit it in two parts because of the videosplit!
    UWORD firstPartRows = BITMAP_HEIGHT - by;
    UWORD skipRows = firstPartRows * image->bob_sheet->BytesPerRow;
    UBYTE* bltdpt = bitmapTop + by * BITMAP_BYTES_PER_ROW * SCREEN_DEPTH + ((bx >> 3) & 0xFFFE);

    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;
    custom.bltsize = ((firstPartRows * image->bob_sheet->Depth) << 6) | words;

    bltdpt = bitmapTop + ((bx >> 3) & 0xFFFE);
    busyWaitBlit();

    custom.bltapt  = bob->lastBlt.bltapt + skipRows;
    custom.bltbpt  = bltbpt + skipRows;
    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;
    custom.bltsize = (((rows - firstPartRows) * image->bob_sheet->Depth) << 6) | words;
  }
}
///
///LD_unBlitBOB(gameobject)
VOID LD_unBlitBOB(struct GameObject* go)
{
  struct BOB* bob = (struct BOB*)go->medium;

  UWORD bx = vidPosX + TILESIZE + bob->lastBlt.x - *mapPosX;
  UWORD by = vidPosY + TILESIZE + bob->lastBlt.y - *mapPosY;

  waitVBeam(bob->lastBlt.y2 + SCREEN_START);
  busyWaitBlit();

  custom.bltcon0 = 0x0FCA | bob->lastBlt.x_s; // use A = Mask, B = Source, C, D = Destination (cookie-cut)
  custom.bltcon1 = bob->lastBlt.x_s;
  custom.bltafwm = bob->lastBlt.bltafwm;
  custom.bltalwm = bob->lastBlt.bltalwm;
  custom.bltamod = bob->lastBlt.bltamod;
  custom.bltbmod = bob->lastBlt.bltbmod;
  custom.bltcmod = bob->lastBlt.bltdmod;
  custom.bltdmod = bob->lastBlt.bltdmod;
  custom.bltapt  = bob->lastBlt.bltapt;
  custom.bltbpt  = bob->lastBlt.bltbpt;

  if ((by + bob->lastBlt.rows) <= BITMAP_HEIGHT) {
    UBYTE* bltdpt = bitmapTop + by * (BITMAP_BYTES_PER_ROW * SCREEN_DEPTH) + ((bx >> 3) & 0xFFFE);

    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;
    custom.bltsize = ((bob->lastBlt.rows * bob->lastBlt.bob_sheet->Depth) << 6) | bob->lastBlt.words;
  }
  else if (by >= BITMAP_HEIGHT) {
    UBYTE* bltdpt = bitmapTop + (by - BITMAP_HEIGHT) * BITMAP_BYTES_PER_ROW * SCREEN_DEPTH + ((bx >> 3) & 0xFFFE);

    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;
    custom.bltsize = ((bob->lastBlt.rows * bob->lastBlt.bob_sheet->Depth) << 6) | bob->lastBlt.words;
  }
  else { // blit it in two parts because of the videosplit!
    UWORD firstPartRows = BITMAP_HEIGHT - by;
    UWORD skipRows = firstPartRows * bob->lastBlt.bob_sheet->BytesPerRow;
    UBYTE* bltdpt = bitmapTop + by * BITMAP_BYTES_PER_ROW * SCREEN_DEPTH + ((bx >> 3) & 0xFFFE);

    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;
    custom.bltsize = ((firstPartRows * bob->lastBlt.bob_sheet->Depth) << 6) | bob->lastBlt.words;

    bltdpt = bitmapTop + ((bx >> 3) & 0xFFFE);
    busyWaitBlit();
    custom.bltapt  = bob->lastBlt.bltapt + skipRows;
    custom.bltdpt  = bob->lastBlt.bltbpt + firstPartRows * bobs_back_buffer_mod * bob->lastBlt.bob_sheet->Depth;
    custom.bltcpt  = bltdpt;
    custom.bltdpt  = bltdpt;
    custom.bltsize = (((bob->lastBlt.rows - firstPartRows) * bob->lastBlt.bob_sheet->Depth) << 6) | bob->lastBlt.words;
  }

}
///

///vblankEvents()
/******************************************************************************
 * This should be moved to the display.c of level display.                    *
 ******************************************************************************/
STATIC VOID vblankEvents(VOID)
{
  updateCopperList();
  //No need to fade color 0
  setColorTable_REG(color_table, 1, color_table->colors);
}
///

///levelDisplayLoop()
#include "level_display_loop.c"
///

///createCopperList()
/******************************************************************************
 * WARNING: Always to be called after a successful openDisplay()!             *
 ******************************************************************************/
#ifdef DYNAMIC_COPPERLIST
STATIC UWORD* createCopperList()
{
#ifdef DUALPLAYFIELD
  ULONG videoSplit_Instructions[] = {
    WAIT(0, 0),
    MOVE(BPL1MOD, VSPLTMOD_V),
    WAIT(0, 0),
    MOVE(BPL1PTH, 0),
    MOVE(BPL3PTH, 0),
    MOVE(BPL5PTH, 0),
    MOVE(BPL7PTH, 0),
    MOVE(BPL1MOD, BPLXMOD_V)
  };
  #define VSPLIT_COPOP0_SIZE 2
  #define VSPLIT_COPOP1_SIZE 6
#else
  ULONG videoSplit_Instructions[] = {
    WAIT(0, 0),
    MOVE(BPL1MOD, VSPLTMOD_V),
    MOVE(BPL2MOD, VSPLTMOD_V),
    WAIT(0, 0),
    MOVE(BPL1PTH, 0),
    MOVE(BPL2PTH, 0),
    MOVE(BPL3PTH, 0),
    MOVE(BPL4PTH, 0),
    MOVE(BPL5PTH, 0),
    MOVE(BPL6PTH, 0),
    MOVE(BPL7PTH, 0),
    MOVE(BPL8PTH, 0),
    MOVE(BPL1MOD, BPLXMOD_V),
    MOVE(BPL2MOD, BPLXMOD_V)
  };
  #define VSPLIT_COPOP0_SIZE 3
  #define VSPLIT_COPOP1_SIZE 11
#endif


  if (rainbow || (rainbow = empty_rainbow = createEmptyRainbow())) {

    // Get the instruction numbers for the actual allocation
    if (allocCopperList(copperList_Instructions, CopperList, CL_SINGLE)) {
      ULONG num_copperlist_instructions;
      ULONG num_cl_footer_instructions;
      ULONG num_cl_vsplit_instructions = sizeof(videoSplit_Instructions) / sizeof(CLINST);
#ifdef SMART_SPRITES
      ULONG num_cl_smartsprite_instructions = (NUM_SPRITES * 5);
#else
      ULONG num_cl_smartsprite_instructions = 0;
#endif
      num_cl_header_instructions = ((ULONG)CL_VIDSPLT - (ULONG)CopperList) / sizeof(CLINST);
      num_cl_footer_instructions = (((ULONG)CL_END - (ULONG)CL_VIDSPLT) / sizeof(CLINST)) + 1;
      num_copperlist_instructions = num_cl_header_instructions + rainbow->num_insts + num_cl_vsplit_instructions + num_cl_smartsprite_instructions;
      freeCopperList(CopperList); // numbers are gotten, get rid of this copperlist

      // Allocate the actual doublebuffered copperlist
      if (copperAllocator(copperList_Instructions, CL_NUM_INSTS(copperList_Instructions), &CopperList, CL_SINGLE,
        num_copperlist_instructions * 2
        + num_cl_vsplit_instructions
        + num_cl_smartsprite_instructions
        - CL_NUM_INSTS(copperList_Instructions))) {

        CopperList1 = CopperList;
        CopperList2 = (UWORD*)((ULONG)CopperList1 + num_copperlist_instructions * sizeof(CLINST));

        //Set top panel bitplanes
#if TOP_PANEL_HEIGHT > 0
        {
          UWORD* wp = TP_BPL1PTH;
          PLANEPTR* planes = top_panel_rastport->BitMap->Planes;
          ULONG i;

          for (i = 0; i < TOP_PANEL_DEPTH; i++)
          {
            ULONG plane = ((ULONG)planes[i]);

            *wp = (UWORD)(plane >> 16);     wp += 2;
            *wp = (UWORD)(plane & 0xFFFF);  wp += 2;
          }
        }
#endif

        //Copy header to the double buffer
        CopyMem(CopperList, CopperList2, num_cl_header_instructions * sizeof(CLINST));

        vsplit_list = (ULONG*)CopperList2 + num_copperlist_instructions;

        CopyMem(videoSplit_Instructions, vsplit_list, num_cl_vsplit_instructions * sizeof(CLINST));

        //Set the COP2LCH pointers on both buffers
        CL_COP2LCH_1 = CL_COP2LCH;
        CL_COP2LCH_2 = (UWORD*)((ULONG)CL_COP2LCH_1 + num_copperlist_instructions * sizeof(CLINST));

        *CL_COP2LCH_1 = (UWORD)((ULONG)CopperList2 >> 16);
        *(CL_COP2LCH_1 + 2) = (UWORD)((ULONG)CopperList2 & 0xFFFF);
        *CL_COP2LCH_2 = (UWORD)((ULONG)CopperList1 >> 16);
        *(CL_COP2LCH_2 + 2) = (UWORD)((ULONG)CopperList1 & 0xFFFF);

        //In doublebuffered copperlist the access pointers to the header are offsets
        CL_BPLCON1 = (UWORD*)((ULONG)CL_BPLCON1 - (ULONG)CopperList);
        CL_BPL1PTH = (UWORD*)((ULONG)CL_BPL1PTH - (ULONG)CopperList);
        CL_BPL1PTL = (UWORD*)((ULONG)CL_BPL1PTL - (ULONG)CopperList);

#ifdef SMART_SPRITES
        CL_SPR0POS = (UWORD*)((ULONG)CL_SPR0POS - (ULONG)CopperList);
        CL_SPR0PTH = (UWORD*)((ULONG)CL_SPR0PTH - (ULONG)CopperList);
  #if TOP_PANEL_HEIGHT > 0
        //disable OS's mouse pointer
        *TP_SPR0PTH = NULL_SPRITE_ADDRESS_H;
        *(TP_SPR0PTH + 2) = NULL_SPRITE_ADDRESS_L;
  #endif
#else
        CL_SPR0PTH = (UWORD*)((ULONG)CL_SPR0PTH - (ULONG)CopperList);
#endif

        CL_VIDSPLT = (UWORD*)vsplit_list;
        CL_VIDSPLT2 = (UWORD*)(vsplit_list + VSPLIT_COPOP0_SIZE);
        CL_BPL1PTH_2 = CL_VIDSPLT2 + 3;

        vsplit_CopOps[0].size = VSPLIT_COPOP0_SIZE;
        vsplit_CopOps[0].pointer = vsplit_list;
        vsplit_CopOps[1].size = VSPLIT_COPOP1_SIZE;
        vsplit_CopOps[1].pointer = vsplit_list + VSPLIT_COPOP0_SIZE;
        vsplit_CopOps[2].wait = 0xFFFF;

        sprite_CopOps[0].wait = 0xFFFF;
        sprite_CopOps_list[0] = &sprite_CopOps[0];

#ifdef SMART_SPRITES
        sprite_list = vsplit_list + num_cl_vsplit_instructions;
#endif

        // plane pointers
        planes = level_bitmap->Planes;
#ifdef DUALPLAYFIELD
        planes_pf2 = level_bitmap_pf2->Planes;
#endif

        CopperList = CopperList1;
        updateDynamicCopperList();

        return CopperList;
      }
    }
  }

  return NULL;
}
#else //DYNAMIC_COPPERLIST
STATIC UWORD* createCopperList()
{
  if (allocCopperList(copperList_Instructions, CopperList, CL_SINGLE)) {
    UWORD* sp;

    //Set top panel bitplanes
    #if TOP_PANEL_HEIGHT > 0
    {
      UWORD* wp = TP_BPL1PTH;
      PLANEPTR* planes = top_panel_rastport->BitMap->Planes;
      ULONG i;

      for (i = 0; i < TOP_PANEL_DEPTH; i++)
      {
        ULONG plane = ((ULONG)planes[i]);

        *wp = (UWORD)(plane >> 16);     wp += 2;
        *wp = (UWORD)(plane & 0xFFFF);  wp += 2;
      }
    }
    #endif

    //Set bottom panel bitplanes
    #if BOTTOM_PANEL_HEIGHT > 0
    {
      UWORD* wp = BP_BPL1PTH;
      PLANEPTR* planes = bottom_panel_rastport->BitMap->Planes;
      ULONG i;

      for (i = 0; i < BOTTOM_PANEL_DEPTH; i++)
      {
        ULONG plane = ((ULONG)planes[i]);

        *wp = (UWORD)(plane >> 16);     wp += 2;
        *wp = (UWORD)(plane & 0xFFFF);  wp += 2;
      }

      wp = BP_COP2LCH;
      *wp = (ULONG)CopperList >> 16; wp += 2;
      *wp = (ULONG)CopperList & 0xFFFF;
    }
    #endif

    //Set sprites to null sprite
    for (sp = CL_SPR0PTH; sp < CL_SPR0PTH + 32; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }

    // plane pointers
    planes = level_bitmap->Planes;
    #ifdef DUALPLAYFIELD
    planes_pf2 = level_bitmap_pf2->Planes;
    #endif
  }

  return CopperList;
}
#endif
///
///switchToLevelCopperList()
STATIC VOID switchToLevelCopperList()
{
  custom.cop2lc = (ULONG)CopperList;
  setVBlankEvents(vblankEvents);
  new_frame_flag = 1;
  waitTOF();
}
///
///startLevelDisplay(level_num)
VOID startLevelDisplay(ULONG level_num)
{
  if ((openDisplay(level_num))) {
    startLoadingDisplay();
    if (loadLevel(level_num)) {
      endLoadingDisplay();
      initLevelDisplay();

#ifdef DUALPLAYFIELD
      // prep the background bitmap
      if (current_level.num.bitmaps) {
        ULONG bitmap_height = current_level.bitmap[0]->Rows;
        BltBitMap(current_level.bitmap[0], 0, 0, level_bitmap_pf2, 16, 0, SCREEN_WIDTH, bitmap_height, 0x0C0, 0xFF, NULL);
        BltBitMap(current_level.bitmap[0], 0, 0, level_bitmap_pf2, SCREEN_WIDTH + 16, 0, SCREEN_WIDTH, bitmap_height, 0x0C0, 0xFF, NULL);
      }
#endif

      if (current_level.music_module) {
        PT_InitModule(current_level.music_module[current_level.current.music_module], 0);
        PT_PlayModule();
      }

      volume_table.state = PTVT_FADE_IN;

      // take over blitter
      OwnBlitter();
      WaitBlit();

      fillDisplay(current_level.initial_mapPosX, current_level.initial_mapPosY);

      switchToLevelCopperList();

      levelDisplayLoop();
      removeVBlankEvents();

      switchToNullCopperList();

      // give back blitter
      WaitBlit();
      DisownBlitter();

      PT_StopAudio();

      unloadLevel();
    }
    else endLoadingDisplay();

    closeDisplay();
  }
}
///

///initCopperListBlit()
/******************************************************************************
 * Initializes the blitter control registers to the pre-determined values for *
 * mass copying copperlist instructions to create the next frame's dynamic    *
 * copperlist with blitCopperInstruction().                                   *
 ******************************************************************************/
#ifdef DYNAMIC_COPPERLIST
STATIC INLINE VOID initCopperListBlit()
{
  busyWaitBlit();
  custom.bltcon0 = 0x9F0; // use A and D. Op: D = A (direct copy)
  custom.bltcon1 = 0;
  custom.bltafwm = 0xFFFF;
  custom.bltalwm = 0xFFFF;
  custom.bltamod = 0;
  custom.bltdmod = 0;
}
#endif
///
///blitCopperInstruction(dst, src, size)
/******************************************************************************
 * Copies size LONG's from the src pointer to dst pointer using Blitter.      *
 ******************************************************************************/
#ifdef DYNAMIC_COPPERLIST
INLINE VOID blitCopperInstruction(ULONG* dst, ULONG* src, WORD size)
{
  do {
    UWORD blt_size = size >= 1024 ? 1024 : size;

    busyWaitBlit();
    custom.bltapt  = src;
    custom.bltdpt  = dst;
    custom.bltsize = (blt_size << 6) | 2;

    size -= 1024;
    src += 1024;
    dst += 1024;
  } while(size > 0);
}
#endif
///
///updateDynamicCopperList()
/******************************************************************************
 * Creates the next frame's dynamic copperlist, applying new scroll values    *
 * and sorting CopOps from the current rainbow regarding videosplit smart     *
 * sprite positions and top and bottom dashboard panel sizes.                 *
 * NOTE: Implement the case where previous CopOp and current CopOp has the    *
 * same wait in which the blit should skip the WAIT() instruction.            *
 ******************************************************************************/
#ifdef DYNAMIC_COPPERLIST
STATIC VOID updateDynamicCopperList()
{
  ULONG plane;
  LONG  planeAdd;
  LONG  planeAddX;
  LONG  i;
  WORD  xpos;
  WORD  scroll;
  WORD  yoffset;
  WORD* wp;

  // Swap CopperLists
  if (CopperList == CopperList1)
    CopperList = CopperList2;
  else
    CopperList = CopperList1;

  // Calculate the scroll value for playfield 1 (tilemap)
  xpos = vidPosX + SCROLL_MASK;                       // xpos = vidPosX + 15
  planeAddX = (xpos / SCROLL_PIXELS) * SCROLL_BYTES;  // planeAddX = (xpos / 16) * 2

  // prepare scroll bits as BPLCON1 accepts
#ifdef DUALPLAYFIELD
  i = SCROLL_MASK - (xpos & SCROLL_MASK);             // i = 15 - (xpos % 16)
  scroll = i & 0xF;                                   // scroll = i & 0xF
  #if BPL_FMODE > 1
  if (i & 0x10) scroll |= 0x0400;
  #endif
  #if BPL_FMODE == 4
  if (i & 0x20) scroll |= 0x0800;
  #endif

  // Calculate the scroll value for playfield 2 (background)
  //WARNING: Does not support higher BPL_FMODEs
  xpos = ((*mapPosX >> 1) % SCREEN_WIDTH) + SCROLL_MASK;
  i = SCROLL_MASK - (xpos & SCROLL_MASK);
  scroll |= (i & 0xF) << 4;
#else //DUALPLAYFIELD
  i = SCROLL_MASK - (xpos & SCROLL_MASK);             // i = 15 - (xpos % 16)
  scroll = (i & 0xF) * 0x11;                          // scroll = (i & 0xF) | ((i & 0xF) << 4)
  #if BPL_FMODE > 1
  if (i & 0x10) scroll |= 0x4400;
  #endif
  #if BPL_FMODE == 4
  if (i & 0x20) scroll |= 0x8800;
  #endif
#endif //DUALPLAYFIELD

  // set scroll bits in BPLCON1
  *(WORD*)((ULONG)CopperList + (ULONG)CL_BPLCON1) = scroll;

  // set plane pointers at display start
  yoffset = (vidPosY + TILESIZE) % BITMAP_HEIGHT;
  planeAdd = ((LONG)yoffset) * (SCREEN_DEPTH * BITMAP_WIDTH / 8);

#ifdef DUALPLAYFIELD
  #ifdef UNROLL_LOOPS
  wp = (WORD*)((ULONG)CopperList + (ULONG)CL_BPL1PTL + (16 * (SCREEN_DEPTH - 1)));
  plane = ((ULONG)planes[SCREEN_DEPTH - 1]) + planeAdd + planeAddX;
  switch (SCREEN_DEPTH) {
    case 4:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 6;
    plane -= BITMAP_BYTES_PER_ROW;
    case 3:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 6;
    plane -= BITMAP_BYTES_PER_ROW;
    case 2:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 6;
    plane -= BITMAP_BYTES_PER_ROW;
    case 1:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);
  }
  #else //UNROLL_LOOPS
  wp = (WORD*)((ULONG)CopperList + (ULONG)CL_BPL1PTH);
  for (i = 0; i < SCREEN_DEPTH; i++)
  {
    plane = ((ULONG)planes[i]) + planeAdd + planeAddX;

    *wp = (WORD)(plane >> 16);     wp += 2;
    *wp = (WORD)(plane & 0xFFFF);  wp += 6;
  }
  #endif
#else //DUALPLAYFIELD
  #ifdef UNROLL_LOOPS
  wp = (WORD*)((ULONG)CopperList + (ULONG)CL_BPL1PTL + (8 * (SCREEN_DEPTH - 1)));
  plane = ((ULONG)planes[SCREEN_DEPTH - 1]) + planeAdd + planeAddX;
  switch (SCREEN_DEPTH) {
    case 8:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW;
    case 7:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW;
    case 6:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW;
    case 5:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW;
    case 4:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW;
    case 3:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW;
    case 2:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW;
    case 1:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);
  }
  #else //UNROLL_LOOPS
  wp = (WORD*)((ULONG)CopperList + (ULONG)CL_BPL1PTH);
  for (i = 0; i < SCREEN_DEPTH; i++)
  {
    plane = ((ULONG)planes[i]) + planeAdd + planeAddX;

    *wp = (WORD)(plane >> 16);     wp += 2;
    *wp = (WORD)(plane & 0xFFFF);  wp += 2;
  }
  #endif
#endif

  // set video split wait
  yoffset = (BITMAP_HEIGHT + SCREEN_START) - yoffset;

  if (yoffset >= SCREEN_END) {
    vsplit_CopOps[0].wait = 0xFFFF;
    vsplit_CopOps[1].wait = 0xFFFF;
  }
  else {
    vsplit_CopOps[0].wait = yoffset - 1;
    vsplit_CopOps[1].wait = yoffset;
    *CL_VIDSPLT  = ((yoffset > 256 ? yoffset - 257 : yoffset - 1) << 8) | 0x0001;
    *CL_VIDSPLT2 = ((yoffset > 255 ? yoffset - 256 : yoffset    ) << 8) | 0x0001;
  }

  // set high words of plane pointers after video split
#ifdef DUALPLAYFIELD
  #ifdef UNROLL_LOOPS
  plane = (ULONG)*planes + (BITMAP_BYTES_PER_ROW * SCREEN_DEPTH * (SCREEN_DEPTH - 1)) + planeAddX;
  wp = CL_BPL1PTH_2 + (2 * (SCREEN_DEPTH - 1));
  switch (SCREEN_DEPTH) {
    case 4:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 3:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 2:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 1:
    *wp = (WORD)(plane >> 16);
  }
  #else //UNROLL_LOOPS
  plane = (ULONG)*planes + planeAddX;
  wp = CL_BPL1PTH_2;
  for (i = 0; i < SCREEN_DEPTH; i++)
  {
    *wp = (WORD)(plane >> 16); wp += 2;

    plane += BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
  }
  #endif
#else //DUALPLAYFIELD
  #ifdef UNROLL_LOOPS
  plane = (ULONG)*planes + (BITMAP_BYTES_PER_ROW * SCREEN_DEPTH * (SCREEN_DEPTH - 1)) + planeAddX;
  wp = CL_BPL1PTH_2 + (2 * (SCREEN_DEPTH - 1));
  switch (SCREEN_DEPTH) {
    case 8:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 7:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 6:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 5:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 4:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 3:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 2:
    *wp = (WORD)(plane >> 16); wp -= 2;
    plane -= BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
    case 1:
    *wp = (WORD)(plane >> 16);
  }
  #else //UNROLL_LOOPS
  plane = (ULONG)*planes + planeAddX;
  wp = CL_BPL1PTH_2;
  for (i = 0; i < SCREEN_DEPTH; i++)
  {
    *wp = (WORD)(plane >> 16); wp += 2;

    plane += BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
  }
  #endif
#endif

#ifdef DUALPLAYFIELD
  // set plane pointers at display start for playfield 2 (scroll of the background)
  #if BITMAP_HEIGHT_PF2 <= SCREEN_HEIGHT
  planeAdd = 0;
  #else
  yoffset = (*mapPosY >> 1);  // mapPosY / 2
  //clamp vertical scroll to BITMAP_HEIGHT_PF2
  if (yoffset > (BITMAP_HEIGHT_PF2 - SCREEN_HEIGHT)) yoffset = yoffset % (BITMAP_HEIGHT_PF2 - SCREEN_HEIGHT);
    #ifdef INTERLEAVED_PF2_BITMAP
      planeAdd = yoffset * (BITMAP_BYTES_PER_ROW_PF2 * BITMAP_DEPTH_PF2);
    #else //INTERLEAVED_PF2_BITMAP
      planeAdd = yoffset * BITMAP_BYTES_PER_ROW_PF2;
    #endif
  #endif

  planeAddX = (xpos / SCROLL_PIXELS) * SCROLL_BYTES;

  #if defined UNROLL_LOOPS && defined INTERLEAVED_PF2_BITMAP
  wp = (WORD*)((ULONG)CopperList + (ULONG)CL_BPL1PTL + 8 + (16 * (BITMAP_DEPTH_PF2 - 1)));
  plane = ((ULONG)planes_pf2[BITMAP_DEPTH_PF2 - 1]) + planeAdd + planeAddX;
  switch (BITMAP_DEPTH_PF2) {
    case 4:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 6;
    plane -= BITMAP_BYTES_PER_ROW_PF2;
    case 3:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 6;
    plane -= BITMAP_BYTES_PER_ROW_PF2 ;
    case 2:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);     wp -= 6;
    plane -= BITMAP_BYTES_PER_ROW_PF2;
    case 1:
    *wp = (WORD)(plane & 0xFFFF);  wp -= 2;
    *wp = (WORD)(plane >> 16);
  }
  #else //UNROLL_LOOPS
  wp = (WORD*)((ULONG)CopperList + (ULONG)CL_BPL1PTH + 8); // Start from BPL2PTH...
  for (i = 0; i < BITMAP_DEPTH_PF2; i++)            // OPTIMIZE Unroll this loop
  {
    plane = ((ULONG)planes_pf2[i]) + planeAdd + planeAddX;

    *wp = (WORD)(plane >> 16);     wp += 2;
    *wp = (WORD)(plane & 0xFFFF);  wp += 6; // ...and traverse even planes
  }
  #endif
#endif

  #ifdef SMART_SPRITES
  setSprites();
  #endif

  //Create new copperlist by sorting CopOps
  {
    struct CopOp* r = rainbow->copOps;
    struct CopOp** s_ptr = sprite_CopOps_list;
    struct CopOp* s = *s_ptr;
    struct CopOp* v = vsplit_CopOps;
    struct CopOp* min;
    struct CopOp* med;
    ULONG* CL_ptr = (ULONG*)CopperList + num_cl_header_instructions;

    initCopperListBlit();

    while (TRUE) {
      //DEBUG: printf("step: %lu: r->wait = %u, s->wait = %u, v->wait = %u\n", step++, r->wait, s->wait, v->wait);
      if (r->wait <= s->wait) {
        if (r->wait <= v->wait) {
          UWORD size;
          UWORD index;

          //Smallest is the rainbow end CopOp so finalize the copperList and exit
          //min = r;
          if (r->wait == 0xFFFF) {
            blitCopperInstruction(CL_ptr, r->pointer, r->size);
            break;
          }

          //Do the big rainbow blit and also the second smallest
          if (s->wait <= v->wait) {
            med = s;
            s_ptr++;
            s = *s_ptr;
          }
          else {
            med = v++;
          }

          if (med->wait == 0xFFFF) {
            //The second smallest (med) is an end CopOp so finalize and exit
            struct CopOp* end = &rainbow->copOps[rainbow->num_ops];
            size = (UWORD)((ULONG)end->pointer - (ULONG)r->pointer) / 4 + end->size;
            blitCopperInstruction(CL_ptr, r->pointer, size);
            break;
          }
          else {
            ULONG i = med->wait - SCREEN_START;
            //NOTE: instead of clamping correspondance i to 255 just prevent that value to be set on any CopOp waits
            index = rainbow->correspondance[i > SCREEN_HEIGHT - 1 ? SCREEN_HEIGHT - 1 : i];
            //DEBUG: printf("index : %u\n", index);
            size = (UWORD)((ULONG)rainbow->copOps[index].pointer - (ULONG)r->pointer) / 4 + rainbow->copOps[index].size;
            //DEBUG: printf("size : %u\n", size);
            blitCopperInstruction(CL_ptr, r->pointer, size);
            CL_ptr += size;
            r = &rainbow->copOps[index + 1];
            blitCopperInstruction(CL_ptr, med->pointer, med->size);
            CL_ptr += med->size;
          }
          continue;
        }
        else {
          min = v++;
        }
      }
      else {
        if (s->wait <= v->wait) {
          min = s;
          s_ptr++;
          s = *s_ptr;
        }
        else {
          min = v++;
        }
      }
      blitCopperInstruction(CL_ptr, min->pointer, min->size);
      //DEBUG: printf("v->size : %u\n", min->size);
      CL_ptr += min->size;
    }
  }
}
#endif
///
///updateCopperList()
/******************************************************************************
 * Applies scroll values on to the copperlist regarding top and bottom        *
 * dashboard sizes.                                                           *
 * WARNING: This function gets called from vblank interrupt.                  *
 ******************************************************************************/
#ifdef DYNAMIC_COPPERLIST
STATIC VOID updateCopperList()
{
  #ifndef SMART_SPRITES
  setSprites();
  #endif
}
#else
STATIC VOID updateCopperList()
{
  ULONG plane;
  LONG  planeAdd;
  LONG  planeAddX;
  LONG  i;
  WORD  xpos;
  WORD  scroll;
  WORD  yoffset;
  WORD* wp;

  // Calculate the scroll value for playfield 1 (tilemap)
  xpos = vidPosX + SCROLL_MASK;                      // xpos = vidPosX + 15
  planeAddX = (xpos / SCROLL_PIXELS) * SCROLL_BYTES; // planeAddX = (xpos / 16) * 2

  // prepare scroll bits as BPLCON1 accepts
#ifdef DUALPLAYFIELD
  i = SCROLL_MASK - (xpos & SCROLL_MASK);             // i = 15 - (xpos % 16)
  scroll = i & 0xF;                                   // scroll = i & 0xF
  #if BPL_FMODE > 1
  if (i & 0x10) scroll |= 0x0400;
  #endif
  #if BPL_FMODE == 4
  if (i & 0x20) scroll |= 0x0800;
  #endif

  // Calculate the scroll value for playfield 2 (background)
  //WARNING: Does not support higher BPL_FMODEs
  xpos = ((*mapPosX >> 1) % SCREEN_WIDTH) + SCROLL_MASK;
  i = SCROLL_MASK - (xpos & SCROLL_MASK);
  scroll |= (i & 0xF) << 4;
#else //DUALPLAYFIELD
  i = SCROLL_MASK - (xpos & SCROLL_MASK);             // i = 15 - (xpos % 16)
  scroll = (i & 0xF) * 0x11;                          // scroll = (i & 0xF) | ((i & 0xF) << 4)
  #if BPL_FMODE > 1
  if (i & 0x10) scroll |= 0x4400;
  #endif
  #if BPL_FMODE == 4
  if (i & 0x20) scroll |= 0x8800;
  #endif
#endif //DUALPLAYFIELD

  // set scroll bits in BPLCON1
  *CL_BPLCON1 = scroll;

  // set plane pointers at display start for playfield 1 (tilemap)
  yoffset = (vidPosY + TILESIZE) % BITMAP_HEIGHT;
  planeAdd = ((LONG)yoffset) * (SCREEN_DEPTH * BITMAP_WIDTH / 8);

#ifdef DUALPLAYFIELD
  wp = CL_BPL1PTH;
  for (i = 0; i < SCREEN_DEPTH; i++)
  {
    plane = ((ULONG)planes[i]) + planeAdd + planeAddX;

    *wp = (WORD)(plane >> 16);     wp += 2;
    *wp = (WORD)(plane & 0xFFFF);  wp += 6;
  }
#else
  wp = CL_BPL1PTH;
  for (i = 0; i < SCREEN_DEPTH; i++)
  {
    plane = ((ULONG)planes[i]) + planeAdd + planeAddX;

    *wp = (WORD)(plane >> 16);     wp += 2;
    *wp = (WORD)(plane & 0xFFFF);  wp += 2;
  }
#endif

  // set video split wait
  yoffset = BITMAP_HEIGHT + SCREEN_START - yoffset;

#if SCREEN_END >= 256
  #if BOTTOM_PANEL_HEIGHT > 0
  if (yoffset < SCREEN_END) {
    #ifdef DUALPLAYFIELD
    *(ULONG*)(CL_VIDSPLT+4) = MOVE(BPL1MOD, VSPLTMOD_V);
    *(ULONG*)(CL_VIDSPLT+6) = WAIT(0, 0);
    *(ULONG*)(CL_VIDSPLT+8) = WAIT(0, 0);
    #else
    *(ULONG*)(CL_VIDSPLT+4) = MOVE(BPL1MOD, VSPLTMOD_V);
    *(ULONG*)(CL_VIDSPLT+6) = MOVE(BPL2MOD, VSPLTMOD_V);
    *(ULONG*)(CL_VIDSPLT+8) = WAIT(0, 0);
    #endif

    if (yoffset <= 255) {
      * CL_VIDSPLT     = 0x0001;
      *(CL_VIDSPLT+2)  = ((yoffset - 1) << 8) | 0x0001;

      * CL_VIDSPLT2    = 0x0001;
      *(CL_VIDSPLT2+2) = (yoffset << 8) | 0x0001;

      * CL_ENDWAIT     = 0xFFDF;
    }
    else if (yoffset == 256) {
      * CL_VIDSPLT     = 0x0001;
      *(CL_VIDSPLT+2)  = 0xFF01;

      * CL_VIDSPLT2    = 0xFFDF;
      *(CL_VIDSPLT2+2) = 0x0001;

      * CL_ENDWAIT     = 0x0001;
    }
    else {
      * CL_VIDSPLT     = 0xFFDF;
      *(CL_VIDSPLT+2)  = ((yoffset - 257) << 8) | 0x0001;

      * CL_VIDSPLT2    = 0x0001;
      *(CL_VIDSPLT2+2) = ((yoffset - 256) << 8) | 0x0001;

      * CL_ENDWAIT     = 0x0001;
    }
  }
  else {
    * CL_VIDSPLT     = 0xFFDF;
    *(CL_VIDSPLT+2)  = ((SCREEN_END - 256) << 8) | 0x0001;
    *(ULONG*)(CL_VIDSPLT+4) = MOVE(COP2LCH, (ULONG)(CL_ENDWAIT+4) >> 16);
    *(ULONG*)(CL_VIDSPLT+6) = MOVE(COP2LCL, (ULONG)(CL_ENDWAIT+4) & 0xFFFF);
    *(ULONG*)(CL_VIDSPLT+8) = MOVE(COPJMP2, 0x01);
  }
  #else // BOTTOM_PANEL_HEIGHT
    if (yoffset <= 255) {
      * CL_VIDSPLT     = 0x0001;
      *(CL_VIDSPLT+2)  = ((yoffset - 1) << 8) | 0x0001;

      * CL_VIDSPLT2    = 0x0001;
      *(CL_VIDSPLT2+2) = (yoffset << 8) | 0x0001;

      * CL_ENDWAIT     = 0xFFDF;
    }
    else if (yoffset == 256) {
      * CL_VIDSPLT     = 0x0001;
      *(CL_VIDSPLT+2)  = 0xFF01;

      * CL_VIDSPLT2    = 0xFFDF;
      *(CL_VIDSPLT2+2) = 0x0001;

      * CL_ENDWAIT     = 0x0001;
    }
    else {
      * CL_VIDSPLT     = 0xFFDF;
      *(CL_VIDSPLT+2)  = ((yoffset - 257) << 8) | 0x0001;

      * CL_VIDSPLT2    = 0x0001;
      *(CL_VIDSPLT2+2) = ((yoffset - 256) << 8) | 0x0001;

      * CL_ENDWAIT     = 0x0001;
    }
  #endif
#else // SCREEN_END
  #if BOTTOM_PANEL_HEIGHT > 0
    if (yoffset < SCREEN_END) {
    #ifdef DUALPLAYFIELD
      *(ULONG*)(CL_VIDSPLT+4) = MOVE(BPL1MOD, VSPLTMOD_V);
      *(ULONG*)(CL_VIDSPLT+6) = WAIT(0, 0);
      *(ULONG*)(CL_VIDSPLT+8) = WAIT(0, 0);
    #else
      *(ULONG*)(CL_VIDSPLT+4) = MOVE(BPL1MOD, VSPLTMOD_V);
      *(ULONG*)(CL_VIDSPLT+6) = MOVE(BPL2MOD, VSPLTMOD_V);
      *(ULONG*)(CL_VIDSPLT+8) = WAIT(0, 0);
    #endif

      * CL_VIDSPLT     = 0x0001;
      *(CL_VIDSPLT+2)  = ((yoffset - 1) << 8) | 0x0001;

      * CL_VIDSPLT2    = 0x0001;
      *(CL_VIDSPLT2+2) = (yoffset << 8) | 0x0001;

      * CL_ENDWAIT     = 0x0001;
    }
    else {
      * CL_VIDSPLT     = 0x0001;
      *(CL_VIDSPLT+2)  = (SCREEN_END << 8) | 0x0001;
      *(ULONG*)(CL_VIDSPLT+4) = MOVE(COP2LCH, (ULONG)(CL_ENDWAIT+4) >> 16);
      *(ULONG*)(CL_VIDSPLT+6) = MOVE(COP2LCL, (ULONG)(CL_ENDWAIT+4) & 0xFFFF);
      *(ULONG*)(CL_VIDSPLT+8) = MOVE(COPJMP2, 0x01);
    }
  #else // BOTTOM_PANEL_HEIGHT
    if (yoffset < SCREEN_END) {
      * CL_VIDSPLT     = 0x0001;
      *(CL_VIDSPLT+2)  = ((yoffset - 1) << 8) | 0x0001;

      * CL_VIDSPLT2    = 0x0001;
      *(CL_VIDSPLT2+2) = (yoffset << 8) | 0x0001;

      * CL_ENDWAIT     = 0x0001;
    }
  #endif
#endif

  // set plane pointers after video split
  plane = (ULONG)*planes + planeAddX;
  wp = CL_BPL1PTH_2;
  for (i = 0; i < SCREEN_DEPTH; i++)
  {
    *wp = (WORD)(plane >> 16); wp += 2;

    plane += BITMAP_BYTES_PER_ROW * SCREEN_DEPTH;
  }

#ifdef DUALPLAYFIELD
  // set plane pointers at display start for playfield 2 (scroll of the background)
  #if BITMAP_HEIGHT_PF2 <= SCREEN_HEIGHT
  planeAdd = 0;
  #else
  yoffset = (*mapPosY >> 1);  // mapPosY / 2
  //clamp vertical scroll to BITMAP_HEIGHT_PF2
  if (yoffset > (BITMAP_HEIGHT_PF2 - SCREEN_HEIGHT)) yoffset = yoffset % (BITMAP_HEIGHT_PF2 - SCREEN_HEIGHT);
    #ifdef INTERLEAVED_PF2_BITMAP
      planeAdd = yoffset * (BITMAP_BYTES_PER_ROW_PF2 * BITMAP_DEPTH_PF2);
    #else //INTERLEAVED_PF2_BITMAP
      planeAdd = yoffset * BITMAP_BYTES_PER_ROW_PF2;
    #endif
  #endif

  planeAddX = (xpos / SCROLL_PIXELS) * SCROLL_BYTES;

  wp = CL_BPL1PTH + 4;  // Start from BPL2PTH...
  for (i = 0; i < BITMAP_DEPTH_PF2; i++)            // OPTIMIZE Unroll this loop
  {
    plane = ((ULONG)planes_pf2[i]) + planeAdd + planeAddX;

    *wp = (WORD)(plane >> 16);     wp += 2;
    *wp = (WORD)(plane & 0xFFFF);  wp += 6; // ...and traverse even planes
  }
#endif

  setSprites();
}
#endif
///
///disposeCopperList()
STATIC VOID disposeCopperList()
{
  #ifdef DYNAMIC_COPPERLIST
  if (CopperList1) {
    freeCopperList(CopperList1);
    CopperList1 = NULL; CopperList2 = NULL; CopperList = NULL;
    if (empty_rainbow) {
      freeRainbow(empty_rainbow);
      rainbow = NULL; empty_rainbow = NULL;
    }
  }
  #else
  if (CopperList) {
    freeCopperList(CopperList)
    CopperList = NULL;
  }
  #endif
}
///

///sortSpriteCopOps()
/******************************************************************************
 * Does an insertion sort on the sprite_CopOps_list.                          *
 * NOTE: The case this is required is predictable. We can skip this if        *
 * unnecessary.                                                               *
 ******************************************************************************/
#ifdef SMART_SPRITES
STATIC VOID sortSpriteCopOps()
{
  struct CopOp** i = sprite_CopOps_list;

  for (++i; (*i)->wait != 0xFFFF; i++) {
    struct CopOp** l;
    struct CopOp* v = *(i);

    for (l = i - 1; l >= sprite_CopOps_list; l--) {
      if ((*l)->wait <= v->wait) break;
      else *(l + 1) = *l;
    }
    *(l + 1) = v;
  }
}
#endif
///
///setSprites()
/******************************************************************************
 * Sets the values required to display sprites for the gameobjects that use   *
 * sprite mediums.                                                            *
 * WARNING: When DYNAMIC_COPPERLIST is not set this function gets called      *
 * during VBL in updateCopperList() function, which is called during an       *
 * interrupt. So be warned!!!                                                 *
 ******************************************************************************/
STATIC VOID setSprites()
{
  struct GameObject** go_ptr = spriteList;
  struct GameObject* go = *go_ptr;

  #ifdef SMART_SPRITES
    WORD* sp;
    WORD* sp_end;
    ULONG i;
    WORD* paddr = (WORD*)((ULONG)CopperList + (ULONG)CL_SPR0POS);
    //reset hardSpriteUsage[]
    *(ULONG*)hardSpriteUsage = 0;
    *((ULONG*)hardSpriteUsage + 1) = 0;
    *((ULONG*)hardSpriteUsage + 2) = 0;
    *((ULONG*)hardSpriteUsage + 3) = 0;
    sprite_CopOp_Index = 0;
    sprite_CopOps[sprite_CopOp_Index].wait = 0xFFFF;
    sprite_list_i = sprite_list;

    for (sp = (WORD*)((ULONG)CopperList + (ULONG)CL_SPR0PTH), sp_end = sp + 32; sp < sp_end; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }
  #else
    WORD* sp;
    WORD* sp_end;
    //reset all sprites to dummy sprite
    #ifdef DYNAMIC_COPPERLIST
    for (sp = (WORD*)((ULONG)CopperList + (ULONG)CL_SPR0PTH), sp_end = sp + 32; sp < sp_end; sp += 2) {
    #else
    for (sp = CL_SPR0PTH, sp_end = sp + 32; sp < sp_end; sp += 2) {
    #endif
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }
  #endif

  while (go) {
    setSprite(go);

    go = *(++go_ptr);
  }

  #ifdef SMART_SPRITES
    //create end CopOp
    sprite_CopOps[sprite_CopOp_Index].wait = 0xFFFF;
    sprite_CopOps_list[sprite_CopOp_Index] = &sprite_CopOps[sprite_CopOp_Index];

    if (sprite_CopOp_Index) {
      sortSpriteCopOps();
    }

    for (i = 0; i < 8; i++) {
      if (hardSpriteUsage[i] == 0) {
        *paddr = 0; paddr += 2;
        *paddr = 0; paddr += 2;
      }
      else paddr += 4;
    }
  #endif
}
///

///setSprite(gameobject)
/******************************************************************************
 * Gets image data from the spritebank for a gameobject that has a sprite     *
 * medium and a sprite image and sets the required values to display it.      *
 * NOTE: Too much repeated code. Please refactor!                             *
 ******************************************************************************/
#ifdef SMART_SPRITES
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
    UWORD height = image->height;

    UBYTE* saddr;
    WORD*  paddr = (WORD*)((ULONG)CopperList + (ULONG)CL_SPR0POS) + hsn * 4;
    WORD*  haddr = paddr + 32;

    ULONG x, y, s;
    ULONG ctl_l0, ctl_l1;

    LONG y_s = go->y1 - *mapPosY;
    x = go->x1 + (DIWSTART_V & 0xFF) - *mapPosX;
    //Clip top of sprite if needed
    if (y_s < 0) {
      y = SCREEN_START;
      s = y + height + y_s;
      saddr = image->sprite_bank->data + offset + (1 - y_s) * (4 * SPR_FMODE);
    }
    else {
      y = y_s + SCREEN_START;
      s = y + height;
      saddr = image->sprite_bank->data + offset + (SPR_FMODE * 4);
    }
    //Clip bottom of sprite if needed
    /*if (s > SCREEN_HEIGHT + DISPLAY_START) {
      s = SCREEN_HEIGHT + DISPLAY_START;
    }*/

    //Prepare the two sprite control words (in one long) for the static vertical coords.
    ctl_l0 = (y << 24) | ((s << 8) & 0xFF00) | ((y >> 8) << 2) | ((s >> 8) << 1);

    if (type & 0x10) {
      while (numSprites) {
        //finalize the control words with the current x value for this glued sprite
        ctl_l1 = ctl_l0 | ((x >> 1) << 16) | 0x80 | (x & 0x1);

        if (hardSpriteUsage[hsn]) {
          UWORD wait = hardSpriteUsage[hsn];

          //set this sprite on the sprite copOp
          //NOTE: IMPLEMENT HERE!
          /*NOTES***************************************************************
           * - All sprites in this loop are going to go into separate CopOps   *
           * - If the previous last CopOp has the same wait with this we must  *
           *   append to the the same CopOp.                                   *
           * - setSprites() must create a terminating end CopOp after all this *
           * - we need pointers to keep and iterate the current CopOp's and    *
           *   the position on sprite_list to write new instructions.          *
           * - In worst case the CopOps will need to be sorted, so we also     *
           *   fill pointers to a sortable list.                               *
           * - This code section below is repeated 2 more times below. Care to *
           *   functionalize?                                                  *
           *********************************************************************/
          if (sprite_CopOp_Index > 0 && sprite_CopOps[sprite_CopOp_Index - 1].wait == wait) {
            //Do not create new CopOp, append to the previous one
            sprite_CopOps[sprite_CopOp_Index - 1].size += 4;

            *sprite_list_i = MOVE(SPR0POS + hsn * 8, *(WORD*)&ctl_l1);               sprite_list_i++;
            *sprite_list_i = MOVE(SPR0CTL + hsn * 8, *((WORD*)&ctl_l1 + 1));         sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTH + hsn * 4, (WORD)((ULONG)saddr >> 16));    sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTL + hsn * 4, (WORD)((ULONG)saddr & 0xFFFF)); sprite_list_i++;
          }
          else {
            //Create new CopOp
            sprite_CopOps[sprite_CopOp_Index].wait = wait;
            sprite_CopOps[sprite_CopOp_Index].size = 5;
            sprite_CopOps[sprite_CopOp_Index].pointer = sprite_list_i;

            *sprite_list_i = WAIT(0, wait > 255 ? wait - 256 : wait);                sprite_list_i++;
            *sprite_list_i = MOVE(SPR0POS + hsn * 8, *(WORD*)&ctl_l1);               sprite_list_i++;
            *sprite_list_i = MOVE(SPR0CTL + hsn * 8, *((WORD*)&ctl_l1 + 1));         sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTH + hsn * 4, (WORD)((ULONG)saddr >> 16));    sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTL + hsn * 4, (WORD)((ULONG)saddr & 0xFFFF)); sprite_list_i++;

            sprite_CopOps_list[sprite_CopOp_Index] = &sprite_CopOps[sprite_CopOp_Index];
            sprite_CopOp_Index++;
          }

          paddr += 4;
          haddr += 4;
        }
        else {
          //set this sprite on the copperlist header
          *paddr = *(WORD*)&ctl_l1;       paddr += 2;
          *paddr = *((WORD*)&ctl_l1 + 1); paddr += 2;
          *haddr = (WORD)((ULONG)saddr >> 16);    haddr += 2;
          *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;
        }

        hardSpriteUsage[hsn++] = s;

        //get to the attached sprite
        saddr += ssize;

        if (hardSpriteUsage[hsn]) {
          UWORD wait = hardSpriteUsage[hsn];

          //set this sprite on the sprite copOp
          if (sprite_CopOp_Index > 0 && sprite_CopOps[sprite_CopOp_Index - 1].wait == wait) {
            //Do not create new CopOp, append to the previous one
            sprite_CopOps[sprite_CopOp_Index - 1].size += 4;

            *sprite_list_i = MOVE(SPR0POS + hsn * 8, *(WORD*)&ctl_l1);               sprite_list_i++;
            *sprite_list_i = MOVE(SPR0CTL + hsn * 8, *((WORD*)&ctl_l1 + 1));         sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTH + hsn * 4, (WORD)((ULONG)saddr >> 16));    sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTL + hsn * 4, (WORD)((ULONG)saddr & 0xFFFF)); sprite_list_i++;
          }
          else {
            //Create new CopOp
            sprite_CopOps[sprite_CopOp_Index].wait = wait;
            sprite_CopOps[sprite_CopOp_Index].size = 5;
            sprite_CopOps[sprite_CopOp_Index].pointer = sprite_list_i;

            *sprite_list_i = WAIT(0, wait > 255 ? wait - 256 : wait);                sprite_list_i++;
            *sprite_list_i = MOVE(SPR0POS + hsn * 8, *(WORD*)&ctl_l1);               sprite_list_i++;
            *sprite_list_i = MOVE(SPR0CTL + hsn * 8, *((WORD*)&ctl_l1 + 1));         sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTH + hsn * 4, (WORD)((ULONG)saddr >> 16));    sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTL + hsn * 4, (WORD)((ULONG)saddr & 0xFFFF)); sprite_list_i++;

            sprite_CopOps_list[sprite_CopOp_Index] = &sprite_CopOps[sprite_CopOp_Index];
            sprite_CopOp_Index++;
          }

          paddr += 4;
          haddr += 4;
        }
        else {
          //set this sprite on the copperlist header
          *paddr = *(WORD*)&ctl_l1; paddr += 2;
          *paddr = *((WORD*)&ctl_l1 + 1); paddr += 2;
          *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
          *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;
        }

        //get to the next glued sprite (if there is any)
        saddr += ssize;
        x += (16 * SPR_FMODE);
        numSprites -= 2;
        hardSpriteUsage[hsn++] = s;
      }
    }
    else
    {
      while (numSprites) {
        //finalize the control words with the current x value for this glued sprite
        ctl_l1 = ctl_l0 | ((x >> 1) << 16) | 0x80 | (x & 0x1);

        if (hardSpriteUsage[hsn]) {
          UWORD wait = hardSpriteUsage[hsn];

          //set this sprite on the sprite copOp
          if (sprite_CopOp_Index > 0 && sprite_CopOps[sprite_CopOp_Index - 1].wait == wait) {
            //Do not create new CopOp, append to the previous one
            sprite_CopOps[sprite_CopOp_Index - 1].size += 4;

            *sprite_list_i = MOVE(SPR0POS + hsn * 8, *(WORD*)&ctl_l1);               sprite_list_i++;
            *sprite_list_i = MOVE(SPR0CTL + hsn * 8, *((WORD*)&ctl_l1 + 1));         sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTH + hsn * 4, (WORD)((ULONG)saddr >> 16));    sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTL + hsn * 4, (WORD)((ULONG)saddr & 0xFFFF)); sprite_list_i++;
          }
          else {
            //Create new CopOp
            sprite_CopOps[sprite_CopOp_Index].wait = wait;
            sprite_CopOps[sprite_CopOp_Index].size = 5;
            sprite_CopOps[sprite_CopOp_Index].pointer = sprite_list_i;

            *sprite_list_i = WAIT(0, wait > 255 ? wait - 256 : wait);                sprite_list_i++;
            *sprite_list_i = MOVE(SPR0POS + hsn * 8, *(WORD*)&ctl_l1);               sprite_list_i++;
            *sprite_list_i = MOVE(SPR0CTL + hsn * 8, *((WORD*)&ctl_l1 + 1));         sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTH + hsn * 4, (WORD)((ULONG)saddr >> 16));    sprite_list_i++;
            *sprite_list_i = MOVE(SPR0PTL + hsn * 4, (WORD)((ULONG)saddr & 0xFFFF)); sprite_list_i++;

            sprite_CopOps_list[sprite_CopOp_Index] = &sprite_CopOps[sprite_CopOp_Index];
            sprite_CopOp_Index++;
          }

          paddr += 4;
          haddr += 4;
        }
        else {
          //set this sprite on the copperlist header
          *paddr = *(WORD*)&ctl_l1; paddr += 2;
          *paddr = *((WORD*)&ctl_l1 + 1); paddr += 2;
          *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
          *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;
        }

        //get to the next glued sprite (if there is any)
        saddr += ssize;
        x += (16 * SPR_FMODE);
        numSprites--;
        hardSpriteUsage[hsn++] = s;
      }
    }
  }
}
#else /* NOT SMART_SPRITES */
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
    UWORD height = image->height; // (ssize / (4 * SPR_FMODE)) - 2;

    ULONG x = go->x1 + (DIWSTART_V & 0xFF) - *mapPosX ;
    ULONG y = go->y1 + SCREEN_START - *mapPosY;
    ULONG s = y + height;

    UBYTE* saddr = image->sprite_bank->data + offset;
    #ifdef DYNAMIC_COPPERLIST
      WORD* haddr = (WORD*)((ULONG)CopperList + (ULONG)CL_SPR0PTH) + hsn * 4;
    #else
      WORD* haddr = CL_SPR0PTH + hsn * 4;
    #endif

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
        switch (SPR_FMODE) {
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
        switch (SPR_FMODE) {
          case 4: *((ULONG*)saddr + 2) = ctl_l2;
          case 2: *((ULONG*)saddr + 1) = ctl_l2;
          case 1: *((ULONG*)saddr) = ctl_l1;
        }

        //set this sprite address on CopperList as well
        *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
        *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;

        //get to the next glued sprite (if there is any)
        saddr += ssize;
        x += (16 * SPR_FMODE);
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
        switch (SPR_FMODE) {
          case 4: *((ULONG*)saddr + 2) = ctl_l2;
          case 2: *((ULONG*)saddr + 1) = ctl_l2;
          case 1: *((ULONG*)saddr) = ctl_l1;
        }

        //set the sprite address on CopperList
        *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
        *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;

        //get to the next glued sprite (if there is any)
        saddr += ssize;
        x += (16 * SPR_FMODE);
        numSprites--;
      }
    }
  }
}
#endif
///
