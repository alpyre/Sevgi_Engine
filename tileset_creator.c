/******************************************************************************
 * TilesetCreator                                                             *
 ******************************************************************************/
///Defines
//private methods
#define MUIM_TilesetCreator_CheckValidity     0x8043040C
#define MUIM_TilesetCreator_SourceAcknowledge 0x8043040D
#define MUIM_TilesetCreator_WindowClose       0x8043040E
#define MUIM_TilesetCreator_Reset             0x8043040F
#define MUIM_TilesetCreator_Create            0x80430410
#define MUIM_TilesetCreator_DisplaySource     0x80430411

//private attributes

///
///Includes
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <datatypes/pictureclass.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()
#include <clib/alib_stdio_protos.h>

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "popasl_string.h"
#include "integer_gadget.h"
#include "dtpicdisplay.h"
#include "tileset_creator.h"
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;
extern struct {
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
}g_Tools;

extern Object* App;
extern Object* g_dtPicDisplay;
extern struct MUI_CustomClass *MUIC_PopASLString;
extern struct MUI_CustomClass *MUIC_Integer;
extern struct MUI_CustomClass *MUIC_AckString;

extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;

static struct {
  STRPTR source;
  STRPTR depth;
  STRPTR tilesize;
  STRPTR margin;
  STRPTR spacing;
  STRPTR add_zero;
  STRPTR num_tiles;
  STRPTR create;
  STRPTR cancel;
}help_string = {
  "The ILBM tileset file as source.",
  "The color depth of the ILBM tileset file.",
  "Width and height of each tile.",
  "The inital pixel spacing at the top\nand left side of the ILBM file.",
  "Pixel spacing between each tile image on the ILBM file.",
  "Adds an empty tile at ID=0 of the created tileset.",
  "Number of tiles to extract from the source ILBM.",
  "Create the engine compatible tileset\nfile with the properties above.",
  "Cancel and quit the creator."
};
///
///Structs
struct cl_ObjTable
{
  Object* source;
  Object* source_btn;
  Object* depth;
  Object* tilesize;
  Object* margin;
  Object* spacing;
  Object* add_zero;
  Object* num_tiles;
  Object* create;
  Object* cancel;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  UWORD width;
  UWORD height;
};

struct cl_Msg
{
  ULONG MethodID;
  STRPTR source;
};

struct Sizes {
  UWORD width;
  UWORD height;
  UWORD depth;
};
///

//Utility Functions
///getBitMapSizes(file, sizesPtr)
STATIC BOOL getBitMapSizes(STRPTR file, struct Sizes* sizes)
{
  BPTR fh;
  struct BitMapHeader bmhd;
  BOOL result = FALSE;

  fh = Open(file, MODE_OLDFILE);
  if (fh) {
    if (locateStrInFile(fh, "FORM")) {
      if (locateStrInFile(fh, "ILBM")) {
        if (locateStrInFile(fh, "BMHD")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct BitMapHeader) == Read(fh, &bmhd, sizeof(struct BitMapHeader))) {
            sizes->width = bmhd.bmh_Width;
            sizes->height = bmhd.bmh_Height;
            sizes->depth = bmhd.bmh_Depth;
            result = TRUE;
          }
        }
      }
    }
    Close(fh);
  }

  return result;
}
///

