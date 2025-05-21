/******************************************************************************
 * Sevgi_Editor                                                               *
 * TODO: - A loading window that displays each load progress.                 *
 *       - Localize strings                                                   *
 ******************************************************************************/

///defines
#define PROGRAMNAME     "Sevgi Editor"
#define VERSION         0
#define REVISION        140
#define VERSIONSTRING   "0.140"
#define AUTHOR          "Ibrahim Alper Sönmez"
#define COPYRIGHT       "@ 2024 " AUTHOR
#define CONTACT         "amithlondestek@gmail.com"
#define DESCRIPTION     "Game Editor for Sevgi Engine"

//define command line syntax and number of options
#define RDARGS_TEMPLATE ""
#define RDARGS_OPTIONS  0

//#define or #undef GENERATEWBMAIN to enable workbench startup
#define GENERATEWBMAIN

#define MUIV_App_RetID_New_Project_Window_Open  0x01
#define MUIV_App_RetID_New_Project_Window_Close 0x02
#define MUIV_App_RetID_New_Project_Create       0x03
#define MUIV_App_RetID_Load_Project             0x04
#define MUIV_App_RetID_Save_Project             0x05
#define MUIV_App_RetID_Save_Project_As          0x06
#define MUIV_App_RetID_Edited                   0x07
#define MUIV_App_RetID_Open_In_IDE              0x08

#define MEN_PROJECT         50
#define MEN_NEW_PROJECT     MUIV_App_RetID_New_Project_Window_Open
#define MEN_LOAD_PROJECT    MUIV_App_RetID_Load_Project
#define MEN_SAVE_PROJECT    MUIV_App_RetID_Save_Project
#define MEN_SAVE_PROJECT_AS MUIV_App_RetID_Save_Project_As
#define MEN_ABOUT           51
#define MEN_ABOUTMUI        52
#define MEN_QUIT            MUIV_Application_ReturnID_Quit
#define MEN_SETTINGS        53
#define MEN_EDITOR_SETTINGS 54
#define MEN_HELP            55
#define MEN_HELP_ENGINE     56
#define MEN_HELP_EDITOR     57

#define PROJECTS_DIR  "Projects"
///
///includes
//standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

//Amiga headers
#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/layers.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <datatypes/pictureclass.h>
#include <libraries/asl.h>
#include <libraries/commodities.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/locale.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <utility/hooks.h>

//Amiga protos
#include <clib/alib_protos.h>
#include <proto/asl.h>
#include <proto/commodities.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/locale.h>
#include <proto/rexxsyslib.h>
#include <proto/utility.h>
#include <proto/wb.h>

/* MUI headers */
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <workbench/workbench.h>

//SDI headers
#include <SDI_compiler.h>
#include <SDI_hook.h>
#include <SDI_interrupt.h>
#include <SDI_lib.h>
#include <SDI_misc.h>
#include <SDI_stdarg.h>

#include "utility.h"
#include "toolbar.h"
#include "popasl_string.h"
#include "ackstring.h"
#include "integer_gadget.h"
#include "gamefont_creator.h"
#include "tileset_creator.h"
#include "tilemap_creator.h"
#include "bobsheet_creator.h"
#include "spritebank_creator.h"
#include "color_gadget.h"
#include "color_palette.h"
#include "palette_editor.h"
#include "image_display.h"
#include "image_editor.h"
#include "game_settings.h"
#include "assets_editor.h"
#include "dtpicdisplay.h"
#include "new_project.h"
#include "editor_settings.h"
#include "palette_selector.h"
#include "display_creator.h"
#include "bitmap_selector.h"
///
///structs
/***********************************************
* Global configuration struct for this program *
************************************************/
struct Config
{
  struct RDArgs *RDArgs;

  //command line options
  #if RDARGS_OPTIONS
  LONG Options[RDARGS_OPTIONS];
  #endif

  //<YOUR GLOBAL DATA HERE>

};

//<YOUR STRUCTS HERE>

///
///globals
/***********************************************
* Version string for this program              *
************************************************/
#if defined(__SASC)
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " "  __AMIGADATE__ "\n\0";
#elif defined(_DCC)
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __COMMODORE_DATE__ ")\n\0";
#elif defined(__GNUC__)
__attribute__((section(".text"))) volatile static const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";
#else
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";
#endif

APTR g_MemoryPool = NULL;
struct FileRequester* g_FileReq = NULL;
struct FontRequester* g_FontReq = NULL;
struct {
  STRPTR convert_tiles;
  STRPTR convert_map;
  STRPTR bob_sheeter;
  STRPTR sprite_banker;
  STRPTR gob_banker;
  struct {
    BOOL convert_tiles;
    BOOL convert_map;
    BOOL bob_sheeter;
    BOOL sprite_banker;
    BOOL gob_banker;
  }avail;
}g_Tools = {"Tools/ConvertTiles",
            "Tools/ConvertMap",
            "Tools/BobSheeter",
            "Tools/SpriteBanker",
            "Tools/GOBBanker",
            {FALSE, FALSE, FALSE, FALSE, FALSE}};

BPTR   g_System_Path_List = NULL;
BPTR   g_Program_Directory_Lock = NULL;
STRPTR g_Program_Directory = NULL;
STRPTR g_Program_Executable = NULL;
STRPTR g_Win_Title = NULL;

struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project = {NULL, NULL, NULL, NULL, NULL, NULL};
///
///MUI globals
#ifdef __GNUC__
/* Otherwise auto open will try version 37, and muimaster.library has version
   19.x for MUI 3.8 */
int __oslibversion = 0;
#endif

struct Library *AslBase;
struct Library *MUIMasterBase;
#if defined(__amigaos4__)
  struct AslIFace *IAsl;
  struct MUIMasterIFace *IMUIMaster;
#endif

