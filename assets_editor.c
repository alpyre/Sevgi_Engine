/******************************************************************************
 * AssetsEditor                                                               *
 ******************************************************************************/
///Defines
#define MUIM_AssetsEditor_AddFont      0x80430701
#define MUIM_AssetsEditor_AddGamefont  0x80430702
#define MUIM_AssetsEditor_AddLevel     0x80430703
#define MUIM_AssetsEditor_AckLevelName 0x80430705
#define MUIM_AssetsEditor_SelectLevel  0x80430706
#define MUIM_AssetsEditor_SelectAType  0x80430707
#define MUIM_AssetsEditor_AddAsset     0x80430708
#define MUIM_AssetsEditor_RemoveAsset  0x80430709
#define MUIM_AssetsEditor_MoveLVItem   0x8043070C // This can go into utility.h
#define MUIM_AssetsEditor_AddPalette   0x8043070D
#define MUIM_AssetsEditor_UpdateMapPos 0x8043070E
#define MUIM_AssetsEditor_AddBitmap    0x80430710

#define WriteSaveString(s) Write(fh, save_strings[s], strlen(save_strings[s]))
#define WriteString(s) Write(fh, s, strlen(s))

#define MENU_LEVEL 0
#define GAME_LEVEL 1

#define MAP_POS_X 0
#define MAP_POS_Y 1
///
///Includes
#include <string.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <graphics/text.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>          // <-- Required for tag redirection
#include <proto/muimaster.h>
#include <clib/alib_protos.h>       // <-- Required for DoSuperMethod()
#include <clib/alib_stdio_protos.h> // <-- Required for sprintf()

#include <SDI_compiler.h>           //     Required for
#include <SDI_hook.h>               // <-- multi platform
#include <SDI_stdarg.h>             //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "integer_gadget.h"
#include "palette_selector.h"
#include "bitmap_selector.h"
#include "assets_editor.h"
///
///Structs
struct PaletteItem {
  STRPTR name;
  UBYTE* palette;
};

struct cl_ObjTable
{
  Object* lv_fonts;
  Object* btn_font_add;
  Object* btn_font_remove;
  Object* btn_font_up;
  Object* btn_font_down;
  Object* lv_gamefonts;
  Object* btn_gamefont_add;
  Object* btn_gamefont_remove;
  Object* btn_gamefont_up;
  Object* btn_gamefont_down;
  Object* lv_levels;
  Object* str_level_name;
  Object* int_initial_mapPosX;
  Object* int_initial_mapPosY;
  Object* btn_level_add;
  Object* btn_level_remove;
  Object* btn_level_up;
  Object* btn_level_down;
  Object* lv_asset_types;
  Object* lv_assets;
  Object* btn_asset_add;
  Object* btn_asset_remove;
  Object* btn_asset_up;
  Object* btn_asset_down;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  Object* palette_selector;
  Object* bitmap_selector;
  BOOL subscribed_to_palette_selector;
  BOOL edited;
};

struct cl_Msg
{
  ULONG MethodID;
  STRPTR name;
};

struct cl_Msg2
{
  ULONG MethodID;
  ULONG pos;
};

struct cl_Msg3
{
  ULONG MethodID;
  struct PaletteItem* palette_item;
};

struct cl_Msg4
{
  ULONG MethodID;
  ULONG axis;
  LONG pos;
};

struct cl_Msg5
{
  ULONG MethodID;
  STRPTR bitmap_string;
};

struct cl_MoveMsg
{
  ULONG MethodID;
  Object* listview;
  ULONG direction; // UP or DOWN
};

struct StringItem {
  struct MinNode node;
  STRPTR string;
};

struct Level {
  STRPTR name;
  STRPTR blit_func;
  STRPTR unblit_func;
  STRPTR width_define;
  STRPTR height_define;
  LONG mapPosX;
  LONG mapPosY;
  struct MinList tilesets;
  struct MinList tilemaps;
  struct MinList bob_sheets;
  struct MinList sprite_banks;
  struct MinList music_modules;
  struct MinList sound_samples;
  struct MinList palettes;
  struct MinList gameobject_banks;
  struct MinList bitmaps;
};

//level data offsets (has to match struct LevelData from engine code)
#define LEVEL_DATA_STRINGS 16

enum {
  LDO_NAME,
  LDO_TILESETS,
  LDO_TILEMAPS,
  LDO_BOB_SHEETS,
  LDO_SPRITEBANKS,
  LDO_MUSIC_MODULES,
  LDO_SOUND_SAMPLES,
  LDO_PALETTES,
  LDO_GAMEOOBJECT_BANKS,
  LDO_BITMAPS,
  LDO_BLTBOBFUNC,
  LDO_UNBLTBOBFUNC,
  LDO_SCREEN_WIDTH,
  LDO_SCREEN_HEIGHT,
  LDO_INITIAL_MAPPOSX,
  LDO_INITIAL_MAPPOSY
};

struct LevelDataItem {
  struct MinNode node;
  STRPTR string[LEVEL_DATA_STRINGS];
};
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;
extern struct FontRequester* g_FontReq;

extern Object* App;
extern struct MUI_CustomClass* MUIC_Integer;
extern struct MUI_CustomClass* MUIC_ImageDisplay;

extern STRPTR g_Program_Directory;
extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;

#define NUM_ASSETS 9

STATIC STRPTR asset_types[] = {
  "Tilesets",
  "Tilemaps",
  "BobSheets",
  "SpriteBanks",
  "Music Modules",
  "Sound Samples",
  "Palettes",
  "Gameobject Banks",
  "BitMaps",
  NULL
};

enum {
  AT_TILESET,
  AT_TILEMAP,
  AT_BOBSHEET,
  AT_SPRITEBANK,
  AT_MUSICMODULE,
  AT_SOUNDSAMPLE,
  AT_PALETTE,
  AT_GAMEOBJECTBANK,
  AT_BITMAP
};

STATIC STRPTR save_strings[] = {
"#ifndef ASSETS_H\n#define ASSETS_H\n\n",
"#ifdef FONTS_H\nSTATIC struct TextAttr textAttrs[NUM_TEXTFONTS] = {",
",\n                                                   ",
"};\nSTATIC STRPTR gameFontFiles[NUM_GAMEFONTS] = {",
",\n                                              ",
"};\n#endif\n\n",
"#ifdef LEVEL_H\n",
"\nSTATIC struct LevelData levelData[NUM_LEVELS] = {",
"                                                  ",
"                                                 }",
",\n                                                 ",
"\n                                                };\n#endif\n\n",
"#endif /* ASSETS_H */"
};

enum {
  SS_HEADER,
  SS_FONTS_HEADER,
  SS_FONTS_ALIGNER,
  SS_GAMEFONTS_HEADER,
  SS_GAMEFONTS_ALIGNER,
  SS_FONTS_FOOTER,
  SS_LEVEL_HEADER,
  SS_LEVEL_DATA,
  SS_LEVEL_ALIGNER,
  SS_LEVEL_SEPARATOR,
  SS_LEVEL_ALIGNER2,
  SS_LEVEL_FOOTER,
  SS_FOOTER
};

STATIC STRPTR asset_strings[] = {
"tilesets",
"tilemaps",
"bob_sheets",
"sprite_banks",
"music_modules",
"sound_samples",
"palettes",
"gameobj_banks",
"bitmaps"
};

