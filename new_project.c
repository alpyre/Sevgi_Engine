/******************************************************************************
 * NewProject                                                                *
 ******************************************************************************/
///Defines
#define PROJECTS_DIR  "Projects"
#define TEMPLATES_DIR "Templates"
#define CODE_DIR      "Code"
#define EXTRAS_DIR    "Extras"
#define EXTRAS_SASC_MAKEFILE "makefile_sas_c"
#define EXTRAS_SASC_PTPLAYER "ptplayer_sas_c.o"
#define DEFAULT_THUMBNAIL "Images/no_image.ilbm"

#define MUIM_NewProject_Create         0x80430901
#define MUIM_NewProject_CheckValidity  0x80430902
#define MUIM_NewProject_CheckDrawer    0x80430903
#define MUIM_NewProject_SelectTemplate 0x80430904
///
///Includes
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
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
#include "editor_settings.h"
#include "new_project.h"
///
///Globals
extern APTR g_MemoryPool;
extern STRPTR g_Program_Directory;
extern Object* g_Settings;
extern struct FileRequester* g_FileReq;

extern struct MUI_CustomClass *MUIC_PopASLString;

struct {
  STRPTR name;
  STRPTR drawer;
  STRPTR create;
  STRPTR cancel;
}help_string = {"Name of the new game project.",
                "Directory to create the game project directory.\n(A subdirectory with the game name will be created!)",
                "Create the new game project.",
                "Cancel and close this window."};
///
///Structs
struct cl_ObjTable
{
  Object* name;
  Object* drawer;
  Object* lv_templates;
  Object* img_group;
  Object* img_thumbnail;
  Object* info_disp;
  Object* btn_create;
  Object* btn_cancel;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  STRPTR thumbnail;
  STRPTR info;
  BOOL drawer_valid;
};

struct cl_Msg
{
  ULONG MethodID;
};

struct TemplateItem {
  STRPTR name;
  STRPTR drawer;
};
///
///Hooks
HOOKPROTO(template_item_dispfunc, LONG, char **array, struct TemplateItem* ti)
{
  if (ti) *array = ti->name;
  return(0);
}
MakeStaticHook(template_item_disphook, template_item_dispfunc);
///

//private funtions
///newTemplateItem(drawer)
struct TemplateItem* newTemplateItem(STRPTR drawername) {
  struct TemplateItem* ti = (struct TemplateItem*)AllocPooled(g_MemoryPool, sizeof(struct TemplateItem));

  if (ti) {
    ti->name = makeString(drawername); replaceChars(ti->name, "_", ' ');
    ti->drawer = makeString(drawername);
  }

