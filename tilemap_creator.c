/******************************************************************************
 * TilemapCreator                                                             *
 ******************************************************************************/
///Defines
//private methods
#define MUIM_TilemapCreator_CheckValidity     0x80430411
#define MUIM_TilemapCreator_SourceAcknowledge 0x80430412
#define MUIM_TilemapCreator_Create            0x80430413
///
///Includes
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
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
#include "tilemap_creator.h"
///
///Structs
struct cl_ObjTable
{
  Object* source;
  Object* add_zero;
  Object* create;
  Object* cancel;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  BOOL source_valid;
};

struct cl_Msg
{
  ULONG MethodID;
  STRPTR source;
};
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
  STRPTR name;
  STRPTR source;
  STRPTR add_zero; // NOTE: rename?
  STRPTR create;
  STRPTR cancel;
}help_string = {
  "Filename for the tilemap to create.",
  "The JSON tilemap file from Tiled.",
  "Select this if you've created the tileset for this map with ADDZERO.",
  "Converts and saves the engine compatible tilemap.",
  "Cancel and quit the creator."
};
///

//Utility Functions
///isValidTilemapFile(file)
BOOL isValidTilemapFile(STRPTR file)
{
  BOOL result = FALSE;
  BPTR fh = Open(file, MODE_OLDFILE);
  if (fh) {
    result = locateStrInFile(fh, "\"data\":[");

    Close(fh);
  }

  return result;
}
///

//Private Methods
///m_CheckValidity()
STATIC ULONG m_CheckValidity(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  DoMethod(data->obj_table.create, MUIM_Set, MUIA_Disabled, !data->source_valid);

  return 0;
}
///
///m_SourceAcknowledge()
STATIC ULONG m_SourceAcknowledge(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (isValidTilemapFile(msg->source)) {
    data->source_valid = TRUE;
  }
  else {
    data->source_valid = FALSE;
  }

  DoMethod(obj, MUIM_TilemapCreator_CheckValidity);

  return 0;
}
///
///m_Create()
STATIC ULONG m_Create(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct ReturnInfo rtrn;
  UBYTE command[MAX_CMD_LENGTH];
  STRPTR source;
  ULONG add_zero;
  STRPTR output_dir = NULL;
  STRPTR output_path = NULL;
  STRPTR error_string = NULL;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);
  GetAttr(MUIA_String_Contents, data->obj_table.source, (ULONG*)&source);
  GetAttr(MUIA_Selected, data->obj_table.add_zero, &add_zero);

  if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Save tilemap",
                                    ASLFR_Window, window,
                                    ASLFR_SleepWindow, TRUE,
                                    ASLFR_PositiveText, "Create",
                                    ASLFR_DrawersOnly, TRUE,
                                    ASLFR_DoSaveMode, TRUE,
                                    ASLFR_DoPatterns, FALSE,
                                    g_Project.data_drawer ? ASLFR_InitialDrawer : TAG_IGNORE, g_Project.data_drawer,
                                    TAG_END)) {

    output_dir = stripExtension(FilePart(source));
    output_path = makePath(g_FileReq->fr_Drawer, output_dir, NULL);
    if (output_path) {
      if (Exists(output_path)) {
        if (!MUI_Request(App, obj, NULL, "Tilemap Creator", "*_Overwrite|_Cancel", "Output directory already exists!")) {
          goto error;
        }
      }

      DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

      /************************************************************************
       ConvertMap tool can parse the custom members on the GameObject and
       GameObjectBank structs and create the compatible gameobject files. For
       this to happen, the project with the custom headers has to be opened.
       Otherwise it will create gameobject files compatible with the vanilla
       headers only.
      *************************************************************************/
      if (g_Project.directory) {
        STRPTR header = makePath(g_Project.directory, "gameobject.h", NULL);
        if (header) {
          sprintf(command, "%s \"%s\" Path=\"%s\" Header=\"%s\" %s",
          g_Tools.convert_map, source, g_FileReq->fr_Drawer, header, add_zero ? "ADDZERO" : "");

          freeString(header);
        }
      }
      else {
        sprintf(command, "%s \"%s\" Path=\"%s\" %s",
        g_Tools.convert_map, source, g_FileReq->fr_Drawer, add_zero ? "ADDZERO" : "");
      }

      if (execute(&rtrn, command))
        error_string = rtrn.string;
      else
        error_string = "Tools/ConvertMap could not be found!";

      DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);
    }
  }

error:
  freeString(output_path);
  freeString(output_dir);

  if (error_string) {
    MUI_Request(App, obj, NULL, "Tilemap Creator", "*_OK", error_string);
  }

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;

  struct {
    Object* source;
    Object* source_string;
    Object* source_pop;
    Object* add_zero;
    Object* create;
    Object* cancel;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','7'),
    MUIA_Window_Title, "Tilemap Creator",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Source:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.source, TAG_END),
        MUIA_Group_Child, (objects.source = NewObject(MUIC_PopASLString->mcc_Class, NULL,
          MUIA_PopASLString_Requester, g_FileReq,
          MUIA_PopASLString_IgnoreContents, TRUE,
          MUIA_ShortHelp, help_string.source,
          ASLFR_TitleText, "Please select js file...",
          ASLFR_PositiveText, "Open",
          ASLFR_DoSaveMode, FALSE,
          ASLFR_DrawersOnly, FALSE,
          ASLFR_DoPatterns, TRUE,
          ASLFR_InitialPattern, "#?.js",
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Add zero:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.add_zero, TAG_END), //NOTE: Rename this?
        MUIA_Group_Child, MUI_NewCheckMark(&objects.add_zero, FALSE, NULL, 0, help_string.add_zero),
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
    data->obj_table.add_zero = objects.add_zero;
    data->obj_table.create = objects.create;
    data->obj_table.cancel = objects.cancel;

    data->source_valid = FALSE;

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.source_string,
                                             objects.source_pop,
                                             objects.add_zero,
                                             objects.create,
                                             objects.cancel,
                                             NULL);

    DoMethod(objects.create, MUIM_Set, MUIA_Disabled, TRUE);

    DoMethod(objects.create, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_TilemapCreator_Create);

    DoMethod(objects.cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.source, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 2,
      MUIM_TilemapCreator_SourceAcknowledge, MUIV_TriggerValue);

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
      case MUIA_TilemapCreator_AssetsDrawer:
        DoMethod(data->obj_table.source, MUIM_Set, ASLFR_InitialDrawer, tag->ti_Data);
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
  struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->opg_AttrID)
  {
    //<SUBCLASS ATTRIBUTES HERE>
    case MUIA_TilemapCreator_AddZeroButton:
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
    case MUIM_TilemapCreator_CheckValidity:
      return m_CheckValidity(cl, obj, (struct cl_Msg*) msg);
    case MUIM_TilemapCreator_SourceAcknowledge:
      return m_SourceAcknowledge(cl, obj, (struct cl_Msg*) msg);
    case MUIM_TilemapCreator_Create:
      return m_Create(cl, obj, (struct cl_Msg*) msg);

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_TilemapCreator(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
