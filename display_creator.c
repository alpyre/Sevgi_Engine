/******************************************************************************
 * DisplayCreator                                                             *
 ******************************************************************************/
///Defines
#define MUIA_Enabled 0 //Not a real attribute, just a value to make reading easier

#define MUIM_DisplayCreator_OpenPaletteSelector  0x80430C0B
#define MUIM_DisplayCreator_SetPalette           0x80430C0C
#define MUIM_DisplayCreator_CheckValidity        0x80430C0D
#define MUIM_DisplayCreator_Create               0x80430C0E
#define MUIM_DisplayCreator_Reset                0x80430C0F
///
///Includes
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "integer_gadget.h"
#include "palette_selector.h"
#include "display_creator.h"
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;

extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;

extern Object* App;
extern struct MUI_CustomClass *MUIC_PopASLString;
extern struct MUI_CustomClass *MUIC_Integer;
extern struct MUI_CustomClass *MUIC_AckString;
extern struct MUI_CustomClass* MUIC_DisplayCreator;

STRPTR black_list[] = {
  "level",
  "loading",
  "menu",
  "splash",
  NULL
};

static struct {
  STRPTR str_name;
  STRPTR scr_width;
  STRPTR scr_height;
  STRPTR scr_depth;
  STRPTR palette;
  STRPTR AGA;
  STRPTR interleaved;
  STRPTR hires;
  STRPTR interlaced;
  STRPTR layer;
  STRPTR tmpras;
  STRPTR area;
  STRPTR maxvectors;
  STRPTR pltOnCL;
  STRPTR create;
  STRPTR cancel;
}help_string = {
  "Name for your new display.\nThis string will be used in filenames, defines and\nfunction names in proper letter casing.\nYou can use either snake or camel case if your\ndisplay name consists of more than one word.",
  "Width of your display.",
  "Height of your display.",
  "Color depth of your display.",
  "The color palette to be used for this display.\nIt must match the color depth selected.",
  "Enable AGA features for this display.",
  "Selecting this will allocate an interleaved bitmap for this display.",
  "Enable 640 pixel horizontal resolution.",
  "Enable 512 pixel vertical resolution.\nWill flicker on Amigas without a flicker fixer",
  "Allocate a Layer struct for your display's RastPort.",
  "Allocate a TempRas for your display's RastPort.",
  "Allocate an Area struct for your dispay's RastPort.\nThis makes possible using the Area functions\nfrom the API for drawing.",
  "Size of the vector buffer for Area functions.",
  "Creates a section of MOVE instructions for the color registers\non the CopperList created for this display.\nThis mandates the use of color functions named with the \"_CLP\"\ntag for color assignments and fade in/out effects.\nAn access pointer pointing to this section named CL_PALETTE\nwill also be defined.",
  "Create new display.",
  "Close this window."
};

