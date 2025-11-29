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
//#include <libraries/mathffp.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "system.h"
#include "color.h"
#include "cop_inst_macros.h"
#include "input.h"
#include "keyboard.h"
#include "diskio.h"
#include "settings.h"

#include "version.h"
#include "fonts.h"
#include "display.h"
#include "display_splash.h"
///
///defines (private)
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

#define SPLASH_SCREEN_WIDTH  320
#define SPLASH_SCREEN_HEIGHT 256
#define SPLASH_SCREEN_DEPTH  3
#define SPLASH_SCREEN_COLORS (1 << SPLASH_SCREEN_DEPTH)

#define SPLASH_BITMAP_WIDTH  SPLASH_SCREEN_WIDTH
#define SPLASH_BITMAP_HEIGHT SPLASH_SCREEN_HEIGHT
#define SPLASH_BITMAP_DEPTH  SPLASH_SCREEN_DEPTH

#define DDFSTRT_V DEFAULT_DDFSTRT_LORES
#define DDFSTOP_V DEFAULT_DDFSTOP_LORES
#define DIWSTRT_V DEFAULT_DIWSTRT
#define DIWSTOP_V DEFAULT_DIWSTOP

#define BPLXMOD_V 0

#define BPLCON0_V ((SPLASH_SCREEN_DEPTH * BPLCON0_BPU0) | BPLCON0_COLOR)

#define MAXVECTORS 8

#define ORBIT_STEPS   210
#define ORBIT_SQUEEZE   4
#define ORBIT_RADIUS   80
#define BOING_SKIP      3

#define PIXEL_DATA_BPR 8
#define PIXEL_SIZE 3
#define VERSION_TEXT_X 280
#define VERSION_TEXT_Y 210
#define CURSOR_FRAMES 10
#define INIT_IDLE_CYCLES (CURSOR_FRAMES * 6)

#define HEART_X 113
#define HEART_Y  72
#define BOING_X 160
#define BOING_Y 110

#define LOGO_TEXT_X   44
#define LOGO_TEXT_Y  174
#define LOGO_LETTERS  12
#define LOGO_FRAMES    4

#define BLACK 0
#define WHITE 1
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
STATIC struct BitMap* splash_bitmap = NULL;

STATIC struct BitMap* bmc_heart = NULL;
STATIC struct BitMap* bmc_heart_mask = NULL;
STATIC struct BitMap* bmc_boing = NULL;
STATIC struct BitMap* bmc_boing_mask = NULL;
STATIC struct BitMap* bmc_text = NULL;

FLOAT g_rad_conv;
FLOAT g_radius;
struct Coords {
  LONG x;
  LONG y;
};
//struct Coords* g_coords = NULL;
//pre-calculated and pre-allocated orbit coordinates so we don't need mathtrans.library
struct Coords g_coords[ORBIT_STEPS] = {
{ 80,   0}, { 79,   0}, { 79,   1}, { 79,   1}, { 79,   2}, { 79,   2}, { 78,   3},
{ 78,   4}, { 77,   4}, { 77,   5}, { 76,   5}, { 75,   6}, { 74,   7}, { 74,   7},
{ 73,   8}, { 72,   8}, { 71,   9}, { 69,   9}, { 68,  10}, { 67,  10}, { 66,  11},
{ 64,  11}, { 63,  12}, { 61,  12}, { 60,  13}, { 58,  13}, { 56,  14}, { 55,  14},
{ 53,  14}, { 51,  15}, { 49,  15}, { 47,  16}, { 46,  16}, { 44,  16}, { 42,  17},
{ 39,  17}, { 37,  17}, { 35,  17}, { 33,  18}, { 31,  18}, { 29,  18}, { 26,  18},
{ 24,  19}, { 22,  19}, { 20,  19}, { 17,  19}, { 15,  19}, { 13,  19}, { 10,  19},
{  8,  19}, {  5,  19}, {  3,  19}, {  1,  19}, { -1,  19}, { -3,  19}, { -5,  19},
{ -8,  19}, {-10,  19}, {-13,  19}, {-15,  19}, {-17,  19}, {-20,  19}, {-22,  19},
{-24,  19}, {-26,  18}, {-29,  18}, {-31,  18}, {-33,  18}, {-35,  17}, {-37,  17},
{-40,  17}, {-42,  17}, {-44,  16}, {-46,  16}, {-47,  16}, {-49,  15}, {-51,  15},
{-53,  14}, {-55,  14}, {-56,  14}, {-58,  13}, {-60,  13}, {-61,  12}, {-63,  12},
{-64,  11}, {-66,  11}, {-67,  10}, {-68,  10}, {-69,   9}, {-71,   9}, {-72,   8},
{-73,   8}, {-74,   7}, {-74,   7}, {-75,   6}, {-76,   5}, {-77,   5}, {-77,   4},
{-78,   4}, {-78,   3}, {-79,   2}, {-79,   2}, {-79,   1}, {-79,   1}, {-79,   0},
{-80,   0}, {-79,   0}, {-79,  -1}, {-79,  -1}, {-79,  -2}, {-79,  -2}, {-78,  -3},
{-78,  -4}, {-77,  -4}, {-77,  -5}, {-76,  -5}, {-75,  -6}, {-74,  -7}, {-74,  -7},
{-73,  -8}, {-72,  -8}, {-71,  -9}, {-69,  -9}, {-68, -10}, {-67, -10}, {-66, -11},
{-64, -11}, {-63, -12}, {-61, -12}, {-60, -13}, {-58, -13}, {-56, -14}, {-55, -14},
{-53, -14}, {-51, -15}, {-49, -15}, {-47, -16}, {-46, -16}, {-44, -16}, {-42, -17},
{-39, -17}, {-37, -17}, {-35, -17}, {-33, -18}, {-31, -18}, {-29, -18}, {-26, -18},
{-24, -19}, {-22, -19}, {-20, -19}, {-17, -19}, {-15, -19}, {-13, -19}, {-10, -19},
{ -8, -19}, { -5, -19}, { -3, -19}, { -1, -19}, {  1, -19}, {  3, -19}, {  5, -19},
{  8, -19}, { 10, -19}, { 13, -19}, { 15, -19}, { 17, -19}, { 20, -19}, { 22, -19},
{ 24, -19}, { 26, -18}, { 29, -18}, { 31, -18}, { 33, -18}, { 35, -17}, { 37, -17},
{ 40, -17}, { 42, -17}, { 44, -16}, { 46, -16}, { 47, -16}, { 49, -15}, { 51, -15},
{ 53, -14}, { 55, -14}, { 56, -14}, { 58, -13}, { 60, -13}, { 61, -12}, { 63, -12},
{ 64, -11}, { 66, -11}, { 67, -10}, { 68, -10}, { 69,  -9}, { 71,  -9}, { 72,  -8},
{ 73,  -8}, { 74,  -7}, { 74,  -7}, { 75,  -6}, { 76,  -5}, { 77,  -5}, { 77,  -4},
{ 78,  -4}, { 78,  -3}, { 79,  -2}, { 79,  -2}, { 79,  -1}, { 79,  -1}, { 79,   0}};
///
///copperlist
STATIC UWORD* CopperList = (UWORD*) 0;