struct MUI_CustomClass* MUIC_PopASLString = NULL;
struct MUI_CustomClass* MUIC_Integer = NULL;
struct MUI_CustomClass* MUIC_AckString = NULL;
struct MUI_CustomClass* MUIC_GamefontCreator = NULL;
struct MUI_CustomClass* MUIC_TilesetCreator = NULL;
struct MUI_CustomClass* MUIC_TilemapCreator = NULL;
struct MUI_CustomClass* MUIC_BobsheetCreator = NULL;
struct MUI_CustomClass* MUIC_SpritebankCreator = NULL;
struct MUI_CustomClass* MUIC_ImageDisplay = NULL;
struct MUI_CustomClass* MUIC_ImageEditor = NULL;
struct MUI_CustomClass* MUIC_ColorGadget  = NULL;
struct MUI_CustomClass* MUIC_ColorPalette = NULL;
struct MUI_CustomClass* MUIC_PaletteEditor = NULL;
struct MUI_CustomClass* MUIC_SettingsEditor = NULL;
struct MUI_CustomClass* MUIC_AssetsEditor = NULL;
struct MUI_CustomClass* MUIC_DtPicDisplay = NULL;
struct MUI_CustomClass* MUIC_NewProject = NULL;
struct MUI_CustomClass* MUIC_EditorSettings = NULL;
struct MUI_CustomClass* MUIC_PaletteSelector = NULL;
struct MUI_CustomClass* MUIC_DisplayCreator = NULL;
struct MUI_CustomClass* MUIC_BitmapSelector = NULL;

struct Window* g_Window;
Object *App, *Win;
Object *AboutMUIWin = NULL;
Object *g_dtPicDisplay = NULL;
Object *g_Settings = NULL;

struct {
  Object* newProject;
  Object* gameFontCreator;
  Object* tilesetCreator;
  Object* tilemapCreator;
  Object* bobsheetCreator;
  Object* spritebankCreator;
  Object* imageEditor;
  Object* settingsEditor;
  Object* assetsEditor;
  Object* paletteEditor;
  Object* dtPicDisplay;
  Object* settings;
  Object* paletteSelector;
  Object* displayCreator;
  Object* bitmapSelector;
}window;

struct {
  Object* new;
  Object* load;
  Object* save;
  Object* settings;
  Object* assets;
  Object* editor;
  Object* font;
  Object* tileset;
  Object* tilemap;
  Object* bobsheet;
  Object* sprite;
  Object* image;
  Object* object;
  Object* palette;
  Object* display;
}toolbar;
///
///prototypes
/***********************************************
* Function forward declarations                *
************************************************/
int            main   (int argc, char **argv);
int            wbmain (struct WBStartup *wbs);
struct Config *Init   (void);
int            Main   (struct Config *config);
void           CleanUp(struct Config *config);
Object *       buildGUI(void);

VOID checkAvailTools(VOID);
VOID sleepAll(BOOL sleep);
VOID openProject(STRPTR directory);
VOID saveProject();
VOID saveProjectAs();
BOOL checkEditedState();
VOID openInIDE();
VOID openGuide(ULONG id);
///
///init
/***********************************************
* Program initialization                       *
* - Allocates the config struct to store the   *
*   global configuration data.                 *
* - Do your other initial allocations here.    *
************************************************/
struct Config *Init()
{
  struct Config *config;

  #if defined(__amigaos4__)
  g_MemoryPool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_ANY | MEMF_CLEAR,
                                                  ASOPOOL_Puddle, 2048,
                                                  ASOPOOL_Threshold, 2048,
                                                  ASOPOOL_Name, "Sevgi_Editor pool",
                                                  ASOPOOL_LockMem, FALSE,
                                                  TAG_DONE);
  #else
  g_MemoryPool = CreatePool(MEMF_ANY | MEMF_CLEAR, 2048, 2048);
  #endif

  if (!g_MemoryPool) return NULL;

  config = (struct Config*)AllocPooled(g_MemoryPool, sizeof(struct Config));

  if (config)
  {
    checkAvailTools();
    g_Program_Directory = getProgDir();
  }

  return(config);
}
///
///entry
/***********************************************
 * Ground level entry point                    *
 * - Branches regarding Shell/WB call.         *
 ***********************************************/
int main(int argc, char **argv)
{
  int rc = 20;

  //argc != 0 identifies call from shell
  if (argc)
  {
    struct Config *config = Init();

    g_Program_Executable = FilePart(argv[0]);

    if (config) {
      #if RDARGS_OPTIONS
        // parse command line arguments
        if (config->RDArgs = ReadArgs(RDARGS_TEMPLATE, config->Options, NULL))
          rc = Main(config);
        else
          PrintFault(IoErr(), PROGRAMNAME);
      #else
        rc = Main(config);
      #endif

      CleanUp(config);
    }
  }
  else {
    g_Program_Executable = ((struct WBStartup*) argv)->sm_ArgList->wa_Name;
    g_System_Path_List = getSystemPathList((struct WBStartup *)argv);
    rc = wbmain((struct WBStartup *)argv);
  }

  return(rc);
}

/***********************************************
 * Workbench main                              *
 * - This executable was called from Workbench *
 ***********************************************/
int wbmain(struct WBStartup *wbs)
{
  int rc = 20;

  #ifdef GENERATEWBMAIN
    struct Config *config = Init();

    if (config) {
      //<SET Config->Options[] HERE>

      rc = Main(config);

      CleanUp(config);
    }
  #endif

  return(rc);
}
///
///prepareMenu()
struct NewMenu* prepareMenu()
{
  #define MENU_SIZE 16
  struct NewMenu* menu = (struct NewMenu*) AllocPooled(g_MemoryPool, sizeof(struct NewMenu) * MENU_SIZE);