STATIC STRPTR asset_aligners[] = {
"      ",
"      ",
"    ",
"  ",
" ",
" ",
"      ",
" ",
"       "
};
///
///Protos
STATIC VOID freeTextAttr(struct TextAttr* text_attr);
STATIC VOID freeLevel(struct Level* level);
///
///Hooks
//DISPLAY HOOK FOR struct TextAttr items
HOOKPROTO(TextAttr_dispfunc, LONG, char **array, struct TextAttr* p)
{
  static UBYTE size[8];
  static UBYTE style[8];
  strcpy(style, ".....");

  if (p) {
    *array++ = p->ta_Name;

    sprintf(size, "%lu", (ULONG)p->ta_YSize);
    *array++ = size;

    if (p->ta_Style & FSF_UNDERLINED) style[0] = 'U';
    if (p->ta_Style & FSF_BOLD)       style[1] = 'B';
    if (p->ta_Style & FSF_ITALIC)     style[2] = 'I';
    if (p->ta_Style & FSF_EXTENDED)   style[3] = 'E';
    if (p->ta_Style & FSF_COLORFONT)  style[4] = 'C';
    *array++ = style;
  }
  else {
    *array++ = "\33b\33uName";
    *array++ = "\33b\33uSize";
    *array++ = "\33b\33uStyle";
  }

  return(0);
}
MakeStaticHook(TextAttr_disphook, TextAttr_dispfunc);

//DESTRUCT HOOK FOR struct TextAttr items
HOOKPROTO(TextAttr_destructfunc, VOID, APTR pool, struct TextAttr* p)
{
  freeTextAttr(p);
}
MakeStaticHook(TextAttr_destructhook, TextAttr_destructfunc);

//DESTRUCT HOOK FOR gamefont paths
HOOKPROTO(Gamefont_destructfunc, VOID, APTR pool, STRPTR p)
{
  freeString(p);
}
MakeStaticHook(Gamefont_destructhook, Gamefont_destructfunc);

//DISPLAY HOOK FOR Level items
HOOKPROTO(level_dispfunc, LONG, char **array, struct Level* p)
{
  if (p) {
    static UBYTE pos[8];
    sprintf(pos, "%ld", (LONG)array[-1]);

    *array++ = pos;
    *array++ = p->name;
  }
  else {
    *array++ = "\33b\33uNum";
    *array++ = "\33b\33uName";
  }

  return(0);
}
MakeStaticHook(level_disphook, level_dispfunc);

//DESTRUCT HOOK FOR Level items
HOOKPROTO(level_destructfunc, VOID, APTR pool, struct Level* p)
{
  freeLevel(p);
}
MakeStaticHook(level_destructhook, level_destructfunc);

//DISPLAY HOOK FOR StringItems
HOOKPROTO(string_dispfunc, LONG, char **array, struct StringItem* p)
{
  if (p) *array++ = p->string;

  return(0);
}
MakeStaticHook(string_disphook, string_dispfunc);
///

///newStringItem(string)
STATIC struct StringItem* newStringItem(STRPTR string)
{
  struct StringItem* str_item = (struct StringItem*)AllocPooled(g_MemoryPool, sizeof(struct StringItem));

  if (str_item) {
    str_item->string = makeString(string);
  }

  return str_item;
}
///
///freeStringItem(string_item)
STATIC VOID freeStringItem(struct StringItem* str_item)
{
  if (str_item) {
    freeString(str_item->string);
    FreePooled(g_MemoryPool, str_item, sizeof(struct StringItem));
  }
}
///
///newLevel(type)
STATIC struct Level* newLevel(ULONG type)
{
  struct Level* level = (struct Level*)AllocPooled(g_MemoryPool, sizeof(struct Level));

  if (level) {
    level->name = makeString("");
    level->blit_func = makeString(type == MENU_LEVEL ? "MD_blitBOB" : "LD_blitBOB");
    level->unblit_func = makeString(type == MENU_LEVEL ? "MD_unBlitBOB" : "LD_unBlitBOB");
    level->width_define = makeString(type == MENU_LEVEL ? "MENU_SCREEN_WIDTH" : "SCREEN_WIDTH");
    level->height_define = makeString(type == MENU_LEVEL ? "MENU_SCREEN_HEIGHT" : "SCREEN_HEIGHT");
    NewList((struct List*)&level->tilesets);
    NewList((struct List*)&level->tilemaps);
    NewList((struct List*)&level->bob_sheets);
    NewList((struct List*)&level->sprite_banks);
    NewList((struct List*)&level->music_modules);
    NewList((struct List*)&level->sound_samples);
    NewList((struct List*)&level->palettes);
    NewList((struct List*)&level->gameobject_banks);
    NewList((struct List*)&level->bitmaps);
  }

  return level;
}
///
///freeLevel(level)
STATIC VOID freeLevel(struct Level* level)
{
  if (level) {
    struct MinList* list;
    struct StringItem* str, *next;

    for (list = &level->tilesets; list <= &level->bitmaps; list++) {
      for (str = (struct StringItem*)list->mlh_Head; (next = (struct StringItem*)str->node.mln_Succ); str = next) {
        freeStringItem(str);
      }
    }

    freeString(level->name);
    freeString(level->blit_func);
    freeString(level->unblit_func);
    freeString(level->width_define);
    freeString(level->height_define);
    FreePooled(g_MemoryPool, level, sizeof(struct Level));
  }
}
///
///newTextAttr(source)
STATIC struct TextAttr* newTextAttr(struct TextAttr* source)
{
  struct TextAttr* text_attr = NULL;

  if (source && source->ta_Name && strlen(source->ta_Name)) {
    text_attr = (struct TextAttr*) AllocPooled(g_MemoryPool, sizeof(struct TextAttr));
    if (text_attr) {
      text_attr->ta_Name  = makeString(source->ta_Name);
      text_attr->ta_YSize = source->ta_YSize;
      text_attr->ta_Style = source->ta_Style;
      text_attr->ta_Flags = source->ta_Flags;
    }
  }

  return text_attr;
}
///
///freeTextAttr(TextAttr)
STATIC VOID freeTextAttr(struct TextAttr* text_attr)
{
  if (text_attr) {
    freeString(text_attr->ta_Name);
    FreePooled(g_MemoryPool, text_attr, sizeof(struct TextAttr));
  }
}
///
///fillAssetsList(lv_assets, asset_type, level)
STATIC VOID fillAssetsList(Object* lv_assets, ULONG asset_type, struct Level* level)
{
  DoMethod(lv_assets, MUIM_List_Clear);

  if (level && asset_type != MUIV_List_Active_Off) {
    struct MinList* list;
    struct StringItem* str;

    switch (asset_type) {
      case AT_TILESET:
        list = &level->tilesets;
      break;
      case AT_TILEMAP:
        list = &level->tilemaps;
      break;
      case AT_BOBSHEET:
        list = &level->bob_sheets;
      break;
      case AT_SPRITEBANK:
        list = &level->sprite_banks;
      break;
      case AT_MUSICMODULE:
        list = &level->music_modules;
      break;
      case AT_SOUNDSAMPLE:
        list = &level->sound_samples;
      break;
      case AT_PALETTE:
        list = &level->palettes;
      break;
      case AT_GAMEOBJECTBANK:
        list = &level->gameobject_banks;
      break;
      case AT_BITMAP:
        list = &level->bitmaps;
      break;
      default:
        return;
    }

    for (str = (struct StringItem*)list->mlh_Head; str->node.mln_Succ; str = (struct StringItem*)str->node.mln_Succ) {
      DoMethod(lv_assets, MUIM_List_InsertSingle, str, MUIV_List_Insert_Bottom);
    }
  }
}
///
///stripProjectDataDrawer(path)
VOID stripProjectDataDrawer(STRPTR* pathname) {
  STRPTR path = *pathname;

  ULONG len = strlen(g_Project.data_drawer);

  if (path && g_Project.data_drawer && !strncmp(path, g_Project.data_drawer, len)) {
    STRPTR stripped = makeString(&path[len]);
    if (stripped) {
      freeString(path);
      *pathname = stripped;
    }
  }
}
///
///getAssetList(data)
struct MinList* getAssetList(struct cl_Data *data)
{
  struct MinList* list = NULL;
  struct Level* level;
  ULONG asset_type;

  DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &level);
  get(data->obj_table.lv_asset_types, MUIA_List_Active, &asset_type);

  if (level) {
    switch (asset_type) {
      case AT_TILESET:
      list = &level->tilesets;
      break;
      case AT_TILEMAP:
      list = &level->tilemaps;
      break;
      case AT_BOBSHEET:
      list = &level->bob_sheets;
      break;
      case AT_SPRITEBANK:
      list = &level->sprite_banks;
      break;
      case AT_MUSICMODULE:
      list = &level->music_modules;
      break;
      case AT_SOUNDSAMPLE:
      list = &level->sound_samples;
      break;
      case AT_PALETTE:
      list = &level->palettes;
      break;
      case AT_GAMEOBJECTBANK:
      list = &level->gameobject_banks;
      break;
      case AT_BITMAP:
      list = &level->bitmaps;
      break;
    }
  }

  return list;
}
///

///m_MoveListviewItem(listview, direction)
STATIC ULONG m_MoveListviewItem(struct IClass* cl, Object* obj, struct cl_MoveMsg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG entries;
  ULONG active;

  get(msg->listview, MUIA_List_Entries, &entries);
  get(msg->listview, MUIA_List_Active, &active);

  if (entries > 1) {
    switch (msg->direction) {
      case MUIV_List_Move_Previous:
        if (active > 0) {
          if (msg->listview == data->obj_table.lv_assets) {
            struct MinList* list = getAssetList(data);
            if (list) {
              struct StringItem* selected;
              struct StringItem* prev;
              DoMethod(data->obj_table.lv_assets, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected);
              prev = (struct StringItem*)selected->node.mln_Pred;
              Remove((struct Node*)selected);
              if (active == 1) AddHead((struct List*)list, (struct Node*)selected);
              else Insert((struct List*)list, (struct Node*)selected, (struct Node*)prev->node.mln_Pred);
            }
          }
          DoMethod(msg->listview, MUIM_List_Move, MUIV_List_Move_Active, MUIV_List_Move_Previous);
          DoMethod(msg->listview, MUIM_Set, MUIA_List_Active, active - 1);
          DoMethod(obj, MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
        }
      break;
      case MUIV_List_Move_Next:
        if (active < entries - 1) {
          if (msg->listview == data->obj_table.lv_assets) {
            struct MinList* list = getAssetList(data);
            if (list) {
              struct StringItem* selected;
              struct StringItem* next;
              DoMethod(data->obj_table.lv_assets, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected);
              next = (struct StringItem*)selected->node.mln_Succ;
              Remove((struct Node*)selected);
              Insert((struct List*)list, (struct Node*)selected, (struct Node*)next);
            }
          }
          DoMethod(msg->listview, MUIM_List_Move, MUIV_List_Move_Active, MUIV_List_Move_Next);
          DoMethod(msg->listview, MUIM_Set, MUIA_List_Active, active + 1);
          DoMethod(obj, MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
        }
      break;
    }
  }

  return 0;
}
///
///m_Reset()
STATIC ULONG m_Reset(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  DoMethod(data->obj_table.lv_fonts, MUIM_List_Clear);
  DoMethod(data->obj_table.lv_gamefonts, MUIM_List_Clear);
  DoMethod(data->obj_table.lv_levels, MUIM_List_Clear);
  DoMethod(data->obj_table.int_initial_mapPosX, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.int_initial_mapPosY, MUIM_NoNotifySet, MUIA_Integer_Value, 0);

  //TODO: Add un-removable level 0 (Main Menu) here!

  return 0;
}
///
///m_AddFont()
STATIC ULONG m_AddFont(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

  if (MUI_AslRequestTags(g_FontReq, ASLFO_TitleText, "Select Amiga Font",
                                    ASLFO_PositiveText, "Select",
                                    ASLFO_DoStyle, TRUE,
                                    TAG_END) && strcmp(g_FontReq->fo_Attr.ta_Name, ".font")) {
    struct TextAttr* text_attr = newTextAttr(&g_FontReq->fo_Attr);
    if (text_attr) {
      DoMethod(data->obj_table.lv_fonts, MUIM_List_InsertSingle, text_attr, MUIV_List_Insert_Bottom);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);

  return 0;
}
///
///m_AddGamefont()
STATIC ULONG m_AddGamefont(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

  if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Select gamefont",
                                    ASLFR_Window, window,
                                    ASLFR_PositiveText, "Select",
                                    ASLFR_DrawersOnly, FALSE,
                                    ASLFR_DoSaveMode, FALSE,
                                    ASLFR_DoPatterns, TRUE,
                                    ASLFR_InitialPattern, "*.fnt",
                                    ASLFR_InitialDrawer, g_Project.data_drawer,
                                    ASLFR_InitialFile, "",
                                    TAG_END) && strlen(g_FileReq->fr_File)) {
    STRPTR pathname = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);
    stripProjectDataDrawer(&pathname);

    if (pathname) {
      DoMethod(data->obj_table.lv_gamefonts, MUIM_List_InsertSingle, pathname, MUIV_List_Insert_Bottom);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);

  return 0;
}
///
///m_AckLevelName()
STATIC ULONG m_AckLevelName(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Level* level;

  DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &level);

  if (level && msg->name) {
    freeString(level->name);
    level->name = makeString(msg->name);

    DoMethod(data->obj_table.lv_levels, MUIM_List_Redraw, MUIV_List_Redraw_Active);
  }

  return 0;
}
///
///m_UpdateMapPos()
STATIC ULONG m_UpdateMapPos(struct IClass* cl, Object* obj, struct cl_Msg4* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Level* level;

  DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &level);

  if (level) {
    switch (msg->axis) {
      case MAP_POS_X:
        level->mapPosX = msg->pos;
      break;
      case MAP_POS_Y:
        level->mapPosY = msg->pos;
      break;
    }
  }

  return 0;
}
///
///m_SelectLevel()
STATIC ULONG m_SelectLevel(struct IClass* cl, Object* obj, struct cl_Msg2* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (msg->pos == MUIV_List_Active_Off) {
    DoMethod(data->obj_table.str_level_name, MUIM_NoNotifySet, MUIA_String_Contents, "");
    DoMethod(data->obj_table.lv_assets, MUIM_List_Clear);
  }
  else {
    struct Level* level = NULL;
    ULONG pos;

    DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &level);
    DoMethod(data->obj_table.str_level_name, MUIM_NoNotifySet, MUIA_String_Contents, level->name);
    DoMethod(data->obj_table.int_initial_mapPosX, MUIM_NoNotifySet, MUIA_Integer_Value, level->mapPosX);
    DoMethod(data->obj_table.int_initial_mapPosY, MUIM_NoNotifySet, MUIA_Integer_Value, level->mapPosY);
    get(data->obj_table.lv_asset_types, MUIA_List_Active, &pos);

    fillAssetsList(data->obj_table.lv_assets, pos, level);
  }

  return 0;
}
///
///m_SelectAssetType()
STATIC ULONG m_SelectAssetType(struct IClass* cl, Object* obj, struct cl_Msg2* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Level* level;

  DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &level);

  fillAssetsList(data->obj_table.lv_assets, msg->pos, level);

  return 0;
}
///
///m_AddLevel()
STATIC ULONG m_AddLevel(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Level* level = newLevel(GAME_LEVEL); //TODO: get type from msg

  if (level) {
    ULONG pos;
    ULONG entries;

    get(data->obj_table.lv_levels, MUIA_List_Active, &pos);
    get(data->obj_table.lv_levels, MUIA_List_Entries, &entries);

    if (pos == MUIV_List_Active_Off || pos == entries - 1) {
      DoMethod(data->obj_table.lv_levels, MUIM_List_InsertSingle, level, MUIV_List_Insert_Bottom);
      DoMethod(data->obj_table.lv_levels, MUIM_Set, MUIA_List_Active, entries);
    }
    else {
      DoMethod(data->obj_table.lv_levels, MUIM_List_InsertSingle, level, pos + 1);
      DoMethod(data->obj_table.lv_levels, MUIM_Set, MUIA_List_Active, pos + 1);
    }
    DoMethod(data->obj_table.lv_levels, MUIM_List_Redraw, MUIV_List_Redraw_All);
    DoMethod(obj, MUIM_Set, MUIA_Window_ActiveObject, data->obj_table.str_level_name);
  }

  return 0;
}
///
///m_AddAsset()
STATIC ULONG m_AddAsset(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Level* level;
  ULONG asset_type;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);
  DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &level);
  get(data->obj_table.lv_asset_types, MUIA_List_Active, &asset_type);

  if (level && asset_type != MUIV_List_Active_Off) {
    struct MinList* list = NULL;
    STRPTR title_text = "";
    STRPTR pattern = "";

    switch (asset_type) {
      case AT_TILESET:
        list = &level->tilesets;
        title_text = "Select a Tileset";
        pattern = "#?.tls";
      break;
      case AT_TILEMAP:
        list = &level->tilemaps;
        title_text = "Select a Tilemap";
        pattern = "#?.map";
      break;
      case AT_BOBSHEET:
        list = &level->bob_sheets;
        title_text = "Select a BOBSheet";
        pattern = "#?.sht";
      break;
      case AT_SPRITEBANK:
        list = &level->sprite_banks;
        title_text = "Select a SpriteBank";
        pattern = "#?.spr";
      break;
      case AT_MUSICMODULE:
        list = &level->music_modules;
        title_text = "Select a Protracker Module";
        pattern = "#?.MOD|MOD.#?";
      break;
      case AT_SOUNDSAMPLE:
        list = &level->sound_samples;
        title_text = "Select a Sound Sample";
        pattern = "#?.iff";
      break;
      case AT_PALETTE:

      break;
      case AT_GAMEOBJECTBANK:
        list = &level->gameobject_banks;
        title_text = "Select a GameObject bank";
        pattern = "#?.obj";
      break;
      case AT_BITMAP:

      break;
      default:
        return 0;
    }

    if (asset_type == AT_PALETTE) {
      DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);
      data->subscribed_to_palette_selector = TRUE;
      DoMethod(data->palette_selector, MUIM_Set, MUIA_Window_Open, TRUE);
    }
    else if (asset_type == AT_BITMAP) {
      DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);
      DoMethod(data->bitmap_selector, MUIM_Set, MUIA_Window_Open, TRUE);
    }
    else {
      DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

      if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, title_text,
                                        ASLFR_Window, window,
                                        ASLFR_PositiveText, "Select",
                                        ASLFR_DrawersOnly, FALSE,
                                        ASLFR_DoSaveMode, FALSE,
                                        ASLFR_DoPatterns, TRUE,
                                        ASLFR_InitialFile, "",
                                        ASLFR_InitialDrawer, g_Project.data_drawer,
                                        ASLFR_InitialPattern, pattern,
                                        TAG_END) && strlen(g_FileReq->fr_File)) {
        STRPTR pathname = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);
        stripProjectDataDrawer(&pathname);

        if (pathname) {
          struct StringItem* str_item = newStringItem(pathname);

          if (str_item) {
            ULONG entries;
            ULONG pos;

            get(data->obj_table.lv_assets, MUIA_List_Entries, &entries);
            get(data->obj_table.lv_assets, MUIA_List_Active, &pos);

            if (!entries || pos == MUIV_List_Active_Off || pos == entries - 1) {
              AddTail((struct List*)list, (struct Node*)str_item);
              DoMethod(data->obj_table.lv_assets, MUIM_List_InsertSingle, str_item, MUIV_List_Insert_Bottom);
              DoMethod(data->obj_table.lv_assets, MUIM_Set, MUIA_List_Active, entries);
            }
            else {
              struct Node* insert_node;
              DoMethod(data->obj_table.lv_assets, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &insert_node);

              Insert((struct List*)list, (struct Node*)str_item, insert_node);
              DoMethod(data->obj_table.lv_assets, MUIM_List_InsertSingle, str_item, pos + 1);
              DoMethod(data->obj_table.lv_assets, MUIM_Set, MUIA_List_Active, pos + 1);
            }
          }

          freeString(pathname);
        }
      }
      DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);
    }
  }

  return 0;
}
///
///m_RemoveAsset()
STATIC ULONG m_RemoveAsset(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct StringItem* str_item = NULL;

  DoMethod(data->obj_table.lv_assets, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &str_item);

  if (str_item) {
    DoMethod(data->obj_table.lv_assets, MUIM_List_Remove, MUIV_List_Remove_Active);
    Remove((struct Node*)str_item);

    freeStringItem(str_item);
  }

  return 0;
}
///
///m_AddPalette()
STATIC ULONG m_AddPalette(struct IClass* cl, Object* obj, struct cl_Msg3* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Level* level;
  struct MinList* list;
  struct StringItem* str_item = NULL;

  if (data->subscribed_to_palette_selector) {

    DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &level);
    list = &level->palettes;

    str_item = newStringItem(msg->palette_item->name);
    if (str_item) {
      ULONG entries;
      ULONG pos;

      get(data->obj_table.lv_assets, MUIA_List_Entries, &entries);
      get(data->obj_table.lv_assets, MUIA_List_Active, &pos);

      if (!entries || pos == MUIV_List_Active_Off || pos == entries - 1) {
        AddTail((struct List*)list, (struct Node*)str_item);
        DoMethod(data->obj_table.lv_assets, MUIM_List_InsertSingle, str_item, MUIV_List_Insert_Bottom);
        DoMethod(data->obj_table.lv_assets, MUIM_Set, MUIA_List_Active, entries);
      }
      else {
        struct Node* insert_node;
        DoMethod(data->obj_table.lv_assets, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &insert_node);

        Insert((struct List*)list, (struct Node*)str_item, insert_node);
        DoMethod(data->obj_table.lv_assets, MUIM_List_InsertSingle, str_item, pos + 1);
        DoMethod(data->obj_table.lv_assets, MUIM_Set, MUIA_List_Active, pos + 1);
      }
    }

    data->subscribed_to_palette_selector = FALSE;
  }

  return 0;
}
///
///m_AddBitmap()
STATIC ULONG m_AddBitmap(struct IClass* cl, Object* obj, struct cl_Msg5* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Level* level;
  struct MinList* list;
  struct StringItem* str_item = NULL;

  DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &level);
  list = &level->bitmaps;

  str_item = newStringItem(msg->bitmap_string);
  if (str_item) {
    ULONG entries;
    ULONG pos;

    get(data->obj_table.lv_assets, MUIA_List_Entries, &entries);
    get(data->obj_table.lv_assets, MUIA_List_Active, &pos);

    if (!entries || pos == MUIV_List_Active_Off || pos == entries - 1) {
      AddTail((struct List*)list, (struct Node*)str_item);
      DoMethod(data->obj_table.lv_assets, MUIM_List_InsertSingle, str_item, MUIV_List_Insert_Bottom);
      DoMethod(data->obj_table.lv_assets, MUIM_Set, MUIA_List_Active, entries);
    }
    else {
      struct Node* insert_node;
      DoMethod(data->obj_table.lv_assets, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &insert_node);

      Insert((struct List*)list, (struct Node*)str_item, insert_node);
      DoMethod(data->obj_table.lv_assets, MUIM_List_InsertSingle, str_item, pos + 1);
      DoMethod(data->obj_table.lv_assets, MUIM_Set, MUIA_List_Active, pos + 1);
    }
  }

  return 0;
}
///

