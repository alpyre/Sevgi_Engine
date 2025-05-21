/******************************************************************************
 * GamefontCreator                                                            *
 ******************************************************************************/
///Defines
#define MUIA_Enabled 0 //Not a real attribute, just a value to make reading easier

#define FIXED_WIDTH  0
#define PROPORTIONAL 1

#define FILE_ID "GAMEFONT"

#define STATE_NEW     0
#define STATE_EDITED  1
#define STATE_SAVED   2

//private methods
#define MUIM_GamefontCreator_TypeChange       0x80430401
#define MUIM_GamefontCreator_CharChange       0x80430402
#define MUIM_GamefontCreator_WidthChange      0x80430403
#define MUIM_GamefontCreator_StartChange      0x80430404
#define MUIM_GamefontCreator_EndChange        0x80430405
#define MUIM_GamefontCreator_c_widthChange    0x80430406
#define MUIM_GamefontCreator_StripAcknowledge 0x80430407
#define MUIM_GamefontCreator_WindowClose      0x80430408
#define MUIM_GamefontCreator_Reset            0x80430409
#define MUIM_GamefontCreator_Load             0x8043040A
#define MUIM_GamefontCreator_Save             0x8043040B
#define MUIM_GamefontCreator_DisplayStrip     0x8043040C

//private attributes
#define MUIA_GamefontCreator_State            0x8043040A
///
///Includes
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <libraries/asl.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()
#include <datatypes/pictureclass.h>

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "toolbar.h"
#include "popasl_string.h"
#include "integer_gadget.h"
#include "dtpicdisplay.h"
#include "gamefont_creator.h"
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;

extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;

extern Object* App;
extern Object* g_dtPicDisplay;
extern struct MUI_CustomClass *MUIC_PopASLString;
extern struct MUI_CustomClass *MUIC_Integer;
extern struct MUI_CustomClass *MUIC_AckString;
STATIC CONST STRPTR g_gameFontCreator_types[] = {"Fixed Width", "Proportional", NULL};

static struct {
  STRPTR strip;
  STRPTR width;
  STRPTR height;
  STRPTR spacing;
  STRPTR start;
  STRPTR end;
  STRPTR type;
  STRPTR character;
  STRPTR c_width;
  STRPTR c_offset;
}help_string = {
  "The ILBM file which contains character\nimages as a strip without spacing.",
  "In Fixed Width type: the width of each character.\nIn Proportional type: the width of space character.",
  "The height of each character.\nShould be identical to strip height.",
  "The spacing between characters when rendering text.",
  "The first character contained in the strip.",
  "The last character contained in the strip.",
  "Gamefont type.",
  "Select the character to set the width for.",
  "Width of the selected character.",
  "Offset of the selected character from strip start."
};
///
///Structs
struct Character {
  UBYTE width;
  UWORD offset;
};

struct cl_ObjTable
{
  Object* new;
  Object* load;
  Object* save;
  Object* strip;
  Object* strip_btn;
  Object* width;
  Object* height;
  Object* spacing;
  Object* start;
  Object* end;
  Object* type;
  Object* grp_offsets;
  Object* character;
  Object* c_width;
  Object* c_offset;
  Object* create;
  Object* cancel;
};

struct cl_Data
{
  struct cl_ObjTable* obj_table;
  struct Character* characters;
  ULONG state;
};

struct cl_Msg
{
  ULONG MethodID;
  ULONG value;
};

///

//Utility Functions
///getBitMapHeight()
STATIC ULONG getBitMapHeight(STRPTR file)
{
  ULONG height = 0;
  BPTR fh;
  struct BitMapHeader bmhd;

  fh = Open(file, MODE_OLDFILE);
  if (fh) {
    if (locateStrInFile(fh, "FORM")) {
      if (locateStrInFile(fh, "ILBM")) {
        if (locateStrInFile(fh, "BMHD")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct BitMapHeader) == Read(fh, &bmhd, sizeof(struct BitMapHeader))) {
            height = bmhd.bmh_Height;
          }
        }
      }
    }
    Close(fh);
  }

  return height;
}
///
///initiateCharacters(data, width)
STATIC VOID initiateCharacters(struct cl_Data* data, UBYTE width)
{
  ULONG i;
  ULONG start = '!';

  for (i = start; i < 256; i++) {
    data->characters[i].width = width;
  }
}
///
///reinitCharacters(data, start, end, width)
STATIC VOID reinitCharacters(struct cl_Data* data, UBYTE start, UBYTE end, UBYTE width)
{
  LONG i;

  data->characters[start].offset = width;

  for (i = start + 1; i <= end; i++) {
    data->characters[i].offset = data->characters[i - 1].width + data->characters[i - 1].offset;
  }
}