//Code:
struct {
  STRPTR includes;
  STRPTR include_AGA;
  STRPTR define_1;
  STRPTR define_2;
  STRPTR define_2_interleaved;
  STRPTR define_3;
  STRPTR define_3_AGA_0;
  STRPTR define_end;
  STRPTR define_3_hires;
  STRPTR define_3_laced;
  STRPTR define_3_AGA_1;
  STRPTR define_3_AGA_2;
  STRPTR globals;
  STRPTR copperlist_0;
  STRPTR copperlist_1;
  STRPTR copperlist_1_laced;
  STRPTR copperlist_CLP;
  STRPTR copperlist_CLP_laced;
  STRPTR copperlist_2;
  STRPTR copperlist_2_disp;
  STRPTR copperlist_2_AGA;
  STRPTR copperlist_bitplanes;
  STRPTR copperlist_2_laced;
  STRPTR copperlist_3;
  STRPTR colors;
  STRPTR vblank;
  STRPTR vblank_laced;
  STRPTR color_table;
  STRPTR color_table_CLP;
  STRPTR color_table_CLP_laced;
  STRPTR screen;
  STRPTR copperlist_functions;
  STRPTR copperlist_functions_laced;
  STRPTR display;
  STRPTR switch_functions;
  STRPTR switch_functions_laced;
  STRPTR show;
  STRPTR header_file;
}code_string = {
  "///includes\n%s"                                                                       // includes
  "#include <stdio.h>\n"
  "#include <string.h>\n\n"

  "#include <exec/exec.h>\n"
  "#include <intuition/screens.h>\n"
  "#include <graphics/gfx.h>\n"
  "#include <graphics/display.h>\n"
  "#include <hardware/custom.h>\n"
  "#include <hardware/cia.h>\n"
  "#include <hardware/dmabits.h>\n"
  "#include <hardware/intbits.h>\n"

  "#include <proto/exec.h>\n"
  "#include <proto/graphics.h>\n"
  "#include <proto/intuition.h>\n\n"

  "#include \"system.h\"\n"
  "#include \"color.h\"\n"
  "#include \"cop_inst_macros.h\"\n"
  "#include \"input.h\"\n"
  "#include \"keyboard.h\"\n"
  "#include \"diskio.h\"\n"
  "#include \"fonts.h\"\n"
  "#include \"palettes.h\"\n"
  "#include \"settings.h\"\n\n"

  "#include \"display.h\"\n"
  "#include \"display_%s.h\"\n"
  "///\n",

  "#define ECS_SPECIFIC\n\n",                                                             // include_AGA

  "///defines (private)\n"                                                                // define_1
  "#define %s_SCREEN_WIDTH  %lu\n"
  "#define %s_SCREEN_HEIGHT %lu\n"
  "#define %s_SCREEN_DEPTH  %lu\n"
  "#define %s_SCREEN_COLORS (1 << %s_SCREEN_DEPTH)\n\n"

  "#define %s_BITMAP_WIDTH  %s_SCREEN_WIDTH\n"
  "#define %s_BITMAP_HEIGHT %s_SCREEN_HEIGHT\n"
  "#define %s_BITMAP_DEPTH  %s_SCREEN_DEPTH\n\n"

  "#define DDFSTRT_V DEFAULT_DDFSTRT_LORES\n"
  "#define DDFSTOP_V DEFAULT_DDFSTOP_LORES\n"
  "#define DIWSTRT_V DEFAULT_DIWSTRT\n"
  "#define DIWSTOP_V DEFAULT_DIWSTOP\n\n",

  "#define BPLXMOD_V ((%s_BITMAP_WIDTH / 8) & 0xFFFF)\n\n",                               // define_2

  "#define BPLXMOD_V ((%s_BITMAP_WIDTH / 8 * (%s_BITMAP_DEPTH%s - 1)) & 0xFFFF)\n\n",     // define_2_interleaved

  "#define BPLCON0_V ((%s%s%s_SCREEN_DEPTH * BPLCON0_BPU0) | BPLCON0_COLOR%s%s%s)\n"    // define_3

  "#define MAXVECTORS %lu\n",

  "#define SPR_FMODE %lu\n"                                                               // define_3_AGA_0
  "#define BPL_FMODE %lu\n\n"

  "#if BPL_FMODE == 1\n"
  "  #define BPL_FMODE_V 0x0\n"
  "#elif BPL_FMODE == 2\n"
  "  #define BPL_FMODE_V FMODE_BLP32\n"
  "#elif BPL_FMODE == 4\n"
  "  #define BPL_FMODE_V FMODE_BLP32 | FMODE_BPAGEM\n"
  "#endif\n\n"

  "#if SPR_FMODE == 1\n"
  "  #define FMODE_V (BPL_FMODE_V)\n"
  "#elif SPR_FMODE == 2\n"
  "  #define FMODE_V (BPL_FMODE_V | FMODE_SPR32)\n"
  "#elif SPR_FMODE == 4\n"
  "  #define FMODE_V (BPL_FMODE_V | FMODE_SPR32 | FMODE_SPAGEM)\n"
  "#endif\n",

  "///\n",                                                                                // define_end

  " | BPLCON0_HIRES",                                                                     // define_3_hires
  " | BPLCON0_LACE",                                                                      // define_3_laced
  "_SCREEN_DEPTH == 8 ? BPLCON0_BPU3 : ",                                                 // define_3_AGA_1
  " | USE_BPLCON3",                                                                       // define_3_AGA_2

  "///globals\n"                                                                          //  globals
  "// imported globals\n"
  "extern struct Custom custom;\n"
  "extern struct CIA ciaa, ciab;\n"
  "extern volatile LONG new_frame_flag;\n"
  "extern volatile ULONG g_frame_counter;\n"
  "extern UWORD NULL_SPRITE_ADDRESS_H;\n"
  "extern UWORD NULL_SPRITE_ADDRESS_L;\n"
  "extern struct TextFont* textFonts[NUM_TEXTFONTS];\n"
  "extern struct GameFont* gameFonts[NUM_GAMEFONTS];\n\n"

  "// private globals\n"
  "STATIC struct RastPort* rastPort = NULL;\n"
  "STATIC struct BitMap* bitmap = NULL;\n"
  "///\n",

  "///copperlist\n"                                                                       // copperlist_0
  "STATIC UWORD* CopperList = (UWORD*) 0;\n\n",

  "%sSTATIC UWORD* CL_BPL1PTH = (UWORD*) 0;\n"                                            // copperlist_1
  "STATIC UWORD* CL_SPR0PTH = (UWORD*) 0;\n\n",

  "STATIC UWORD* CopperList1 = (UWORD*) 0;\n"                                             // copperlist_1_laced
  "STATIC UWORD* CopperList2 = (UWORD*) 0;\n\n"

  "%sSTATIC UWORD* CL_BPL1PTH_1 = (UWORD*) 0;\n"
  "STATIC UWORD* CL_BPL1PTH_2 = (UWORD*) 0;\n"
  "STATIC UWORD* CL_COP2LCH_1 = (UWORD*) 0;\n"
  "STATIC UWORD* CL_COP2LCH_2 = (UWORD*) 0;\n"
  "STATIC UWORD* CL_COP2LCL_1 = (UWORD*) 0;\n"
  "STATIC UWORD* CL_COP2LCL_2 = (UWORD*) 0;\n"
  "STATIC UWORD* CL_SPR0PTH_1 = (UWORD*) 0;\n"
  "STATIC UWORD* CL_SPR0PTH_2 = (UWORD*) 0;\n\n",

  "STATIC UWORD* CL_PALETTE = (UWORD*) 0;\n",                                             // copperlist_CLP

  "STATIC UWORD* CL_PALETTE_1 = (UWORD*) 0;\n"                                            // copperlist_CLP_laced
  "STATIC UWORD* CL_PALETTE_2 = (UWORD*) 0;\n",

  "STATIC ULONG copperList_Instructions[] = {\n"                                          // copperlist_2
  "                                              // Access Ptr:  Action:\n",
  "  MOVE(BPLCON0, BPLCON0_V),                   //              Set display properties\n"// copperlist_2_disp
  "  MOVE(BPLCON1, 0),                           //              Set h_scroll register\n"
  "  MOVE(BPLCON2, 0x264),                       //              Set sprite priorities\n"
  "  MOVE(BPL1MOD, BPLXMOD_V),                   //              Set bitplane mods\n"
  "  MOVE(BPL2MOD, BPLXMOD_V),                   //               \"     \"       \"\n"
  "  MOVE(DIWSTRT, DIWSTRT_V),                   //              Set Display Window Start\n"
  "  MOVE(DIWSTOP, DIWSTOP_V),                   //              Set Display Window Stop\n"
  "  MOVE(DDFSTRT, DDFSTRT_V),                   //              Set Data Fetch Start to fetch early\n"
  "  MOVE(DDFSTOP, DDFSTOP_V),                   //              Set Data Fetch Stop\n"
  "  MOVE_PH(BPL1PTH, 0),                        // CL_BPL1PTH   Set bitplane addresses\n"
  "  MOVE(BPL1PTL, 0),                           //               \"      \"       \"\n",

  "  MOVE(FMODE,   FMODE_V),                     //              Set Sprite/Bitplane Fetch Modes\n",

  "  MOVE(BPL%ldPTH, 0),                           //               \"      \"       \"\n"
  "  MOVE(BPL%ldPTL, 0),                           //               \"      \"       \"\n",

  "  MOVE_PH(COP2LCH, 0),                        // CL_COP2LCH   Points to the other copper...\n"
  "  MOVE_PH(COP2LCL, 0),                        // CL_COP2LCL   ...for auto jump every other frame\n",

  "  MOVE_PH(SPR0PTH, 0),                        // CL_SPR0PTH   Set sprite pointers\n"
  "  MOVE(SPR0PTL, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR1PTH, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR1PTL, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR2PTH, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR2PTL, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR3PTH, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR3PTL, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR4PTH, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR4PTL, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR5PTH, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR5PTL, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR6PTH, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR6PTL, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR7PTH, 0),                           //               \"     \"      \"\n"
  "  MOVE(SPR7PTL, 0),                           //               \"     \"      \"\n"
  "  END\n"
  "};\n"
  "///\n",

  "///colors\n"                                                                           // colors
  "STATIC struct ColorTable* color_table = NULL;\n"
  "///\n"
  "///protos (private)\n"
  "STATIC VOID closeScreen(VOID);\n"
  "STATIC VOID closeDisplay(VOID);\n"
  "///\n\n",

  "///vblankEvents()\n"                                                                   // vblank
  "STATIC VOID vblankEvents(VOID)\n"
  "{\n%s"
  "}\n"
  "///\n",

  "///vblankEvents()\n"                                                                   // vblank_laced
  "STATIC VOID vblankEvents(VOID)\n"
  "{\n"
  "  if (CopperList == CopperList2) {\n"
  "    CopperList = CopperList1;\n"
  "  }\n"
  "  else {\n"
  "    CopperList = CopperList2;\n"
  "  }\n%s"
  "}\n"
  "///\n",

  "  setColorTable(color_table);\n",                                                      // color_table
  "    waitVBeam(8);\n"                                                                   // color_table_CLP
  "    setColorTable_CLP(color_table, CL_PALETTE, 0, color_table->colors);\n",
  "    if (CopperList == CopperList1)\n"                                                  // color_table_CLP_laced
  "      setColorTable_CLP(color_table, CL_PALETTE_1, 0, color_table->colors);\n"
  "    else\n"
  "      setColorTable_CLP(color_table, CL_PALETTE_2, 0, color_table->colors);\n",

  "///openScreen()\n"                                                                     // screen
  "/******************************************************************************\n"
  " * Albeit the name, this one does not open a screen. It just allocates a      *\n"
  " * bitmap and sets up a RastPort for it. A bitmap and a copperlist is enough  *\n"
  " * for all our displays (except the null_display of course).                  *\n"
  " ******************************************************************************/\n"
  "STATIC struct RastPort* openScreen(VOID)\n"
  "{\n"
  "  rastPort = allocRastPort(%s_BITMAP_WIDTH, %s_BITMAP_HEIGHT, %s_BITMAP_DEPTH,\n"
  "                           BMF_STANDARD | BMF_DISPLAYABLE | BMF_CLEAR%s, 0,\n"
  "                           RPF_BITMAP%s%s%s, MAXVECTORS);\n\n"

  "  if (rastPort) {\n"
  "    bitmap = rastPort->BitMap;\n"
  "  }\n\n"

  "  return rastPort;\n"
  "}\n"
  "///\n"
  "///closeScreen()\n"
  "STATIC VOID closeScreen(VOID)\n"
  "{\n"
  "  freeRastPort(rastPort, RPF_ALL);\n"
  "  rastPort = NULL;\n"
  "  bitmap = NULL;\n"
  "}\n"
  "///\n"
  "///drawScreen()\n"
  "STATIC VOID drawScreen(VOID)\n"
  "{\n"
  "  //DRAW THE INITIAL SCREEN FEATURES HERE!\n\n"

  "}\n"
  "///\n",

  "///createCopperList()\n"                                                                // copperlist_functions
  "STATIC UWORD* createCopperList(VOID)\n"
  "{\n"
  "  if (allocCopperList(copperList_Instructions, CopperList, CL_SINGLE)) {\n"
  "    UWORD* wp;\n"
  "    UWORD* sp;\n"
  "    ULONG i;\n\n"

  "    //Set copperlist bitmap instruction point to screen bitmap\n"
  "    for (wp = CL_BPL1PTH, i = 0; i < %s_SCREEN_DEPTH; i++) {\n"
  "      *wp = (WORD)((ULONG)bitmap->Planes[i] >> 16); wp += 2;\n"
  "      *wp = (WORD)((ULONG)bitmap->Planes[i] & 0xFFFF); wp += 2;\n"
  "    }\n\n"

  "    //Set all sprite pointers to null_sprite\n"
  "    for (sp = CL_SPR0PTH; sp < CL_SPR0PTH + 32; sp += 2) {\n"
  "      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;\n"
  "      *sp = NULL_SPRITE_ADDRESS_L;\n"
  "    }\n"
  "  }\n"
  "  else\n"
  "    puts(\"Couldn't allocate %s CopperList!\");\n\n"

  "  return CopperList;\n"
  "}\n"
  "///\n"
  "///disposeCopperList()\n"
  "STATIC VOID disposeCopperList(VOID)\n"
  "{\n"
  "  if (CopperList) {\n"
  "    freeCopperList(CopperList);\n"
  "    CopperList = NULL;\n"
  "  }\n"
  "}\n"
  "///\n",

  "///createCopperList()\n"                                                               // copperlist_functions_laced
  "STATIC UWORD* createCopperList()\n"
  "{\n"
  "  if (allocCopperList(copperList_Instructions, CopperList, CL_DOUBLE)) {\n"
  "    UWORD* wp;\n"
  "    UWORD* sp;\n"
  "    ULONG i;\n\n"

  "    //Set copperlist bitmap instructions point to screen bitmap\n"
  "    for (wp = CL_BPL1PTH_1, i = 0; i < %s_SCREEN_DEPTH; i++) {\n"
  "      *wp = (WORD)((ULONG)bitmap->Planes[i] >> 16); wp += 2;\n"
  "      *wp = (WORD)((ULONG)bitmap->Planes[i] & 0xFFFF); wp += 2;\n"
  "    }\n\n"

  "    for (wp = CL_BPL1PTH_2, i = 0; i < %s_SCREEN_DEPTH; i++) {\n"
  "      *wp = (WORD)(((ULONG)bitmap->Planes[i] + %s_BITMAP_WIDTH%s%s%s / 8) >> 16); wp += 2;\n"
  "      *wp = (WORD)(((ULONG)bitmap->Planes[i] + %s_BITMAP_WIDTH%s%s%s / 8) & 0xFFFF); wp += 2;\n"
  "    }\n\n"

  "    //Set all sprite pointers to null_sprite\n"
  "    for (sp = CL_SPR0PTH_1; sp < CL_SPR0PTH_1 + 32; sp += 2) {\n"
  "      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;\n"
  "      *sp = NULL_SPRITE_ADDRESS_L;\n"
  "    }\n\n"

  "    for (sp = CL_SPR0PTH_2; sp < CL_SPR0PTH_2 + 32; sp += 2) {\n"
  "      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;\n"
  "      *sp = NULL_SPRITE_ADDRESS_L;\n"
  "    }\n\n"

  "    //Set both copperlists to point to each other (interlaced copper)\n"
  "    *CL_COP2LCH_1 = (WORD)((ULONG)CopperList2 >> 16);\n"
  "    *CL_COP2LCL_1 = (WORD)((ULONG)CopperList2 & 0xFFFF);\n"
  "    *CL_COP2LCH_2 = (WORD)((ULONG)CopperList1 >> 16);\n"
  "    *CL_COP2LCL_2 = (WORD)((ULONG)CopperList1 & 0xFFFF);\n"
  "  }\n"
  "  else\n"
  "    puts(\"Couldn't allocate %s CopperList!\");\n\n"

  "  return CopperList;\n"
  "}\n"
  "///\n"
  "///disposeCopperList()\n"
  "STATIC VOID disposeCopperList()\n"
  "{\n"
  "  if (CopperList) {\n"
  "    freeCopperList(CopperList1);\n"
  "    CopperList = NULL; CopperList1 = NULL; CopperList2 = NULL;\n"
  "  }\n"
  "}\n"
  "///\n",

  "///openDisplay()\n"                                                                    // display
  "STATIC BOOL openDisplay(VOID)\n"
  "{\n"
  "  if ((color_table = newColorTable(%s, CT_DEFAULT_STEPS, 0))) {\n"
  "    color_table->state = CT_FADE_IN;\n"
  "    if (openScreen()) {\n"
  "      if (createCopperList()) {\n"
  "        return TRUE;\n"
  "      }\n"
  "      else closeDisplay();\n"
  "    }\n"
  "    else closeDisplay();\n"
  "  }\n\n"

  "  return FALSE;\n"
  "}\n"
  "///\n"
  "///closeDisplay()\n"
  "STATIC VOID closeDisplay(VOID)\n"
  "{\n"
  "  disposeCopperList();\n"
  "  closeScreen();\n"
  "  if (color_table) {\n"
  "    freeColorTable(color_table); color_table = NULL;\n"
  "  }\n"
  "}\n"
  "///\n\n"

  "///displayLoop()\n"
  "STATIC VOID displayLoop(VOID)\n"
  "{\n"
  "  struct MouseState ms;\n"
  "  UWORD exiting = FALSE;\n\n"

  "  while(TRUE) {\n"
  "    new_frame_flag = 1;\n"
  "    doKeyboardIO();\n\n"

  "    if (keyState(RAW_ESC) && !exiting) {\n"
  "      exiting = TRUE;\n"
  "      color_table->state = CT_FADE_OUT;\n"
  "    }\n"
  "    if (exiting == TRUE && color_table->state == CT_IDLE) {\n"
  "      break;\n"
  "    }\n\n"

  "    updateColorTable(color_table);\n%s"
  "    waitTOF();\n"
  "  }\n"
  "}\n"
  "///\n",

  "///switchTo%sCopperList()\n"                                                           // switch_functions
  "STATIC VOID switchTo%sCopperList(VOID)\n"
  "{\n"
  "  custom.cop2lc = (ULONG)CopperList;\n"
  "  setVBlankEvents(vblankEvents);\n"
  "  new_frame_flag = 1;\n"
  "  waitTOF();\n"
  "}\n"
  "///\n",

  "///switchTo%sCopperList()\n"                                                           // switch_functions_laced
  "STATIC VOID switchTo%sCopperList(VOID)\n"
  "{\n"
  "  WaitVBeam(299);                 // We have to wait Copper to set cop2lc on an interlaced display!\n"
  "  custom.dmacon = DMAF_COPPER;    // Disable copperlist DMA so it does not reset our lace bit below!\n"
  "  custom.bplcon0 = BPLCON0_LACE;  // Set LACE bit\n"
  "  waitTOF();                      // Wait until you get a vertical blank\n"
  "  new_frame_flag = 1;\n\n"

  "  if (custom.vposr & VPOS_LOF) {  // Check if this frame is a long frame or a short frame!\n"
  "    CopperList = CopperList1;\n"
  "    custom.cop2lc = (ULONG)CopperList1;\n"
  "  }\n"
  "  else {\n"
  "    CopperList = CopperList2;\n"
  "    custom.cop2lc = (ULONG)CopperList2;\n"
  "  }\n\n"

  "  custom.dmacon = DMAF_SETCLR | DMAF_COPPER; // Reactivate copperlist DMA!\n\n"

  "  setVBlankEvents(vblankEvents);\n"
  "  waitTOF();\n"
  "}\n"
  "///\n",

  "///show%sDisplay()\n"                                                                  // show
  "VOID show%sDisplay(VOID)\n"
  "{\n"
  "  if(openDisplay()) {\n"
  "    drawScreen();\n"
  "    switchTo%sCopperList();\n"
  "    displayLoop();\n"
  "    switchToNullCopperList();\n"
  "    closeDisplay();\n"
  "  }\n"
  "}\n"
  "///\n",

  "#ifndef %s_DISPLAY_H\n"                                                                // header_file
  "#define %s_DISPLAY_H\n\n"

  "VOID show%sDisplay(VOID);\n\n"

  "#endif /* %s_DISPLAY_H */"
};
///
///Structs
struct PaletteItem {
  STRPTR name;
  UBYTE* palette;
};