///readTextAttrFromFile(fh, &textAttr)
/******************************************************************************
 * Reads a whole textattr definition from a c file (from the current pos) and *
 * creates a newly allocated textAttr from it. Returns TRUE if there are more *
 * textattr definitions in the array.                                         *
 ******************************************************************************/
BOOL readTextAttrFromFile(BPTR fh, struct TextAttr** textAttr)
{
  struct TextAttr text_attr;
  STRPTR font_size_str = NULL;
  STRPTR style_str = NULL;
  STRPTR flags_str = NULL;
  LONG font_size = 0;
  UBYTE style = FS_NORMAL;
  BOOL retval = FALSE;

  if (locateArrayStart(fh)) {
    readString(fh, &text_attr.ta_Name);

    readString(fh, &font_size_str);
    aToi(font_size_str, &font_size);
    freeString(font_size_str);
    text_attr.ta_YSize = font_size;

    readString(fh, &style_str);
    if (searchString(style_str, "FSF_UNDERLINED", NULL)) {
      style |= FSF_UNDERLINED;
    }
    if (searchString(style_str, "FSF_BOLD", NULL)) {
      style |= FSF_BOLD;
    }
    if (searchString(style_str, "FSF_ITALIC", NULL)) {
      style |= FSF_ITALIC;
    }
    freeString(style_str);
    text_attr.ta_Style = style;

    readString(fh, &flags_str);
    freeString(flags_str);

    retval = readString(fh, &flags_str);
    freeString(flags_str);

    *textAttr = newTextAttr(&text_attr);
  }
  else *textAttr = NULL;

  return retval;
}
///
///readLevelDataFromFile(fh, &textAttr)
/******************************************************************************
 * Reads a whole struct LevelData (declared in engine code) definition from a *
 * c file (from the current pos) and fills the strings read in a newly        *
 * allocated struct LevelDataItem. Returns TRUE if there are more definitions *
 * in the array.                                                              *
 ******************************************************************************/
