/******************************************************************************
 * PopASLString                                                               *
 * In contrary to MUI's built in MUIC_Popasl class which creates it's own sub *
 * task and own file requester, this object uses the file requester passed at *
 * init with MUIA_PopASLString_Requester. So every pop up is modal.           *
 * It recognizes and stores all ASLFR_* tags at init time and you can set new *
 * values to them with set method.                                            *
 * Passing a valid file requester pointer in MUIA_PopASLString_Requester at   *
 * at init is MANDATORY.                                                      *
 * It will handle the requester call and will set the path selected into the  *
 * string gadget by itself. But if you wish to interpret the selections made  *
 * on the requester, you can set a function pointer to the attribute          *
 * MUIA_PopASLString_StringFunc which will be called every time the requester *
 * is called by this object and returns TRUE.                                 *
 * The function should be declared as below:                                  *
 *   VOID stringFunc(struct FileRequester* file_req, Object* string)          *
 * ...and it will receive the requester pointer on 'file_req', and the string *
 * gadget pointer on 'string'. You should set the desired string contents to  *
 * the string gadget and create a notification on MUIA_String_Acknowledge in  *
 * this function.                                                             *
 * Other than these it will behave just like a regular MUIC_String object. So *
 * you can set/get attributes and set notifications just as you do to a       *
 * regular string object.                                                     *
 ******************************************************************************/
///Defines
#define MUIM_GoActive             0x8042491A
#define MUIM_GoInactive           0x80422C0C

#define MUIM_PopASLString_Pop_ASL 0x80430FF1
///
///Includes
#include <strings.h>

#include <proto/exec.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "popasl_string.h"
///
///Structs
struct cl_ObjTable {
  Object* string;
  Object* btn_pop;
};

struct cl_Data {
  struct cl_ObjTable obj_table;
  struct FileRequester* file_req;
  struct TagItem* initial_tags;
  ULONG num_initial_tags;
  VOID (*stringFunc)(struct FileRequester*, Object*);
  BOOL ignore_contents;
};
///
///Globals
extern APTR g_MemoryPool;

extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;
///

///createInitialTags(passed_tags, &initial_tags)
ULONG createInitialTags(struct TagItem* passed_tags, struct TagItem** initial_tags)
{
  ULONG num_new_tags = 1; //1 for the TAG_END tag
  struct TagItem* tag;
  struct TagItem* tag_state = passed_tags;
  struct TagItem* new_tags = NULL;
  BOOL initial_drawer_tag_is_present = FALSE;
  BOOL initial_file_tag_is_present = FALSE;
  BOOL window_tag_is_present = FALSE;
  BOOL sleep_tag_is_present = FALSE;

  //count ASLFR tags
  while ((tag = NextTagItem(&tag_state))) {
    if (tag->ti_Tag >= ASLFR_TitleText && tag->ti_Tag <= ASLFR_Activate) {
      num_new_tags++;
    }
    if (tag->ti_Tag == ASLFR_InitialDrawer) initial_drawer_tag_is_present = TRUE;
    if (tag->ti_Tag == ASLFR_InitialFile) initial_file_tag_is_present = TRUE;
    if (tag->ti_Tag == ASLFR_Window) window_tag_is_present = TRUE;
    if (tag->ti_Tag == ASLFR_SleepWindow) sleep_tag_is_present = TRUE;
  }

  if (!initial_drawer_tag_is_present) num_new_tags++;
  if (!initial_file_tag_is_present) num_new_tags++;
  if (!window_tag_is_present) num_new_tags++;
  if (!sleep_tag_is_present) num_new_tags++;

  //allocate memory to store initial_tags
  new_tags = (struct TagItem*)AllocPooled(g_MemoryPool, sizeof(struct TagItem) * num_new_tags);

  //copy ALSFR tags into the newly created tag list
  if (new_tags) {
    ULONG i = 0;

    tag_state = passed_tags;
    while ((tag = NextTagItem(&tag_state))) {
      if (tag->ti_Tag >= ASLFR_TitleText && tag->ti_Tag <= ASLFR_Activate) {
        new_tags[i].ti_Tag = tag->ti_Tag;
        new_tags[i].ti_Data = tag->ti_Data;
        i++;
      }
    }

    if (!initial_drawer_tag_is_present) {
      new_tags[i].ti_Tag = ASLFR_InitialDrawer;
      new_tags[i].ti_Data = (ULONG)"";
      i++;
    }

    if (!initial_file_tag_is_present) {
      new_tags[i].ti_Tag = ASLFR_InitialFile;
      new_tags[i].ti_Data = (ULONG)"";
      i++;
    }

    if (!window_tag_is_present) {
      new_tags[i].ti_Tag = ASLFR_Window;
      new_tags[i].ti_Data = NULL;
      i++;
    }

    if (!sleep_tag_is_present) {
      new_tags[i].ti_Tag = ASLFR_SleepWindow;
      new_tags[i].ti_Data = TRUE;
      i++;
    }

    new_tags[i].ti_Tag = TAG_END;
  }

  *initial_tags = new_tags;

  return num_new_tags;
}
///
///defaultStringFunc()
VOID defaultStringFunc(struct FileRequester* file_req, Object* string)
{
  ULONG drawer_len = strlen(file_req->fr_Drawer);
  ULONG str_len = drawer_len + strlen(file_req->fr_File) + 2;
  STRPTR str = AllocPooled(g_MemoryPool, str_len);
  if (str) {
    strcpy(str, file_req->fr_Drawer);
    if (drawer_len && str[drawer_len - 1] != ':' && str[drawer_len - 1] != '/') {
      strcat(str, "/");
    }
    strcat(str, file_req->fr_File);
  }

  DoMethod(string, MUIM_Set, MUIA_String_Contents, str);
  DoMethod(string, MUIM_Set, MUIA_String_Acknowledge, str);

  FreePooled(g_MemoryPool, str, str_len);
}
///