struct cl_ObjTable
{
  Object* str_name;
  Object* int_scrWidth;
  Object* int_scrHeight;
  Object* int_scrDepth;
  Object* txt_palette;
  Object* pop_palette;
  Object* int_sprFetchMode;
  Object* int_bplFetchMode;
  Object* chk_AGA;
  Object* chk_interleaved;
  Object* chk_hires;
  Object* chk_interlaced;
  Object* chk_Layer;
  Object* chk_TmpRas;
  Object* chk_Area;
  Object* int_maxvectors;
  Object* chk_pltOnCL;
  Object* btn_create;
  Object* btn_cancel;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  Object* AGACheck;
  Object* CLPCheck;
  UBYTE* palette;
  Object* palette_selector;
  BOOL subscribed_to_palette_selector;
};

struct cl_Msg
{
  ULONG MethodID;
  //<SUBCLASS METHOD MESSAGE PAYLOAD HERE>
};

struct cl_Msg3
{
  ULONG MethodID;
  struct PaletteItem* palette_item;
};
///

///isCamelCase(str)
BOOL isCamelCase(STRPTR str)
{
  BOOL upper_case_found = FALSE;
  BOOL lower_case_found = FALSE;
  UBYTE* c = str;

  while (*c) {
    if (*c >= 'A' && *c <= 'Z') upper_case_found = TRUE;
    if (*c >= 'a' && *c <= 'z') lower_case_found = TRUE;

    c++;
  }

  return upper_case_found && lower_case_found;
}
///
///isBlackListed(str)
BOOL isBlackListed(STRPTR str)
{
  STRPTR* bl_item = black_list;

  while (*bl_item) {
    STRPTR c = str;
    STRPTR b = *bl_item;

    while (*c) {
      UBYTE ch = *c;
      if (ch >= 'A' && ch <= 'Z') ch += 32;
      if (ch != *b) goto next;
      c++;
      b++;
    }
    if (*b == NULL) return TRUE;

    next:
    bl_item++;
  }

  return FALSE;
}
///
///createPaletteSection(file_handle, num_colors, aga)
/******************************************************************************
 * Creates a literal CLP section.                                             *
 ******************************************************************************/