#ifdef USE_CLP
STATIC UWORD* CL_PALETTE = (UWORD*) 0;
#endif // USE_CLP

STATIC UWORD* CL_BPL1PTH = (UWORD*) 0;
STATIC UWORD* CL_SPR0PTH = (UWORD*) 0;

STATIC ULONG copperList_Instructions[] = {
                                              // Access Ptr:  Action:
#ifdef USE_CLP
  #define CLP_DEPTH SPLASH_SCREEN_DEPTH
  #include "clp.c"
#endif // USE_CLP
  MOVE(FMODE,   0),                           //              Set Sprite/Bitplane Fetch Modes
  MOVE(BPLCON0, BPLCON0_V),                   //              Set a ECS Lowres display
  MOVE(BPLCON1, 0),                           // CL_BPLCON1   Set h_scroll register
  MOVE(BPLCON2, 0x264),
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
STATIC UBYTE colors[] = {(UBYTE)(SPLASH_SCREEN_COLORS - 1),
  0,   0,   0,
243, 235, 235,
 94,  85,  85,
178, 149, 149,
252, 137, 137,
131,  10,  10,
242,   1,   1,
229,  64,  64};
///
///protos (private)
STATIC VOID closeScreen(VOID);
STATIC VOID closeDisplay(VOID);
VOID drawPixel(ULONG, ULONG);
///

///data
#define HEART_DATA_WIDTH  31
#define HEART_DATA_HEIGHT 31
#define HEART_SCALE        3
#define HEART_WIDTH  (HEART_DATA_WIDTH * HEART_SCALE)
#define HEART_HEIGHT (HEART_DATA_HEIGHT * HEART_SCALE)
#define HEART_DEPTH SPLASH_SCREEN_DEPTH
STATIC ULONG heart_p0[31] = {
0x00000040, 0x00000020, 0x03C00E10, 0x07001808, 0x0E003004, 0x1C002004, 0x18000002, 0x10000002,
0x10000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000004, 0x00000004,
0x00000008, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080, 0x00000100, 0x00000200,
0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000, 0x00010000};
STATIC ULONG heart_p1[31] = {
0x03F01F80, 0x0FF83FC0, 0x1C3C71E0, 0x38FEE7F0, 0x71FFCFF8, 0x63FFDFF8, 0xE7FFFFFC, 0xEFFFFFFC,
0xEFFFFFFC, 0xFFFFFFFC, 0xFFFFFFFC, 0xFFFFFFFC, 0xFFFFFFFC, 0xFFFFFFFC, 0x7FFFFFF8, 0x7FFFFFF8,
0x3FFFFFF0, 0x3FFFFFF0, 0x1FFFFFE0, 0x0FFFFFC0, 0x07FFFF80, 0x03FFFF00, 0x01FFFE00, 0x00FFFC00,
0x007FF800, 0x003FF000, 0x001FE000, 0x000FC000, 0x00078000, 0x00030000, 0x00000000};
STATIC ULONG heart_p2[31] = {
0x03F01FC0, 0x0FF83FE0, 0x1C3C71F0, 0x38FEE7F8, 0x71FFCFFC, 0x63FFDFFC, 0xE7FFFFFE, 0xEFFFFFFE,
0xEFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0x7FFFFFFC, 0x7FFFFFFC,
0x3FFFFFF8, 0x3FFFFFF8, 0x1FFFFFF0, 0x0FFFFFE0, 0x07FFFFC0, 0x03FFFF80, 0x01FFFF00, 0x00FFFE00,
0x007FFC00, 0x003FF800, 0x001FF000, 0x000FE000, 0x0007C000, 0x00038000, 0x00010000};
static struct BitMap bm_heart = {4, 31, BMF_STANDARD, HEART_DEPTH, 0, {(UBYTE*)heart_p0, (UBYTE*)heart_p1, (UBYTE*)heart_p2, 0, 0, 0, 0, 0}};

STATIC ULONG heart_mask_p0[31] = {
0x03F01FC0, 0x0FF83FE0, 0x1FFC7FF0, 0x3FFEFFF8, 0x7FFFFFFC, 0x7FFFFFFC, 0xFFFFFFFE, 0xFFFFFFFE,
0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0xFFFFFFFE, 0x7FFFFFFC, 0x7FFFFFFC,
0x3FFFFFF8, 0x3FFFFFF8, 0x1FFFFFF0, 0x0FFFFFE0, 0x07FFFFC0, 0x03FFFF80, 0x01FFFF00, 0x00FFFE00,
0x007FFC00, 0x003FF800, 0x001FF000, 0x000FE000, 0x0007C000, 0x00038000, 0x00010000};
static struct BitMap bm_heart_mask = {4, 31, BMF_STANDARD, 1, 0, {(UBYTE*)heart_mask_p0, 0, 0, 0, 0, 0, 0, 0}};

#define BOING_WIDTH  40
#define BOING_HEIGHT 40
#define BOING_FRAMES  4
#define BOING_DEPTH SPLASH_SCREEN_DEPTH
STATIC UWORD boing_p0[480] = {
0x0003, 0xD740, 0x0000, 0x001F, 0x8378, 0x0000, 0x007E, 0x1FFE, 0x0000, 0x00CC, 0x3FEB, 0x0000,
0x011F, 0xBFC3, 0x8000, 0x063F, 0xAF83, 0xE000, 0x047F, 0x05C1, 0xE000, 0x01FF, 0x0041, 0xF000,
0x09FF, 0x00F5, 0xF800, 0x3BFC, 0x01FF, 0xFC00, 0x3EFC, 0x01FF, 0xFC00, 0x7C59, 0xE1FF, 0x7600,
0x7C05, 0xF1FF, 0x7A00, 0x780F, 0xBBFE, 0x7C00, 0xF00F, 0xF7FE, 0x7C00, 0xF01F, 0xFFFF, 0x7800,
0xF03F, 0xF87E, 0xF800, 0xE01F, 0xFC2E, 0xF800, 0xE03F, 0xF807, 0xF800, 0xDC3F, 0xF003, 0xF800,
0xFE3F, 0xF003, 0xE800, 0xFFFF, 0xF007, 0xF000, 0xFF8F, 0xE00F, 0xF700, 0xFFC1, 0xE01F, 0xE700,
0xFF80, 0x807F, 0xCF00, 0xFF80, 0x3DFF, 0xCE00, 0x7F80, 0x3FFF, 0x8E00, 0x1FE0, 0x7FFF, 0x1E00,
0x0FFF, 0xFFFA, 0x1C00, 0x0FFF, 0xFFFE, 0x3C00, 0x0FFF, 0xFFFF, 0x8800, 0x077F, 0xFFFF, 0x8000,
0x03FD, 0xFF7F, 0x0000, 0x039F, 0xB8FE, 0x0000, 0x0181, 0xE1FC, 0x2000, 0x0101, 0xFCF8, 0x0000,
0x0003, 0xF810, 0x0000, 0x001B, 0xF01C, 0x0000, 0x0018, 0xC078, 0x0000, 0x0000, 0xC3C0, 0x0000,
0x0000, 0xF3C0, 0x0000, 0x0017, 0xF2B8, 0x0000, 0x005F, 0xC0EE, 0x0000, 0x00D6, 0x01FF, 0x0000,
0x01C7, 0xA7F9, 0x8000, 0x070F, 0xF7F8, 0xC000, 0x060F, 0xFDF8, 0xE000, 0x0C1F, 0xF0F8, 0x6000,
0x1C1F, 0xF004, 0x7000, 0x2A7F, 0xE00E, 0x7000, 0x0F7F, 0xE00F, 0xF800, 0x5FDF, 0xD01F, 0xF800,
0x5F8B, 0xF00F, 0xF800, 0x7F82, 0xF81F, 0xFC00, 0xBF03, 0x701F, 0xFC00, 0xBE03, 0xFF3F, 0xFE00,
0xFE01, 0xFF9F, 0xFE00, 0xFE00, 0xFFCF, 0xFE00, 0xBC01, 0xFFE7, 0xFE00, 0xDE01, 0xFFC1, 0xFE00,
0xC681, 0xFF83, 0xEE00, 0xC3C3, 0xFFC7, 0xF000, 0xE3FF, 0xFF8F, 0xF100, 0xE7FD, 0xFF1F, 0xA100,
0x77FC, 0xBF3F, 0xC000, 0x3FF8, 0x1FFF, 0xC200, 0x3FF8, 0x03FF, 0x0200, 0x5FF8, 0x1FFF, 0x0000,
0x1FFF, 0xFFF6, 0x0400, 0x0FFF, 0xFFFC, 0x0000, 0x07FF, 0xFFF3, 0xE800, 0x03FF, 0xFFC3, 0xE000,
0x01FD, 0xFF07, 0xC000, 0x01FF, 0x680F, 0x8000, 0x00E0, 0x001F, 0x0000, 0x00E0, 0x3C3E, 0x0000,
0x0000, 0x3F9C, 0x0000, 0x0008, 0x7E00, 0x0000, 0x0006, 0xF800, 0x0000, 0x0000, 0x3000, 0x0000,
0x0000, 0x3AC0, 0x0000, 0x0013, 0xFCF8, 0x0000, 0x0043, 0xF02E, 0x0000, 0x00F7, 0xE03F, 0x0000,
0x01F1, 0x403F, 0x8000, 0x07E0, 0xF47E, 0xC000, 0x07C0, 0xFEFF, 0xE000, 0x0F80, 0xFFFE, 0x7000,
0x1E03, 0xFF0F, 0x7000, 0x2603, 0xFF85, 0x7800, 0x2743, 0xFF00, 0xF800, 0x67EF, 0xFE00, 0xF800,
0x4FFB, 0xFE00, 0xF800, 0x47F2, 0xFE01, 0xFE00, 0x8FF3, 0x1C01, 0xFF00, 0x9FF3, 0xF101, 0xFF00,
0x8FF1, 0xE781, 0xFF00, 0x9FE0, 0x07F3, 0xFB00, 0xDFC0, 0x07FF, 0xFF00, 0xFFE0, 0x0FFF, 0xFF00,
0xE1C0, 0x1FFF, 0xE700, 0xE080, 0x0FFF, 0xFB00, 0xE074, 0x1FFF, 0xF800, 0xE07E, 0x3FFF, 0xF000,
0x707F, 0xFFFF, 0xF000, 0x7CFF, 0xFFFF, 0xF000, 0x3EFF, 0xC3FF, 0xE000, 0x5FFF, 0xDFFF, 0xE000,
0x7FFF, 0xFFFF, 0xC000, 0x3FFF, 0xFFD8, 0x4000, 0x37FF, 0xFFF0, 0x0400, 0x117F, 0xFF80, 0x7800,
0x087D, 0xFF00, 0xF000, 0x047E, 0x7F01, 0xE000, 0x047E, 0x0601, 0xC000, 0x003C, 0x0007, 0x8000,
0x00DC, 0x07CF, 0x0000, 0x0060, 0x0FC2, 0x0000, 0x0002, 0x1F00, 0x0000, 0x0002, 0x1800, 0x0000,
0x0002, 0x5DC0, 0x0000, 0x001A, 0x7F78, 0x0000, 0x0078, 0x7F1E, 0x0000, 0x003D, 0xFF0F, 0x0000,
0x007D, 0x7C07, 0x8000, 0x05F8, 0x2807, 0xC000, 0x03F0, 0x060F, 0xE000, 0x03F0, 0x0FCF, 0xF000,
0x0FE0, 0x0FFF, 0xF000, 0x37E0, 0x1FF5, 0xF800, 0x32C0, 0x1FF0, 0xF800, 0x60E9, 0xFFF0, 0x7C00,
0x60FD, 0xBFF0, 0x7800, 0x41FF, 0xFFE0, 0x7C00, 0xE1FF, 0x9FF0, 0x7D00, 0xC1FF, 0xF3E0, 0x7D00,
0xC1FF, 0xE060, 0xFD00, 0x83FF, 0x0030, 0xFD00, 0xC3FE, 0x007D, 0xF900, 0xFFFF, 0x003F, 0xF900,
0xFDFE, 0x007F, 0xE100, 0xFCFC, 0x00FF, 0xFD00, 0xFC0E, 0x00FF, 0xFE00, 0xFC02, 0x00FF, 0xFC00,
0xF807, 0xC1FF, 0xFC00, 0xB807, 0xFDFF, 0x7C00, 0x3F0F, 0xFFFF, 0xFC00, 0x1FE7, 0xFFFE, 0xF800,
0x1FFF, 0xFFFB, 0xF800, 0x1FFF, 0xFFFE, 0x7000, 0x0FFF, 0xFFFC, 0x0000, 0x0DFF, 0xFFF8, 0x1800,
0x04FD, 0xFFF0, 0x1000, 0x060F, 0xAFF0, 0x2000, 0x020F, 0xE7E0, 0x6000, 0x000F, 0xC0C0, 0x8000,
0x004F, 0x8043, 0x0000, 0x0037, 0x00F2, 0x0000, 0x0011, 0x03C0, 0x0000, 0x0003, 0xCF00, 0x0000};
STATIC UWORD boing_p1[480] = {
0x0000, 0x1040, 0x0000, 0x0000, 0xF868, 0x0000, 0x0043, 0xF004, 0x0000, 0x00E1, 0xE03B, 0x0000,
0x01C0, 0x003F, 0x8000, 0x0380, 0x203E, 0x4000, 0x0700, 0x7C7E, 0x6000, 0x0700, 0xFF1E, 0x2000,
0x0E01, 0xFF06, 0x3000, 0x2001, 0xFF00, 0x1000, 0x0203, 0xFE00, 0xD000, 0x43C3, 0xFE00, 0xB800,
0x07F1, 0xFC00, 0x8C00, 0x07F0, 0x3C00, 0x8600, 0x87E0, 0x0400, 0x8700, 0x8FE0, 0x0001, 0x8700,
0x8FE0, 0x0301, 0x0700, 0x8FC0, 0x07E1, 0x0700, 0x4FC0, 0x07FF, 0x0700, 0xA7C0, 0x07FE, 0xC700,
0xA080, 0x0FFC, 0x7700, 0x2000, 0x0FFC, 0xFF00, 0x2020, 0x0FF5, 0xF800, 0x207C, 0x1FE7, 0xF800,
0x207F, 0x9F8F, 0xF000, 0x007F, 0xCE0F, 0xF100, 0x10FF, 0x819F, 0xF000, 0x7C9F, 0xC07F, 0xE000,
0x7E80, 0x81EF, 0xE200, 0x3380, 0x8FE1, 0xC000, 0x30C0, 0xFFC0, 0x7400, 0x18F9, 0xFF80, 0x7800,
0x0CFF, 0xFF80, 0xF000, 0x047E, 0xFF01, 0xE000, 0x067E, 0x1E03, 0xC000, 0x00FE, 0x0307, 0x8000,
0x00FC, 0x07EF, 0x0000, 0x0064, 0x0FE2, 0x0000, 0x0007, 0x3F80, 0x0000, 0x0003, 0x3C00, 0x0000,
0x0000, 0x0040, 0x0000, 0x0000, 0x1E38, 0x0000, 0x0000, 0x7E06, 0x0000, 0x0010, 0xFC07, 0x0000,
0x007C, 0x3C07, 0x8000, 0x04F8, 0x0007, 0xE000, 0x01F0, 0x0407, 0xE000, 0x03E0, 0x0787, 0xF000,
0x07C0, 0x0FE7, 0xF800, 0x23C0, 0x0FF0, 0xBC00, 0x0000, 0x1FF0, 0x7C00, 0x6040, 0x1FF0, 0x3E00,
0x4078, 0x3FE0, 0x2600, 0x40FE, 0x3FE0, 0x2200, 0xC0FF, 0x07E0, 0x2300, 0x80FF, 0x01E0, 0x2100,
0xC1FF, 0x0000, 0x2100, 0x81FE, 0x0000, 0x2100, 0x81FE, 0x003E, 0x6100, 0xF3FE, 0x003E, 0xE100,
0xBCFC, 0x003C, 0x3100, 0xB81C, 0x0078, 0x3F00, 0xD804, 0x0070, 0x7E00, 0xDC00, 0x0060, 0x7E00,
0xCC03, 0x80C0, 0x7F00, 0xC403, 0xF900, 0xFD00, 0x4407, 0xFF80, 0xFC00, 0x2407, 0xE2E0, 0xFE00,
0x7F08, 0x03FD, 0xFA00, 0x3FE8, 0x07FF, 0xFC00, 0x3CF8, 0x07FC, 0x1400, 0x1E18, 0x0FFC, 0x1800,
0x0E1E, 0x0FF8, 0x3000, 0x061F, 0xDFF0, 0x6000, 0x071F, 0xFFE0, 0xE000, 0x011F, 0xC3C1, 0x8000,
0x00FF, 0xC063, 0x0000, 0x0077, 0x81FE, 0x0000, 0x0019, 0x07F8, 0x0000, 0x0003, 0xCFC0, 0x0000,
0x0003, 0x8280, 0x0000, 0x001E, 0x0008, 0x0000, 0x0018, 0x07CE, 0x0000, 0x0000, 0x0FC1, 0x0000,
0x001F, 0x1F83, 0x8000, 0x003F, 0x8781, 0x6000, 0x007F, 0x0081, 0x6000, 0x00FE, 0x0000, 0xB000,
0x00FE, 0x0060, 0x9800, 0x01FC, 0x00FD, 0x9C00, 0x3C78, 0x00FE, 0x4C00, 0x3808, 0x00FE, 0x1E00,
0x7800, 0x01FE, 0x1E00, 0x7006, 0x01FF, 0x1800, 0xF00F, 0x01FE, 0x1800, 0xF00F, 0xF9FE, 0x1C00,
0xE01F, 0xF83E, 0x3C00, 0xE01F, 0xF806, 0x3C00, 0x001F, 0xF002, 0x3800, 0x903F, 0xF002, 0xF800,
0x9C3F, 0xF002, 0x3800, 0x9F9F, 0xE000, 0x0400, 0x9F87, 0xE004, 0x0700, 0xFF80, 0xE004, 0x0F00,
0xEF80, 0x0008, 0x0F00, 0xE380, 0x3808, 0x0F00, 0x7180, 0x3F10, 0x1E00, 0x3080, 0x6070, 0x1E00,
0x1C80, 0x401C, 0x3E00, 0x0FC0, 0x803F, 0xBC00, 0x0FFF, 0x807F, 0xF800, 0x0F9F, 0x007F, 0x8000,
0x0787, 0x00FF, 0x0000, 0x0381, 0xC0FE, 0x0000, 0x0381, 0xF9FE, 0x2000, 0x01C3, 0xFFF8, 0x0000,
0x0023, 0xF830, 0x0000, 0x001F, 0xF03C, 0x0000, 0x001D, 0xE0F8, 0x0000, 0x0001, 0xE7C0, 0x0000,
0x0000, 0x7180, 0x0000, 0x0003, 0xC000, 0x0000, 0x004F, 0x00CE, 0x0000, 0x0080, 0x01FF, 0x0000,
0x0101, 0x01F1, 0x8000, 0x0203, 0xE3F0, 0xE000, 0x0407, 0xF0F0, 0x4000, 0x080F, 0xF010, 0x5000,
0x181F, 0xE000, 0x0800, 0x003F, 0xE004, 0x2400, 0x0E3F, 0xC00F, 0xE400, 0x0F8F, 0xC00F, 0xB200,
0x1F01, 0x800F, 0xBE00, 0x1F00, 0x000F, 0xBE00, 0x3E00, 0x001F, 0xBE00, 0x3E00, 0x781F, 0xBE00,
0x7C00, 0xFF1F, 0x3E00, 0xBC00, 0xFFC7, 0x3E00, 0x5C00, 0xFFC1, 0x3E00, 0x0401, 0xFF80, 0xFE00,
0x0001, 0xFF80, 0xFE00, 0x0381, 0xFF80, 0xC200, 0x43E3, 0xFF81, 0xC100, 0x47F8, 0xFF03, 0xC300,
0x63F8, 0x1F07, 0x8300, 0x73F8, 0x0F0F, 0x8300, 0x7CF8, 0x033F, 0x0200, 0x7C10, 0x027F, 0x0600,
0x6C08, 0x020E, 0x0600, 0x2708, 0x3C01, 0x8C00, 0x33CF, 0xFC03, 0xFC00, 0x13FF, 0xF807, 0xE000,
0x0BF7, 0xF00F, 0xE000, 0x01F0, 0xF00F, 0xC000, 0x05F0, 0x181F, 0x8000, 0x01F0, 0x3F3F, 0x0000,
0x00B0, 0x7FBC, 0x0000, 0x0048, 0xFF0C, 0x0000, 0x000E, 0xFC38, 0x0000, 0x0000, 0x30C0, 0x0000};
STATIC UWORD boing_p2[480] = {
0x0000, 0x38C0, 0x0000, 0x0000, 0xFCC0, 0x0000, 0x0043, 0xF002, 0x0000, 0x00F3, 0xE03F, 0x0000,
0x01E0, 0x403C, 0x0000, 0x03C0, 0x707E, 0x2000, 0x0780, 0xFE7E, 0x2000, 0x0F00, 0xFFBE, 0x1000,
0x1E01, 0xFF0E, 0x1800, 0x2403, 0xFF00, 0x0C00, 0x0303, 0xFE00, 0x0C00, 0x03E7, 0xFE00, 0xE600,
0x07FB, 0xFE00, 0xFA00, 0x07F0, 0x7C01, 0xF800, 0x0FF0, 0x0C01, 0xF800, 0x0FE0, 0x0000, 0xF800,
0x0FE0, 0x0781, 0xF800, 0x1FE0, 0x07F1, 0xF800, 0x9FC0, 0x07F9, 0xF800, 0xE7C0, 0x0FFC, 0x3800,
0xC1C0, 0x0FFC, 0x0800, 0xC000, 0x0FF8, 0x0000, 0xC070, 0x1FF8, 0x0700, 0xC07E, 0x1FF8, 0x0700,
0xE07F, 0xFFF0, 0x0F00, 0xE07F, 0xC7F0, 0x0E00, 0x607F, 0xC0F0, 0x0E00, 0x007F, 0x8000, 0x1E00,
0x007F, 0x8018, 0x1C00, 0x0C7F, 0x003E, 0x3C00, 0x0F3F, 0x003F, 0x8800, 0x070F, 0x007F, 0x8000,
0x0380, 0x007F, 0x0000, 0x0381, 0x80FE, 0x0000, 0x0181, 0xE1FC, 0x2000, 0x0101, 0xFCF8, 0x0000,
0x0003, 0xF810, 0x0000, 0x001B, 0xF01C, 0x0000, 0x0018, 0xC078, 0x0000, 0x0000, 0xC3C0, 0x0000,
0x0003, 0x0C00, 0x0000, 0x0008, 0x1F60, 0x0000, 0x0020, 0x7F10, 0x0000, 0x0039, 0xFE00, 0x0000,
0x007C, 0x7C07, 0x0000, 0x00F8, 0x0807, 0x0000, 0x01F0, 0x0607, 0x8000, 0x03E0, 0x0F87, 0x8000,
0x07E0, 0x0FFF, 0xC000, 0x17C0, 0x1FF1, 0xC000, 0x3080, 0x1FF0, 0x0000, 0x2060, 0x3FF0, 0x0000,
0x207C, 0x3FF0, 0x1800, 0x40FF, 0x3FE0, 0x1C00, 0x40FF, 0x8FE0, 0x1C00, 0x41FF, 0x01E0, 0x1E00,
0x81FF, 0x0060, 0x1E00, 0x01FF, 0x0030, 0x1E00, 0x43FE, 0x0038, 0x1E00, 0x73FE, 0x003F, 0x1E00,
0x7DFE, 0x007F, 0xCE00, 0x7C3C, 0x007F, 0xC000, 0x7C04, 0x007F, 0xC100, 0x7802, 0x00FF, 0x8100,
0x7803, 0xC0FF, 0x8000, 0x3807, 0xE0FF, 0x8200, 0x3807, 0xFCFF, 0x0200, 0x5807, 0xFC1F, 0x0000,
0x0007, 0xFC06, 0x0400, 0x0007, 0xF800, 0x0000, 0x038F, 0xF803, 0xE800, 0x03EF, 0xF003, 0xE000,
0x01F1, 0xF007, 0xC000, 0x01F0, 0x600F, 0x8000, 0x00E0, 0x001F, 0x0000, 0x00E0, 0x3C3E, 0x0000,
0x0000, 0x3F9C, 0x0000, 0x0008, 0x7E00, 0x0000, 0x0006, 0xF800, 0x0000, 0x0000, 0x3000, 0x0000,
0x0003, 0xC700, 0x0000, 0x001E, 0x0318, 0x0000, 0x003C, 0x0FD8, 0x0000, 0x0008, 0x1FC0, 0x0000,
0x001F, 0xBFC0, 0x0000, 0x043F, 0x8F81, 0x8000, 0x007F, 0x0180, 0xC000, 0x00FF, 0x0001, 0xE000,
0x01FE, 0x00F0, 0xE000, 0x19FC, 0x00FE, 0xF000, 0x1CFC, 0x00FF, 0x3000, 0x3818, 0x01FF, 0x0000,
0x3804, 0x01FF, 0x0000, 0x780F, 0x01FE, 0x0600, 0x700F, 0xE3FE, 0x0700, 0x700F, 0xFFFE, 0x0700,
0xF01F, 0xF87E, 0x0700, 0x601F, 0xF80E, 0x0300, 0x203F, 0xF800, 0x0700, 0x103F, 0xF001, 0x0700,
0x1E3F, 0xF003, 0xE700, 0x1FFF, 0xF003, 0xFB00, 0x1F8F, 0xE003, 0xF800, 0x1F81, 0xE007, 0xF000,
0x1F80, 0x0007, 0xF000, 0x1F00, 0x0007, 0xF000, 0x1F00, 0x3C0F, 0xE000, 0x5F00, 0x3F8F, 0xE000,
0x7700, 0x7FE7, 0xC000, 0x3100, 0x7FC0, 0x4000, 0x3000, 0x7FC0, 0x0400, 0x1060, 0xFF80, 0x7800,
0x087C, 0xFF00, 0xF000, 0x047E, 0x7F01, 0xE000, 0x047E, 0x0601, 0xC000, 0x003C, 0x0007, 0x8000,
0x00DC, 0x07CF, 0x0000, 0x0060, 0x0FC2, 0x0000, 0x0002, 0x1F00, 0x0000, 0x0002, 0x1800, 0x0000,
0x0001, 0xF3C0, 0x0000, 0x0007, 0xC088, 0x0000, 0x004F, 0x80E4, 0x0000, 0x00C2, 0x01F0, 0x0000,
0x0183, 0x83F8, 0x0000, 0x0607, 0xF7F8, 0x4000, 0x040F, 0xF9F0, 0x6000, 0x0C0F, 0xF030, 0x3000,
0x181F, 0xF000, 0x3000, 0x083F, 0xE00E, 0x3800, 0x0F3F, 0xE00F, 0x1800, 0x1F9F, 0xC00F, 0xCC00,
0x1F03, 0xC00F, 0xC000, 0x3F00, 0x001F, 0xC000, 0x3E00, 0x601F, 0xC100, 0x3E00, 0x7C1F, 0xC100,
0x3E00, 0xFF9F, 0xC100, 0x7C00, 0xFFCF, 0xC100, 0xBC01, 0xFFC3, 0xC100, 0x8401, 0xFFC0, 0x0100,
0x8201, 0xFF80, 0x2100, 0x8383, 0xFF80, 0x3D00, 0x83F3, 0xFF00, 0x3E00, 0x83FD, 0xFF00, 0x7C00,
0x87F8, 0x3E00, 0x7C00, 0x87F8, 0x0600, 0x7C00, 0x03F8, 0x0000, 0xFC00, 0x03F8, 0x0180, 0xF800,
0x13F0, 0x03F9, 0xF800, 0x1DF0, 0x03FE, 0x7000, 0x0C30, 0x07FC, 0x0000, 0x0C00, 0x07F8, 0x1800,
0x040C, 0x0FF0, 0x1000, 0x060F, 0x8FF0, 0x2000, 0x020F, 0xE7E0, 0x6000, 0x000F, 0xC0C0, 0x8000,
0x004F, 0x8043, 0x0000, 0x0037, 0x00F2, 0x0000, 0x0011, 0x03C0, 0x0000, 0x0003, 0xCF00, 0x0000};
static struct BitMap bm_boing = {6, 160, 0, BOING_DEPTH, 0, {(UBYTE*)boing_p0, (UBYTE*)boing_p1, (UBYTE*)boing_p2, 0, 0, 0, 0, 0}};

STATIC UWORD boing_mask_p0[120] = {
0x0003, 0xFFC0, 0x0000, 0x001F, 0xFFF8, 0x0000, 0x007F, 0xFFFE, 0x0000, 0x00FF, 0xFFFF, 0x0000, 0x01FF, 0xFFFF, 0x8000, 0x07FF, 0xFFFF, 0xE000, 0x07FF, 0xFFFF, 0xE000, 0x0FFF, 0xFFFF, 0xF000,
0x1FFF, 0xFFFF, 0xF800, 0x3FFF, 0xFFFF, 0xFC00, 0x3FFF, 0xFFFF, 0xFC00, 0x7FFF, 0xFFFF, 0xFE00, 0x7FFF, 0xFFFF, 0xFE00, 0x7FFF, 0xFFFF, 0xFE00, 0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00,
0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00,
0xFFFF, 0xFFFF, 0xFF00, 0xFFFF, 0xFFFF, 0xFF00, 0x7FFF, 0xFFFF, 0xFE00, 0x7FFF, 0xFFFF, 0xFE00, 0x7FFF, 0xFFFF, 0xFE00, 0x3FFF, 0xFFFF, 0xFC00, 0x3FFF, 0xFFFF, 0xFC00, 0x1FFF, 0xFFFF, 0xF800,
0x0FFF, 0xFFFF, 0xF000, 0x07FF, 0xFFFF, 0xE000, 0x07FF, 0xFFFF, 0xE000, 0x01FF, 0xFFFF, 0x8000, 0x00FF, 0xFFFF, 0x0000, 0x007F, 0xFFFE, 0x0000, 0x001F, 0xFFF8, 0x0000, 0x0003, 0xFFC0, 0x0000};
static struct BitMap bm_boing_mask = {6, 40, 0, 1, 0, {(UBYTE*)boing_mask_p0, 0, 0, 0, 0, 0, 0, 0}};

//                          s  e  v  g  i  _  n
static ULONG font_data[] = {6, 6, 6, 6, 2, 7, 6};
static ULONG offs_data[] = {0, 0, 0, 0, 0, 0, 0};
//                          s  e  v  g  i  _  e  n  g  i  n  e
static ULONG text_data[] = {0, 1, 2, 3, 4, 5, 1, 6, 3, 4, 6, 1};

STATIC ULONG pixel_data[] = {
0x00000000, 0xC0000000,
0x00000000, 0x00000000,
0x7878CC7C, 0xC000F800,
0xC0CCCCCC, 0xC000CC00,
0x78F8CCCC, 0xC000CC00,
0x0CC0787C, 0xC000CC00,
0x7878300C, 0xC000CC00,
0x00000078, 0x00FE0000};
///

///copyBitMap(dst, src, dst_row)
/******************************************************************************
 * bm_boing is a non-standart bitmap data with 6 as its BytesPerRow.          *
 * BltBitMap() does not work on that (when it's not patched by SetPatch).     *
 * This is a basic CUSTOMIZED data copier for it.                             *
 * NOTE: This function is customized for this single use case only.           *
 * WARNING: Source bitmap has to be NON-INTERLEAVED!                          *
 ******************************************************************************/
VOID copyBitMap(struct BitMap* dst, struct BitMap* src, ULONG dst_row)
{
  ULONG bytes_per_plane_line;
  ULONG depth_to_copy = MIN(dst->Depth, src->Depth);
  ULONG rows_to_copy  = MIN(dst->Rows, src->Rows);
  ULONG p;
  ULONG r;

  if (GetBitMapAttr(dst, BMA_FLAGS) & BMF_INTERLEAVED)
    bytes_per_plane_line = MIN(dst->BytesPerRow / dst->Depth, src->BytesPerRow);
  else
    bytes_per_plane_line = MIN(dst->BytesPerRow, src->BytesPerRow);

  for (p = 0; p < depth_to_copy; p++) {
    for (r = 0; r < rows_to_copy; r++) {
      PLANEPTR plane_line = src->Planes[p] + (r * src->BytesPerRow);
      PLANEPTR dst_ptr = dst->Planes[p] + ((r + dst_row) * dst->BytesPerRow);
      CopyMem(plane_line, dst_ptr, bytes_per_plane_line);
    }
  }
}
///
///createChipData()
STATIC BOOL createChipData(VOID)
{
  ULONG text_bm_width = 0;
  ULONG text_bm_height = 8 * PIXEL_SIZE;
  ULONG i;

  for (i = 0; i < (sizeof(font_data) / sizeof(ULONG)); i++) {
    offs_data[i] = text_bm_width;
    text_bm_width += (font_data[i] * PIXEL_SIZE) + 1;
  }

  bmc_heart = allocBitMap(HEART_WIDTH, HEART_HEIGHT, HEART_DEPTH, BMF_DISPLAYABLE, splash_bitmap);
  bmc_heart_mask = allocBitMap(HEART_WIDTH, HEART_HEIGHT, 1, BMF_DISPLAYABLE, splash_bitmap);
  bmc_boing = allocBitMap(BOING_WIDTH, BOING_HEIGHT * (BOING_FRAMES + 1), BOING_DEPTH, BMF_DISPLAYABLE, splash_bitmap);
  bmc_boing_mask = allocBitMap(BOING_WIDTH, BOING_HEIGHT * BOING_FRAMES, 1, BMF_DISPLAYABLE, splash_bitmap);
  bmc_text = allocBitMap(text_bm_width, text_bm_height, 1, BMF_DISPLAYABLE, splash_bitmap);

  if (bmc_heart && bmc_heart_mask) {
    struct BitScaleArgs bsa;
    bsa.bsa_SrcX = 0;
    bsa.bsa_SrcY = 0;
    bsa.bsa_SrcWidth = HEART_DATA_WIDTH;
    bsa.bsa_SrcHeight = HEART_DATA_HEIGHT;
    bsa.bsa_XSrcFactor = 1;
    bsa.bsa_YSrcFactor = 1;
    bsa.bsa_DestX = 0;
    bsa.bsa_DestY = 0;
    bsa.bsa_XDestFactor = HEART_SCALE;
    bsa.bsa_YDestFactor = HEART_SCALE;
    bsa.bsa_SrcBitMap = &bm_heart;
    bsa.bsa_DestBitMap = bmc_heart;
    bsa.bsa_Flags = 0;

    BitMapScale(&bsa);
    bsa.bsa_SrcBitMap = &bm_heart_mask;
    bsa.bsa_DestBitMap = bmc_heart_mask;
    BitMapScale(&bsa);
  }

  if (bmc_boing) {
    //BltBitMap(&bm_boing, 0, 0, bmc_boing, 0, 0, BOING_WIDTH, BOING_HEIGHT * BOING_FRAMES, 0x0C0, 0x07, NULL);
    copyBitMap(bmc_boing, &bm_boing, 0);
    if (bmc_heart) {
      //copy initial background preserve into preserve buffer
      BltBitMap(bmc_heart, (BOING_X - BOING_WIDTH / 2) - HEART_X, (BOING_Y - BOING_HEIGHT / 2) - HEART_Y, bmc_boing, 0, BOING_HEIGHT * BOING_FRAMES, BOING_WIDTH, BOING_HEIGHT, 0xC0, 0x07, NULL);
    }
  }

  if (bmc_boing_mask) {
    ULONG i;
    for (i = 0; i < 4; i++) {
      //BltBitMap(&bm_boing_mask, 0, 0, bmc_boing_mask, 0, i * BOING_HEIGHT, BOING_WIDTH, BOING_HEIGHT, 0x0C0, 0x07, NULL);
      copyBitMap(bmc_boing_mask, &bm_boing_mask, BOING_HEIGHT * i);
    }
  }

  if (bmc_text) {
    ULONG x = 0;
    ULONG y = 0;
    ULONG i;
    UBYTE* data;

    SetRast(rastPort, 1);
    SetAPen(rastPort, BLACK);
    for (i = 0; i < (sizeof(font_data) / sizeof(ULONG)); i++) {
      ULONG r, c;
      data = (UBYTE*)pixel_data + i;

      for (r = 0; r < 8; r++, data += PIXEL_DATA_BPR) {
        UBYTE mask = 1 << 7;
        for (c = 0; c < font_data[i]; c++, mask >>= 1) {
          if (*data & mask) {
            drawPixel(x + c * PIXEL_SIZE, y + r * PIXEL_SIZE);
          }
        }
      }

      x += (font_data[i] * PIXEL_SIZE) + 1;
    }

    BltBitMap(rastPort->BitMap, 0, 0, bmc_text, 0, 0, text_bm_width, text_bm_height, 0x0C0, 0x01, NULL);
  }

  return (BOOL)(bmc_heart && bmc_boing && bmc_heart_mask && bmc_boing_mask);
}
///
///calculateOrbit(VOID)
/*
VOID calculateOrbit(ULONG step, LONG squeeze, LONG* x, LONG* y)
{
  FLOAT radians = SPMul(SPFlt(step), g_rad_conv);

  *x = SPFix(SPMul(g_radius, SPCos(radians)));
  *y = SPFix(SPMul(g_radius, SPSin(radians))) / squeeze;
}
*/
///
///initOrbit(radius, steps)
/*
VOID initOrbit(LONG radius, ULONG steps)
{
  ULONG l_pi     = 3373259586UL;           //value of FLOAT pi as an ULONG
  FLOAT pi       = *((FLOAT*) &l_pi);      //afp("3.1415926535897");

  g_rad_conv = SPDiv(SPFlt((LONG)steps), SPMul(SPFlt(2L), pi));
  g_radius = SPFlt(radius);

  g_coords = AllocMem(sizeof(struct Coords) * steps, MEMF_ANY);
  if (g_coords) {
    ULONG i;
    for (i = 0; i < steps; i++) {
      calculateOrbit(i, ORBIT_SQUEEZE, &g_coords[i].x, &g_coords[i].y);
    }
  }
}
*/
///
///animate()
VOID animate(VOID)
{
  static ULONG step = 0;
  static ULONG frame = 0;
  static ULONG skip = 0;
  static LONG x = (BOING_X - BOING_WIDTH / 2);
  static LONG y = (BOING_Y - BOING_HEIGHT / 2);
  static BOOL behind = TRUE;

  if (!new_frame_flag) return;
  //chase the beam
  WaitVBeam((DIWSTRT_V >> 8) + 150);

  //restore background
  WaitBlit();
  BltBitMap(bmc_boing, 0, BOING_HEIGHT * 4, splash_bitmap, x, y, BOING_WIDTH, BOING_HEIGHT, 0x0C0, 0x07, NULL);

  x = g_coords[step].x;
  y = g_coords[step].y;
  if (y < 0) behind = TRUE;
  else behind = FALSE;
  x += (BOING_X - BOING_WIDTH / 2);
  y += (BOING_Y - BOING_HEIGHT / 2);

  //store background
  WaitBlit();
  BltBitMap(splash_bitmap, x, y, bmc_boing, 0, BOING_HEIGHT * 4, BOING_WIDTH, BOING_HEIGHT, 0x0C0, 0x07, NULL);

  if (behind) {
    LONG cropped_x = (x - HEART_X) < 0 ? 0 : (x - HEART_X);
    LONG cropped_y = (y - HEART_Y) < 0 ? 0 : (y - HEART_Y);
    LONG cropped_x_dst = HEART_X + cropped_x;
    LONG cropped_y_dst = HEART_Y + cropped_y;
    LONG cropped_x_size;
    if (x < HEART_X) {
      cropped_x_size = x + BOING_WIDTH - HEART_X;
      if (cropped_x_size < 0) cropped_x_size = 0;
    }
    else {
      cropped_x_size = HEART_X + HEART_WIDTH - x;
      if (cropped_x_size < 0) cropped_x_size = 0;
      if (cropped_x_size > BOING_WIDTH) cropped_x_size = BOING_WIDTH;
    }
    //paste new boing
    WaitBlit();
    BltMaskBitMapRastPort(bmc_boing, 0, frame * BOING_HEIGHT, rastPort, x, y, BOING_WIDTH, BOING_HEIGHT, (ABC|ABNC|ANBC), bmc_boing_mask->Planes[0]);
    //fix damaged heart
    if (cropped_x_size) {
      WaitBlit();
      BltMaskBitMapRastPort(bmc_heart, cropped_x, cropped_y, rastPort, cropped_x_dst, cropped_y_dst, cropped_x_size, BOING_HEIGHT, (ABC|ABNC|ANBC), bmc_heart_mask->Planes[0]);
    }
  }
  else {
    //paste new boing
    WaitBlit();
    BltMaskBitMapRastPort(bmc_boing, 0, frame * BOING_HEIGHT, rastPort, x, y, BOING_WIDTH, BOING_HEIGHT, (ABC|ABNC|ANBC), bmc_boing_mask->Planes[0]);
  }

  step++;
  skip++;
  if (skip > BOING_SKIP) {
    skip = 0;
    frame++;
  }
  if (step >= ORBIT_STEPS) step = 0;
  if (frame > 3) frame = 0;
}
///
///drawPixel()
VOID drawPixel(ULONG x, ULONG y)
{
  AreaMove(rastPort, x, y); x += PIXEL_SIZE;
  AreaDraw(rastPort, x, y); y += PIXEL_SIZE;
  AreaDraw(rastPort, x, y); x -= PIXEL_SIZE;
  AreaDraw(rastPort, x, y);
  AreaEnd(rastPort);
}
///
///drawCursor()
VOID drawCursor(ULONG on, ULONG x, ULONG y)
{
  if (on)
    SetAPen(rastPort, BLACK);
  else
    SetAPen(rastPort, WHITE);

  AreaMove(rastPort, x, y); x += (PIXEL_SIZE * 2);
  AreaDraw(rastPort, x, y); y += (PIXEL_SIZE * 8);
  AreaDraw(rastPort, x, y); x -= (PIXEL_SIZE * 2);
  AreaDraw(rastPort, x, y);
  AreaEnd(rastPort);
}
///
///drawVersionText()
STATIC VOID drawVersionText(VOID)
{
  STRPTR version = "v" ENGINE_VERSION_STRING;
  struct TextExtent te;

  SetFont(rastPort, textFonts[TF_DEFAULT]);
  SetAPen(rastPort, BLACK);
  SetBPen(rastPort, WHITE);
  TextExtent(rastPort, version, strlen(version), &te);
  Move(rastPort, VERSION_TEXT_X - te.te_Width, VERSION_TEXT_Y);
  Text(rastPort, version, strlen(version));
}
///
///drawText()
STATIC VOID drawText(VOID)
{
  static ULONG step = 0;
  static ULONG letter = 0;
  static ULONG x = LOGO_TEXT_X;
  static ULONG y = LOGO_TEXT_Y;

  rastPort->Mask = 0x01;

  if (step < INIT_IDLE_CYCLES) {
    if (step % CURSOR_FRAMES == 0) {
      drawCursor(!((step / CURSOR_FRAMES) % 2), x, y);
    }
  }
  else if (step < (INIT_IDLE_CYCLES + LOGO_LETTERS * LOGO_FRAMES) && !(step % LOGO_FRAMES)) {
    if (letter <= LOGO_LETTERS) {
      ULONG letter_i = text_data[letter];
      //remove the cursor
      drawCursor(0, x, y);

      SetAPen(rastPort, BLACK);
      //draw letter
      BltBitMap(bmc_text, offs_data[letter_i], 0, rastPort->BitMap, x, y, font_data[letter_i] * PIXEL_SIZE + 1, 8 * PIXEL_SIZE, 0x0C0, 0x01, NULL);

      if (letter == LOGO_LETTERS - 1) {
        drawVersionText();
      }

      x += (font_data[letter_i] + 1) * PIXEL_SIZE;

      //draw cursor to new position
      drawCursor(1, x, y);

      letter++;
    }
  }
  else {
    // animate idling cursor
    if (step % CURSOR_FRAMES == 0) {
      drawCursor(!((step / CURSOR_FRAMES) % 2), x, y);
    }
  }

  rastPort->Mask = 0x07;

  step++;
}
///
///drawScreen()
STATIC VOID drawScreen(VOID)
{
  SetRast(rastPort, 1);
  WaitBlit();
  BltMaskBitMapRastPort(bmc_heart, 0, 0, rastPort, HEART_X, HEART_Y, HEART_WIDTH, HEART_HEIGHT, (ABC|ABNC|ANBC), bmc_heart_mask->Planes[0]);
  waitTOF();
}
///

///vblankEvents()
STATIC VOID vblankEvents(VOID)
{
#ifdef USE_CLP
  setColorTable_CLP(color_table, CL_PALETTE, 1, color_table->colors);
#else
  setColorTable(color_table);
#endif // USE_CLP
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
  rastPort = allocRastPort(SPLASH_BITMAP_WIDTH, SPLASH_BITMAP_HEIGHT, SPLASH_BITMAP_DEPTH,
                           BMF_STANDARD | BMF_DISPLAYABLE | BMF_CLEAR, 0,
                           RPF_BITMAP | RPF_TMPRAS | RPF_AREA, MAXVECTORS);

  if (rastPort) {
    splash_bitmap = rastPort->BitMap;
  }

  return rastPort;
}
///
///closeScreen()
STATIC VOID closeScreen(VOID)
{
  freeRastPort(rastPort, RPF_ALL);
  rastPort = NULL;
  splash_bitmap = NULL;
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
    for (wp = CL_BPL1PTH, i = 0; i < SPLASH_SCREEN_DEPTH; i++) {
      *wp = (WORD)((ULONG)splash_bitmap->Planes[i] >> 16); wp += 2;
      *wp = (WORD)((ULONG)splash_bitmap->Planes[i] & 0xFFFF); wp += 2;
    }

    //Set all sprite pointers to null_sprite
    for (sp = CL_SPR0PTH; sp < CL_SPR0PTH + 32; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }
  }
  else
    puts("Couldn't allocate Splash CopperList!");

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

///openDisplay()
STATIC BOOL openDisplay(VOID)
{
  //DO YOUR EXTRA ALLOCATIONS FOR YOUR SCREEN ANIMATIONS ECT. HERE
  //initOrbit(ORBIT_RADIUS, ORBIT_STEPS);

  if ((color_table = newColorTable(colors, CT_DEFAULT_STEPS, 0))) {
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
  //FREE THE EXTRA ALLOCATIONS FOR YOUR SCREEN ANIMATIONS ECT. HERE
  //FreeMem(g_coords, sizeof(struct Coords) * ORBIT_STEPS);

  if (bmc_boing) FreeBitMap(bmc_boing); bmc_boing = NULL;
  if (bmc_heart) FreeBitMap(bmc_heart); bmc_heart = NULL;
  if (bmc_boing_mask) FreeBitMap(bmc_boing_mask); bmc_boing_mask = NULL;
  if (bmc_heart_mask) FreeBitMap(bmc_heart_mask); bmc_heart_mask = NULL;
  if (bmc_text) FreeBitMap(bmc_text); bmc_text = NULL;

  disposeCopperList();
  closeScreen();
  if (color_table) {
    freeColorTable(color_table); color_table = NULL;
  }
}
///

///splashDisplayLoop()
STATIC VOID splashDisplayLoop(VOID)
{
  ULONG frameCount = 240;
  struct MouseState ms;

  while(color_table->state != CT_IDLE) {
    new_frame_flag = 1;
    animate();
    drawText();
    updateColorTable(color_table);
    waitTOF();
  }
  while (frameCount--) {
    new_frame_flag = 1;
    animate();
    drawText();

    doKeyboardIO();
    UL_VALUE(ms) = readMouse(0);

    if (keyState(RAW_ESC) || keyState(RAW_SPACE) || JOY_BUTTON1(0) || JOY_BUTTON1(1)) {
      waitTOF();
      break;
    };
    waitTOF();
  }
  color_table->state = CT_FADE_OUT;
  while (color_table->state != CT_IDLE) {
    new_frame_flag = 1;
    animate();
    drawText();
    updateColorTable(color_table);
    waitTOF();
  }
}
///
///switchToSplashCopperList()
STATIC VOID switchToSplashCopperList(VOID)
{
  custom.cop2lc = (ULONG)CopperList;
  setVBlankEvents(vblankEvents);
  new_frame_flag = 1;
  waitTOF();
}
///

///showSplashDisplay()
VOID showSplashDisplay(VOID)
{
  if(openDisplay()) {
    //DRAW INITIAL SCREEN HERE!
    drawScreen();
    switchToSplashCopperList();
    splashDisplayLoop();
    switchToNullCopperList();
    closeDisplay();
  }
}
///