///m_PopASL()
static ULONG m_PopASL(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR string = NULL;
  STRPTR path_part = NULL;

  if (!data->ignore_contents) {
    get(data->obj_table.string, MUIA_String_Contents, &string);
    path_part = pathPart(string);
    if (path_part) {
      DoMethod(obj, MUIM_NoNotifySet, ASLFR_InitialDrawer, path_part);
      DoMethod(obj, MUIM_NoNotifySet, ASLFR_InitialFile, FilePart(string));
    }
  }

  if (MUI_AslRequest(data->file_req, data->initial_tags)) {
    data->stringFunc(data->file_req, data->obj_table.string);
  }

  freeString(path_part);
  return 0;
}
///
///m_Show
static ULONG m_Show(struct IClass* cl, Object* obj, Msg msg)
{
  ULONG retVal = DoSuperMethodA(cl, obj, msg);

  if (retVal) {
    struct cl_Data* data = INST_DATA(cl, obj);
    struct TagItem* tag;
    struct Window* window;

    get(obj, MUIA_Window, &window);

    tag = FindTagItem(ASLFR_Window, data->initial_tags);
    tag->ti_Data = (ULONG)window;
  }

  return retVal;
}
///
///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data* data;
  struct TagItem* tag;
  BOOL ignore_contents = FALSE;

  struct {
    Object* string;
    Object* btn_pop;
  }objects;

  struct {
    ULONG image_spec;
    struct FileRequester* file_req;
    APTR stringFunc;
  }tags;

  if ((tag = FindTagItem(MUIA_Group_Horiz, msg->ops_AttrList))) {
    tag->ti_Tag = TAG_IGNORE;
  }

  if ((tag = FindTagItem(MUIA_Group_HorizSpacing, msg->ops_AttrList))) {
    tag->ti_Tag = TAG_IGNORE;
  }

  if ((tag = FindTagItem(MUIA_Image_Spec, msg->ops_AttrList))) {
    tags.image_spec = tag->ti_Data;
    tag->ti_Tag = TAG_IGNORE;
  }
  else tags.image_spec = MUII_PopFile;

  if ((tag = FindTagItem(MUIA_PopASLString_Requester, msg->ops_AttrList))) {
    tag->ti_Tag = TAG_IGNORE;
    tags.file_req = (struct FileRequester*)tag->ti_Data;
  }
  else return (ULONG) NULL;

  if ((tag = FindTagItem(MUIA_PopASLString_StringFunc, msg->ops_AttrList))) {
    tag->ti_Tag = TAG_IGNORE;
    tags.stringFunc = (APTR)tag->ti_Tag;
  }
  else tags.stringFunc = (APTR)defaultStringFunc;

  if ((tag = FindTagItem(MUIA_PopASLString_IgnoreContents, msg->ops_AttrList))) {
    ignore_contents = TRUE;
    tag->ti_Tag = TAG_IGNORE;
  }

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Group_Horiz, TRUE,
    MUIA_Group_HorizSpacing, 1,
    MUIA_Group_Child, (objects.string = MUI_NewObject(MUIC_String,
      MUIA_Frame, MUIV_Frame_String,
    TAG_MORE, msg->ops_AttrList)),
    MUIA_Group_Child, (objects.btn_pop = MUI_NewObject(MUIC_Image,
      MUIA_Image_Spec, tags.image_spec,
      MUIA_InputMode, MUIV_InputMode_RelVerify,
      MUIA_Frame, MUIV_Frame_ImageButton,
      MUIA_Background, MUII_ButtonBack,
    TAG_END)),
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    DoMethod(objects.btn_pop, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_PopASLString_Pop_ASL);

    data->obj_table.string = objects.string;
    data->obj_table.btn_pop = objects.btn_pop;
    data->ignore_contents = ignore_contents;

    data->file_req = tags.file_req;
    data->stringFunc = (VOID (*)(struct FileRequester*, Object*))tags.stringFunc;
    data->num_initial_tags = createInitialTags(msg->ops_AttrList, &data->initial_tags);

    //<SUBCLASS INITIALIZATION HERE>
    if (data->initial_tags) {

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

  if (data->initial_tags) {
    FreePooled(g_MemoryPool, data->initial_tags, sizeof(struct TagItem) * data->num_initial_tags);
  }

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
      case ASLFR_TitleText:
      case ASLFR_Window:
      case ASLFR_InitialLeftEdge:
      case ASLFR_InitialTopEdge:
      case ASLFR_InitialWidth:
      case ASLFR_InitialHeight:
      case ASLFR_HookFunc:
      case ASLFR_InitialFile:
      case ASLFR_InitialDrawer:
      case ASLFR_InitialPattern:
      case ASLFR_PositiveText:
      case ASLFR_NegativeText:
      case ASLFR_Flags1:
      case ASLFR_Flags2:
      case ASLFR_Screen:
      case ASLFR_PubScreenName:
      case ASLFR_PrivateIDCMP:
      case ASLFR_SleepWindow:
      case ASLFR_DoSaveMode:
      case ASLFR_DoMultiSelect:
      case ASLFR_DoPatterns:
      case ASLFR_DrawersOnly:
      case ASLFR_FilterFunc:
      case ASLFR_Locale:
      case ASLFR_TextAttr:
      case ASLFR_UserData:
      case ASLFR_RejectIcons:
      case ASLFR_RejectPattern:
      case ASLFR_AcceptPattern:
      case ASLFR_FilterDrawers:
      case ASLFR_IntuiMsgFunc:
      case ASLFR_SetSortBy:
      case ASLFR_GetSortBy:
      case ASLFR_SetSortDrawers:
      case ASLFR_GetSortDrawers:
      case ASLFR_SetSortOrder:
      case ASLFR_GetSortOrder:
      case ASLFR_InitialShowVolumes:
      case ASLFR_PopToFront:
      case ASLFR_Activate:
      {
        struct TagItem* tag_id = FindTagItem(tag->ti_Tag, data->initial_tags);
        if (tag_id) tag_id->ti_Data = tag->ti_Data;
      }
      break;
      case MUIA_String_Accept:
      case MUIA_String_AdvanceOnCR:
      case MUIA_String_AttachedList:
      case MUIA_String_BufferPos:
      case MUIA_String_Contents:
      case MUIA_String_DisplayPos:
      case MUIA_String_EditHook:
      case MUIA_String_Integer:
      case MUIA_String_LonelyEditHook:
      case MUIA_String_Reject:
      	DoMethod(data->obj_table.string, MUIM_Set, tag->ti_Tag, tag->ti_Data);
      break;
      case MUIA_PopASLString_StringFunc:
        data->stringFunc = (VOID (*)(struct FileRequester*, Object*))tag->ti_Data;
      break;
      case MUIA_PopASLString_IgnoreContents:
        data->ignore_contents = tag->ti_Data;
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
  ULONG value;

  switch (msg->opg_AttrID)
  {
    case MUIA_String_Accept:
    case MUIA_String_Acknowledge:
    case MUIA_String_AdvanceOnCR:
    case MUIA_String_AttachedList:
    case MUIA_String_BufferPos:
    case MUIA_String_Contents:
    case MUIA_String_DisplayPos:
    case MUIA_String_EditHook:
    case MUIA_String_Format:
    case MUIA_String_Integer:
    case MUIA_String_LonelyEditHook:
    case MUIA_String_MaxLen:
    case MUIA_String_Reject:
    case MUIA_String_Secret:
      get(data->obj_table.string, msg->opg_AttrID, &value);
      *msg->opg_Storage = value;
    return TRUE;
    case MUIA_PopASLString_StringFunc:
      *msg->opg_Storage = (ULONG)data->stringFunc;
    return TRUE;
    case MUIA_PopASLString_StringObject:
      *msg->opg_Storage = (ULONG)data->obj_table.string;
    return TRUE;
    case MUIA_PopASLString_PopButton:
      *msg->opg_Storage = (ULONG)data->obj_table.btn_pop;
    return TRUE;
    case MUIA_PopASLString_IgnoreContents:
      *msg->opg_Storage = (ULONG)data->ignore_contents;
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
    case MUIM_Show:
      return m_Show(cl, obj, msg);
    case MUIM_PopASLString_Pop_ASL:
      return m_PopASL(cl, obj, msg);
    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_PopASLString(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