  if (menu) {
    menu[0].nm_Type     = NM_TITLE;
    menu[0].nm_Label    = "Project";
    menu[0].nm_UserData = (APTR)MEN_PROJECT;

    menu[1].nm_Type     = NM_ITEM;
    menu[1].nm_Label    = "New Project...";
    menu[1].nm_CommKey  = "N";
    menu[1].nm_UserData = (APTR)MEN_NEW_PROJECT;

    menu[2].nm_Type     = NM_ITEM;
    menu[2].nm_Label    = "Load Project...";
    menu[2].nm_CommKey  = "L";
    menu[2].nm_UserData = (APTR)MEN_LOAD_PROJECT;

    menu[3].nm_Type     = NM_ITEM;
    menu[3].nm_Label    = "Save Project";
    menu[3].nm_CommKey  = "S";
    menu[3].nm_Flags    = NM_ITEMDISABLED;
    menu[3].nm_UserData = (APTR)MEN_SAVE_PROJECT;

    menu[4].nm_Type     = NM_ITEM;
    menu[4].nm_Label    = "Save Project As...";
    menu[4].nm_CommKey  = "A";
    menu[4].nm_Flags    = NM_ITEMDISABLED;
    menu[4].nm_UserData = (APTR)MEN_SAVE_PROJECT_AS;

    menu[5].nm_Type     = NM_ITEM;
    menu[5].nm_Label    = NM_BARLABEL;

    menu[6].nm_Type     = NM_ITEM;
    menu[6].nm_Label    = "About...";
    menu[6].nm_UserData = (APTR)MEN_ABOUT;

    menu[7].nm_Type     = NM_ITEM;
    menu[7].nm_Label    = "About MUI...";
    menu[7].nm_UserData = (APTR)MEN_ABOUTMUI;

    menu[8].nm_Type     = NM_ITEM;
    menu[8].nm_Label    = NM_BARLABEL;

    menu[9].nm_Type     = NM_ITEM;
    menu[9].nm_Label    = "Quit";
    menu[9].nm_CommKey  = "Q";
    menu[9].nm_UserData = (APTR)MEN_QUIT;

    menu[10].nm_Type     = NM_TITLE;
    menu[10].nm_Label    = "Settings";
    menu[10].nm_UserData = (APTR)MEN_SETTINGS;

    menu[11].nm_Type     = NM_ITEM;
    menu[11].nm_Label    = "Editor Settings";
    menu[11].nm_UserData = (APTR)MEN_EDITOR_SETTINGS;

    menu[12].nm_Type     = NM_TITLE;
    menu[12].nm_Label    = "Help";
    menu[12].nm_UserData = (APTR)MEN_HELP;

    menu[13].nm_Type     = NM_ITEM;
    menu[13].nm_Label    = "Engine...";
    menu[13].nm_UserData = (APTR)MEN_HELP_ENGINE;

    menu[14].nm_Type     = NM_ITEM;
    menu[14].nm_Label    = "Editor...";
    menu[14].nm_UserData = (APTR)MEN_HELP_EDITOR;

    menu[15].nm_Type     = NM_END;

    return menu;
  }
  return NULL;
}
///
///buildGUI
/***********************************************
 * Program main window                         *
 * - Creates the MUI Application object.       *
 ***********************************************/