VOID updateCharacters(struct cl_Data* data, UBYTE start, UBYTE end)
{
  LONG i;

  for (i = start + 1; i <= end; i++) {
    data->characters[i].offset = data->characters[i - 1].width + data->characters[i - 1].offset;
  }
}
///

//private methods
///m_TypeChange()
STATIC ULONG m_TypeChange(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->value) {
    case FIXED_WIDTH:
      DoMethod(data->obj_table->grp_offsets, MUIM_Set, MUIA_Disabled, TRUE);
    break;
    case PROPORTIONAL:
    {
      ULONG width = 0;
      ULONG start = '!';
      ULONG end = '!';

      GetAttr(MUIA_Integer_Value, data->obj_table->width, &width);
      GetAttr(MUIA_Integer_Value, data->obj_table->start, &start);
      GetAttr(MUIA_Integer_Value, data->obj_table->end, &end);

      initiateCharacters(data, width);
      reinitCharacters(data, start, end, width);

      DoMethod(data->obj_table->character, MUIM_Set, MUIA_Integer_Min, start);
      DoMethod(data->obj_table->character, MUIM_Set, MUIA_Integer_Max, end);
      DoMethod(data->obj_table->character, MUIM_Set, MUIA_Integer_Value, start);
      DoMethod(data->obj_table->grp_offsets, MUIM_Set, MUIA_Disabled, FALSE);
    }
    break;
  }

  DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_EDITED);

  return 0;
}
///
///m_CharChange()
STATIC ULONG m_CharChange(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  DoMethod(data->obj_table->c_width, MUIM_NoNotifySet, MUIA_Integer_Value, data->characters[msg->value].width);
  DoMethod(data->obj_table->c_offset, MUIM_NoNotifySet, MUIA_Integer_Value, data->characters[msg->value].offset);

  return 0;
}
///
///m_WidthChange()
STATIC ULONG m_WidthChange(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG type = 0;

  GetAttr(MUIA_Cycle_Active, data->obj_table->type, &type);

  if (type == PROPORTIONAL) {
    ULONG width = msg->value;
    ULONG start = '!';
    ULONG end = '!';
    ULONG current = '!';

    GetAttr(MUIA_Integer_Value, data->obj_table->start, &start);
    GetAttr(MUIA_Integer_Value, data->obj_table->end, &end);
    GetAttr(MUIA_Integer_Value, data->obj_table->character, &current);

    reinitCharacters(data, start, end, width);

    DoMethod(data->obj_table->c_offset, MUIM_NoNotifySet, MUIA_Integer_Value, data->characters[current].offset);
  }

  DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_EDITED);

  return 0;
}
///
///m_StartChange()
STATIC ULONG m_StartChange(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG type = 0;
  ULONG start = msg->value;
  ULONG end = 0;

  GetAttr(MUIA_Cycle_Active, data->obj_table->type, &type);
  GetAttr(MUIA_Integer_Value, data->obj_table->end, &end);

  if (end < start) {
    DoMethod(data->obj_table->end, MUIM_NoNotifySet, MUIA_Integer_Value, start);
    end = start;
  }

  if (type == PROPORTIONAL) {
    ULONG width = 0;
    ULONG current = '!';

    GetAttr(MUIA_Integer_Value, data->obj_table->width, &width);
    GetAttr(MUIA_Integer_Value, data->obj_table->character, &current);

    reinitCharacters(data, start, end, width);

    DoMethod(data->obj_table->character, MUIM_Set, MUIA_Integer_Min, start);

    if (current > start) {
      DoMethod(data->obj_table->c_offset, MUIM_Set, MUIA_Integer_Value, data->characters[current].offset);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_EDITED);

  return 0;
}
///
///m_EndChange()
STATIC ULONG m_EndChange(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG type = 0;
  ULONG start = 0;
  ULONG end = msg->value;
  BOOL start_changed = FALSE;

  GetAttr(MUIA_Cycle_Active, data->obj_table->type, &type);
  GetAttr(MUIA_Integer_Value, data->obj_table->start, &start);

  if (start > end) {
    DoMethod(data->obj_table->start, MUIM_NoNotifySet, MUIA_Integer_Value, end);
    start = end;
    start_changed = TRUE;
  }

  if (type == PROPORTIONAL) {
    ULONG width = 0;
    ULONG current = '!';

    GetAttr(MUIA_Integer_Value, data->obj_table->width, &width);
    GetAttr(MUIA_Integer_Value, data->obj_table->character, &current);

    if (start_changed) reinitCharacters(data, start, end, width);

    DoMethod(data->obj_table->character, MUIM_Set, MUIA_Integer_Max, end);

    if (current < end) {
      DoMethod(data->obj_table->c_offset, MUIM_Set, MUIA_Integer_Value, data->characters[current].offset);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_EDITED);

  return 0;
}
///
///m_c_widthChange
STATIC ULONG m_c_widthChange(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  ULONG new_c_width = msg->value;
  ULONG current = 0;
  ULONG end = 0;

  GetAttr(MUIA_Integer_Value, data->obj_table->character, &current);
  GetAttr(MUIA_Integer_Value, data->obj_table->end, &end);

  data->characters[current].width = new_c_width;

  updateCharacters(data, current, end);

  DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_EDITED);

  return 0;
}
///
///m_StripAcknowledge()
STATIC ULONG m_StripAcknowledge(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR strip_file = (STRPTR)msg->value;


  if (strlen(strip_file)) {
    ULONG height = getBitMapHeight(strip_file);
    if (height) {
      DoMethod(data->obj_table->height, MUIM_Set, MUIA_Integer_Value, height);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_EDITED);

  return 0;
}
///
///m_Reset()
STATIC ULONG m_Reset(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->state == STATE_EDITED) {
    if (!MUI_Request(App, obj, NULL, "Gamefont Creator", "*_Discard|_Cancel", "Discard changes?")) {
      return 0;
    }
  }

  DoMethod(data->obj_table->strip,     MUIM_NoNotifySet, MUIA_String_Contents, "");
  DoMethod(data->obj_table->width,     MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table->height,    MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table->spacing,   MUIM_NoNotifySet, MUIA_Integer_Value, 1);
  DoMethod(data->obj_table->start,     MUIM_Set, MUIA_Integer_Value, '!');
  DoMethod(data->obj_table->end,       MUIM_Set, MUIA_Integer_Value, '~');
  DoMethod(data->obj_table->type,      MUIM_Set, MUIA_Cycle_Active, FIXED_WIDTH);
  DoMethod(data->obj_table->character, MUIM_NoNotifySet, MUIA_Integer_Value, '!');
  DoMethod(data->obj_table->c_width,   MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table->c_offset,  MUIM_NoNotifySet, MUIA_Integer_Value, 0);

  DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_NEW);

  return 0;
}
///
///m_WindowClose()
STATIC ULONG m_WindowClose(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->state == STATE_EDITED) {
    if (!MUI_Request(App, obj, NULL, "Gamefont Creator", "*_Quit|_Cancel", "Gamefont is not saved.\nDo you really want to quit?")) {
      return 0;
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Open, FALSE);
  data->state = STATE_NEW;
  DoMethod(obj, MUIM_GamefontCreator_Reset);

  return 0;
}
///
///m_Load()
STATIC ULONG m_Load(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->state == STATE_EDITED) {
    if (!MUI_Request(App, obj, NULL, "Gamefont Creator", "*_Discard|_Cancel", "Current gamefont is not saved.\nDiscard changes?")) {
      return 0;
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

  if (AslRequestTags(g_FileReq, ASLFR_TitleText, "Load a gamefont",
                                ASLFR_PositiveText, "Load",
                                ASLFR_DrawersOnly, FALSE,
                                ASLFR_DoSaveMode, FALSE,
                                ASLFR_DoPatterns, TRUE,
                                ASLFR_InitialFile, "",
                                g_Project.data_drawer ? ASLFR_InitialDrawer : TAG_IGNORE, g_Project.data_drawer,
                                ASLFR_InitialPattern, "*.fnt",
                                TAG_END) && strlen(g_FileReq->fr_File)) {
    STRPTR pathname = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);

    if (pathname) {
      BPTR fh = Open(pathname, MODE_OLDFILE);
      if (fh) {
        UBYTE id[8];
        STRPTR strip_file = NULL;
        struct {
          UBYTE type;
          UBYTE width;
          UBYTE height;
          UBYTE spacing;
          UBYTE start;
          UBYTE end;
        }properties;

        Read(fh, id, 8);
        if (strncmp(id, FILE_ID, 8) == 0) {
          strip_file = readStringFromFile(fh);
          if (strip_file) {
            STRPTR strip_path = makePath(g_FileReq->fr_Drawer, strip_file, NULL);
            if (strip_path) {
              DoMethod(data->obj_table->strip, MUIM_NoNotifySet, MUIA_String_Contents, strip_path);

              Read(fh, &properties, sizeof(properties));

              DoMethod(data->obj_table->width, MUIM_Set, MUIA_Integer_Value, properties.width);
              DoMethod(data->obj_table->height, MUIM_Set, MUIA_Integer_Value, properties.height);
              DoMethod(data->obj_table->spacing, MUIM_Set, MUIA_Integer_Value, properties.spacing);
              DoMethod(data->obj_table->start, MUIM_Set, MUIA_Integer_Value, properties.start);
              DoMethod(data->obj_table->end, MUIM_Set, MUIA_Integer_Value, properties.end);
              DoMethod(data->obj_table->type, MUIM_Set, MUIA_Cycle_Active, properties.type);

              if (properties.type == PROPORTIONAL) {
                UBYTE chars = properties.end - properties.start + 2;
                UWORD *offsets = AllocPooled(g_MemoryPool, sizeof(UWORD) * chars);

                if (offsets) {
                  ULONG i;
                  Read(fh, offsets, sizeof(UWORD) * chars);

                  for (i = 0; i < chars - 1; i++) {
                    data->characters[properties.start + i].width = offsets[i + 1] - offsets[i];
                  }
                  reinitCharacters(data, properties.start, properties.end, properties.width);

                  DoMethod(data->obj_table->c_width, MUIM_NoNotifySet, MUIA_Integer_Value, data->characters[properties.start].width);
                  DoMethod(data->obj_table->c_offset, MUIM_NoNotifySet, MUIA_Integer_Value, data->characters[properties.start].offset);

                  FreePooled(g_MemoryPool, offsets, sizeof(UWORD) * chars);
                }
              }

              DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_SAVED);

              freeString(strip_path);
            }
            freeString(strip_file);
          }
          //else puts("Invalid game font ilbm filename!");
        }
        //else puts("Invalid game font file!");

        Close(fh);
      }

      freeString(pathname);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);

  return 0;
}
///
///m_Save()
STATIC ULONG m_Save(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  STRPTR error_strings[] = {
    "File saved successfully!",
    "Invalid strip file!",
    "Width can not be zero!",
    "Height can not be zero!",
    "Height does not match strip file!",
    "Invalid save file!",
    "File could not be saved!",
    "Strip file could not be copied!",
    "File not saved!"
  };

  LONG error_string = -1;
  BOOL copy_strip = FALSE;
  STRPTR strip = NULL;
  STRPTR initial_name = NULL;
  STRPTR initial_drawer = NULL;
  STRPTR initial_file = NULL;
  STRPTR final_name = NULL;
  STRPTR final_strip = NULL;
  STRPTR final_file = NULL;
  ULONG strip_height;
  ULONG width = 0;
  ULONG height = 0;
  ULONG spacing = 1;
  ULONG start = '!';
  ULONG end = '~';
  ULONG type = FIXED_WIDTH;
  struct {
    UBYTE type;
    UBYTE width;
    UBYTE height;
    UBYTE spacing;
    UBYTE start;
    UBYTE end;
  }properties;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);
  GetAttr(MUIA_String_Contents, data->obj_table->strip,   (ULONG*)&strip);
  GetAttr(MUIA_Integer_Value,   data->obj_table->width,   &width);
  GetAttr(MUIA_Integer_Value,   data->obj_table->height,  &height);
  GetAttr(MUIA_Integer_Value,   data->obj_table->spacing, &spacing);
  GetAttr(MUIA_Integer_Value,   data->obj_table->start,   &start);
  GetAttr(MUIA_Integer_Value,   data->obj_table->end,     &end);
  GetAttr(MUIA_Cycle_Active,    data->obj_table->type,    &type);

  properties.type = type;
  properties.width = width;
  properties.height = height;
  properties.spacing = spacing;
  properties.start = start;
  properties.end = end;

  strip_height = getBitMapHeight(strip);

  //Check values:
  if (!strip_height) { error_string = 1; goto error; }
  if (type == FIXED_WIDTH && width == 0) { error_string = 2; goto error; }
  if (!height) { error_string = 3; goto error; }
  if (height != strip_height) { error_string = 4; goto error;}

  initial_drawer = pathPart(strip);
  initial_name   = stripExtension(strip);
  initial_file   = makeString2(initial_name, ".fnt");
  initial_name   = setString(initial_name, FilePart(initial_name));

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

  if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Save gamefont",
                                    ASLFR_Window, window,
                                    ASLFR_PositiveText, "Save",
                                    ASLFR_DrawersOnly, FALSE,
                                    ASLFR_DoSaveMode, TRUE,
                                    ASLFR_DoPatterns, TRUE,
                                    ASLFR_InitialPattern, "*.fnt",
                                    ASLFR_InitialDrawer, initial_drawer,
                                    ASLFR_InitialFile, FilePart(initial_file),
                                    TAG_END) && strlen(g_FileReq->fr_File)) {
    BPTR fh;
    UWORD final_offset;

    final_name = stripExtension(g_FileReq->fr_File);

    if (strcmp(initial_drawer, g_FileReq->fr_Drawer)) {
      final_strip = makePath(g_FileReq->fr_Drawer, final_name, ".iff");
      copy_strip = TRUE;
    }
    else {
      if (strcmp(final_name, initial_name)) {
        final_strip = makePath(g_FileReq->fr_Drawer, final_name, ".iff");
        copy_strip = TRUE;
      }
      else {
        final_strip = makeString(strip);
      }
    }

    final_file = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);

    if (Exists(final_file)) {
      if (!MUI_Request(App, obj, 0, "Gamefont Creator", "*_Overwrite|_Cancel", "File already exist!\nDo you want to overwrite it?")) {
        error_string = 8; goto error;
      }
    }

    fh = Open(final_file, MODE_NEWFILE);
    if (fh) {
      Write(fh, "GAMEFONT", 8);
      Write(fh, FilePart(final_strip), strlen(FilePart(final_strip)) + 1);
      Write(fh, &properties, sizeof(properties));

      if (type == PROPORTIONAL) {
        ULONG i;
        for (i = start; i <= end; i++) {
          Write(fh, &data->characters[i].offset, sizeof(UWORD));
        }
        i--;
        final_offset = data->characters[i].offset + data->characters[i].width;
        Write(fh, &final_offset, sizeof(UWORD));
      }

      Close(fh);
    }
    else { error_string = 6; goto error; }

    if (copy_strip) {
      if (!CopyFile(strip, final_strip)) {
        DeleteFile(final_file);
        error_string = 7; goto error;
      }
    }
  }