  return ti;
}
///
///getTemplates(data)
VOID getTemplates(struct cl_Data *data)
{
  BPTR lock = Lock(TEMPLATES_DIR, SHARED_LOCK);
  if (lock) {
    struct FileInfoBlock* fib = AllocDosObject(DOS_FIB, NULL);
    if (fib) {
      if (Examine(lock, fib)) {
        while (ExNext(lock, fib)) {
          if (fib->fib_DirEntryType > 0) {
            struct TemplateItem* template = newTemplateItem(fib->fib_FileName);
            if (template) {
              DoMethod(data->obj_table.lv_templates, MUIM_List_InsertSingle, template, MUIV_List_Insert_Bottom);
            }
          }
        }
      }
      FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
  }
}
///
///setThumbnail(data)
VOID setThumbnail(struct cl_Data* data)
{
  STRPTR filename = data->thumbnail;
  Object* image = NULL;

  if (DoMethod(data->obj_table.img_group, MUIM_Group_InitChange)) {
    if (filename && strlen(filename) && Exists(filename)) {
      image = MUI_NewObject(MUIC_Dtpic, MUIA_Dtpic_Name, filename, TAG_END);
    }

    if (!image) {
      image = MUI_NewObject(MUIC_Dtpic, MUIA_Dtpic_Name, DEFAULT_THUMBNAIL, TAG_END);
      if (!image) {
        image = MUI_NewObject(MUIC_Rectangle,
                  MUIA_Background, MUII_FILLBACK2,
                  MUIA_FixWidth, 110,
                  MUIA_FixHeight, 90,
                TAG_END);
      }
    }

    if (image) {
      DoMethod(data->obj_table.img_group, OM_REMMEMBER, data->obj_table.img_thumbnail);
      DoMethod(data->obj_table.img_group, OM_ADDMEMBER, image);
      MUI_DisposeObject(data->obj_table.img_thumbnail);
      data->obj_table.img_thumbnail = image;
    }

    DoMethod(data->obj_table.img_group, MUIM_Group_ExitChange);
  }
}
///
///setInfo(data)
VOID setInfo(struct cl_Data* data)
{
  STRPTR no_info = "No information";
  STRPTR info_text = no_info;
  STRPTR filename = data->info;
  LONG size = getFileSize(filename);

  if (size > 0) {
    BPTR fh = Open(filename, MODE_OLDFILE);

    if (fh) {
      info_text = AllocPooled(g_MemoryPool, size + 1);

      if (info_text) {
        Read(fh, info_text, size);
      }
      else {
        info_text = no_info;
      }

      Close(fh);
    }
  }

  DoMethod(data->obj_table.info_disp, MUIM_Set, MUIA_Floattext_Text, info_text);

  if (info_text != no_info) {
    FreePooled(g_MemoryPool, info_text, size + 1);
  }
}
///

///setProjectNameInMakefile(project_path)
/******************************************************************************
 * Opens the makefile in the project directory (project_path) and replaces    *
 * EXE string with game's given name by the user.                             *
 ******************************************************************************/
VOID setProjectNameInMakefile(STRPTR project_path)
{
  STRPTR exe_string = FilePart(project_path);
  STRPTR makefilename = makePath(project_path, "makefile", NULL);
  if (makefilename) {
    LONG buffer_size = getFileSize(makefilename);
    if (buffer_size > 0) {
      UBYTE* buffer = (UBYTE*)AllocPooled(g_MemoryPool, buffer_size + 1);
      if (buffer) {
        BPTR fh = Open(makefilename, MODE_READWRITE);
        if (fh) {
          ULONG exe_pos;
          Read(fh, buffer, buffer_size);

          if (searchString(buffer, "\nEXE ", &exe_pos)) {
            ULONG newline_pos;
            exe_pos += 5;
            Seek(fh, exe_pos, OFFSET_BEGINNING);
            Write(fh, "= ", 2);
            Write(fh, exe_string, strlen(exe_string));
            searchString(&buffer[exe_pos], "\n", &newline_pos);
            newline_pos += exe_pos;
            Write(fh, &buffer[newline_pos], buffer_size - newline_pos);
            SetFileSize(fh, 0, OFFSET_CURRENT);
          }
          Close(fh);
        }
        FreePooled(g_MemoryPool, buffer, buffer_size + 1);
      }
    }
    freeString(makefilename);
  }
}
///
///setProjectNameInMakefile(project_path)
/******************************************************************************
 * Opens the version.h in the project directory (project_path) and replaces   *
 * GAME_NAME define with game's given name by the user.                       *
 ******************************************************************************/
VOID setProjectNameInVersionFile(STRPTR project_path, STRPTR name)
{
  STRPTR search_string = "#define GAME_NAME";
  STRPTR versionfilename = makePath(project_path, "version.h", NULL);
  if (versionfilename) {
    LONG buffer_size = getFileSize(versionfilename);
    if (buffer_size > 0) {
      UBYTE* buffer = (UBYTE*)AllocPooled(g_MemoryPool, buffer_size + 1);
      if (buffer) {
        BPTR fh = Open(versionfilename, MODE_READWRITE);
        if (fh) {
          ULONG name_pos;
          Read(fh, buffer, buffer_size);

          if (searchString(buffer, search_string, &name_pos)) {
            ULONG newline_pos;
            name_pos += strlen(search_string) + 13;
            Seek(fh, name_pos, OFFSET_BEGINNING);
            Write(fh, "\"", 1);
            Write(fh, name, strlen(name));
            Write(fh, "\"", 1);
            searchString(&buffer[name_pos], "\n", &newline_pos);
            newline_pos += name_pos;
            Write(fh, &buffer[newline_pos], buffer_size - newline_pos);
            SetFileSize(fh, 0, OFFSET_CURRENT);
          }
          Close(fh);
        }
        FreePooled(g_MemoryPool, buffer, buffer_size + 1);
      }
    }
    freeString(versionfilename);
  }
}
///
///createCubicIDEoptions(project_path)
VOID createCubicIDEoptions(STRPTR project_path, ULONG compiler)
{
  struct {
    STRPTR intro;
    STRPTR alias;
    STRPTR gcc;
    STRPTR sas_c;
    STRPTR closure;
    STRPTR makefile;
    STRPTR make;
    STRPTR executable;
    STRPTR stack_gcc;
    STRPTR stack_sas_c;
    STRPTR target;
    STRPTR arguments;
    STRPTR end;
  }optionStrings = {
    "; project configuration (do not delete)\n\n(compiler\n\n",
    "\t(alias \"",
    "gcc/classic",
    "sas-c/classic",
    "\")\n\n",
    "\t(makefile \"makefile\")\n\n",
    "\t(make \"make -f %makefile %target\")\n\n",
    "\t(executable \"",
    "\t(stack \"65536\")\n\n",
    "\t(stack \"16384\")\n\n",
    "\t(target \"\")\n\n",
    "\t(arguments \"\")\n",
    ")"
  };
  STRPTR project_name = FilePart(project_path);
  STRPTR etc_directory = makePath(project_path, "etc", NULL);

  if (etc_directory) {
    BPTR dir_lock = CreateDir(etc_directory);

    if (dir_lock) {
      STRPTR options_filename = makePath(etc_directory, "project.options", NULL);
      if (options_filename) {
        BPTR fh = Open(options_filename, MODE_NEWFILE);
        if (fh) {
          Write(fh, optionStrings.intro, strlen(optionStrings.intro));
          Write(fh, optionStrings.alias, strlen(optionStrings.alias));
          switch (compiler) {
            case COMPILER_GCC:
              Write(fh, optionStrings.gcc, strlen(optionStrings.gcc));
            break;
            case COMPILER_SAS_C:
              Write(fh, optionStrings.sas_c, strlen(optionStrings.sas_c));
            break;
          }
          Write(fh, optionStrings.closure, strlen(optionStrings.closure));
          Write(fh, optionStrings.makefile, strlen(optionStrings.makefile));
          Write(fh, optionStrings.make, strlen(optionStrings.make));
          Write(fh, optionStrings.executable, strlen(optionStrings.executable));
          Write(fh, project_name, strlen(project_name));
          Write(fh, optionStrings.closure, strlen(optionStrings.closure));
          switch (compiler) {
            case COMPILER_GCC:
              Write(fh, optionStrings.stack_gcc, strlen(optionStrings.stack_gcc));
            break;
            case COMPILER_SAS_C:
              Write(fh, optionStrings.stack_sas_c, strlen(optionStrings.stack_sas_c));
            break;
          }
          Write(fh, optionStrings.target, strlen(optionStrings.target));
          Write(fh, optionStrings.arguments, strlen(optionStrings.arguments));
          Write(fh, optionStrings.end, strlen(optionStrings.end));

          Close(fh);
        }
        freeString(options_filename);
      }
      UnLock(dir_lock);
    }

    freeString(etc_directory);
  }
}
///

//private methods
///m_CheckValidity()
STATIC ULONG m_CheckValidity(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  BOOL validity = FALSE;

  if (data->drawer_valid) {
    STRPTR name = NULL;

    get(data->obj_table.name, MUIA_String_Contents, &name);

    if (name && strlen(name)) validity = TRUE;
  }

  DoMethod(data->obj_table.btn_create, MUIM_Set, MUIA_Disabled, !validity);

  return 0;
}
///
///m_CheckDrawer()
STATIC ULONG m_CheckDrawer(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR drawer = NULL;

  get(data->obj_table.drawer, MUIA_String_Contents, &drawer);

  if (drawer && Exists(drawer)) {
    data->drawer_valid = TRUE;
    m_CheckValidity(cl, obj, msg);
  }
  else {
    data->drawer_valid = FALSE;
    DoMethod(data->obj_table.btn_create, MUIM_Set, MUIA_Disabled, TRUE);
  }

  return 0;
}
///
///m_Create()
STATIC ULONG m_Create(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct TemplateItem* ti = NULL;
  STRPTR name = NULL;
  STRPTR drawer = NULL;
  STRPTR project_path = NULL;
  STRPTR IDE;
  ULONG compiler;

  DoMethod(data->obj_table.lv_templates, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &ti);
  get(data->obj_table.name, MUIA_String_Contents, &name);
  get(data->obj_table.drawer, MUIA_String_Contents, &drawer);
  get(g_Settings, MUIA_EditorSettings_IDE, &IDE);
  get(g_Settings, MUIA_EditorSettings_Compiler, &compiler);

  project_path = makePath(drawer, name, NULL);
  if (project_path) {
    BPTR lock;

    replaceChars(FilePart(project_path), " ", '_');

    lock = Lock(project_path, SHARED_LOCK);
    if (!lock) lock = CreateDir(project_path);
    if (lock) {
      STRPTR srcDir = makePath(g_Program_Directory, CODE_DIR, NULL);
      if (srcDir) {
        DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);
        CopyDir(srcDir, project_path);
        if (ti) {
          STRPTR template_path = makePath(TEMPLATES_DIR, ti->drawer, NULL);
          if (template_path) {
            CopyDir(template_path, project_path);
            freeString(template_path);
          }
        }
        if (!strcmp(FilePart(IDE), "Cubic IDE")) {
          createCubicIDEoptions(project_path, compiler);
        }
        if (compiler == COMPILER_SAS_C) {
          STRPTR src_makefile_name = makePath(g_Program_Directory, EXTRAS_DIR "/" EXTRAS_SASC_MAKEFILE, NULL);
          STRPTR dest_makefile_name = makePath(project_path, "makefile", NULL);
          STRPTR src_ptplayer_name = makePath(g_Program_Directory, EXTRAS_DIR "/" EXTRAS_SASC_PTPLAYER, NULL);
          STRPTR dst_ptplayer_name = makePath(project_path, "ptplayer.o", NULL);

          CopyFile(src_makefile_name, dest_makefile_name);
          CopyFile(src_ptplayer_name, dst_ptplayer_name);

          freeString(dst_ptplayer_name);
          freeString(src_ptplayer_name);
          freeString(dest_makefile_name);
          freeString(src_makefile_name);
        }

        setProjectNameInMakefile(project_path);
        setProjectNameInVersionFile(project_path, name);

        DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);
        freeString(srcDir);
      }

      UnLock(lock);
    }
    freeString(project_path);
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
  DoMethod(obj, MUIM_Set, MUIA_NewProject_Create, TRUE);

  return 0;
}
///
///m_SelectTemplate()
STATIC ULONG m_SelectTemplate(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct TemplateItem* ti = NULL;

  freeString(data->thumbnail); data->thumbnail = NULL;
  freeString(data->info); data->info = NULL;

  DoMethod(data->obj_table.lv_templates, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &ti);

  if (ti) {
    data->thumbnail = makePath(TEMPLATES_DIR, ti->drawer, ".ilbm");
    data->info = makePath(TEMPLATES_DIR, ti->drawer, ".text");
  }

  setThumbnail(data);
  setInfo(data);

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;

  struct {
    Object* name;
    Object* drawer;
    Object* drawer_pop;
    Object* drawer_str;
    Object* lv_templates;
    Object* img_group;
    Object* img_thumbnail;
    Object* info_disp;
    Object* btn_create;
    Object* btn_cancel;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','1'),
    MUIA_Window_Title, "New Game",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Name:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.name, TAG_END),
        MUIA_Group_Child, (objects.name = MUI_NewObject(MUIC_String,
          MUIA_ShortHelp, help_string.name,
          MUIA_String_Reject, DOS_RESERVED DOS_UNRECOMMENDED,
          MUIA_Frame, MUIV_Frame_String,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Drawer:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.drawer, TAG_END),
        MUIA_Group_Child, (objects.drawer = NewObject(MUIC_PopASLString->mcc_Class, NULL,
          MUIA_PopASLString_Requester, g_FileReq,
          MUIA_ShortHelp, help_string.drawer,
          MUIA_Image_Spec, MUII_PopDrawer,
          ASLFR_TitleText, "Select project directory",
          ASLFR_PositiveText, "Select",
          ASLFR_DrawersOnly, TRUE,
          ASLFR_DoPatterns, FALSE,
          ASLFR_InitialDrawer, PROJECTS_DIR "/",
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Frame, MUIV_Frame_Group,
        MUIA_FrameTitle, "Templates",
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Horiz, TRUE,
          MUIA_Group_Child, (objects.lv_templates = MUI_NewObject(MUIC_Listview,
            MUIA_Weight, 200,
            MUIA_Listview_Input, TRUE,
            MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
            MUIA_ShortHelp, "Available game templates.",
            MUIA_Listview_List, MUI_NewObject(MUIC_List,
              MUIA_Frame, MUIV_Frame_InputList,
              MUIA_List_Active, 0,
              MUIA_List_AutoVisible, TRUE,
              MUIA_List_DragSortable, TRUE,
              MUIA_List_DisplayHook, &template_item_disphook,
            TAG_END),
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
            MUIA_Group_Child, MUI_NewObject(MUIC_Group,
              MUIA_Group_Horiz, TRUE,
              MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
              MUIA_Group_Child, (objects.img_group = MUI_NewObject(MUIC_Group,
                MUIA_Group_Child, (objects.img_thumbnail = MUI_NewObject(MUIC_Rectangle,
                  MUIA_Background, MUII_FILLBACK2,
                  MUIA_FixWidth, 110,
                  MUIA_FixHeight, 90,
                TAG_END)),
              TAG_END)),
              MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
            TAG_END),
            MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
          TAG_END),
        TAG_END),
        MUIA_Group_Child, (MUI_NewObject(MUIC_Listview,
          MUIA_Listview_Input, FALSE,
          MUIA_Listview_List, (objects.info_disp = MUI_NewObject(MUIC_Floattext,
            MUIA_Frame, MUIV_Frame_Text,
            MUIA_Floattext_Text, NULL,
          TAG_END)),
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (objects.btn_create = MUI_NewButton("Create", 'r', help_string.create)),
        MUIA_Group_Child, (objects.btn_cancel = MUI_NewButton("Cancel", 'c', help_string.cancel)),
      TAG_END),
    TAG_END),
  TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    get(objects.drawer, MUIA_PopASLString_StringObject, &objects.drawer_str);
    get(objects.drawer, MUIA_PopASLString_PopButton, &objects.drawer_pop);
    data->drawer_valid = TRUE;
    data->thumbnail = NULL;
    data->info = NULL;
    data->obj_table.name = objects.name;
    data->obj_table.drawer = objects.drawer;
    data->obj_table.lv_templates = objects.lv_templates;
    data->obj_table.img_group = objects.img_group;
    data->obj_table.img_thumbnail = objects.img_thumbnail;
    data->obj_table.info_disp = objects.info_disp;
    data->obj_table.btn_create = objects.btn_create;
    data->obj_table.btn_cancel = objects.btn_cancel;

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.name,
                                             objects.drawer_str,
                                             objects.drawer_pop,
                                             objects.lv_templates,
                                             objects.btn_create,
                                             objects.btn_cancel,
                                             NULL);

    DoMethod(objects.btn_create, MUIM_Set, MUIA_Disabled, TRUE);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.btn_cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.btn_create, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_NewProject_Create);

    DoMethod(objects.name, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1,
      MUIM_NewProject_CheckValidity);

    DoMethod(objects.drawer, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1,
      MUIM_NewProject_CheckDrawer);

    DoMethod(objects.lv_templates, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1,
      MUIM_NewProject_SelectTemplate);

    getTemplates(data);

    DoMethod(objects.lv_templates, MUIM_Set, MUIA_List_Active, 0);

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
      if (tag->ti_Data) {
        DoMethod(data->obj_table.name, MUIM_Set, MUIA_String_Contents, "");
        DoMethod(data->obj_table.drawer, MUIM_Set, MUIA_String_Contents, PROJECTS_DIR "/");
        DoMethod(obj, MUIM_Set, MUIA_Window_ActiveObject, data->obj_table.name);
      }
      break;
      /*
      case MUIA_new_project_{Attribute}:
        data->{Variable} = tag->ti_Data;
      break;
      */
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
    case MUIA_NewProject_Drawer:
    {
      STRPTR drawer;
      get(data->obj_table.drawer, MUIA_String_Contents, &drawer);
      *msg->opg_Storage = (ULONG)drawer;
    }
    return TRUE;
    case MUIA_NewProject_Name:
    {
      STRPTR name;
      get(data->obj_table.name, MUIA_String_Contents, &name);
      *msg->opg_Storage = (ULONG)name;
    }
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
    case MUIM_NewProject_Create:
      return m_Create(cl, obj, msg);
    case MUIM_NewProject_CheckValidity:
      return m_CheckValidity(cl, obj, msg);
    case MUIM_NewProject_CheckDrawer:
      return m_CheckDrawer(cl, obj, msg);
    case MUIM_NewProject_SelectTemplate:
      return m_SelectTemplate(cl, obj, msg);

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_NewProject(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