Object *buildGUI()
{
  App = MUI_NewObject(MUIC_Application,
    MUIA_Application_Author, (ULONG)AUTHOR,
    MUIA_Application_Base, (ULONG)PROGRAMNAME,
    MUIA_Application_Copyright, (ULONG)COPYRIGHT,
    MUIA_Application_Description, (ULONG)DESCRIPTION,
    MUIA_Application_Title, (ULONG)PROGRAMNAME,
    MUIA_Application_Version, (ULONG)VersionTag,
    MUIA_Application_Window, (Win = MUI_NewObject(MUIC_Window,
      MUIA_Window_ID, MAKE_ID('S','V','G','E'),
      MUIA_Window_Title, (ULONG)PROGRAMNAME,
      MUIA_Window_Menustrip, MUI_MakeObject(MUIO_MenustripNM, (ULONG)prepareMenu(), 0),
      MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Horiz, TRUE,
          MUIA_Group_HorizSpacing, 0,
          MUIA_Group_Child, (toolbar.new   = MUI_NewImageButton(IMG_SPEC_NEW,  "New Game", MUIA_Enabled)),
          MUIA_Group_Child, (toolbar.load  = MUI_NewImageButton(IMG_SPEC_LOAD, "Load Game", MUIA_Enabled)),
          MUIA_Group_Child, (toolbar.save  = MUI_NewImageButton(IMG_SPEC_SAVE, "Save Game", MUIA_Disabled)),
          MUIA_Group_Child, MUI_NewImageButtonSeparator(),
          MUIA_Group_Child, (toolbar.settings = MUI_NewImageButton(IMG_SPEC_SETTINGS, "Game Settings",  MUIA_Disabled)),
          MUIA_Group_Child, (toolbar.assets   = MUI_NewImageButton(IMG_SPEC_ASSETS,   "Assets Editor",  MUIA_Disabled)),
          MUIA_Group_Child, (toolbar.palette  = MUI_NewImageButton(IMG_SPEC_PALETTE,  "Palette Editor", MUIA_Disabled)),
          MUIA_Group_Child, (toolbar.display  = MUI_NewImageButton(IMG_SPEC_DISPLAY,  "Display Creator", MUIA_Disabled)),
          MUIA_Group_Child, MUI_NewImageButtonSeparator(),
          MUIA_Group_Child, (toolbar.editor   = MUI_NewImageButton(IMG_SPEC_EDITOR, "Open in IDE", MUIA_Disabled)),
          MUIA_Group_Child, MUI_NewImageButtonSeparator(),
          MUIA_Group_Child, (toolbar.font     = MUI_NewImageButton(IMG_SPEC_FONT,    "Gamefont Creator", MUIA_Enabled)),
          MUIA_Group_Child, (toolbar.tileset  = MUI_NewImageButton(IMG_SPEC_TILE,    "Tileset Creator", g_Tools.avail.convert_tiles ? MUIA_Enabled : MUIA_Disabled)),
          MUIA_Group_Child, (toolbar.tilemap  = MUI_NewImageButton(IMG_SPEC_MAP,     "Tilemap Creator", g_Tools.avail.convert_map ? MUIA_Enabled : MUIA_Disabled)),
          MUIA_Group_Child, (toolbar.bobsheet = MUI_NewImageButton(IMG_SPEC_BOB,     "BobSheet Creator", g_Tools.avail.bob_sheeter ? MUIA_Enabled : MUIA_Disabled)),
          MUIA_Group_Child, (toolbar.sprite   = MUI_NewImageButton(IMG_SPEC_SPRITE,  "Sprite Bank Creator", g_Tools.avail.sprite_banker ? MUIA_Enabled : MUIA_Disabled)),
          MUIA_Group_Child, (toolbar.image    = MUI_NewImageButton(IMG_SPEC_IMAGE,   "Image Hotspot and Hitbox Editor", MUIA_Enabled)),
          MUIA_Group_Child, (toolbar.object   = MUI_NewImageButton(IMG_SPEC_OBJECTS, "GameObject Editor (NOT IMPLEMENTED)", g_Tools.avail.gob_banker ? MUIA_Enabled : MUIA_Disabled)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
        TAG_END),
      TAG_END),
    TAG_END)),
    MUIA_Application_Window, (window.gameFontCreator = NewObject(MUIC_GamefontCreator->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.tilesetCreator = NewObject(MUIC_TilesetCreator->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.tilemapCreator = NewObject(MUIC_TilemapCreator->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.bobsheetCreator = NewObject(MUIC_BobsheetCreator->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.spritebankCreator = NewObject(MUIC_SpritebankCreator->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.imageEditor = NewObject(MUIC_ImageEditor->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.paletteEditor = NewObject(MUIC_PaletteEditor->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.settingsEditor = NewObject(MUIC_SettingsEditor->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.assetsEditor = NewObject(MUIC_AssetsEditor->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.dtPicDisplay = g_dtPicDisplay = NewObject(MUIC_DtPicDisplay->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.newProject = NewObject(MUIC_NewProject->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.settings = g_Settings = NewObject(MUIC_EditorSettings->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.paletteSelector = NewObject(MUIC_PaletteSelector->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.displayCreator = NewObject(MUIC_DisplayCreator->mcc_Class, NULL, TAG_END)),
    MUIA_Application_Window, (window.bitmapSelector = NewObject(MUIC_BitmapSelector->mcc_Class, NULL, TAG_END)),
  TAG_END);

  //Notifications
  if (App)
  {
    Object* tsc_add_zero;
    Object* tmc_add_zero;
    Object* lst_palettes;
    Object* chk_aga;

    get(Win, MUIA_Window_Window, &g_Window);

    DoMethod(Win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, App, 2,
      MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(toolbar.font, MUIM_Notify, MUIA_Pressed, FALSE, window.gameFontCreator, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.tileset, MUIM_Notify, MUIA_Pressed, FALSE, window.tilesetCreator, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.tilemap, MUIM_Notify, MUIA_Pressed, FALSE, window.tilemapCreator, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.bobsheet, MUIM_Notify, MUIA_Pressed, FALSE, window.bobsheetCreator, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.sprite, MUIM_Notify, MUIA_Pressed, FALSE, window.spritebankCreator, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.image, MUIM_Notify, MUIA_Pressed, FALSE, window.imageEditor, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.palette, MUIM_Notify, MUIA_Pressed, FALSE, window.paletteEditor, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.display, MUIM_Notify, MUIA_Pressed, FALSE, window.displayCreator, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.settings, MUIM_Notify, MUIA_Pressed, FALSE, window.settingsEditor, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    DoMethod(toolbar.assets, MUIM_Notify, MUIA_Pressed, FALSE, window.assetsEditor, 3,
      MUIM_Set, MUIA_Window_Open, TRUE);

    get(window.tilesetCreator, MUIA_TilesetCreator_AddZeroButton, &tsc_add_zero);
    get(window.tilemapCreator, MUIA_TilemapCreator_AddZeroButton, &tmc_add_zero);

    DoMethod(tsc_add_zero, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, tmc_add_zero, 3,
      MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);

    DoMethod(tmc_add_zero, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, tsc_add_zero, 3,
      MUIM_NoNotifySet, MUIA_Selected, MUIV_TriggerValue);

    DoMethod(toolbar.new, MUIM_Notify, MUIA_Pressed, FALSE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_New_Project_Window_Open);

    DoMethod(window.newProject, MUIM_Notify, MUIA_Window_Open, FALSE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_New_Project_Window_Close);

    DoMethod(window.newProject, MUIM_Notify, MUIA_NewProject_Create, TRUE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_New_Project_Create);

    DoMethod(toolbar.load, MUIM_Notify, MUIA_Pressed, FALSE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_Load_Project);

    DoMethod(toolbar.save, MUIM_Notify, MUIA_Pressed, FALSE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_Save_Project);

    DoMethod(window.settingsEditor, MUIM_Notify, MUIA_GameSettings_Edited, TRUE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_Edited);

    DoMethod(window.assetsEditor, MUIM_Notify, MUIA_AssetsEditor_Edited, TRUE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_Edited);

    DoMethod(window.paletteEditor, MUIM_Notify, MUIA_PaletteEditor_Edited, TRUE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_Edited);

    get(window.paletteEditor, MUIA_PaletteEditor_List, &lst_palettes);
    DoMethod(window.paletteSelector, MUIM_Set, MUIA_PaletteSelector_SourceList, lst_palettes);
    DoMethod(window.assetsEditor, MUIM_Set, MUIA_AssetsEditor_PaletteSelector, window.paletteSelector);
    DoMethod(window.displayCreator, MUIM_Set, MUIA_DisplayCreator_PaletteSelector, window.paletteSelector);
    DoMethod(window.assetsEditor, MUIM_Set, MUIA_AssetsEditor_BitmapSelector, window.bitmapSelector);

    get(window.settingsEditor, MUIA_GameSettings_AGACheck, &chk_aga);
    DoMethod(window.displayCreator, MUIM_Set, MUIA_DisplayCreator_AGACheck, chk_aga);

    DoMethod(toolbar.editor, MUIM_Notify, MUIA_Pressed, FALSE, App, 2,
      MUIM_Application_ReturnID, MUIV_App_RetID_Open_In_IDE);
  }

  return App;
}
///
///main
/***********************************************
 * Developer level main                        *
 * - Code your program here.                   *
 ***********************************************/
int Main(struct Config *config)
{
  int rc = 0;

  AslBase = OpenLibrary("asl.library", 0);
  if (AslBase) {
#if defined(__amigaos4__)
    if ((IAsl = (struct AslIFace*)GetInterface(AslBase, "main", 1, NULL))) {
#endif
      MUIMasterBase = OpenLibrary("muimaster.library", 0);
      if (MUIMasterBase) {
#if defined(__amigaos4__)
        if ((IMUIMaster = (struct MUIMasterIFace *)GetInterface(MUIMasterBase, "main", 1, NULL))) {
#endif
          if ((g_FileReq = MUI_AllocAslRequestTags(ASL_FileRequest, TAG_END))) {
            if ((g_FontReq = MUI_AllocAslRequestTags(ASL_FontRequest, TAG_END))) {
              if ((MUIC_PopASLString = MUI_Create_PopASLString())) {
                if ((MUIC_AckString = MUI_Create_AckString())) {
                  if ((MUIC_Integer = MUI_Create_Integer())) {
                    if ((MUIC_DtPicDisplay = MUI_Create_DtPicDisplay())) {
                      if ((MUIC_GamefontCreator = MUI_Create_GamefontCreator())) {
                        if ((MUIC_TilesetCreator = MUI_Create_TilesetCreator())) {
                          if ((MUIC_TilemapCreator = MUI_Create_TilemapCreator())) {
                            if ((MUIC_BobsheetCreator = MUI_Create_BobsheetCreator())) {
                              if ((MUIC_SpritebankCreator = MUI_Create_SpritebankCreator())) {
                                if ((MUIC_ColorGadget = MUI_Create_ColorGadget())) {
                                  if ((MUIC_ColorPalette = MUI_Create_ColorPalette())) {
                                    if ((MUIC_PaletteEditor = MUI_Create_PaletteEditor())) {
                                      if ((MUIC_ImageDisplay = MUI_Create_ImageDisplay())) {
                                        if ((MUIC_ImageEditor = MUI_Create_ImageEditor())) {
                                          if ((MUIC_SettingsEditor = MUI_Create_GameSettings())) {
                                            if ((MUIC_AssetsEditor = MUI_Create_AssetsEditor())) {
                                              if ((MUIC_NewProject = MUI_Create_NewProject())) {
                                                if ((MUIC_EditorSettings = MUI_Create_EditorSettings())) {
                                                  if ((MUIC_PaletteSelector = MUI_Create_PaletteSelector())) {
                                                    if ((MUIC_DisplayCreator = MUI_Create_DisplayCreator())) {
                                                      if ((MUIC_BitmapSelector = MUI_Create_BitmapSelector())) {
                                                        if (createImageSpecs()) {
                                                          if (buildGUI()) {
                                                            ULONG signals = 0;
                                                            BOOL running = TRUE;

                                                            set(Win, MUIA_Window_Open, TRUE);

                                                            while(running)
                                                            {
                                                              ULONG id = DoMethod (App, MUIM_Application_NewInput, &signals);
                                                              switch(id)
                                                              {
                                                                case MEN_ABOUT:
                                                                  MUI_Request(App, Win, NULL, "About " PROGRAMNAME, "*_Ok",
                                                                  PROGRAMNAME " v" VERSIONSTRING "\n\n%s " AUTHOR "\n%s " CONTACT, (ULONG)"Programming:", (ULONG)"Contact:");
                                                                break;
                                                                case MEN_ABOUTMUI:
                                                                  if (!AboutMUIWin) {
                                                                  AboutMUIWin = MUI_NewObject(MUIC_Aboutmui,
                                                                    MUIA_Window_RefWindow, Win,
                                                                    MUIA_Aboutmui_Application, App,
                                                                  TAG_END);
                                                                  }
                                                                  if (AboutMUIWin)
                                                                    set(AboutMUIWin, MUIA_Window_Open, TRUE);
                                                                break;
                                                                case MUIV_Application_ReturnID_Quit:
                                                                  if (checkEditedState()) {
                                                                    running = FALSE;
                                                                  }
                                                                break;
                                                                case MUIV_App_RetID_New_Project_Window_Open:
                                                                  if (checkEditedState()) {
                                                                    sleepAll(TRUE);
                                                                    DoMethod(window.newProject, MUIM_Set, MUIA_Window_Open, TRUE);
                                                                  }
                                                                break;
                                                                case MUIV_App_RetID_New_Project_Window_Close:
                                                                  sleepAll(FALSE);
                                                                break;
                                                                case MUIV_App_RetID_New_Project_Create:
                                                                {
                                                                  STRPTR name = NULL;
                                                                  STRPTR drawer = NULL;
                                                                  STRPTR directory = NULL;

                                                                  get(window.newProject, MUIA_NewProject_Name, &name);
                                                                  get(window.newProject, MUIA_NewProject_Drawer, &drawer);

                                                                  directory = makePath(drawer, name, NULL);
                                                                  replaceChars(FilePart(directory), " ", '_');

                                                                  openProject(directory);
                                                                  freeString(directory);
                                                                }
                                                                break;
                                                                case MUIV_App_RetID_Load_Project:
                                                                {
                                                                  if (checkEditedState()) {
                                                                    sleepAll(TRUE);
                                                                    if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Select the project drawer to open",
                                                                                                      ASLFR_Window, g_Window,
                                                                                                      ASLFR_SleepWindow, TRUE,
                                                                                                      ASLFR_PositiveText, "Open",
                                                                                                      ASLFR_DrawersOnly, TRUE,
                                                                                                      ASLFR_DoSaveMode, FALSE,
                                                                                                      ASLFR_DoPatterns, FALSE,
                                                                                                      ASLFR_InitialDrawer, PROJECTS_DIR "/",
                                                                                                      TAG_END) && strlen(g_FileReq->fr_Drawer)) {
                                                                      openProject(g_FileReq->fr_Drawer);
                                                                    }
                                                                    sleepAll(FALSE);
                                                                  }
                                                                }
                                                                break;
                                                                case MUIV_App_RetID_Save_Project:
                                                                  saveProject();
                                                                break;
                                                                case MUIV_App_RetID_Save_Project_As:
                                                                  saveProjectAs();
                                                                break;
                                                                case MUIV_App_RetID_Edited:
                                                                  DoMethod(toolbar.save, MUIM_Set, MUIA_Disabled, FALSE);
                                                                  DoMethod(Win, MUIM_Window_SetMenuState, MEN_SAVE_PROJECT, TRUE);
                                                                break;
                                                                case MEN_EDITOR_SETTINGS:
                                                                  DoMethod(window.settings, MUIM_Set, MUIA_Window_Open, TRUE);
                                                                break;
                                                                case MEN_HELP_ENGINE:
                                                                  openGuide(MEN_HELP_ENGINE);
                                                                break;
                                                                case MEN_HELP_EDITOR:
                                                                  openGuide(MEN_HELP_EDITOR);
                                                                break;
                                                                case MUIV_App_RetID_Open_In_IDE:
                                                                  openInIDE();
                                                                break;
                                                              }
                                                              if(running && signals) signals = Wait(signals | SIGBREAKF_CTRL_C);
                                                              if (signals & SIGBREAKF_CTRL_C) break;
                                                            }

                                                            set(Win, MUIA_Window_Open, FALSE);

                                                            MUI_DisposeObject(App);
                                                          }
                                                          else rc = 20;

                                                          freeImageSpecs();
                                                        }

                                                        MUI_DeleteCustomClass(MUIC_BitmapSelector);
                                                      }
                                                      MUI_DeleteCustomClass(MUIC_DisplayCreator);
                                                    }
                                                    MUI_DeleteCustomClass(MUIC_PaletteSelector);
                                                  }
                                                  MUI_DeleteCustomClass(MUIC_EditorSettings);
                                                }
                                                MUI_DeleteCustomClass(MUIC_NewProject);
                                              }
                                              MUI_DeleteCustomClass(MUIC_AssetsEditor);
                                            }
                                            MUI_DeleteCustomClass(MUIC_SettingsEditor);
                                          }
                                          MUI_DeleteCustomClass(MUIC_ImageEditor);
                                        }
                                        MUI_DeleteCustomClass(MUIC_ImageDisplay);
                                      }
                                      MUI_DeleteCustomClass(MUIC_PaletteEditor);
                                    }
                                    MUI_DeleteCustomClass(MUIC_ColorPalette);
                                  }
                                  MUI_DeleteCustomClass(MUIC_ColorGadget);
                                }
                                MUI_DeleteCustomClass(MUIC_SpritebankCreator);
                              }
                              MUI_DeleteCustomClass(MUIC_BobsheetCreator);
                            }
                            MUI_DeleteCustomClass(MUIC_TilemapCreator);
                          }
                          MUI_DeleteCustomClass(MUIC_TilesetCreator);
                        }
                        MUI_DeleteCustomClass(MUIC_GamefontCreator);
                      }
                      MUI_DeleteCustomClass(MUIC_DtPicDisplay);
                    }
                    MUI_DeleteCustomClass(MUIC_Integer);
                  }
                  MUI_DeleteCustomClass(MUIC_AckString);
                }
                MUI_DeleteCustomClass(MUIC_PopASLString);
              }
              MUI_FreeAslRequest(g_FontReq);
            }
            MUI_FreeAslRequest(g_FileReq);
          }
#if defined(__amigaos4__)
          DropInterface((struct Interface *)IMUIMaster);
        }
        else rc = 20;
#endif
        CloseLibrary(MUIMasterBase);
      }
      else rc = 20;
#if defined(__amigaos4__)
      DropInterface((struct Interface *)IAsl);
    }
    else rc = 20;
#endif
    CloseLibrary(AslBase);
  }
  else rc = 20;

  return(rc);
}
///
///cleanup
/***********************************************
 * Clean up before exit                        *
 * - Free allocated resources here.            *
 ***********************************************/
void CleanUp(struct Config *config)
{
  if (config)
  {
    //<YOUR CLEAN UP CODE HERE>

    // free command line arguments
    #if RDARGS_OPTIONS
      if (config->RDArgs)
        FreeArgs(config->RDArgs);
    #endif

    FreePooled(g_MemoryPool, config, sizeof(struct Config));
  }

  if (g_MemoryPool) {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_MEMPOOL, g_MemoryPool);
    #else
    DeletePool(g_MemoryPool);
    #endif
  }
}
///

///checkAvailTools()
VOID checkAvailTools()
{
  ULONG num_tools = sizeof(g_Tools) / (sizeof(STRPTR) + sizeof(BOOL));
  ULONG i;
  STRPTR* strings = (STRPTR*)&g_Tools;
  BOOL* avails = (BOOL*)&g_Tools.avail;

  for (i = 0; i < num_tools; i++) {
    if (Exists(strings[i])) avails[i] = TRUE;
  }
}
///
///sleepAll(sleep)
/******************************************************************************
 * Sleeps/Wakes all windows except newProject window.                         *
 ******************************************************************************/
VOID sleepAll(BOOL sleep)
{
  Object** windows = (Object**)&window;
  ULONG i;
  ULONG i_max = sizeof(window) / sizeof(Object*);

  for (i = 1; i < i_max; i++) {
    DoMethod(windows[i], MUIM_Set, MUIA_Window_Sleep, sleep);
  }

  DoMethod(Win, MUIM_Set, MUIA_Window_Sleep, sleep);
}
///
///isProjectDirectory(directory)
BOOL isProjectDirectory(STRPTR directory)
{
  BOOL retval = FALSE;

  if (directory) {
    BPTR lock = Lock(directory, SHARED_LOCK);

    if (lock) {
      struct FileInfoBlock* fib = AllocDosObject(DOS_FIB, NULL);

      if (fib) {
        if (Examine(lock, fib)) {
          UBYTE bitfield = 0x00;

          while (ExNext(lock, fib)) {
            if (fib->fib_DirEntryType < 0) {
              if (!strcmp(fib->fib_FileName, "settings.h")) bitfield |= 0x01;
              if (!strcmp(fib->fib_FileName, "assets.h"))   bitfield |= 0x02;
              if (!strcmp(fib->fib_FileName, "palettes.h")) bitfield |= 0x04;
              if (bitfield == 0x07) {
                retval = TRUE;
                break;
              }
            }
          }
        }
        FreeDosObject(DOS_FIB, fib);
      }
      UnLock(lock);
    }
  }

  return retval;
}
///
///openProject()
VOID openProject(STRPTR directory)
{
  if (isProjectDirectory(directory)) {

    freeString(g_Project.directory);
    freeString(g_Project.settings_header);
    freeString(g_Project.assets_header);
    freeString(g_Project.palettes_header);
    freeString(g_Project.data_drawer);
    freeString(g_Project.assets_drawer);
    freeString(g_Win_Title);

    g_Project.directory = makeString(directory);
    if (g_Project.directory) {
      g_Project.settings_header = makePath(g_Project.directory, "settings.h", NULL);
      g_Project.assets_header = makePath(g_Project.directory, "assets.h", NULL);
      g_Project.palettes_header = makePath(g_Project.directory, "palettes.h", NULL);
      g_Project.data_drawer = makePath(g_Project.directory, "data/", NULL);
      g_Project.assets_drawer = makePath(g_Project.directory, "assets", NULL);
      g_Win_Title = makeString2(PROGRAMNAME " - ", FilePart(g_Project.directory));
      replaceChars(g_Win_Title, "_", ' ');

      DoMethod(window.settingsEditor, MUIM_GameSettings_Load,  g_Project.settings_header);
      DoMethod(window.assetsEditor,   MUIM_AssetsEditor_Load,  g_Project.assets_header);
      DoMethod(window.paletteEditor,  MUIM_PaletteEditor_Load, g_Project.palettes_header);

      DoMethod(toolbar.save, MUIM_Set, MUIA_Disabled, TRUE);
      DoMethod(Win, MUIM_Window_SetMenuState, MEN_SAVE_PROJECT, FALSE);
      DoMethod(Win, MUIM_Window_SetMenuState, MEN_SAVE_PROJECT_AS, FALSE);
      DoMethod(toolbar.settings, MUIM_Set, MUIA_Disabled, FALSE);
      DoMethod(toolbar.assets, MUIM_Set, MUIA_Disabled, FALSE);
      DoMethod(toolbar.palette, MUIM_Set, MUIA_Disabled, FALSE);
      DoMethod(toolbar.display, MUIM_Set, MUIA_Disabled, FALSE);
      DoMethod(toolbar.editor, MUIM_Set, MUIA_Disabled, FALSE);
      DoMethod(Win, MUIM_Set, MUIA_Window_Title, g_Win_Title);
      DoMethod(window.gameFontCreator, MUIM_Set, MUIA_GamefontCreator_AssetsDrawer, g_Project.assets_drawer);
      DoMethod(window.tilesetCreator, MUIM_Set, MUIA_TilesetCreator_AssetsDrawer, g_Project.assets_drawer);
      DoMethod(window.tilemapCreator, MUIM_Set, MUIA_TilemapCreator_AssetsDrawer, g_Project.assets_drawer);
      DoMethod(window.bobsheetCreator, MUIM_Set, MUIA_BobsheetCreator_AssetsDrawer, g_Project.assets_drawer);
      DoMethod(window.spritebankCreator, MUIM_Set, MUIA_SpritebankCreator_AssetsDrawer, g_Project.assets_drawer);
      DoMethod(window.bitmapSelector, MUIM_Set, MUIA_BitmapSelector_DataDrawer, g_Project.data_drawer);
    }
  }
  else {
    MUI_Request(App, Win, NULL, "Sevgi Editor", "*_Ok", "Selected directory does not contain a Sevgi Engine project!");
  }
}
///
///saveProject()
VOID saveProject()
{
  ULONG num_levels;
  ULONG num_textfonts;
  ULONG num_gamefonts;

  sleepAll(TRUE);

  get(window.assetsEditor, MUIA_AssetsEditor_NumLevels,    &num_levels);
  get(window.assetsEditor, MUIA_AssetsEditor_NumTextfonts, &num_textfonts);
  get(window.assetsEditor, MUIA_AssetsEditor_NumGamefonts, &num_gamefonts);

  DoMethod(window.settingsEditor, MUIM_GameSettings_Save,  g_Project.settings_header, num_levels, num_textfonts, num_gamefonts);
  DoMethod(window.assetsEditor,   MUIM_AssetsEditor_Save,  g_Project.assets_header);
  DoMethod(window.paletteEditor,  MUIM_PaletteEditor_Save, g_Project.palettes_header);

  DoMethod(toolbar.save, MUIM_Set, MUIA_Disabled, TRUE);
  DoMethod(Win, MUIM_Window_SetMenuState, MEN_SAVE_PROJECT, FALSE);

  sleepAll(FALSE);
}
///
///saveProjectAs()
VOID saveProjectAs()
{
  BOOL save_confirmed = TRUE;
  STRPTR projects_dir = makePath(g_Program_Directory, PROJECTS_DIR, NULL);

  if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Create a new drawer to save the project as",
                                    ASLFR_Window, g_Window,
                                    ASLFR_SleepWindow, TRUE,
                                    ASLFR_PositiveText, "Save",
                                    ASLFR_DrawersOnly, TRUE,
                                    ASLFR_DoSaveMode, FALSE,
                                    ASLFR_DoPatterns, FALSE,
                                    ASLFR_InitialDrawer, PROJECTS_DIR "/",
                                    TAG_END) && strlen(g_FileReq->fr_Drawer)) {
    if (Exists(g_FileReq->fr_Drawer)) {
      if (strcmp(g_FileReq->fr_Drawer, projects_dir) &&
          strcmp(g_FileReq->fr_Drawer, PROJECTS_DIR) &&
          strcmp(g_FileReq->fr_Drawer, PROJECTS_DIR "/") &&
          strcmp(g_FileReq->fr_Drawer, g_Program_Directory)) {

        if (isProjectDirectory(g_FileReq->fr_Drawer)) {
          if (!MUI_Request(App, Win, NULL, "Warning!", "_Overwrite|*_Cancel", "This drawer already contains another project\nDo you want to overwrite it?")) {
            save_confirmed = FALSE;
          }
        }
      }
      else {
        MUI_Request(App, Win, NULL, "Warning!", "You cannot save a project here!", "*_OK");
        save_confirmed = FALSE;
      }
    }
    else {
      BPTR lock = CreateDir(g_FileReq->fr_Drawer);

      if (lock) UnLock(lock);
      else {
        MUI_Request(App, Win, NULL, "Warning!", "Cannot create save drawer!", "*_OK");
        save_confirmed = FALSE;
      }
    }

    if (save_confirmed) {
      freeString(g_Project.directory);
      freeString(g_Project.settings_header);
      freeString(g_Project.assets_header);
      freeString(g_Project.palettes_header);
      freeString(g_Project.data_drawer);
      freeString(g_Project.assets_drawer);
      freeString(g_Win_Title);

      g_Project.directory = makeString(g_FileReq->fr_Drawer);
      if (g_Project.directory) {
        g_Project.settings_header = makePath(g_Project.directory, "settings.h", NULL);
        g_Project.assets_header = makePath(g_Project.directory, "assets.h", NULL);
        g_Project.palettes_header = makePath(g_Project.directory, "palettes.h", NULL);
        g_Project.data_drawer = makePath(g_Project.directory, "data/", NULL);
        g_Project.assets_drawer = makePath(g_Project.directory, "assets", NULL);
        g_Win_Title = makeString2(PROGRAMNAME " - ", FilePart(g_Project.directory));
        replaceChars(g_Win_Title, "_", ' ');

        saveProject();

        DoMethod(Win, MUIM_Set, MUIA_Window_Title, g_Win_Title);
      }
    }
  }

  freeString(projects_dir);
}
///
///checkEditedState()
/******************************************************************************
 * Retrieves the edited state of the project from editor windows, pops up a   *
 * requester if there are unsaved changes, returns TRUE if it is ok to        *
 * discard current project.                                                   *
 ******************************************************************************/
BOOL checkEditedState()
{
  ULONG edited_project, edited_settings, edited_assets, edited_palettes;
  BOOL edited_discard = TRUE;

  get(window.settingsEditor, MUIA_GameSettings_Edited, &edited_settings);
  get(window.assetsEditor, MUIA_AssetsEditor_Edited, &edited_assets);
  get(window.paletteEditor, MUIA_PaletteEditor_Edited, &edited_palettes);

  edited_project = edited_settings | edited_assets | edited_palettes;

  if (edited_project && !MUI_Request(App, Win, NULL, "Warning!", "*_Discard|_Cancel", "You have unsaved changes on the current project!\nDo you really want to discard them?")) {
    edited_discard = FALSE;
  }

  return edited_discard;
}
///
///openInIDE()
VOID openInIDE()
{
  STRPTR ide_program = NULL;
  STRPTR main_c_path = NULL;
  STRPTR cmd = "RUN";
  STRPTR cmd_str = NULL;
  ULONG cmd_str_len;
  struct ReturnInfo rtrn;

  get(window.settings, MUIA_EditorSettings_IDE, &ide_program);
  main_c_path = makePath(g_Project.directory, "main.c", NULL);
  if (main_c_path && ide_program) {
    cmd_str_len = strlen(cmd) + 2 + strlen(ide_program) + 3 + strlen(main_c_path) + 2;
    cmd_str = AllocPooled(g_MemoryPool, cmd_str_len);

    if (cmd_str) {
      strcpy(cmd_str, cmd);
      strcat(cmd_str, " \"");
      strcat(cmd_str, ide_program);
      strcat(cmd_str, "\" \"");
      strcat(cmd_str, main_c_path);
      strcat(cmd_str, "\"");

      if (execute(&rtrn, cmd_str)) {
        if (rtrn.code > 0) {
          MUI_Request(App, Win, NULL, "Warning!", "*_OK", "Running the IDE failed because of the following error:\n\n%s", (ULONG)rtrn.string);
        }
      }
    }
  }

  freeString(cmd_str);
  freeString(main_c_path);
}
///
///openGuide(id)
VOID openGuide(ULONG id)
{
  BPTR docs_dir = Lock("Docs", SHARED_LOCK);
  if (docs_dir) {
    BPTR current_dir = CurrentDir(docs_dir);
    struct ReturnInfo ri;

    DoMethod(Win, MUIM_Set, MUIA_Window_Sleep, TRUE);

    switch (id) {
      case MEN_HELP_ENGINE:
        execute(&ri, "Multiview Sevgi_Engine.guide");
      break;
      case MEN_HELP_EDITOR:
        execute(&ri, "Multiview Sevgi_Editor.guide");
      break;
    }

    DoMethod(Win, MUIM_Set, MUIA_Window_Sleep, FALSE);

    CurrentDir(current_dir);
    UnLock(docs_dir);
  }
}
///
