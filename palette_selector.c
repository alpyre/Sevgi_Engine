/******************************************************************************
 * PaletteSelector                                                            *
 ******************************************************************************/
///Defines
#define MUIM_PaletteSelector_Close         0x80430B00
#define MUIM_PaletteSelector_Update        0x80430B01
#define MUIM_PaletteSelector_ChangePalette 0x80430B02

#define CLOSE_BY_GADGET 0
#define CLOSE_BY_SELECT 1
#define CLOSE_BY_CANCEL 2
///
///Includes
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
#include "color_palette.h"
#include "palette_selector.h"
///
///Structs
struct PaletteItem {
  STRPTR name;
  UBYTE* palette;
};

struct cl_ObjTable
{
  Object* listview;
  Object* btn_select;
  Object* btn_cancel;
  Object* color_palette;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  Object* source_list;  //The list on the paletteEditor object
  struct PaletteItem* selected;
};

struct close_Msg
{
  ULONG MethodID;
  ULONG close_by;
};
///
///Globals
extern struct MUI_CustomClass* MUIC_ColorPalette;
///
///Hooks
HOOKPROTO(plt_item_dispfunc, LONG, char **array, struct PaletteItem* p)
{
  if (p) *array = p->name;
  return(0);
}
MakeStaticHook(plt_item_disphook, plt_item_dispfunc);
///

//<DEFINE SUBCLASS METHODS HERE>
///m_Update()
static ULONG m_Update(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->source_list) {
    ULONG entries = 0;
    ULONG i;

    DoMethod(data->obj_table.listview, MUIM_List_Clear);
    get(data->source_list, MUIA_List_Entries, &entries);

    for (i = 0; i < entries; i++) {
      struct PaletteItem* pi = NULL;
      DoMethod(data->source_list, MUIM_List_GetEntry, i, &pi);
      DoMethod(data->obj_table.listview, MUIM_List_InsertSingle, pi, MUIV_List_Insert_Bottom);
    }
  }

  return 0;
}
///
///m_Close(close_by)
static ULONG m_Close(struct IClass* cl, Object* obj, struct close_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct PaletteItem* selected;

  switch (msg->close_by) {
    case CLOSE_BY_SELECT:
      DoMethod(data->obj_table.listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected);
      DoMethod(obj, MUIM_Set, MUIA_PaletteSelector_Selected, (ULONG)selected);
    case CLOSE_BY_GADGET:
    case CLOSE_BY_CANCEL:
      DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
    break;
  }

  return 0;
}
///
///m_ChangePalette(close_by)
static ULONG m_ChangePalette(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct PaletteItem* selected;

  DoMethod(data->obj_table.listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &selected);
  if (selected) {
    DoMethod(data->obj_table.color_palette, MUIM_Set, MUIA_ColorPalette_Palette, (ULONG)selected->palette);
    DoMethod(data->obj_table.color_palette, MUIM_Set, MUIA_Disabled, FALSE);
  }
  else {
    DoMethod(data->obj_table.color_palette, MUIM_Set, MUIA_ColorPalette_Palette, (ULONG)NULL);
    DoMethod(data->obj_table.color_palette, MUIM_Set, MUIA_Disabled, TRUE);
  }

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct {
    Object* listview;
    Object* btn_select;
    Object* btn_cancel;
    Object* color_palette;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','B'),
    MUIA_Window_Title, "Select Palette",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (objects.listview = MUI_NewObject(MUIC_Listview,
          MUIA_HorizWeight, 140,
          MUIA_Listview_Input, TRUE,
          MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
          MUIA_ShortHelp, "Palettes on the current palettes header file",
          MUIA_Listview_List, MUI_NewObject(MUIC_List,
            MUIA_Frame, MUIV_Frame_InputList,
            MUIA_List_Active, 0,
            MUIA_List_AutoVisible, TRUE,
            MUIA_List_DragSortable, TRUE,
            MUIA_List_DisplayHook, &plt_item_disphook,
          TAG_END),
        TAG_END)),
        MUIA_Group_Child, (objects.color_palette = NewObject(MUIC_ColorPalette->mcc_Class, NULL,
          MUIA_Disabled, TRUE,
          MUIA_HorizWeight, 60,
          MUIA_ShortHelp, "Colors on the current palette",
          MUIA_ColorPalette_DisplayOnly, TRUE,
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (objects.btn_select = MUI_NewButton("Select", 's', NULL)),
        MUIA_Group_Child, (objects.btn_cancel = MUI_NewButton("Cancel", 'c', NULL)),
      TAG_END),
    TAG_END),
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.listview,
                                             objects.btn_select,
                                             objects.btn_cancel,
                                             NULL);

    //Notifications
    DoMethod(objects.listview, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, objects.btn_select, 3,
      MUIM_Set, MUIA_Disabled, FALSE);

    DoMethod(objects.listview, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1,
      MUIM_PaletteSelector_ChangePalette);

    DoMethod(objects.listview, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, obj, 2,
      MUIM_PaletteSelector_Close, CLOSE_BY_SELECT);

    DoMethod(objects.btn_select, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_PaletteSelector_Close, CLOSE_BY_SELECT);

    DoMethod(objects.btn_cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_PaletteSelector_Close, CLOSE_BY_CANCEL);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 2,
      MUIM_PaletteSelector_Close, CLOSE_BY_GADGET);

    data->source_list = NULL;
    data->selected = NULL;
    data->obj_table.listview = objects.listview;
    data->obj_table.btn_select = objects.btn_select;
    data->obj_table.btn_cancel = objects.btn_cancel;
    data->obj_table.color_palette = objects.color_palette;

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
      case MUIA_PaletteSelector_SourceList:
        data->source_list = (Object*)tag->ti_Data;
        //Putting a notify here can update the listview on every change on the
        //source_list. But updating on window open make more sense!
        //DoMethod(data->source_list, MUIM_Notify, MUIA_List_Entries, MUIV_EveryTime, obj, 1,
        //  MUIM_PaletteSelector_Update);
      break;
      case MUIA_Window_Open:
        if (tag->ti_Data) {
          DoMethod(obj, MUIM_PaletteSelector_Update);
          DoMethod(data->obj_table.btn_select, MUIM_Set, MUIA_Disabled, TRUE);
        }
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
    case MUIA_PaletteSelector_Selected:
      *msg->opg_Storage = (ULONG)data->selected;
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
    case MUIM_PaletteSelector_Update:
      return m_Update(cl, obj, msg);
    case MUIM_PaletteSelector_Close:
      return m_Close(cl, obj, (struct close_Msg*)msg);
    case MUIM_PaletteSelector_ChangePalette:
      return m_ChangePalette(cl, obj, msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_PaletteSelector(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
