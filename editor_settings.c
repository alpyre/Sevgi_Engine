/******************************************************************************
 * EditorSettings                                                             *
 * WARNING: And object of this class has to be a singleton because of the way *
 * icon tooltypes are read!                                                   *
 ******************************************************************************/
///Defines
#define MUIA_EditorSettings_Edited 0x80430A00 //(.S.)

#define MUIM_EditorSettings_Close  0x80430A00

#define CLOSE_BY_GADGET 0
#define CLOSE_BY_SAVE   1
#define CLOSE_BY_USE    2
#define CLOSE_BY_CANCEL 3
///
///Includes
#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/asl.h>
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "tooltypes.h"
#include "popasl_string.h"
#include "editor_settings.h"
///
///Structs
struct cl_ObjTable
{
  Object* IDE;
  Object* cyc_compiler;
  Object* output;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  struct {
    STRPTR IDE;
    ULONG compiler_entry;
    STRPTR output;
  }old_values;
  BOOL edited;
};

struct close_Msg
{
  ULONG MethodID;
  ULONG close_by;
};
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;
extern STRPTR g_Program_Directory;
extern STRPTR g_Program_Executable;
extern BOOL g_First_Run;

extern struct MUI_CustomClass *MUIC_PopASLString;
extern Object *App;

enum {
  TTPK_IDE,
  TTPK_COMPILER,
  TTPK_OUTPUT
};

static struct ToolTypePref ttprefs[] = {
  {TTP_STRING, "ide", {0}},
  {TTP_STRING, "compiler", {0}},
  {TTP_STRING, "output", {0}},
  {TTP_END, NULL, {0}}
};

static struct {
  STRPTR IDE;
  STRPTR compiler;
  STRPTR output;
  STRPTR save;
  STRPTR use;
  STRPTR cancel;
}help_string = {
  "Full path of the IDE software to open game code in.",
  "Compiler choice for the project to be compiled with.",
  "Output console/file to display output.",
  "Save settings and close this window.",
  "Use the current settings on this session without saving them.",
  "Discard the changes made and close this window."
};

static STRPTR compiler_entries[] = {
  "gcc",
  "SAS/C"
};
///

//private functions
///getSettings()
BOOL getSettings() {
  BOOL retval = FALSE;

  STRPTR prog_path = makePath(g_Program_Directory, g_Program_Executable, NULL);
  if (prog_path) {
    retval = getTooltypes(prog_path, ttprefs);
    freeString(prog_path);
  }

  return retval;
}
///
///putSettings()
VOID putSettings() {
  STRPTR prog_path = makePath(g_Program_Directory, g_Program_Executable, NULL);
  if (prog_path) {
    setTooltypes(prog_path, ttprefs);
    freeString(prog_path);
  }
}
///

///storeOldValues(data)
VOID storeOldValues(struct cl_Data* data)
{
  STRPTR IDE = NULL;
  ULONG compiler_entry = 0;
  STRPTR output = NULL;

  get(data->obj_table.IDE, MUIA_String_Contents, &IDE);
  get(data->obj_table.cyc_compiler, MUIA_Cycle_Active, &compiler_entry);
  get(data->obj_table.output, MUIA_String_Contents, &output);

  data->old_values.IDE = makeString(IDE);
  data->old_values.compiler_entry = compiler_entry;
  data->old_values.output = makeString(output);
  data->edited = FALSE;
}
///
///restoreOldValues(data)
VOID restoreOldValues(struct cl_Data* data)
{
  DoMethod(data->obj_table.IDE, MUIM_Set, MUIA_String_Contents, data->old_values.IDE);
  DoMethod(data->obj_table.cyc_compiler, MUIM_Set, MUIA_Cycle_Active, data->old_values.compiler_entry);
  DoMethod(data->obj_table.output, MUIM_Set, MUIA_String_Contents, data->old_values.output);
}
///
///freeOldValues(data)
VOID freeOldValues(struct cl_Data* data)
{
  freeString(data->old_values.IDE); data->old_values.IDE = NULL;
  freeString(data->old_values.output); data->old_values.output = NULL;
}
///