error:
  freeString(initial_name);
  freeString(initial_drawer);
  freeString(initial_file);
  freeString(final_name);
  freeString(final_strip);
  freeString(final_file);

  if (error_string == 0) {
    DoMethod(obj, MUIM_Set, MUIA_GamefontCreator_State, STATE_SAVED);
  }

  if (error_string != -1) {
    MUI_Request(App, obj, 0, "Gamefont Creator", "*_OK", error_strings[error_string]);
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);

  return 0;
}
///
///m_DisplayStrip()
STATIC ULONG m_DisplayStrip(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR strip;

  GetAttr(MUIA_String_Contents, data->obj_table->strip, (ULONG*)&strip);
  if (getBitMapHeight(strip)) {
    DoMethod(g_dtPicDisplay, MUIM_Set, MUIA_DtPicDisplay_Picture, (ULONG)strip);
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
    Object* new;
    Object* load;
    Object* save;
    Object* strip;
    Object* strip_string;
    Object* strip_pop;
    Object* strip_btn;
    Object* width;
    Object* height;
    Object* spacing;
    Object* start;
    Object* end;
    Object* type;
    Object* grp_offsets;
    Object* character;
    Object* c_width;
    Object* c_offset;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','5'),
    MUIA_Window_Title, "Gamefont Creator",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_HorizSpacing, 0,
        MUIA_Group_Child, (objects.new  = MUI_NewImageButton(IMG_SPEC_NEW, "New gamefont", MUIA_Disabled)),
        MUIA_Group_Child, (objects.load = MUI_NewImageButton(IMG_SPEC_LOAD, "Load gamefont", MUIA_Enabled)),
        MUIA_Group_Child, (objects.save = MUI_NewImageButton(IMG_SPEC_SAVE, "Save gamefont", MUIA_Disabled)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
      TAG_END),
      MUIA_Group_Child, MUI_HorizontalSeparator(),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, (objects.strip_btn = MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Strip file:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.strip,
          MUIA_InputMode, MUIV_InputMode_RelVerify,
          MUIA_Frame, MUIV_Frame_Button,
        TAG_END)),
        MUIA_Group_Child, (objects.strip = NewObject(MUIC_PopASLString->mcc_Class, NULL,
          MUIA_PopASLString_Requester, g_FileReq,
          MUIA_PopASLString_IgnoreContents, TRUE,
          MUIA_ShortHelp, help_string.strip,
          ASLFR_TitleText, "Please select gamefont strip ilbm...",
          ASLFR_PositiveText, "Open",
          ASLFR_DrawersOnly, FALSE,
          ASLFR_DoPatterns, TRUE,
          ASLFR_InitialPattern, "#?.(iff|ilbm)",
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Width:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.width, TAG_END),
        MUIA_Group_Child, (objects.width = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.width,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 255,
          MUIA_String_MaxLen, 4,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Height:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.height, TAG_END),
        MUIA_Group_Child, (objects.height = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.height,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 255,
          MUIA_String_MaxLen, 4,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Spacing:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.spacing, TAG_END),
        MUIA_Group_Child, (objects.spacing = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.spacing,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 1,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 255,
          MUIA_String_MaxLen, 4,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Start:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.start, TAG_END),
        MUIA_Group_Child, (objects.start = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.start,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_ASCII, TRUE,
          MUIA_Integer_Value, '!',
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "End:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.end, TAG_END),
        MUIA_Group_Child, (objects.end = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.end,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_ASCII, TRUE,
          MUIA_Integer_Value, '~',
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Type:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.type, TAG_END),
        MUIA_Group_Child, (objects.type = MUI_NewObject(MUIC_Cycle,
          MUIA_ShortHelp, help_string.type,
          MUIA_Cycle_Active, 0,
          MUIA_Cycle_Entries, g_gameFontCreator_types,
        TAG_END)),
      TAG_END),
      MUIA_Group_Child, (objects.grp_offsets = MUI_NewObject(MUIC_Group,
        MUIA_Disabled, TRUE,
        MUIA_Frame, MUIV_Frame_Group,
        MUIA_FrameTitle, "Character Sizes and Offsets",
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Character:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.character, TAG_END),
        MUIA_Group_Child, (objects.character = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.character,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_ASCII, TRUE,
          MUIA_Integer_Button_Inc_Text, ">",
          MUIA_Integer_Button_Dec_Text, "<",
          MUIA_Integer_Buttons_Inverse, TRUE,
          MUIA_Integer_Value, '!',
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "width:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.c_width, TAG_END),
        MUIA_Group_Child, (objects.c_width = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.c_width,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 255,
          MUIA_String_MaxLen, 4,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "offset:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.c_offset, TAG_END),
        MUIA_Group_Child, (objects.c_offset = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.c_offset,
          MUIA_Integer_Input, FALSE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, FALSE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 65536,
          MUIA_String_MaxLen, 4,
        TAG_END)),
      TAG_END)),
    TAG_END),
  TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    if ((data->obj_table = AllocPooled(g_MemoryPool, sizeof(struct cl_ObjTable)))) {
      get(objects.strip, MUIA_PopASLString_StringObject, &objects.strip_string);
      get(objects.strip, MUIA_PopASLString_PopButton, &objects.strip_pop);
      data->obj_table->new = objects.new;
      data->obj_table->load = objects.load;
      data->obj_table->save = objects.save;
      data->obj_table->strip = objects.strip;
      data->obj_table->strip_btn = objects.strip_btn;
      data->obj_table->width = objects.width;
      data->obj_table->height = objects.height;
      data->obj_table->spacing = objects.spacing;
      data->obj_table->start = objects.start;
      data->obj_table->end = objects.end;
      data->obj_table->type = objects.type;
      data->obj_table->grp_offsets = objects.grp_offsets;
      data->obj_table->character = objects.character;
      data->obj_table->c_width = objects.c_width;
      data->obj_table->c_offset = objects.c_offset;
      data->state = STATE_NEW;

      if ((data->characters = (struct Character*)AllocPooled(g_MemoryPool, sizeof(struct Character) * 256))) {
        DoMethod(obj, MUIM_Window_SetCycleChain, objects.strip_string,
                                                 objects.strip_pop,
                                                 MUI_GetChild(objects.width, 1),
                                                 MUI_GetChild(objects.height, 1),
                                                 MUI_GetChild(objects.spacing, 1),
                                                 MUI_GetChild(objects.start, 1),
                                                 MUI_GetChild(objects.end, 1),
                                                 objects.type,
                                                 NULL);

        DoMethod(objects.new, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
          MUIM_GamefontCreator_Reset);

        DoMethod(objects.load, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
          MUIM_GamefontCreator_Load);

        DoMethod(objects.save, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
          MUIM_GamefontCreator_Save);

        DoMethod(objects.type, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 3,
          MUIM_GamefontCreator_TypeChange, MUIV_TriggerValue);

        DoMethod(objects.character, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
          MUIM_GamefontCreator_CharChange, MUIV_TriggerValue);

        DoMethod(objects.width, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
          MUIM_GamefontCreator_WidthChange, MUIV_TriggerValue);

        DoMethod(objects.height, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
          MUIM_Set, MUIA_GamefontCreator_State, STATE_EDITED);

        DoMethod(objects.spacing, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
          MUIM_Set, MUIA_GamefontCreator_State, STATE_EDITED);

        DoMethod(objects.start, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
          MUIM_GamefontCreator_StartChange, MUIV_TriggerValue);

        DoMethod(objects.end, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
          MUIM_GamefontCreator_EndChange, MUIV_TriggerValue);

        DoMethod(objects.c_width, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
          MUIM_GamefontCreator_c_widthChange, MUIV_TriggerValue);

        DoMethod(objects.strip, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 3,
          MUIM_GamefontCreator_StripAcknowledge, MUIV_TriggerValue);

        DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 1,
          MUIM_GamefontCreator_WindowClose);

        DoMethod(objects.strip_btn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
          MUIM_GamefontCreator_DisplayStrip);

        return((ULONG) obj);
      }
    }

    CoerceMethod(cl, obj, OM_DISPOSE);
  }