//private methods
///m_CheckValidity()
STATIC ULONG m_CheckValidity(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  ULONG tilesize;
  ULONG margin;
  ULONG spacing;
  LONG num_tiles;
  LONG max_tiles = 0;
  ULONG validity = FALSE;

  GetAttr(MUIA_Integer_Value, data->obj_table.tilesize, &tilesize);
  GetAttr(MUIA_Integer_Value, data->obj_table.margin, &margin);
  GetAttr(MUIA_Integer_Value, data->obj_table.spacing, &spacing);
  GetAttr(MUIA_Integer_Value, data->obj_table.num_tiles, &num_tiles);

  if (data->width && data->height) {
    BOOL valid_sizes = FALSE;

    if (!((data->width - margin) % (tilesize + spacing) | (data->height - margin) % (tilesize + spacing))) {
      valid_sizes = TRUE;
      max_tiles = ((data->width - margin) / (tilesize + spacing)) * ((data->height - margin) / (tilesize + spacing));
    }
    else if (!((data->width - margin + spacing) % (tilesize + spacing) | (data->height - margin + spacing) % (tilesize + spacing))) {
      valid_sizes = TRUE;
      max_tiles = ((data->width - margin + spacing) / (tilesize + spacing)) * ((data->height - margin + spacing) / (tilesize + spacing));
    }

    if (valid_sizes) {
      if (num_tiles) {
        if (num_tiles <= max_tiles) {
          validity = TRUE;
        }
      }
      else {
        DoMethod(data->obj_table.num_tiles, MUIM_NoNotifySet, MUIA_Integer_Value, max_tiles);
        validity = TRUE;
      }
    }
  }

  DoMethod(data->obj_table.create, MUIM_Set, MUIA_Disabled, !validity);

  return 0;
}
///
///m_SourceAcknowledge()
STATIC ULONG m_SourceAcknowledge(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Sizes sizes;

  if (getBitMapSizes(msg->source, &sizes)) {
    data->width = sizes.width;
    data->height = sizes.height;
    DoMethod(data->obj_table.depth, MUIM_Set, MUIA_Integer_Value, sizes.depth);

    DoMethod(obj, MUIM_TilesetCreator_CheckValidity);
  }
  else {
    data->width = 0;
    data->height = 0;
    DoMethod(data->obj_table.depth, MUIM_Set, MUIA_Integer_Value, 0);
  }

  return 0;
}
///
///m_WindowClose()
STATIC ULONG m_WindowClose(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  //struct cl_Data *data = INST_DATA(cl, obj);

  DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
  DoMethod(obj, MUIM_TilesetCreator_Reset);

  return 0;
}
///
///m_Reset()
STATIC ULONG m_Reset(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  DoMethod(data->obj_table.source, MUIM_NoNotifySet, MUIA_String_Contents, "");
  DoMethod(data->obj_table.tilesize, MUIM_NoNotifySet, MUIA_Integer_Value, 16);
  DoMethod(data->obj_table.margin, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.spacing, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.add_zero, MUIM_Set, MUIA_Selected, FALSE);
  DoMethod(data->obj_table.num_tiles, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.create, MUIM_Set, MUIA_Disabled, TRUE);

  return 0;
}
///
///m_Create()
STATIC ULONG m_Create(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct ReturnInfo rtrn;
  UBYTE command[MAX_CMD_LENGTH];
  STRPTR name;
  STRPTR source;
  ULONG tilesize;
  ULONG margin;
  ULONG spacing;
  LONG num_tiles;
  ULONG add_zero;
  ULONG name_len;
  STRPTR filename = NULL;
  STRPTR filepath = NULL;
  STRPTR error_string = NULL;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);
  GetAttr(MUIA_String_Contents, data->obj_table.source, (ULONG*)&source);
  GetAttr(MUIA_Integer_Value, data->obj_table.tilesize, &tilesize);
  GetAttr(MUIA_Integer_Value, data->obj_table.margin, &margin);
  GetAttr(MUIA_Integer_Value, data->obj_table.spacing, &spacing);
  GetAttr(MUIA_Integer_Value, data->obj_table.num_tiles, &num_tiles);
  GetAttr(MUIA_Selected, data->obj_table.add_zero, &add_zero);

  name = stripExtension(FilePart(source));
  if (name) {
    name_len = strlen(name);
    if (name_len >= 4) {
      if (strcmp(&name[name_len - 4], ".tls")) {
        filename = makePath(NULL, name, ".tls");
      }
      else {
        filename = makeString(name);
      }
    }
    else {
      filename = makePath(NULL, name, ".tls");
    }
    freeString(name);

    DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

    if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Save tileset",
                                      ASLFR_Window, window,
                                      ASLFR_PositiveText, "Create",
                                      ASLFR_DrawersOnly, FALSE,
                                      ASLFR_DoSaveMode, TRUE,
                                      ASLFR_DoPatterns, TRUE,
                                      ASLFR_InitialPattern, "#?.tls",
                                      g_Project.data_drawer ? ASLFR_InitialDrawer : TAG_IGNORE, g_Project.data_drawer,
                                      ASLFR_InitialFile, filename,
                                      TAG_END) && strlen(g_FileReq->fr_File)) {

      filepath = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);
      if (filepath) {
        if (Exists(filepath)) {
          if (!MUI_Request(App, obj, NULL, "Tileset Creator", "*_Overwrite|_Cancel", "Tileset file already exists!")) {
            goto error;
          }
        }

        sprintf(command, "%s \"%s\" \"%s\" %ld %lu %lu %lu %s",
          g_Tools.convert_tiles, source, filepath, num_tiles, tilesize, margin, spacing, add_zero ? "ADDZERO" : "");

        if (execute(&rtrn, command)) {
          error_string = rtrn.string;
        }
        else error_string = "Tools/ConvertTiles could not be found!";
      }
    }
  }