BOOL readLevelDataFromFile(BPTR fh, struct LevelDataItem** ld_item)
{
  BOOL retval = FALSE;

  if (locateArrayStart(fh)) {
    *ld_item = (struct LevelDataItem*)AllocPooled(g_MemoryPool, sizeof(struct LevelDataItem));
    if (*ld_item) {
      ULONG i;
      STRPTR string;

      for (i = 0; i < LEVEL_DATA_STRINGS; i++) {
        readString(fh, &(*ld_item)->string[i]);
      }

      retval = readString(fh, &string);
      freeString(string);
    }
  }
  else *ld_item = NULL;

  return retval;
}
///

///m_LoadAssets(filename)
/******************************************************************************
 * Parses the header file that contains asset arrays and fills them into the  *
 * GUI. File name has to be given as the first method argument.               *
 * This method is public.                                                     *
 ******************************************************************************/
STATIC ULONG m_LoadAssets(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (msg->name) {
    BPTR fh = Open(msg->name, MODE_OLDFILE);

    if (fh) {
      // RESET THE GAME DATA
      m_Reset(cl, obj, (Msg)msg);

      // READ AMIGA FONTS
      if (locateStrInFile(fh, "textAttrs[")) {
        if (locateArrayStart(fh)) {
          struct TextAttr* textAttr;
          BOOL more_elements = FALSE;
          do {
            more_elements = readTextAttrFromFile(fh, &textAttr);
            if (textAttr)
              DoMethod(data->obj_table.lv_fonts, MUIM_List_InsertSingle, textAttr, MUIV_List_Insert_Bottom);
          } while(more_elements);
        }
      }

      // READ GAMEFONTS
      Seek(fh, 0, OFFSET_BEGINNING);
      if (locateStrInFile(fh, "gameFontFiles[")) {
        if (locateArrayStart(fh)) {
          STRPTR string;
          BOOL more_elements = FALSE;
          do {
            more_elements = readString(fh, &string);
            if (string) {
              DoMethod(data->obj_table.lv_gamefonts, MUIM_List_InsertSingle, string, MUIV_List_Insert_Bottom);
            }
          } while(more_elements);
        }
      }

      // READ LEVELS
      Seek(fh, 0, OFFSET_BEGINNING);
      if (locateStrInFile(fh, "levelData[")) {
        if (locateArrayStart(fh)) {
          struct MinList level_data;
          struct LevelDataItem* ld_item, *ld_next;
          ULONG i = 0;
          BOOL more_elements;

          NewList((struct List*)&level_data);

          do {
            more_elements = readLevelDataFromFile(fh, &ld_item);
            if (ld_item) {
              AddTail((struct List*)&level_data, (struct Node*)ld_item);
            }
          } while(more_elements);

          for (ld_item = (struct LevelDataItem*)level_data.mlh_Head; (ld_next = (struct LevelDataItem*)ld_item->node.mln_Succ); ld_item = ld_next) {
            struct Level* level = (struct Level*)AllocPooled(g_MemoryPool, sizeof(struct Level));
            if (level) {
              struct MinList* list = NULL;

              // These seven are single values (not lists of multiple assets)
              level->name = makeString(ld_item->string[LDO_NAME]);
              level->blit_func = makeString(ld_item->string[LDO_BLTBOBFUNC]);
              level->unblit_func = makeString(ld_item->string[LDO_UNBLTBOBFUNC]);
              level->width_define = makeString(ld_item->string[LDO_SCREEN_WIDTH]);
              level->height_define = makeString(ld_item->string[LDO_SCREEN_HEIGHT]);
              aToi(ld_item->string[LDO_INITIAL_MAPPOSX], &level->mapPosX);
              aToi(ld_item->string[LDO_INITIAL_MAPPOSY], &level->mapPosY);

              // Rest are lists of multiple assets
              for (i = 1, list = &level->tilesets; i <= NUM_ASSETS; i++, list++) {
                NewList((struct List*)list);
                if (strcmp(ld_item->string[i], "NULL")) {
                  Seek(fh, 0, OFFSET_BEGINNING);
                  if (locateStrInFile(fh, ld_item->string[i])) {
                    if (locateArrayStart(fh)) {
                      STRPTR string = NULL;

                      do {
                        more_elements = readString(fh, &string);
                        if (string) {
                          if (strcmp(string, "NULL")) {
                            struct StringItem* str_item = newStringItem(string);
                            if (str_item) AddTail((struct List*)list, (struct Node*)str_item);
                          }
                          freeString(string); string = NULL;
                        }
                      } while(more_elements);
                    }
                  }
                }
              }

              DoMethod(data->obj_table.lv_levels, MUIM_List_InsertSingle, level, MUIV_List_Insert_Bottom);
            }

            for (i = 0; i < LEVEL_DATA_STRINGS; i++) {
              freeString(ld_item->string[i]);
            }
            FreePooled(g_MemoryPool, ld_item, sizeof(struct LevelDataItem));
          }
        }
      }

      Close(fh);

      data->edited = FALSE;
    }
  }

  return 0;
}
///
///m_SaveAssets(filename)
/******************************************************************************
 * A public method that saves all the asset files on the lists to a c header  *
 * file. File name has to be given as the first method argument.              *
 ******************************************************************************/