VOID createPaletteSection(BPTR fh, ULONG num_colors, BOOL aga)
{
  if (aga) {
    ULONG num_banks = (num_colors + 31) / 32;
    ULONG c, r, b;

    for (b = 0, c = 0; b < num_banks; b++) {
      if (b == 0)
        FPrintf(fh, "  MOVE_PH(BPLCON3, BPLCON3_V),                // CL_PALETTE   Palette Colors AGA (bank 0)\n");
      else
        FPrintf(fh, "  MOVE(BPLCON3, BPLCON3_V | (%lu << 13)),       //              bank %lu\n", b, b);

      for (r = 0; r < 32 && c < num_colors; r++, c++) {
        FPrintf(fh, "  MOVE(COLOR%02lu, 0x0),                         //\n", r);
      }
    }

    for (b = 0, c = 0; b < num_banks; b++) {
      if (b == 0)
        FPrintf(fh, "  MOVE(BPLCON3, BPLCON3_V | BPLCON3_LOCT),    //              bank 0 LOCT\n");
      else
        FPrintf(fh, "  MOVE(BPLCON3, BPLCON3_V | BPLCON3_LOCT | (%lu << 13)), //     bank %lu LOCT\n", b, b);

      for (r = 0, c = 0; r < 32 && c < num_colors; r++, c++) {
        FPrintf(fh, "  MOVE(COLOR%02lu, 0x0),                         //\n", r);
      }
    }
  }
  else {
    ULONG c;

    FPrintf(fh, "  MOVE_PH(COLOR00, 0x0),                      // CL_PALETTE   Palette Colors OCS/ECS\n");
    for (c = 1; c < num_colors; c++) {
      FPrintf(fh, "  MOVE(COLOR%02lu, 0x0),                         //\n", c);
    }
  }
}
///