error:
  freeString(filename);
  freeString(filepath);

  if (error_string) {
    MUI_Request(App, obj, NULL, "Tileset Creator", "*_OK", error_string);
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);

  return 0;
}
///
///m_DisplaySource()
STATIC ULONG m_DisplaySource(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct Sizes sizes;
  STRPTR picture;

  GetAttr(MUIA_String_Contents, data->obj_table.source, (ULONG*)&picture);
  if (getBitMapSizes(picture, &sizes)) {
    DoMethod(g_dtPicDisplay, MUIM_Set, MUIA_DtPicDisplay_Picture, (ULONG)picture);
  }

  return 0;
}
///

//<DEFINE SUBCLASS METHODS HERE>

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;

  struct {
    Object* source;
    Object* source_string;
    Object* source_pop;
    Object* source_btn;
    Object* depth;
    Object* tilesize;
    Object* margin;
    Object* spacing;
    Object* add_zero;
    Object* num_tiles;
    Object* create;
    Object* cancel;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','6'),
    MUIA_Window_Title, "Tileset Creator",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, (objects.source_btn = MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Source:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.source,
          MUIA_InputMode, MUIV_InputMode_RelVerify,
          MUIA_Frame, MUIV_Frame_Button,
        TAG_END)),
        MUIA_Group_Child, (objects.source = NewObject(MUIC_PopASLString->mcc_Class, NULL,
          MUIA_PopASLString_Requester, g_FileReq,
          MUIA_PopASLString_IgnoreContents, TRUE,
          MUIA_ShortHelp, help_string.source,
          ASLFR_TitleText, "Please select tileset source ilbm...",
          ASLFR_PositiveText, "Open",
          ASLFR_DoSaveMode, FALSE,
          ASLFR_DrawersOnly, FALSE,
          ASLFR_DoPatterns, TRUE,
          ASLFR_InitialPattern, "#?.(iff|ilbm)",
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Depth:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.depth, TAG_END),
        MUIA_Group_Child, (objects.depth = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.depth,
          MUIA_Integer_Input, FALSE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Buttons, FALSE,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Tilesize:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.tilesize, TAG_END),
        MUIA_Group_Child, (objects.tilesize = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.tilesize,
          MUIA_Integer_Input, FALSE,
          MUIA_Integer_Value, 16,
          MUIA_Integer_Incr, 16,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 16,
          MUIA_Integer_Max, 64,
          MUIA_String_MaxLen, 3,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Margin:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.margin, TAG_END),
        MUIA_Group_Child, (objects.margin = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.margin,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 64,
          MUIA_String_MaxLen, 3,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Spacing:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.spacing, TAG_END),
        MUIA_Group_Child, (objects.spacing = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.spacing,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 64,
          MUIA_String_MaxLen, 3,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Add zero:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.add_zero, TAG_END),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.add_zero, FALSE, NULL, 0, "Add an empty tile at ID=0."),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Number of tiles:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.num_tiles, TAG_END),
        MUIA_Group_Child, (objects.num_tiles = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.num_tiles,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, -32768,
          MUIA_Integer_Max, 32767,
          MUIA_String_MaxLen, 6,
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (objects.create = MUI_NewButton("Create", NULL, help_string.create)),
        MUIA_Group_Child, (objects.cancel = MUI_NewButton("Cancel", NULL, help_string.cancel)),
      TAG_END),
    TAG_END),
  TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);
    get(objects.source, MUIA_PopASLString_StringObject, &objects.source_string);
    get(objects.source, MUIA_PopASLString_PopButton, &objects.source_pop);
    data->obj_table.source = objects.source;
    data->obj_table.source_btn = objects.source_btn;
    data->obj_table.depth = objects.depth;
    data->obj_table.tilesize = objects.tilesize;
    data->obj_table.margin = objects.margin;
    data->obj_table.spacing = objects.spacing;
    data->obj_table.add_zero = objects.add_zero;
    data->obj_table.num_tiles = objects.num_tiles;
    data->obj_table.create = objects.create;
    data->obj_table.cancel = objects.cancel;

    data->width = 0;
    data->height = 0;

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.source_string,
                                             objects.source_pop,
                                             MUI_GetChild(objects.tilesize, 1),
                                             MUI_GetChild(objects.margin, 1),
                                             MUI_GetChild(objects.spacing, 1),
                                             objects.add_zero,
                                             MUI_GetChild(objects.num_tiles, 1),
                                             objects.create,
                                             objects.cancel,
                                             NULL);

    DoMethod(objects.create, MUIM_Set, MUIA_Disabled, TRUE);

    DoMethod(objects.cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.create, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_TilesetCreator_Create);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.source, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 2,
      MUIM_TilesetCreator_SourceAcknowledge, MUIV_TriggerValue);

    DoMethod(objects.tilesize, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_TilesetCreator_CheckValidity);

    DoMethod(objects.margin, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_TilesetCreator_CheckValidity);

    DoMethod(objects.spacing, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_TilesetCreator_CheckValidity);

    DoMethod(objects.num_tiles, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_TilesetCreator_CheckValidity);

    DoMethod(data->obj_table.source_btn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_TilesetCreator_DisplaySource);

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
static ULONG m_Set(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct TagItem *tags, *tag;

  for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
  {
    switch (tag->ti_Tag)
    {
      case MUIA_TilesetCreator_AssetsDrawer:
        DoMethod(data->obj_table.source, MUIM_Set, ASLFR_InitialDrawer, tag->ti_Data);
      break;
    }
  }

  return (DoSuperMethodA(cl, obj, (Msg) msg));
}
///
///Overridden OM_GET
static ULONG m_Get(struct IClass* cl, Object* obj, struct opGet* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->opg_AttrID)
  {
    case MUIA_TilesetCreator_AddZeroButton:
      *msg->opg_Storage = (ULONG)data->obj_table.add_zero;
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
    case MUIM_TilesetCreator_CheckValidity:
      return m_CheckValidity(cl, obj, (struct cl_Msg*) msg);
    case MUIM_TilesetCreator_SourceAcknowledge:
      return m_SourceAcknowledge(cl, obj, (struct cl_Msg*) msg);
    case MUIM_TilesetCreator_WindowClose:
      return m_WindowClose(cl, obj, (struct cl_Msg*) msg);
    case MUIM_TilesetCreator_Reset:
      return m_Reset(cl, obj, (struct cl_Msg*) msg);
    case MUIM_TilesetCreator_Create:
      return m_Create(cl, obj, (struct cl_Msg*) msg);
    case MUIM_TilesetCreator_DisplaySource:
      return m_DisplaySource(cl, obj, (struct cl_Msg*) msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_TilesetCreator(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