STATIC ULONG m_SaveAssets(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  UBYTE buf[128];
  BPTR fh = Open(msg->name, MODE_READWRITE);

  if (fh) {
    WriteSaveString(SS_HEADER);
    WriteSaveString(SS_FONTS_HEADER);

    { //WRITE AMIGA FONTs
      ULONG entries;
      ULONG i;
      struct TextAttr* text_attr;

      get(data->obj_table.lv_fonts, MUIA_List_Entries, &entries);

      for (i = 0; i < entries; i++) {
        //Get the TextAttrs to save
        DoMethod(data->obj_table.lv_fonts, MUIM_List_GetEntry, i, &text_attr);
        //Put a comma to separate TextAttrs (also aligns them on a new line)
        if (i != 0) WriteSaveString(SS_FONTS_ALIGNER);

        //Write a complete struct TextAttr element in c syntax
        sprintf(buf, "{\"%s\", %lu, ", text_attr->ta_Name, (ULONG)text_attr->ta_YSize);
        WriteString(buf);
        if (!text_attr->ta_Style) Write(fh, "FS_NORMAL, ", 9);
        else if (text_attr->ta_Style & FSF_UNDERLINED) {
          Write(fh, "FSF_UNDERLINED", 14);
          if (text_attr->ta_Style & FSF_BOLD) Write(fh, " | FSF_BOLD", 11);
          if (text_attr->ta_Style & FSF_ITALIC) Write(fh, " | FSF_ITALIC", 13);
        }
        else if (text_attr->ta_Style & FSF_BOLD) {
          Write(fh, "FSF_BOLD", 8);
          if (text_attr->ta_Style & FSF_ITALIC) Write(fh, " | FSF_ITALIC", 13);
        }
        else if (text_attr->ta_Style & FSF_ITALIC) Write(fh, "FSF_ITALIC", 10);

        Write(fh, ", FPF_DESIGNED}", 15);
      }
    }

    WriteSaveString(SS_GAMEFONTS_HEADER);
    { //WRITE GAMEFONTs
      ULONG entries;
      ULONG i;
      STRPTR string;

      get(data->obj_table.lv_gamefonts, MUIA_List_Entries, &entries);

      for (i = 0; i < entries; i++) {
        //Get the string item to save
        DoMethod(data->obj_table.lv_gamefonts, MUIM_List_GetEntry, i, &string);
        //Put a comma to separate Gamefonts (also aligns them on a new line)
        if (i != 0) WriteSaveString(SS_GAMEFONTS_ALIGNER);
        //Write the string in c type strings form
        Write(fh, "\"", 1);
        WriteString(string);
        Write(fh, "\"", 1);
      }
    }
    WriteSaveString(SS_FONTS_FOOTER);

    WriteSaveString(SS_LEVEL_HEADER);
    { //WRITE ASSETS FOR EACH LEVEL
      ULONG levels;
      ULONG i, j;
      struct MinList* list;
      struct StringItem* str_item;
      struct Level* level;

      get(data->obj_table.lv_levels, MUIA_List_Entries, &levels);

      // Write asset lists for each level
      for (i = 0; i < levels; i++) {
        DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, i, &level);

        sprintf(buf, "// LEVEL %lu (%s)\n", i, level->name);
        WriteString(buf);
        for (j = 0, list = &level->tilesets; j < NUM_ASSETS; j++, list++) {
          if (list->mlh_Head->mln_Succ) {
            if (j == AT_PALETTE) {
              sprintf(buf, "STATIC UBYTE* %s_%lu[]%s= {", asset_strings[j], i, asset_aligners[j]);
              WriteString(buf);
              for (str_item = (struct StringItem*)list->mlh_Head; str_item->node.mln_Succ; str_item = (struct StringItem*)str_item->node.mln_Succ) {
                WriteString(str_item->string);
                Write(fh, ", ", 2);
              }
            }
            else if (j == AT_GAMEOBJECTBANK) {
              sprintf(buf, "STATIC STRPTR %s_%lu[]%s= {", asset_strings[j], i, asset_aligners[j]);
              WriteString(buf);
              for (str_item = (struct StringItem*)list->mlh_Head; str_item->node.mln_Succ; str_item = (struct StringItem*)str_item->node.mln_Succ) {
                if (strncmp(str_item->string, "(STRPTR)", 8)) {
                  Write(fh, "\"", 1);
                  WriteString(str_item->string);
                  Write(fh, "\", ", 3);
                }
                else {
                  WriteString(str_item->string);
                  Write(fh, ", ", 2);
                }
              }
            }
            else {
              sprintf(buf, "STATIC STRPTR %s_%lu[]%s= {", asset_strings[j], i, asset_aligners[j]);
              WriteString(buf);
              for (str_item = (struct StringItem*)list->mlh_Head; str_item->node.mln_Succ; str_item = (struct StringItem*)str_item->node.mln_Succ) {
                Write(fh, "\"", 1);
                WriteString(str_item->string);
                Write(fh, "\", ", 3);
              }
            }
            Write(fh, "NULL};\n", 7);
          }
        }
      }

      // Write the struct LevelData
      WriteSaveString(SS_LEVEL_DATA);
      for (i = 0; i < levels; i++) {
        DoMethod(data->obj_table.lv_levels, MUIM_List_GetEntry, i, &level);

        if (i != 0) WriteSaveString(SS_LEVEL_ALIGNER2);

        Write(fh, "{\"", 2);
        WriteString(level->name);
        Write(fh, "\",\n", 3);
        WriteSaveString(SS_LEVEL_ALIGNER);

        for (j = 0, list = &level->tilesets; j < NUM_ASSETS; j++, list++) {
          if (list->mlh_Head->mln_Succ) {
            sprintf(buf, "%s_%lu,\n", asset_strings[j], i);
            WriteString(buf);
          }
          else Write(fh, "NULL,\n", 6);
          WriteSaveString(SS_LEVEL_ALIGNER);
        }
        WriteString(level->blit_func); Write(fh, ",\n", 2);
        WriteSaveString(SS_LEVEL_ALIGNER);
        WriteString(level->unblit_func); Write(fh, ",\n", 2);
        WriteSaveString(SS_LEVEL_ALIGNER);
        WriteString(level->width_define); Write(fh, ",\n", 2);
        WriteSaveString(SS_LEVEL_ALIGNER);
        WriteString(level->height_define); Write(fh, ",\n", 2);
        WriteSaveString(SS_LEVEL_ALIGNER);
        sprintf(buf, "%ld", level->mapPosX);
        WriteString(buf); Write(fh, ",\n", 2);
        WriteSaveString(SS_LEVEL_ALIGNER);
        sprintf(buf, "%ld", level->mapPosY);
        WriteString(buf); Write(fh, "\n", 1);
        WriteSaveString(SS_LEVEL_SEPARATOR);
      }
      WriteSaveString(SS_LEVEL_FOOTER);
    }
    WriteSaveString(SS_FOOTER);

    SetFileSize(fh, 0, OFFSET_CURRENT);
    Close(fh);

    data->edited = FALSE;
  }

  return 0;
}
///