///m_CheckValidity()
STATIC ULONG m_CheckValidity(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR name;
  ULONG depth;
  UBYTE* palette = data->palette;

  get(data->obj_table.str_name, MUIA_String_Contents, &name);
  get(data->obj_table.int_scrDepth, MUIA_Integer_Value, &depth);

  if (name && strlen(name) && *name != ' ' && !(*name >= '0' && *name <= '9') &&
      !isBlackListed(name) && palette && *palette + 1 == 1 << depth) {
    DoMethod(data->obj_table.btn_create, MUIM_Set, MUIA_Disabled, FALSE);
  }
  else {
    DoMethod(data->obj_table.btn_create, MUIM_Set, MUIA_Disabled, TRUE);
  }

  return 0;
}
///
///m_OpenPaletteSelector()
STATIC ULONG m_OpenPaletteSelector(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);
  data->subscribed_to_palette_selector = TRUE;
  DoMethod(data->palette_selector, MUIM_Set, MUIA_Window_Open, TRUE);

  return 0;
}
///
///m_SetPalette()
STATIC ULONG m_SetPalette(struct IClass* cl, Object* obj, struct cl_Msg3* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->subscribed_to_palette_selector) {
    DoMethod(data->obj_table.txt_palette, MUIM_Set, MUIA_Text_Contents, msg->palette_item->name);
    data->palette = msg->palette_item->palette;
    DoMethod(data->obj_table.int_scrDepth, MUIM_Set, MUIA_Integer_Value, colors2depth((*data->palette + 1)));
    m_CheckValidity(cl, obj, (Msg)msg);

    data->subscribed_to_palette_selector = FALSE;
  }

  return 0;
}
///
///m_Create()
STATIC ULONG m_Create(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  LONG i;
  STRPTR null = (STRPTR)"";

  STRPTR str_name;
  ULONG scr_width;
  ULONG scr_height;
  ULONG scr_depth;
  STRPTR palette;
  ULONG spr_fmode;
  ULONG bpl_fmode;
  ULONG AGA;
  ULONG interleaved;
  ULONG hires;
  ULONG interlaced;
  ULONG layer;
  ULONG tmpras;
  ULONG area;
  ULONG maxvectors;
  ULONG pltOnCL;
  ULONG project_AGA;

  STRPTR name;
  STRPTR Name;
  STRPTR NAME;
  STRPTR filename;
  STRPTR filename_c;
  STRPTR filename_h;
  BPTR fh;

  ULONG file_mode_c = MODE_NEWFILE;
  ULONG file_mode_h = MODE_NEWFILE;
  BOOL failure = FALSE;

  get(data->obj_table.str_name, MUIA_String_Contents, &str_name);
  get(data->obj_table.int_scrWidth, MUIA_Integer_Value, &scr_width);
  get(data->obj_table.int_scrHeight, MUIA_Integer_Value, &scr_height);
  get(data->obj_table.int_scrDepth, MUIA_Integer_Value, &scr_depth);
  get(data->obj_table.txt_palette, MUIA_Text_Contents, &palette);
  get(data->obj_table.int_sprFetchMode, MUIA_Integer_Value, &spr_fmode);
  get(data->obj_table.int_bplFetchMode, MUIA_Integer_Value, &bpl_fmode);
  get(data->obj_table.chk_AGA, MUIA_Selected, &AGA);
  get(data->obj_table.chk_interleaved, MUIA_Selected, &interleaved);
  get(data->obj_table.chk_hires, MUIA_Selected, &hires);
  get(data->obj_table.chk_interlaced, MUIA_Selected, &interlaced);
  get(data->obj_table.chk_Layer, MUIA_Selected, &layer);
  get(data->obj_table.chk_TmpRas, MUIA_Selected, &tmpras);
  get(data->obj_table.chk_Area, MUIA_Selected, &area);
  get(data->obj_table.int_maxvectors, MUIA_Integer_Value, &maxvectors);
  get(data->obj_table.chk_pltOnCL, MUIA_Selected, &pltOnCL);
  get(data->AGACheck, MUIA_Selected, &project_AGA);

  if (str_name) {
    //Prepare name strings
    str_name = makeString(str_name);
    replaceChars(str_name, " ", '_');
    name = makeString(str_name);
    Name = makeString(str_name);
    NAME = makeString(str_name);

    if (isCamelCase(str_name)) {
      if (*str_name >= 'a') *Name -= 32;
      toLower(name);
      toUpper(NAME);
    }
    else {
      toLower(name);
      toLower(Name); *Name -= 32;
      toUpper(NAME);
    }
    filename = makeString2("display_", name);
    filename_c = makePath(g_Project.directory, filename, ".c");
    filename_h = makePath(g_Project.directory, filename, ".h");

    //Open .c file for output
    if (Exists(filename_c)) {
      if (!MUI_Request(App, obj, NULL, "Display Creator", "*_Overwrite|_Cancel", "A display with this name exists!")) {
        goto cancel;
      }
      file_mode_c = MODE_OLDFILE;
      if (Exists(filename_h)) file_mode_h = MODE_OLDFILE;
    }

    fh = Open(filename_c, file_mode_c);
    if (fh) {
      //Write includes
      FPrintf(fh, code_string.includes, (LONG)(AGA ? code_string.include_AGA : null), (LONG)name);
      //Write defines
      FPrintf(fh, code_string.define_1, (LONG)NAME, (LONG)scr_width, (LONG)NAME, (LONG)scr_height, (LONG)NAME, (LONG)scr_depth,
                                        (LONG)NAME, (LONG)NAME, (LONG)NAME, (LONG)NAME, (LONG)NAME, (LONG)NAME, (LONG)NAME, (LONG)NAME);
      if (interleaved)
        FPrintf(fh, code_string.define_2_interleaved, (LONG)NAME, (LONG)NAME, (LONG)(interlaced ? (STRPTR)" * 2" : null));
      else {
        if (interlaced)
          FPrintf(fh, code_string.define_2, (LONG)NAME);
        else
          FPrintf(fh, "#define BPLXMOD_V 0\n\n");
      }
      FPrintf(fh, code_string.define_3, (LONG)(AGA ? NAME : null), (LONG)(AGA ? code_string.define_3_AGA_1 : null), (LONG)NAME,
                                        (LONG)(hires ? code_string.define_3_hires : null),
                                        (LONG)(interlaced ? code_string.define_3_laced : null),
                                        (LONG)(AGA ? code_string.define_3_AGA_2 : null),
                                        (LONG)maxvectors);
      if (AGA)
        FPrintf(fh, code_string.define_3_AGA_0, spr_fmode, bpl_fmode);
      FPrintf(fh, code_string.define_end);
      //Write globals
      FPrintf(fh, code_string.globals);
      //Write Copperlist
      FPrintf(fh, code_string.copperlist_0);
      if (interlaced) FPrintf(fh, code_string.copperlist_1_laced, pltOnCL ? (LONG)code_string.copperlist_CLP_laced : (LONG)"");
      else FPrintf(fh, code_string.copperlist_1, pltOnCL ? (LONG)code_string.copperlist_CLP : (LONG)"");
      //Write Copperlist array
      FPrintf(fh, code_string.copperlist_2);
      if (pltOnCL) {
        if (AGA != project_AGA)
          createPaletteSection(fh, 1 << scr_depth, AGA);
        else
          FPrintf(fh,"  #define CLP_DEPTH %s_SCREEN_DEPTH\n  #include \"clp.c\"\n", (LONG)NAME);
      }
      if (AGA) FPrintf(fh, code_string.copperlist_2_AGA);
      FPrintf(fh, code_string.copperlist_2_disp);
      for (i = 2; i <= scr_depth; i++) {
        FPrintf(fh, code_string.copperlist_bitplanes, i, i);
      }
      if (interlaced) FPrintf(fh, code_string.copperlist_2_laced);
      FPrintf(fh, code_string.copperlist_3);
      //Write color functions
      FPrintf(fh, code_string.colors);
      //Write vblankEvents function
      if (interlaced) {
        if (pltOnCL) FPrintf(fh, code_string.vblank_laced, (LONG)"");
        else FPrintf(fh, code_string.vblank_laced, (LONG)code_string.color_table);
      }
      else {
        if (pltOnCL) FPrintf(fh, code_string.vblank, (LONG)"");
        else FPrintf(fh, code_string.vblank, (LONG)code_string.color_table);
      }
      //Write screen functions
      FPrintf(fh, code_string.screen, (LONG)NAME, (LONG)NAME, (LONG)NAME,
                                      (LONG)(interleaved ? (STRPTR)" | BMF_INTERLEAVED" : null),
                                      (LONG)(layer ? (STRPTR)" | RPF_LAYER" : null),
                                      (LONG)(tmpras ? (STRPTR)" | RPF_TMPRAS" : null),
                                      (LONG)(area ? (STRPTR)" | RPF_AREA" : null));
      //Write copperlist functions
      if (interlaced)
        FPrintf(fh, code_string.copperlist_functions_laced, (LONG)NAME, (LONG)NAME,
                                                            (LONG)NAME, (LONG)(interleaved ? (STRPTR)" * " : null),
                                                            (LONG)(interleaved ? NAME : null),
                                                            (LONG)(interleaved ? (STRPTR)"_BITMAP_DEPTH" : null),
                                                            (LONG)NAME, (LONG)(interleaved ? (STRPTR)" * " : null),
                                                            (LONG)(interleaved ? NAME : null),
                                                            (LONG)(interleaved ? (STRPTR)"_BITMAP_DEPTH" : null),
                                                            (LONG)Name);
      else
        FPrintf(fh, code_string.copperlist_functions, (LONG)NAME, (LONG)Name);
      //Write display functions
      FPrintf(fh, code_string.display, (LONG)palette, pltOnCL ? (interlaced ? (LONG)code_string.color_table_CLP_laced : (LONG)code_string.color_table_CLP) : (LONG)"");
      //Write switch function
      FPrintf(fh, interlaced ? code_string.switch_functions_laced : code_string.switch_functions, (LONG)Name, (LONG)Name);
      //Write show function
      FPrintf(fh, code_string.show, (LONG)Name, (LONG)Name, (LONG)Name);

      SetFileSize(fh, 0, OFFSET_CURRENT);
      Close(fh);

      fh = Open(filename_h, file_mode_h);
      if (fh) {
        FPrintf(fh, code_string.header_file, (LONG)NAME, (LONG)NAME, (LONG)Name, (LONG)NAME);

        SetFileSize(fh, 0, OFFSET_CURRENT);
        Close(fh);
      }
      else {
        DeleteFile(filename_c);
        failure = TRUE;
      }
    }
    else failure = TRUE;

    if (failure) MUI_Request(App, obj, NULL, "Sevgi Editor", "OK", "An error occured!");
    else {
      MUI_Request(App, obj, NULL, "Sevgi Editor", "OK", "Source files:\n   \33b%s.c\33n and\n   \33b%s.h\33n are created in your project directory.\n"
                                  "Calling the function \33bshow%sDisplay()\33n will have it displayed.\n"
                                  "Do not forget to add \33b%s.o\33n in your project's makefile.", (ULONG)filename, (ULONG)filename, (ULONG)Name, (ULONG)filename);

      if (pltOnCL && AGA != project_AGA) {
        MUI_Request(App, obj, NULL, "Sevgi Editor", "OK", "You have created a display that uses CLP with a different\n"
                                                          "graphics architecture (ECS/AGA) than the project.\n"
                                                          "You have to uncomment the proper variant of \33bsetColor#?_CLP()\33n\n"
                                                          "functions on \33bcolor.c\33n and use \33bONLY\33n those variants on this display!");
      }
    }

cancel:
    freeString(filename_h);
    freeString(filename_c);
    freeString(filename);
    freeString(NAME);
    freeString(Name);
    freeString(name);
    freeString(str_name);
  }

  return 0;
}
///
///m_Reset()
STATIC ULONG m_Reset(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  DoMethod(data->obj_table.str_name, MUIM_Set, MUIA_String_Contents, "");
  DoMethod(data->obj_table.int_scrWidth, MUIM_Set, MUIA_Integer_Value, 320);
  DoMethod(data->obj_table.int_scrHeight, MUIM_Set, MUIA_Integer_Value, 256);
  DoMethod(data->obj_table.int_scrDepth, MUIM_Set, MUIA_Integer_Value, 1);
  DoMethod(data->obj_table.txt_palette, MUIM_Set, MUIA_Text_Contents, "");
  data->palette = NULL;
  DoMethod(data->obj_table.int_sprFetchMode, MUIM_Set, MUIA_Integer_Value, 1);
  DoMethod(data->obj_table.int_bplFetchMode, MUIM_Set, MUIA_Integer_Value, 1);
  DoMethod(data->obj_table.chk_AGA, MUIM_Set, MUIA_Selected, FALSE);
  DoMethod(data->obj_table.chk_interleaved, MUIM_Set, MUIA_Selected, FALSE);
  DoMethod(data->obj_table.chk_hires, MUIM_Set, MUIA_Selected, FALSE);
  DoMethod(data->obj_table.chk_interlaced, MUIM_Set, MUIA_Selected, FALSE);
  DoMethod(data->obj_table.chk_Layer, MUIM_Set, MUIA_Selected, FALSE);
  DoMethod(data->obj_table.chk_TmpRas, MUIM_Set, MUIA_Selected, FALSE);
  DoMethod(data->obj_table.chk_Area, MUIM_Set, MUIA_Selected, FALSE);
  DoMethod(data->obj_table.int_maxvectors, MUIM_Set, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.chk_pltOnCL, MUIM_Set, MUIA_Selected, TRUE);

  DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;

  struct {
    Object* str_name;
    Object* int_scrWidth;
    Object* int_scrHeight;
    Object* int_scrDepth;
    Object* txt_palette;
    Object* pop_palette;
    Object* int_sprFetchMode;
    Object* int_bplFetchMode;
    Object* chk_AGA;
    Object* chk_hires;
    Object* chk_interleaved;
    Object* chk_interlaced;
    Object* chk_Layer;
    Object* chk_TmpRas;
    Object* chk_Area;
    Object* int_maxvectors;
    Object* chk_pltOnCL;
    Object* btn_create;
    Object* btn_cancel;
  }objects;

  if ((obj = (Object*) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','C'),
    MUIA_Window_Title, "Display Creator",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Name:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.str_name,
        TAG_END),
        MUIA_Group_Child, (objects.str_name = MUI_NewObject(MUIC_String,
          MUIA_String_Accept, ALPHANUMERIC " _",
          MUIA_Frame, MUIV_Frame_String,
          MUIA_ShortHelp, help_string.str_name,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Screen Width:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.scr_width,
        TAG_END),
        MUIA_Group_Child, (objects.int_scrWidth = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 320,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 320,
          MUIA_Integer_Max, 320,
          MUIA_String_MaxLen, 4,
          MUIA_ShortHelp, help_string.scr_width,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Screen Height:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.scr_height,
        TAG_END),
        MUIA_Group_Child, (objects.int_scrHeight = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 256,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 16,
          MUIA_Integer_Max, 256,
          MUIA_String_MaxLen, 4,
          MUIA_ShortHelp, help_string.scr_height,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Screen Depth:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.scr_depth,
        TAG_END),
        MUIA_Group_Child, (objects.int_scrDepth = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 1,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 1,
          MUIA_Integer_Max, 5,
          MUIA_String_MaxLen, 4,
          MUIA_ShortHelp, help_string.scr_depth,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Palette:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.palette,
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Horiz, TRUE,
          MUIA_Group_HorizSpacing, 1,
          MUIA_Group_Child, (objects.txt_palette = MUI_NewObject(MUIC_Text,
            MUIA_Frame, MUIV_Frame_ReadList,
            MUIA_Background, MUII_ButtonBack,
          TAG_END)),
          MUIA_Group_Child, (objects.pop_palette = MUI_NewObject(MUIC_Image,
            MUIA_Image_Spec, MUII_PopUp,
            MUIA_Background, MUII_ButtonBack,
            MUIA_Frame, MUIV_Frame_Button,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_ShortHelp, help_string.palette,
          TAG_END)),
          MUIA_ShortHelp, help_string.palette,
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Sprite Fetch Mode:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, NULL,
        TAG_END),
        MUIA_Group_Child, (objects.int_sprFetchMode = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_Integer_124, TRUE,
          MUIA_Disabled, TRUE,
          MUIA_ShortHelp, NULL,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Bitplane Fetch Mode:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, NULL,
        TAG_END),
        MUIA_Group_Child, (objects.int_bplFetchMode = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_Integer_124, TRUE,
          MUIA_Disabled, TRUE,
          MUIA_ShortHelp, NULL,
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_AGA, FALSE, "AGA", 'g', help_string.AGA),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_interleaved, FALSE, "Interleaved", 'i', help_string.interleaved),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_hires, FALSE, "Hires", 'h', help_string.hires),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_interlaced, FALSE, "Interlaced", 'c', help_string.interlaced),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_Layer, FALSE, "Layer", 'l', help_string.layer),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_TmpRas, FALSE, "TempRas", 't', help_string.tmpras),
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_Area, FALSE, "Area", 'a', help_string.area),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Max. vectors:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.maxvectors,
        TAG_END),
        MUIA_Group_Child, (objects.int_maxvectors = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 128,
          MUIA_String_MaxLen, 4,
          MUIA_ShortHelp, help_string.maxvectors,
          MUIA_Disabled, TRUE,
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_pltOnCL, TRUE, "Palette on copperlist", 'p', help_string.pltOnCL),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (objects.btn_create = MUI_NewButton("Create", 'r', help_string.create)),
        MUIA_Group_Child, (objects.btn_cancel = MUI_NewButton("Cancel", NULL, help_string.cancel)),
      TAG_END),
    TAG_END),
  TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.str_name,
                                             MUI_GetChild(objects.int_scrWidth, 1),
                                             MUI_GetChild(objects.int_scrHeight, 1),
                                             MUI_GetChild(objects.int_scrDepth, 1),
                                             objects.pop_palette,
                                             MUI_GetChild(objects.int_sprFetchMode, 2),
                                             MUI_GetChild(objects.int_sprFetchMode, 3),
                                             MUI_GetChild(objects.int_bplFetchMode, 2),
                                             MUI_GetChild(objects.int_bplFetchMode, 3),
                                             objects.chk_AGA,
                                             objects.chk_interleaved,
                                             objects.chk_hires,
                                             objects.chk_interlaced,
                                             objects.chk_Layer,
                                             objects.chk_TmpRas,
                                             objects.chk_Area,
                                             MUI_GetChild(objects.int_maxvectors, 1),
                                             objects.chk_pltOnCL,
                                             objects.btn_create,
                                             objects.btn_cancel,
                                             NULL);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 1,
      MUIM_DisplayCreator_Reset);

    DoMethod(objects.str_name, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1,
      MUIM_DisplayCreator_CheckValidity);

    DoMethod(objects.int_scrDepth, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_DisplayCreator_CheckValidity);

    DoMethod(objects.pop_palette, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_DisplayCreator_OpenPaletteSelector);

    DoMethod(objects.chk_AGA, MUIM_Notify, MUIA_Selected, TRUE, objects.int_scrDepth, 3,
      MUIM_Set, MUIA_Integer_Max, 8);

    DoMethod(objects.chk_AGA, MUIM_Notify, MUIA_Selected, FALSE, objects.int_scrDepth, 3,
      MUIM_Set, MUIA_Integer_Max, 5);

    DoMethod(objects.chk_AGA, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, objects.int_sprFetchMode, 3,
      MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

    DoMethod(objects.chk_AGA, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, objects.int_bplFetchMode, 3,
      MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

    DoMethod(objects.chk_hires, MUIM_Notify, MUIA_Selected, TRUE, objects.int_scrWidth, 3,
      MUIM_Set, MUIA_Integer_Max, 640);

    DoMethod(objects.chk_hires, MUIM_Notify, MUIA_Selected, TRUE, objects.int_scrWidth, 3,
      MUIM_Set, MUIA_Integer_Value, 640);

    DoMethod(objects.chk_hires, MUIM_Notify, MUIA_Selected, FALSE, objects.int_scrWidth, 3,
      MUIM_Set, MUIA_Integer_Max, 320);

    DoMethod(objects.chk_interlaced, MUIM_Notify, MUIA_Selected, TRUE, objects.int_scrHeight, 3,
      MUIM_Set, MUIA_Integer_Max, 512);

    DoMethod(objects.chk_interlaced, MUIM_Notify, MUIA_Selected, TRUE, objects.int_scrHeight, 3,
      MUIM_Set, MUIA_Integer_Value, 512);

    DoMethod(objects.chk_interlaced, MUIM_Notify, MUIA_Selected, FALSE, objects.int_scrHeight, 3,
      MUIM_Set, MUIA_Integer_Max, 256);

    DoMethod(objects.chk_Area, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, objects.int_maxvectors, 3,
      MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

    DoMethod(objects.chk_Area, MUIM_Notify, MUIA_Selected, FALSE, objects.int_maxvectors, 3,
      MUIM_Set, MUIA_Integer_Value, 0);

    DoMethod(objects.btn_cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_DisplayCreator_Reset);

    DoMethod(objects.btn_create, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_DisplayCreator_Create);

    DoMethod(objects.btn_create, MUIM_Set, MUIA_Disabled, TRUE);

    data->AGACheck = NULL;
    data->CLPCheck = NULL;
    data->palette = NULL;
    data->palette_selector = NULL;
    data->subscribed_to_palette_selector = FALSE;

    data->obj_table.str_name = objects.str_name;
    data->obj_table.int_scrWidth = objects.int_scrWidth;
    data->obj_table.int_scrHeight = objects.int_scrHeight;
    data->obj_table.int_scrDepth = objects.int_scrDepth;
    data->obj_table.txt_palette = objects.txt_palette;
    data->obj_table.pop_palette = objects.pop_palette;
    data->obj_table.int_sprFetchMode = objects.int_sprFetchMode;
    data->obj_table.int_bplFetchMode = objects.int_bplFetchMode;
    data->obj_table.chk_AGA = objects.chk_AGA;
    data->obj_table.chk_interleaved = objects.chk_interleaved;
    data->obj_table.chk_hires = objects.chk_hires;
    data->obj_table.chk_interlaced = objects.chk_interlaced;
    data->obj_table.chk_Layer = objects.chk_Layer;
    data->obj_table.chk_TmpRas = objects.chk_TmpRas;
    data->obj_table.chk_Area = objects.chk_Area;
    data->obj_table.int_maxvectors = objects.int_maxvectors;
    data->obj_table.chk_pltOnCL = objects.chk_pltOnCL;
    data->obj_table.btn_create = objects.btn_create;
    data->obj_table.btn_cancel = objects.btn_cancel;

    //<SUBCLASS INITIALIZATION HERE>
    if (/*<Success of your initializations>*/ TRUE) {

      return((ULONG) obj);
    }
    else CoerceMethod(cl, obj, OM_DISPOSE);
  }

return (ULONG) NULL;
}
///
///Overridden OM_DISPOSE
static ULONG m_Dispose(struct IClass* cl, Object* obj, Msg msg)
{
  //struct cl_Data *data = INST_DATA(cl, obj);

  //<FREE SUBCLASS INITIALIZATIONS HERE>

  return DoSuperMethodA(cl, obj, msg);
}
///
///Overridden OM_SET
//*****************
static ULONG m_Set(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct TagItem *tags, *tag;

  for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
  {
    switch (tag->ti_Tag)
    {
      case MUIA_Window_Open:
        if (data->AGACheck) {
          ULONG isAGA;
          get(data->AGACheck, MUIA_Selected, &isAGA);
          DoMethod(data->obj_table.chk_AGA, MUIM_Set, MUIA_Selected, isAGA);
        }
        if (data->CLPCheck) {
          ULONG isCLP;
          get(data->CLPCheck, MUIA_Selected, &isCLP);
          DoMethod(data->obj_table.chk_pltOnCL, MUIM_Set, MUIA_Selected, isCLP);
        }
      break;
      case MUIA_DisplayCreator_AGACheck:
        data->AGACheck = (Object*)tag->ti_Data;
      break;
      case MUIA_DisplayCreator_CLPCheck:
        data->CLPCheck = (Object*)tag->ti_Data;
      break;
      case MUIA_DisplayCreator_PaletteSelector:
        data->palette_selector = (Object*)tag->ti_Data;
        DoMethod(data->palette_selector, MUIM_Notify, MUIA_Window_Open, FALSE, obj, 3,
          MUIM_Set, MUIA_Window_Sleep, FALSE);
        DoMethod(data->palette_selector, MUIM_Notify, MUIA_PaletteSelector_Selected, MUIV_EveryTime, obj, 2,
          MUIM_DisplayCreator_SetPalette, MUIV_TriggerValue);
      break;
    }
  }

  return (DoSuperMethodA(cl, obj, (Msg) msg));
}
///
///Overridden OM_GET
//*****************
static ULONG m_Get(struct IClass* cl, Object* obj, struct opGet* msg)
{
  //struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->opg_AttrID)
  {
    //<SUBCLASS ATTRIBUTES HERE>
    /*
    case MUIA_DisplayCreator_{Attribute}:
      *msg->opg_Storage = data->{Variable};
    return TRUE;
    */
  }

  return (DoSuperMethodA(cl, obj, (Msg) msg));
}
///
///Dispatcher
SDISPATCHER(cl_Dispatcher)
{
  struct cl_Data *data;
  if (! (msg->MethodID == OM_NEW)) data = INST_DATA(cl, obj);

  switch(msg->MethodID)
  {
    case OM_NEW:
      return m_New(cl, obj, (struct opSet*) msg);
    case OM_DISPOSE:
      return m_Dispose(cl, obj, msg);
    case OM_SET:
      return m_Set(cl, obj, (struct opSet*) msg);
    case OM_GET:
      return m_Get(cl, obj, (struct opGet*) msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    case MUIM_DisplayCreator_OpenPaletteSelector:
      return m_OpenPaletteSelector(cl, obj, msg);
    case MUIM_DisplayCreator_SetPalette:
      return m_SetPalette(cl, obj, (struct cl_Msg3*)msg);
    case MUIM_DisplayCreator_CheckValidity:
      return m_CheckValidity(cl, obj, msg);
    case MUIM_DisplayCreator_Create:
      return m_Create(cl, obj, msg);
    case MUIM_DisplayCreator_Reset:
      return m_Reset(cl, obj, msg);

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_DisplayCreator(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
