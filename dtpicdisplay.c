/******************************************************************************
 * DtPicDisplay                                                               *
 ******************************************************************************/
///Includes
#include <string.h>

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
#include "dtpicdisplay.h"
///
///Structs
struct cl_ObjTable
{
  Object* root_group;
  Object* dtPic;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  STRPTR picture;
};

struct cl_Msg
{
  ULONG MethodID;
  //<SUBCLASS METHOD MESSAGE PAYLOAD HERE>
};
///

//<DEFINE SUBCLASS METHODS HERE>
///m_SetPicture(cl, obj, picture)
static ULONG m_SetPicture(struct IClass* cl, Object* obj, STRPTR picture)
{
  struct cl_Data* data = INST_DATA(cl, obj);

  if (strcmp(data->picture, picture)) {
    if (DoMethod(data->obj_table.root_group, MUIM_Group_InitChange)) {
      if (data->picture) {
        //free the dtpic object
        DoMethod(data->obj_table.root_group, OM_REMMEMBER, data->obj_table.dtPic);
        MUI_DisposeObject(data->obj_table.dtPic);
        data->obj_table.dtPic = NULL;
        freeString(data->picture);
        data->picture = NULL;
      }

      if (picture && Exists(picture)) {
        data->obj_table.dtPic = MUI_NewObject(MUIC_Dtpic,
          MUIA_Dtpic_Name, picture,
        TAG_END);

        if (data->obj_table.dtPic) {
          DoMethod(data->obj_table.root_group, OM_ADDMEMBER, data->obj_table.dtPic);
          data->picture = makeString(picture);
        }
      }

      DoMethod(data->obj_table.root_group, MUIM_Group_ExitChange);
    }
  }

  if (data->picture) {
    DoMethod(obj, MUIM_Set, MUIA_Window_Open, TRUE);
  }
  else {
    DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
  }

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  Object* root_group;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_Title, "Sevgi Editor - Picture Display",
    MUIA_Window_RootObject, (root_group = MUI_NewObject(MUIC_Group,
    TAG_END)),
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    data->obj_table.root_group = root_group;
    data->obj_table.dtPic = NULL;
    data->picture = NULL;

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
    MUIM_Set, MUIA_Window_Open, FALSE);

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

  freeString(data->picture);

  return DoSuperMethodA(cl, obj, msg);
}
///
///Overridden OM_SET
//*****************
static ULONG m_Set(struct IClass* cl, Object* obj, struct opSet* msg)
{
  //struct cl_Data *data = INST_DATA(cl, obj);
  struct TagItem *tags, *tag;

  for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
  {
    switch (tag->ti_Tag)
    {
      //<SUBCLASS ATTRIBUTES HERE>
      case MUIA_DtPicDisplay_Picture:
        m_SetPicture(cl, obj, (STRPTR)tag->ti_Data);
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
    case MUIA_DtPicDisplay_Picture:
      *msg->opg_Storage = (ULONG)data->picture;
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

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_DtPicDisplay(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