///Overridden OM_NEW
STATIC ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;

  struct {
    Object* lv_fonts;
    Object* btn_font_add;
    Object* btn_font_remove;
    Object* btn_font_up;
    Object* btn_font_down;
    Object* lv_gamefonts;
    Object* btn_gamefont_add;
    Object* btn_gamefont_remove;
    Object* btn_gamefont_up;
    Object* btn_gamefont_down;
    Object* lv_levels;
    Object* str_level_name;
    Object* int_initial_mapPosX;
    Object* int_initial_mapPosY;
    Object* btn_level_add;
    Object* btn_level_remove;
    Object* btn_level_up;
    Object* btn_level_down;
    Object* lv_asset_types;
    Object* lv_assets;
    Object* btn_asset_add;
    Object* btn_asset_remove;
    Object* btn_asset_up;
    Object* btn_asset_down;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_Width, MUIV_Window_Width_Visible(56),
    MUIA_Window_Height, MUIV_Window_Height_Visible(60),
    MUIA_Window_ID, MAKE_ID('S','V','G','3'),
    MUIA_Window_Title, "Game Assets",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Weight, 80,
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_FrameTitle, "Amiga Fonts",
          MUIA_Group_Child, (objects.lv_fonts = MUI_NewObject(MUIC_Listview,
            MUIA_Listview_List, MUI_NewObject(MUIC_List,
              MUIA_Frame, MUIV_Frame_InputList,
              MUIA_List_AutoVisible, TRUE,
              MUIA_List_Title, TRUE,
              MUIA_List_Format, "MAW=68,MAW=14,MAW=18",
              MUIA_List_DisplayHook, &TextAttr_disphook,
              MUIA_List_DestructHook, &TextAttr_destructhook,
            TAG_END),
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (objects.btn_font_add = MUI_NewButton("Add", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_font_remove = MUI_NewButton("Remove", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_font_up = MUI_NewButton("Up", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_font_down = MUI_NewButton("Down", NULL, NULL)),
          TAG_END),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_FrameTitle, "GameFonts",
          MUIA_Group_Child, (objects.lv_gamefonts = MUI_NewObject(MUIC_Listview,
            MUIA_Listview_List, MUI_NewObject(MUIC_List,
              MUIA_Frame, MUIV_Frame_InputList,
              MUIA_List_AutoVisible, TRUE,
              MUIA_List_DestructHook, &Gamefont_destructhook,
            TAG_END),
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (objects.btn_gamefont_add = MUI_NewButton("Add", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_gamefont_remove = MUI_NewButton("Remove", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_gamefont_up = MUI_NewButton("Up", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_gamefont_down = MUI_NewButton("Down", NULL, NULL)),
          TAG_END),
        TAG_END),
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Weight, 100,
        MUIA_Frame, MUIV_Frame_Group,
        MUIA_FrameTitle, "Level Data",
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_FrameTitle, "Levels",
          MUIA_Group_Child, (objects.lv_levels = MUI_NewObject(MUIC_Listview,
            MUIA_Listview_List, MUI_NewObject(MUIC_List,
              MUIA_Frame, MUIV_Frame_InputList,
              MUIA_List_AutoVisible, TRUE,
              MUIA_List_Title, TRUE,
              MUIA_List_Format, "MAW=10,MAW=90",
              MUIA_List_DisplayHook, &level_disphook,
              MUIA_List_DestructHook, &level_destructhook,
            TAG_END),
          TAG_END)),
          MUIA_Group_Child, (objects.str_level_name = MUI_NewObject(MUIC_String,
            MUIA_Frame, MUIV_Frame_String,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (objects.btn_level_add = MUI_NewButton("Add", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_level_remove = MUI_NewButton("Remove", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_level_up = MUI_NewButton("Up", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_level_down = MUI_NewButton("Down", NULL, NULL)),
          TAG_END),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, "Assets",
            MUIA_Group_Child, (objects.lv_asset_types = MUI_NewObject(MUIC_Listview,
              MUIA_Listview_List, MUI_NewObject(MUIC_List,
                MUIA_Frame, MUIV_Frame_InputList,
                MUIA_List_AutoVisible, TRUE,
                MUIA_List_SourceArray, asset_types,
              TAG_END),
            TAG_END)),
          TAG_END),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group, MUIA_Group_Columns, 2,
            MUIA_Frame, MUIV_Frame_Group,
            MUIA_FrameTitle, "Initial Map Position",
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "X:", MUIA_HorizWeight, 0, MUIA_ShortHelp, NULL, TAG_END),
            MUIA_Group_Child, (objects.int_initial_mapPosX = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Value, 0,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Input, TRUE,
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Y:", MUIA_HorizWeight, 0, MUIA_ShortHelp, NULL, TAG_END),
            MUIA_Group_Child, (objects.int_initial_mapPosY = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Value, 0,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Input, TRUE,
            TAG_END)),
          TAG_END),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_FrameTitle, "Files",
          MUIA_Group_Child, (objects.lv_assets = MUI_NewObject(MUIC_Listview,
            MUIA_Listview_List, MUI_NewObject(MUIC_List,
              MUIA_Frame, MUIV_Frame_InputList,
              MUIA_List_AutoVisible, TRUE,
              MUIA_List_DisplayHook, &string_disphook,
            TAG_END),
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (objects.btn_asset_add = MUI_NewButton("Add", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_asset_remove = MUI_NewButton("Remove", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_asset_up = MUI_NewButton("Up", NULL, NULL)),
            MUIA_Group_Child, (objects.btn_asset_down = MUI_NewButton("Down", NULL, NULL)),
          TAG_END),
        TAG_END),
      TAG_END),
    TAG_END),
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    //Notifications
    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.btn_font_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_AssetsEditor_AddFont);

    DoMethod(objects.btn_font_remove, MUIM_Notify, MUIA_Pressed, FALSE, objects.lv_fonts, 2,
      MUIM_List_Remove, MUIV_List_Remove_Active);

    DoMethod(objects.btn_font_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_AssetsEditor_MoveLVItem, objects.lv_fonts, MUIV_List_Move_Previous);

    DoMethod(objects.btn_font_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_AssetsEditor_MoveLVItem, objects.lv_fonts, MUIV_List_Move_Next);

    DoMethod(objects.btn_gamefont_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_AssetsEditor_AddGamefont);

    DoMethod(objects.btn_gamefont_remove, MUIM_Notify, MUIA_Pressed, FALSE, objects.lv_gamefonts, 2,
      MUIM_List_Remove, MUIV_List_Remove_Active);

    DoMethod(objects.btn_gamefont_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_AssetsEditor_MoveLVItem, objects.lv_gamefonts, MUIV_List_Move_Previous);

    DoMethod(objects.btn_gamefont_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_AssetsEditor_MoveLVItem, objects.lv_gamefonts, MUIV_List_Move_Next);

    DoMethod(objects.str_level_name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 2,
      MUIM_AssetsEditor_AckLevelName, MUIV_TriggerValue);

    DoMethod(objects.int_initial_mapPosX, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_AssetsEditor_UpdateMapPos, MAP_POS_X, MUIV_TriggerValue);

    DoMethod(objects.int_initial_mapPosY, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_AssetsEditor_UpdateMapPos, MAP_POS_Y, MUIV_TriggerValue);

    DoMethod(objects.lv_levels, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 2,
      MUIM_AssetsEditor_SelectLevel, MUIV_TriggerValue);

    DoMethod(objects.lv_asset_types, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 2,
      MUIM_AssetsEditor_SelectAType, MUIV_TriggerValue);

    DoMethod(objects.btn_level_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_AssetsEditor_AddLevel);

    DoMethod(objects.btn_level_remove, MUIM_Notify, MUIA_Pressed, FALSE, objects.lv_levels, 2,
      MUIM_List_Remove, MUIV_List_Remove_Active);

    DoMethod(objects.btn_level_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_AssetsEditor_MoveLVItem, objects.lv_levels, MUIV_List_Move_Previous);

    DoMethod(objects.btn_level_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_AssetsEditor_MoveLVItem, objects.lv_levels, MUIV_List_Move_Next);

    DoMethod(objects.btn_asset_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_AssetsEditor_AddAsset);

    DoMethod(objects.btn_asset_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_AssetsEditor_RemoveAsset);

    DoMethod(objects.btn_asset_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_AssetsEditor_MoveLVItem, objects.lv_assets, MUIV_List_Move_Previous);

    DoMethod(objects.btn_asset_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_AssetsEditor_MoveLVItem, objects.lv_assets, MUIV_List_Move_Next);

    /**************************************************************************/
    //A value change on all these gadgets has to set 'edited' state TRUE
    DoMethod(objects.btn_font_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.btn_font_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
/*
    DoMethod(objects.btn_font_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.btn_font_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
*/
    DoMethod(objects.btn_gamefont_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.btn_gamefont_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
/*
    DoMethod(objects.btn_gamefont_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.btn_gamefont_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
*/
    DoMethod(objects.btn_level_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.btn_level_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
/*
    DoMethod(objects.btn_level_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.btn_level_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
*/
    DoMethod(objects.btn_asset_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.btn_asset_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
/*
    DoMethod(objects.btn_asset_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.btn_asset_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
*/
    DoMethod(objects.str_level_name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.int_initial_mapPosX, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);

    DoMethod(objects.int_initial_mapPosY, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_Set, MUIA_AssetsEditor_Edited, TRUE);
    /**************************************************************************/

    data->palette_selector = NULL;
    data->bitmap_selector = NULL;
    data->subscribed_to_palette_selector = FALSE;
    data->edited = FALSE;

    data->obj_table.lv_fonts = objects.lv_fonts;
    data->obj_table.btn_font_add = objects.btn_font_add;
    data->obj_table.btn_font_remove = objects.btn_font_remove;
    data->obj_table.lv_gamefonts = objects.lv_gamefonts;
    data->obj_table.btn_gamefont_add = objects.btn_gamefont_add;
    data->obj_table.btn_gamefont_remove = objects.btn_gamefont_remove;
    data->obj_table.lv_levels = objects.lv_levels;
    data->obj_table.str_level_name = objects.str_level_name;
    data->obj_table.int_initial_mapPosX = objects.int_initial_mapPosX;
    data->obj_table.int_initial_mapPosY = objects.int_initial_mapPosY;
    data->obj_table.btn_level_add = objects.btn_level_add;
    data->obj_table.btn_level_remove = objects.btn_level_remove;
    data->obj_table.lv_asset_types = objects.lv_asset_types;
    data->obj_table.lv_assets = objects.lv_assets;
    data->obj_table.btn_asset_add = objects.btn_asset_add;
    data->obj_table.btn_asset_remove = objects.btn_asset_remove;

    data->obj_table.btn_font_up = objects.btn_font_up;
    data->obj_table.btn_font_down = objects.btn_font_down;
    data->obj_table.btn_gamefont_up = objects.btn_gamefont_up;
    data->obj_table.btn_gamefont_down = objects.btn_gamefont_down;
    data->obj_table.btn_level_up = objects.btn_level_up;
    data->obj_table.btn_level_down = objects.btn_level_down;
    data->obj_table.btn_asset_up = objects.btn_asset_up;
    data->obj_table.btn_asset_down = objects.btn_asset_down;

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
STATIC ULONG m_Dispose(struct IClass* cl, Object* obj, Msg msg)
{
  //struct cl_Data *data = INST_DATA(cl, obj);

  //<FREE SUBCLASS INITIALIZATIONS HERE>

  return DoSuperMethodA(cl, obj, msg);
}
///
///Overridden OM_SET
//*****************
STATIC ULONG m_Set(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct TagItem *tags, *tag;

  for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
  {
    switch (tag->ti_Tag)
    {
      case MUIA_AssetsEditor_Edited:
        data->edited = tag->ti_Data;
      break;
      case MUIA_AssetsEditor_PaletteSelector:
        data->palette_selector = (Object*)tag->ti_Data;
        DoMethod(data->palette_selector, MUIM_Notify, MUIA_Window_Open, FALSE, obj, 3,
          MUIM_Set, MUIA_Window_Sleep, FALSE);
        DoMethod(data->palette_selector, MUIM_Notify, MUIA_PaletteSelector_Selected, MUIV_EveryTime, obj, 2,
          MUIM_AssetsEditor_AddPalette, MUIV_TriggerValue);
      break;
      case MUIA_AssetsEditor_BitmapSelector:
        data->bitmap_selector = (Object*)tag->ti_Data;
        DoMethod(data->bitmap_selector, MUIM_Notify, MUIA_Window_Open, FALSE, obj, 3,
          MUIM_Set, MUIA_Window_Sleep, FALSE);
        DoMethod(data->bitmap_selector, MUIM_Notify, MUIA_BitmapSelector_Selected, MUIV_EveryTime, obj, 2,
          MUIM_AssetsEditor_AddBitmap, MUIV_TriggerValue);
      break;
    }
  }

  return (DoSuperMethodA(cl, obj, (Msg) msg));
}
///
///Overridden OM_GET
//*****************
STATIC ULONG m_Get(struct IClass* cl, Object* obj, struct opGet* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG value;

  switch (msg->opg_AttrID)
  {
    case MUIA_AssetsEditor_NumLevels:
      get(data->obj_table.lv_levels, MUIA_List_Entries, &value);
      *msg->opg_Storage = value;
    return TRUE;
    case MUIA_AssetsEditor_NumTextfonts:
      get(data->obj_table.lv_fonts, MUIA_List_Entries, &value);
      *msg->opg_Storage = value;
    return TRUE;
    case MUIA_AssetsEditor_NumGamefonts:
      get(data->obj_table.lv_gamefonts, MUIA_List_Entries, &value);
      *msg->opg_Storage = value;
    return TRUE;
    case MUIA_AssetsEditor_Edited:
      *msg->opg_Storage = data->edited;
    return TRUE;
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
    case MUIM_AssetsEditor_AddFont:
      return m_AddFont(cl, obj, msg);
    case MUIM_AssetsEditor_AddGamefont:
      return m_AddGamefont(cl, obj, msg);
    case MUIM_AssetsEditor_AckLevelName:
      return m_AckLevelName(cl, obj, (struct cl_Msg*)msg);
    case MUIM_AssetsEditor_SelectLevel:
      return m_SelectLevel(cl, obj, (struct cl_Msg2*)msg);
    case MUIM_AssetsEditor_SelectAType:
      return m_SelectAssetType(cl, obj, (struct cl_Msg2*)msg);
    case MUIM_AssetsEditor_AddLevel:
      return m_AddLevel(cl, obj, msg);
    case MUIM_AssetsEditor_AddAsset:
      return m_AddAsset(cl, obj, msg);
    case MUIM_AssetsEditor_RemoveAsset:
      return m_RemoveAsset(cl, obj, msg);
    case MUIM_AssetsEditor_Load:
      return m_LoadAssets(cl, obj, (struct cl_Msg*)msg);
    case MUIM_AssetsEditor_Save:
      return m_SaveAssets(cl, obj, (struct cl_Msg*)msg);
    case MUIM_AssetsEditor_MoveLVItem:
      return m_MoveListviewItem(cl, obj, (struct cl_MoveMsg*)msg);
    case MUIM_AssetsEditor_AddPalette:
      return m_AddPalette(cl, obj, (struct cl_Msg3*)msg);
    case MUIM_AssetsEditor_AddBitmap:
      return m_AddBitmap(cl, obj, (struct cl_Msg5*)msg);
    case MUIM_AssetsEditor_UpdateMapPos:
      return m_UpdateMapPos(cl, obj, (struct cl_Msg4*)msg);

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_AssetsEditor(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
