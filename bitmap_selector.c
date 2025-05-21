/******************************************************************************
 * BitmapSelector                                                             *
 ******************************************************************************/
///Defines
#define MUIM_BitmapSelector_Close         0x80430D00
#define MUIM_BitmapSelector_Update        0x80430D01
#define MUIM_BitmapSelector_CheckValidity 0x80430D02

#define CLOSE_BY_GADGET 0
#define CLOSE_BY_SELECT 1
#define CLOSE_BY_CANCEL 2
///
///Includes
//standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include <proto/exec.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "popasl_string.h"
#include "bitmap_selector.h"
///
///Structs
struct cl_ObjTable
{
  Object* file;
  Object* chk_displayable;
  Object* chk_interleaved;
  Object* btn_select;
  Object* btn_cancel;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  //<SUBCLASS VARIABLES HERE>
  STRPTR bitmap_string;
};

struct close_Msg
{
  ULONG MethodID;
  ULONG close_by;
};

struct cl_Msg
{
  ULONG MethodID;
  //<SUBCLASS METHOD MESSAGE PAYLOAD HERE>
};
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;
extern struct MUI_CustomClass* MUIC_PopASLString;

extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;

static struct {
  STRPTR file;
  STRPTR displayable;
  STRPTR interleaved;
  STRPTR select;
  STRPTR cancel;
}help_string = {
  "The ILBM file to be loaded with this level.",
  "The bitmap for the ILBM file loaded\nwill be allocated in Chip RAM.",
  "The bitmap for the ILBM file loaded\nwill be an interleaved bitmap.",
  "Add the selected ILBM file to the level\nand close this window.",
  "Close this window."
};
///

///prepBitmapString(data)
STRPTR prepBitmapString(struct cl_Data *data)
{
  STRPTR bitmap_string = NULL;
  STRPTR options = "[NN]";
  STRPTR file;
  ULONG displayable;
  ULONG interleaved;

  get(data->obj_table.file, MUIA_String_Contents, &file);
  get(data->obj_table.chk_displayable, MUIA_Selected, &displayable);
  get(data->obj_table.chk_interleaved, MUIA_Selected, &interleaved);

  if (file && strlen(file)) {
    if (displayable) options[1] = 'D';
    else options[1] = 'N';
    if (interleaved) options[2] = 'I';
    else options[2] = 'N';

    bitmap_string = makeString2(options, file);
  }

  return bitmap_string;
}
///

//<DEFINE SUBCLASS METHODS HERE>
///m_CheckValidity()
STATIC ULONG m_CheckValidity(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR file;
  ULONG disable = FALSE;

  get(data->obj_table.file, MUIA_String_Contents, &file);

  if (file && strlen(file)) {
    ULONG data_drawer_len = strlen(g_Project.data_drawer);

    if (!strncmp(file, g_Project.data_drawer, data_drawer_len)) {
      file += data_drawer_len;
      DoMethod(data->obj_table.file, MUIM_Set, MUIA_String_Contents, (ULONG)file);

      if (!*file) disable = TRUE;
    }
  }
  else disable = TRUE;

  DoMethod(data->obj_table.btn_select, MUIM_Set, MUIA_Disabled, disable);

  return 0;
}
///
///m_Close(close_by)
static ULONG m_Close(struct IClass* cl, Object* obj, struct close_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->close_by) {
    case CLOSE_BY_SELECT:
    freeString(data->bitmap_string);
    if ((data->bitmap_string = prepBitmapString(data))) {
      DoMethod(obj, MUIM_Set, MUIA_BitmapSelector_Selected, (ULONG)data->bitmap_string);
    }
    case CLOSE_BY_GADGET:
    case CLOSE_BY_CANCEL:
      DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
    break;
  }

  DoMethod(data->obj_table.file, MUIM_Set, MUIA_String_Contents, (ULONG)"");

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;

  struct {
    Object* file;
    Object* str_file;
    Object* pop_file;
    Object* chk_displayable;
    Object* chk_interleaved;
    Object* btn_select;
    Object* btn_cancel;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','D'),
    MUIA_Window_Title, "Select ILBM File",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "File:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.file, TAG_END),
        MUIA_Group_Child, (objects.file = NewObject(MUIC_PopASLString->mcc_Class, NULL,
          MUIA_PopASLString_Requester, g_FileReq,
          MUIA_PopASLString_IgnoreContents, TRUE,
          MUIA_ShortHelp, help_string.file,
          MUIA_Image_Spec, MUII_PopFile,
          ASLFR_TitleText, "Select an ILBM file",
          ASLFR_PositiveText, "Select",
          ASLFR_DoSaveMode, FALSE,
          ASLFR_DrawersOnly, FALSE,
          ASLFR_DoPatterns, TRUE,
          ASLFR_InitialPattern, "#?.(iff|ilbm)",
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_displayable, FALSE, "Displayable", 'd', help_string.displayable),
      MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_interleaved, FALSE, "Interleaved", 'i', help_string.interleaved),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (objects.btn_select = MUI_NewButton("Select", 's', help_string.select)),
        MUIA_Group_Child, (objects.btn_cancel = MUI_NewButton("Cancel", 'c', help_string.cancel)),
      TAG_END),
    TAG_END),
    //<SUPERCLASS TAGS HERE>
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);
    get(objects.file, MUIA_PopASLString_StringObject, &objects.str_file);
    get(objects.file, MUIA_PopASLString_PopButton, &objects.pop_file);

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.str_file,
                                             objects.pop_file,
                                             objects.chk_displayable,
                                             objects.chk_interleaved,
                                             objects.btn_select,
                                             objects.btn_cancel,
                                             NULL);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 2,
      MUIM_BitmapSelector_Close, CLOSE_BY_GADGET);

    DoMethod(objects.btn_select, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_BitmapSelector_Close, CLOSE_BY_SELECT);

    DoMethod(objects.btn_cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_BitmapSelector_Close, CLOSE_BY_CANCEL);

    DoMethod(objects.file, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1,
      MUIM_BitmapSelector_CheckValidity);

    DoMethod(objects.btn_select, MUIM_Set, MUIA_Disabled, TRUE);

    data->bitmap_string = NULL;
    data->obj_table.file = objects.file;
    data->obj_table.chk_displayable = objects.chk_displayable;
    data->obj_table.chk_interleaved = objects.chk_interleaved;
    data->obj_table.btn_select = objects.btn_select;
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
  struct cl_Data *data = INST_DATA(cl, obj);

  //<FREE SUBCLASS INITIALIZATIONS HERE>
  freeString(data->bitmap_string);

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
      case MUIA_BitmapSelector_DataDrawer:
        DoMethod(data->obj_table.file, MUIM_Set, ASLFR_InitialDrawer, tag->ti_Data);
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
    case MUIA_bitmap_selector_{Attribute}:
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

    case MUIM_BitmapSelector_Close:
      return m_Close(cl, obj, (struct close_Msg*)msg);
    case MUIM_BitmapSelector_CheckValidity:
      return m_CheckValidity(cl, obj, msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_BitmapSelector(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