//<DEFINE SUBCLASS METHODS HERE>
///m_Close()
static ULONG m_Close(struct IClass* cl, Object* obj, struct close_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->close_by) {
    case CLOSE_BY_GADGET:
      if (data->edited) {
        if (MUI_Request(App, obj, NULL, "Warning!", "*_Discard|_Cancel", "You have made some changes in the settings.\nYou will lose them if you close this window.\n\nHow do you want to continue?")) {
          restoreOldValues(data);
          DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
        }
      }
      else {
        DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
      }
    break;
    case CLOSE_BY_SAVE:
    {
      ULONG compiler_entry;
      get(data->obj_table.IDE, MUIA_String_Contents, &ttprefs[TTPK_IDE].data.string);
      get(data->obj_table.output, MUIA_String_Contents, &ttprefs[TTPK_OUTPUT].data.string);
      get(data->obj_table.cyc_compiler, MUIA_Cycle_Active, &compiler_entry);
      ttprefs[TTPK_COMPILER].data.string = compiler_entries[compiler_entry];
      putSettings();
    }
    case CLOSE_BY_USE:
      DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
    break;
    case CLOSE_BY_CANCEL:
      if (data->edited) {
        restoreOldValues(data);
      }
      DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
    break;
  }

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct {
    Object* IDE;
    Object* IDE_str;
    Object* IDE_pop;
    Object* cyc_compiler;
    Object* output;
    Object* output_str;
    Object* output_pop;
    Object* btn_save;
    Object* btn_use;
    Object* btn_cancel;
  }objects;

  BOOL settings_read = getSettings();
  ULONG compiler_entry = 0;

  if (!strcmp(ttprefs[TTPK_COMPILER].data.string, "__FIRST_RUN__")) {
    g_First_Run = TRUE;
  }

  if (!strcmp(ttprefs[TTPK_COMPILER].data.string, "SAS/C")) {
    compiler_entry = 1;
  }

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','0'),
    MUIA_Window_Title, "Editor Settings",
    //MUIA_Window_Width, MUIV_Window_Width_Visible(56),
    //MUIA_Window_Height, MUIV_Window_Height_Visible(52),
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "IDE:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.IDE, TAG_END),
        MUIA_Group_Child, (objects.IDE = NewObject(MUIC_PopASLString->mcc_Class, NULL,
          MUIA_PopASLString_Requester, g_FileReq,
          MUIA_ShortHelp, help_string.IDE,
          MUIA_String_Contents, ttprefs[TTPK_IDE].data.string ? ttprefs[TTPK_IDE].data.string : (STRPTR)"",
          ASLFR_TitleText, "Please select the executable of your IDE",
          ASLFR_PositiveText, "Select",
          ASLFR_DrawersOnly, FALSE,
          ASLFR_DoPatterns, FALSE,
          ASLFR_InitialPattern, "",
          ASLFR_InitialFile, "",
          ASLFR_InitialDrawer, "SYS:",
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Compiler:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.compiler, TAG_END),
        MUIA_Group_Child, (objects.cyc_compiler = MUI_NewObject(MUIC_Cycle,
          MUIA_ShortHelp, help_string.compiler,
          MUIA_Cycle_Entries, compiler_entries,
          MUIA_Cycle_Active, compiler_entry,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Output:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.output, TAG_END),
        MUIA_Group_Child, (objects.output = NewObject(MUIC_PopASLString->mcc_Class, NULL,
          MUIA_PopASLString_Requester, g_FileReq,
          MUIA_PopASLString_IgnoreContents, TRUE,
          MUIA_ShortHelp, help_string.output,
          MUIA_String_Contents, ttprefs[TTPK_OUTPUT].data.string ? ttprefs[TTPK_OUTPUT].data.string : (STRPTR)"",
          ASLFR_TitleText, "Please select a file for compiler output.",
          ASLFR_PositiveText, "Select",
          ASLFR_DrawersOnly, FALSE,
          ASLFR_DoPatterns, FALSE,
          ASLFR_InitialFile, "",
          ASLFR_InitialDrawer, "T:",
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, MUIA_Rectangle_HBar, TRUE, MUIA_VertWeight, 0, TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (objects.btn_save = MUI_NewButton("Save", 's', help_string.save)),
        MUIA_Group_Child, (objects.btn_use = MUI_NewButton("Use", 'u', help_string.use)),
        MUIA_Group_Child, (objects.btn_cancel = MUI_NewButton("Cancel", 'c', help_string.cancel)),
      TAG_END),
    TAG_END),
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    get(objects.IDE, MUIA_PopASLString_StringObject, &objects.IDE_str);
    get(objects.IDE, MUIA_PopASLString_PopButton, &objects.IDE_pop);
    get(objects.output, MUIA_PopASLString_StringObject, &objects.output_str);
    get(objects.output, MUIA_PopASLString_PopButton, &objects.output_pop);

    data->obj_table.IDE = objects.IDE;
    data->obj_table.cyc_compiler = objects.cyc_compiler;
    data->obj_table.output = objects.output;

    data->old_values.IDE = NULL;
    data->old_values.output = NULL;
    data->edited = FALSE;

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.IDE_str,
                                             objects.IDE_pop,
                                             objects.cyc_compiler,
                                             objects.output_str,
                                             objects.output_pop,
                                             objects.btn_save,
                                             objects.btn_use,
                                             objects.btn_cancel,
                                             NULL);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 2,
      MUIM_EditorSettings_Close, CLOSE_BY_GADGET);

    DoMethod(objects.btn_save, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_EditorSettings_Close, CLOSE_BY_SAVE);

    DoMethod(objects.btn_use, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_EditorSettings_Close, CLOSE_BY_USE);

    DoMethod(objects.btn_cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_EditorSettings_Close, CLOSE_BY_CANCEL);

    /**************************************************************************/
    //A value change on all these gadgets has to set 'edited' state TRUE
    DoMethod(data->obj_table.IDE, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 3,
      MUIM_Set, MUIA_EditorSettings_Edited, TRUE);

    /**************************************************************************/

    //<SUBCLASS INITIALIZATION HERE>
    if (/*<Success of your initializations>*/ TRUE) {
      if (settings_read) freeToolTypePrefStrings(ttprefs);

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
      case MUIA_EditorSettings_Edited:
        data->edited = tag->ti_Data;
      break;
      case MUIA_Window_Open:
        switch (tag->ti_Data) {
          case TRUE:
            storeOldValues(data);
          break;
          case FALSE:
            freeOldValues(data);
          break;
        }
      break;
      case MUIA_EditorSettings_IDE:
        DoMethod(data->obj_table.IDE, MUIM_Set, MUIA_String_Contents, tag->ti_Data);
      break;
      case MUIA_EditorSettings_Compiler:
        DoMethod(data->obj_table.cyc_compiler, MUIM_Set, MUIA_Cycle_Active, tag->ti_Data);
      break;
      case MUIA_EditorSettings_Output:
        DoMethod(data->obj_table.output, MUIM_Set, MUIA_String_Contents, tag->ti_Data);
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
    case MUIA_EditorSettings_IDE:
      get(data->obj_table.IDE, MUIA_String_Contents, msg->opg_Storage);
    return TRUE;
    case MUIA_EditorSettings_Compiler:
      get(data->obj_table.cyc_compiler, MUIA_Cycle_Active, msg->opg_Storage);
    return TRUE;
    case MUIA_EditorSettings_Output:
      get(data->obj_table.output, MUIA_String_Contents, msg->opg_Storage);
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
    case MUIM_EditorSettings_Close:
      return m_Close(cl, obj, (struct close_Msg*) msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_EditorSettings(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