return (ULONG) NULL;
}
///
///Overridden OM_DISPOSE
static ULONG m_Dispose(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->characters) FreePooled(g_MemoryPool, data->characters, sizeof(struct Character) * 256);
  if (data->obj_table) FreePooled(g_MemoryPool, data->obj_table, sizeof(struct cl_ObjTable));

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
      case MUIA_GamefontCreator_State:
        if (data->state != tag->ti_Data) {
          switch (tag->ti_Data) {
            case STATE_NEW:
              DoMethod(data->obj_table->new, MUIM_Set, MUIA_Disabled, TRUE);
              DoMethod(data->obj_table->save, MUIM_Set, MUIA_Disabled, TRUE);
            break;
            case STATE_EDITED:
              DoMethod(data->obj_table->new, MUIM_Set, MUIA_Disabled, FALSE);
              DoMethod(data->obj_table->save, MUIM_Set, MUIA_Disabled, FALSE);
            break;
            case STATE_SAVED:
              DoMethod(data->obj_table->new, MUIM_Set, MUIA_Disabled, FALSE);
              DoMethod(data->obj_table->save, MUIM_Set, MUIA_Disabled, TRUE);
            break;
          }
          data->state = tag->ti_Data;
        }
      break;
      case MUIA_GamefontCreator_AssetsDrawer:
        DoMethod(data->obj_table->strip, MUIM_Set, ASLFR_InitialDrawer, tag->ti_Data);
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
    case MUIA_GamefontCreator_{Attribute}:
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
    case MUIM_GamefontCreator_TypeChange:
      return m_TypeChange(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_CharChange:
      return m_CharChange(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_WidthChange:
      return m_WidthChange(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_StartChange:
      return m_StartChange(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_EndChange:
      return m_EndChange(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_c_widthChange:
      return m_c_widthChange(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_StripAcknowledge:
      return m_StripAcknowledge(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_WindowClose:
      return m_WindowClose(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_Reset:
      return m_Reset(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_Load:
      return m_Load(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_Save:
      return m_Save(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GamefontCreator_DisplayStrip:
      return m_DisplayStrip(cl, obj, (struct cl_Msg*) msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_GamefontCreator(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
