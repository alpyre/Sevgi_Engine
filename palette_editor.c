/******************************************************************************
 * PaletteEditor                                                              *
 ******************************************************************************/
///Defines
#define MUIM_PaletteEditor_Update_Size 0x80430501
#define MUIM_PaletteEditor_New_Palette 0x80430502
#define MUIM_PaletteEditor_Rem_Palette 0x80430503
#define MUIM_PaletteEditor_Chg_Palette 0x80430504
#define MUIM_PaletteEditor_Update_Name 0x80430505
#define MUIM_PaletteEditor_Import_ILBM 0x80430506
#define MUIM_PaletteEditor_Move        0x80430507
#define MUIM_PaletteEditor_LoadAs      0x80430508
#define MUIM_PaletteEditor_SaveAs      0x80430509
#define MUIM_PaletteEditor_Reset       0x8043050A

#define MUIV_PaletteEditor_Up   0x01
#define MUIV_PaletteEditor_Down 0x02
///
///Includes
#include <stdio.h>            // <-- For sprintf()
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/utility.h>    // <-- Required for tag redirection
#include <libraries/asl.h>
#include <datatypes/pictureclass.h> // <-- For BitMapHeader struct

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "toolbar.h"
#include "integer_gadget.h"
#include "color_palette.h"
#include "palette_editor.h"
///
///Structs
struct cl_ObjTable
{
  Object* new;
  Object* load;
  Object* save;
  Object* undo;
  Object* redo;
  Object* listview;
  Object* list_new;
  Object* list_import;
  Object* list_remove;
  Object* list_move_up;
  Object* list_move_down;
  Object* color_palette;
  Object* color_adjust;
  Object* name;
  Object* size;
  Object* selected;
  Object* edit_group;
};

struct PaletteItem {
  STRPTR name;
  UBYTE* palette;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  struct PaletteItem* old_palette_item;
  BOOL edited;
};

struct cl_Msg
{
  ULONG MethodID;
  ULONG value;
};

struct cl_Msg2
{
  ULONG MethodID;
  STRPTR filename;
};
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;

extern Object* App;
extern struct MUI_CustomClass* MUIC_Integer;
extern struct MUI_CustomClass* MUIC_ColorPalette;
extern UBYTE default_palette[];

extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;
///
///Hooks
HOOKPROTO(plt_item_dispfunc, LONG, char **array, struct PaletteItem* p)
{
  if (p) *array = p->name;
  return(0);
}
MakeStaticHook(plt_item_disphook, plt_item_dispfunc);
///
///Utility
STRPTR newPaletteName(struct cl_Data* data)
{
  UBYTE buffer[16];
  STRPTR name_str = "palette";
  ULONG list_size = 0;

  get(data->obj_table.listview, MUIA_List_Entries, &list_size);
  sprintf(buffer, "%s_%lu", name_str, list_size);

  return makeString(buffer);

}

VOID replacePaletteName(struct PaletteItem* p, STRPTR name)
{
  if (p) {
    freeString(p->name);
    p->name = makeString(name);
  }
}

UBYTE* newPalette()
{
  UBYTE* palette = AllocPooled(g_MemoryPool, DEFAULT_PALLETTE_SIZE * 3 + 1);

  if (palette) {
    palette[0] = DEFAULT_PALLETTE_SIZE - 1;
    memcpy(&palette[1], &default_palette[1], DEFAULT_PALLETTE_SIZE * 3);
  }

  return palette;
}

UBYTE* newPaletteFromILBM(STRPTR filename)
{
  UBYTE* palette = NULL;
  struct BitMapHeader bmhd;
  BPTR fh;

  fh = Open(filename, MODE_OLDFILE);
  if (fh) {
    if (locateStrInFile(fh, "FORM")) {
      if (locateStrInFile(fh, "ILBM")) {
        if (locateStrInFile(fh, "BMHD")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct BitMapHeader) == Read(fh, &bmhd, sizeof(struct BitMapHeader))) {
            ULONG num_colors = 1 << bmhd.bmh_Depth;

            if (locateStrInFile(fh, "CMAP")) {
              Seek(fh, 4, OFFSET_CURRENT);
              palette = AllocPooled(g_MemoryPool, num_colors * 3 + 1);
              if (palette) {
                palette[0] = num_colors - 1;
                Read(fh, &palette[1], num_colors * 3);
              }
            }
          }
        }
      }
    }

    Close(fh);
  }

  return palette;
}

struct PaletteItem* newPaletteItem(struct cl_Data* data)
{
  struct PaletteItem* p = AllocPooled(g_MemoryPool, sizeof(struct PaletteItem));

  if (p) {
    p->name = newPaletteName(data);
    p->palette = newPalette();
  }

  return p;
}

struct PaletteItem* newPaletteItemFromILBM(STRPTR filepath)
{
  struct PaletteItem* p = NULL;
  STRPTR name = stripExtension(FilePart(filepath));

  if (name) {
    UBYTE* palette;

    replaceChars(name, NON_ALPHANUMERIC " ", '_');
    if (*name >= '0' && *name <= '9') {
      STRPTR under_scored = makeString2("_", name);
      freeString(name);

      if (under_scored) name = under_scored;
      else return p;
    }

    palette = newPaletteFromILBM(filepath);
    if (palette) {
      p = AllocPooled(g_MemoryPool, sizeof(struct PaletteItem));
      if (p) {
        p->name = name;
        p->palette = palette;
      }
      else {
        freePalette(palette);
        freeString(name);
      }
    }
    else freeString(name);
  }

  return p;
}

VOID freePaletteItem(struct PaletteItem* p)
{
  if (p) {
    if (p->name) freeString(p->name);
    if (p->palette) freePalette(p->palette);

    FreePooled(g_MemoryPool, p, sizeof(struct PaletteItem));
  }
}

VOID updateActivePalette(struct cl_Data* data)
{
  struct PaletteItem* p;

  DoMethod(data->obj_table.listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (APTR)&p);

  if (p) {
    UBYTE* new_palette;
    freePalette(p->palette);

    get(data->obj_table.color_palette, MUIA_ColorPalette_Palette, &new_palette);
    p->palette = new_palette;
  }
}

VOID save_Palettes_File(STRPTR path, ULONG entries, struct cl_Data* data)
{
  BPTR fh = Open(path, MODE_READWRITE);

  if (fh) {
    ULONG i;

    updateActivePalette(data);

    for (i = 0; i < entries; i++) {
      UBYTE buffer[24];
      struct PaletteItem* p;
      ULONG num_hues;
      ULONG h;

      DoMethod(data->obj_table.listview, MUIM_List_GetEntry, i, &p);
      num_hues = (p->palette[0] + 1) * 3;

      if (i > 0) Write(fh, "\n", 1);
      Write(fh, "STATIC UBYTE ", 13);
      Write(fh, p->name, strlen(p->name));
      Write(fh, "[] = {", 6);
      sprintf(buffer, "%lu", (ULONG)p->palette[0]);
      Write(fh, buffer, strlen(buffer));
      for (h = 0; h < num_hues; h += 3) {
        sprintf(buffer, ",\n%3lu, %3lu, %3lu", (ULONG)p->palette[h + 1], (ULONG)p->palette[h + 2], (ULONG)p->palette[h + 3]);
        Write(fh, buffer, strlen(buffer));
      }
      Write(fh, "};\n", 3);
    }
    SetFileSize(fh, 0, OFFSET_CURRENT);
    Close(fh);
  }
}

STRPTR read_Name(BPTR fh)
{
  UBYTE buf[64] = {0};
  UBYTE* ch = buf - 1;
  ULONG name_len;
  STRPTR name = 0;

  do {
    ch++;
    Read(fh, ch, 1);
  } while(*ch != '[');

  name_len = (ULONG)ch - (ULONG)buf;
  name = AllocPooled(g_MemoryPool, name_len + 1);
  if (name) {
    memcpy(name, buf, name_len);
  }

  return name;
}

LONG read_Number(BPTR fh)
{
  UBYTE buf[64] = {0};
  UBYTE* ch = buf - 1;
  LONG number = 0;

  do {
    ch++;
skip:
    Read(fh, ch, 1);
    if (*ch == ' ' || *ch == 9 || *ch == 10 || *ch == 13) goto skip;
  } while(*ch != ',' && *ch != '}');

  *ch = 0;

  aToi(buf, &number);

  return number;
}

/******************************************************************************
 * WARNING: This function and the functions above are written in a great      *
 * hurry, so there are all sorts of vulnerabilities if the .h file has        *
 * unexpected form or values.                                                 *                                                             *
 ******************************************************************************/
VOID load_Palettes_File(STRPTR path, struct cl_Data* data)
{
  BPTR fh = Open(path, MODE_OLDFILE);

  if (fh) {
    while (locateStrInFile(fh, "STATIC UBYTE ")) {
      struct PaletteItem* palette_item = NULL;
      UBYTE* palette = NULL;
      STRPTR name = read_Name(fh);


      if (locateStrInFile(fh, "{")) {
        ULONG num_colors = read_Number(fh) + 1;

        palette = AllocPooled(g_MemoryPool, num_colors * 3 + 1);
        if (palette) {
          ULONG h;
          ULONG hues = num_colors * 3;
          palette[0] = num_colors - 1;

          for (h = 1; h <= hues; h++) {
            palette[h] = read_Number(fh);
          }
        }
      }

      if (palette) {
        palette_item = AllocPooled(g_MemoryPool, sizeof(struct PaletteItem));
        if (palette_item) {
          palette_item->name = name;
          palette_item->palette = palette;

          DoMethod(data->obj_table.listview, MUIM_List_InsertSingle, palette_item, MUIV_List_Insert_Bottom);
          DoMethod(data->obj_table.edit_group, MUIM_Set, MUIA_Disabled, FALSE);
          DoMethod(data->obj_table.new, MUIM_Set, MUIA_Disabled, FALSE);
        }
      }
    }

    Close(fh);
  }
}
///

//<DEFINE SUBCLASS METHODS HERE>
///m_Update_Size(cl, obj, msg)
ULONG m_Update_Size(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG size;

  get(data->obj_table.color_palette, MUIA_ColorPalette_NumColors, &size);
  DoMethod(data->obj_table.size, MUIM_NoNotifySet, MUIA_Integer_Value, size);
  DoMethod(data->obj_table.selected, MUIM_Set, MUIA_Integer_Max, size + 1);

  return 0;
}
///
///m_Update_Name(cl, obj, msg)
ULONG m_Update_Name(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR new_name;
  struct PaletteItem* p;

  DoMethod(data->obj_table.listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (APTR)&p);

  if (p) {
    get(data->obj_table.name, MUIA_String_Contents, &new_name);
    if (new_name && strlen(new_name) && !(*new_name >= '0' && *new_name <= '9')) {
      replacePaletteName(p, new_name);
      DoMethod(data->obj_table.listview, MUIM_List_Redraw, MUIV_List_Redraw_Active);
      DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, TRUE);
    }
    else {
      DoMethod(data->obj_table.name, MUIM_Set, MUIA_String_Contents, p->name);
    }
  }

  return 0;
}
///
///m_New_Palette(cl, obj, msg)
ULONG m_New_Palette(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct PaletteItem* p = newPaletteItem(data);

  if (p) {
    DoMethod(data->obj_table.listview, MUIM_List_InsertSingle, p, MUIV_List_Insert_Bottom);
    DoMethod(data->obj_table.listview, MUIM_Set, MUIA_List_Active, MUIV_List_Active_Bottom);
    DoMethod(data->obj_table.listview, MUIM_List_Redraw, MUIV_List_Redraw_All);
    DoMethod(data->obj_table.edit_group, MUIM_Set, MUIA_Disabled, FALSE);
    DoMethod(data->obj_table.new, MUIM_Set, MUIA_Disabled, FALSE);
    DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, TRUE);
  }

  return 0;
}
///
///m_Remove_Palette(cl, obj, msg)
ULONG m_Remove_Palette(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct PaletteItem* p;
  LONG entries;

  DoMethod(data->obj_table.listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (APTR)&p);

  if (p) {
    data->old_palette_item = NULL;
    DoMethod(data->obj_table.listview, MUIM_List_Remove, MUIV_List_Remove_Selected);
    freePaletteItem(p);
    get(data->obj_table.listview, MUIA_List_Entries, &entries);
    if (!entries) {
      DoMethod(data->obj_table.edit_group, MUIM_Set, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.new, MUIM_Set, MUIA_Disabled, TRUE);
      DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);
    }
    else {
      DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, TRUE);
    }
  }

  return 0;
}
///
///m_Move_Palette(cl, obj, msg)
ULONG m_Move_Palette(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG entries;
  ULONG active;

  get(data->obj_table.listview, MUIA_List_Entries, &entries);
  get(data->obj_table.listview, MUIA_List_Active, &active);

  if (entries > 1) {
    switch (msg->value) {
      case MUIV_PaletteEditor_Up:
        if (active > 0) {
          DoMethod(data->obj_table.listview, MUIM_List_Move, MUIV_List_Move_Active, MUIV_List_Move_Previous);
          DoMethod(data->obj_table.listview, MUIM_Set, MUIA_List_Active, active - 1);
          DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, TRUE);
        }
      break;
      case MUIV_PaletteEditor_Down:
      if (active < entries - 1) {
        DoMethod(data->obj_table.listview, MUIM_List_Move, MUIV_List_Move_Active, MUIV_List_Move_Next);
        DoMethod(data->obj_table.listview, MUIM_Set, MUIA_List_Active, active + 1);
        DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, TRUE);
      }
      break;
    }
  }

  return 0;
}
///
///m_Change_Palette(cl, obj, msg)
/******************************************************************************
 * Designed to be called when the user selects another palette from the       *
 * listview. It frees the palette of the previously selected palette_item and *
 * creates a new palette for it with the current state of the color_palette.  *
 * Then loads the palette of the newly selected palette_item into the         *
 * color_palette.                                                             *
 ******************************************************************************/
ULONG m_Change_Palette(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct PaletteItem* p;

  DoMethod(data->obj_table.listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (APTR)&p);

  if (data->old_palette_item != p) {
    if (data->old_palette_item) {
      UBYTE* new_palette;
      freePalette(data->old_palette_item->palette);

      get(data->obj_table.color_palette, MUIA_ColorPalette_Palette, &new_palette);
      data->old_palette_item->palette = new_palette;
    }

    if (p) {
      DoMethod(data->obj_table.color_palette, MUIM_Set, MUIA_ColorPalette_Palette, p->palette);
      DoMethod(data->obj_table.name, MUIM_Set, MUIA_String_Contents, p->name);

      data->old_palette_item = p;
    }
    else data->old_palette_item = NULL;
  }

  return 0;
}
///
///m_Import_ILBM(cl, obj, msg)
ULONG m_Import_ILBM(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct PaletteItem* p;
  STRPTR path = NULL;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);
  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

  if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Import ILBM palette",
                                    ASLFR_Window, window,
                                    ASLFR_PositiveText, "Import",
                                    ASLFR_DrawersOnly, FALSE,
                                    ASLFR_DoSaveMode, FALSE,
                                    ASLFR_DoPatterns, TRUE,
                                    ASLFR_InitialPattern, "#?.(iff|ilbm|pic|bsh)",
                                    ASLFR_InitialDrawer, g_Project.assets_drawer,
                                    ASLFR_InitialFile, "",
                                    TAG_END) && strlen(g_FileReq->fr_File)) {
    path = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);
    if (path) {
      p = newPaletteItemFromILBM(path);
      if (p) {
        DoMethod(data->obj_table.listview, MUIM_List_InsertSingle, p, MUIV_List_Insert_Bottom);
        DoMethod(data->obj_table.listview, MUIM_Set, MUIA_List_Active, MUIV_List_Active_Bottom);
        DoMethod(data->obj_table.listview, MUIM_List_Redraw, MUIV_List_Redraw_All);
        DoMethod(data->obj_table.edit_group, MUIM_Set, MUIA_Disabled, FALSE);
        DoMethod(data->obj_table.new, MUIM_Set, MUIA_Disabled, FALSE);
        DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, TRUE);
      }

      freeString(path);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);

  return 0;
}
///
///m_Load(cl, obj, msg)
ULONG m_Load(struct IClass* cl, Object* obj, struct cl_Msg2* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (msg->filename) {
    ULONG entries;

    get(data->obj_table.listview, MUIA_List_Entries, &entries);

    if (entries) {
      //WARNING! Does not inform the user for their non saved palette files!
      //NOTE: This is covered in main.c
      DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);
      DoMethod(obj, MUIM_PaletteEditor_Reset);
    }

    load_Palettes_File(msg->filename, data);

    DoMethod(data->obj_table.listview, MUIM_Set, MUIA_List_Active, 0);
    DoMethod(data->obj_table.listview, MUIM_List_Redraw, MUIV_List_Redraw_All);
    DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);
  }

  return 0;
}
///
///m_Save(cl, obj, msg)
ULONG m_Save(struct IClass* cl, Object* obj, struct cl_Msg2* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG entries;

  get(data->obj_table.listview, MUIA_List_Entries, &entries);

  if (entries) {
    save_Palettes_File(msg->filename, entries, data);
    DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);
  }

  return 0;
}
///
///m_Load_As(cl, obj, msg)
ULONG m_Load_As(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG entries;
  STRPTR path;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);
  get(data->obj_table.listview, MUIA_List_Entries, &entries);

  if (entries) {
    if (data->edited) {
      if (DoMethod(obj, MUIM_PaletteEditor_Reset)) {
        return 0;
      }
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

  if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Load palettes file",
                                    ASLFR_Window, window,
                                    ASLFR_PositiveText, "Load",
                                    ASLFR_DrawersOnly, FALSE,
                                    ASLFR_DoSaveMode, FALSE,
                                    ASLFR_DoPatterns, TRUE,
                                    ASLFR_InitialPattern, "#?.h",
                                    ASLFR_DoSaveMode, FALSE,
                                    ASLFR_InitialFile, "palettes.h",
                                    ASLFR_InitialDrawer, g_Project.directory,
                                    TAG_END) && strlen(g_FileReq->fr_File)) {
    path = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);
    if (path) {
      DoMethod(obj, MUIM_PaletteEditor_Reset);
      load_Palettes_File(path, data);

      DoMethod(data->obj_table.listview, MUIM_Set, MUIA_List_Active, 0);
      DoMethod(data->obj_table.listview, MUIM_List_Redraw, MUIV_List_Redraw_All);
      DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);

      freeString(path);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);

  return 0;
}
///
///m_Save_As(cl, obj, msg)
ULONG m_Save_As(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG entries;
  ULONG edited;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);
  get(obj, MUIA_PaletteEditor_Edited, &edited);
  get(data->obj_table.listview, MUIA_List_Entries, &entries);

  if (entries) {
    STRPTR path;

    if (edited) {
      struct PaletteItem* p;

      DoMethod(data->obj_table.listview, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &p);
      if (p) {
        UBYTE* new_palette;
        freePalette(p->palette);

        get(data->obj_table.color_palette, MUIA_ColorPalette_Palette, &new_palette);
        data->old_palette_item->palette = new_palette;
      }
    }

    DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

    if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Save palettes file",
                                      ASLFR_Window, window,
                                      ASLFR_PositiveText, "Save",
                                      ASLFR_DrawersOnly, FALSE,
                                      ASLFR_DoPatterns, TRUE,
                                      ASLFR_DoSaveMode, TRUE,
                                      ASLFR_InitialPattern, "#?.h",
                                      ASLFR_InitialFile, "palettes.h",
                                      TAG_END) && strlen(g_FileReq->fr_File)) {
      path = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);
      if (path) {
        if (Exists(path)) {
          if (!MUI_Request(App, obj, NULL, "Palette Editor", "*_Overwrite|_Cancel", "Palettes file already exists!")) {
            goto cancel;
          }
        }

        save_Palettes_File(path, entries, data);
        DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);

cancel:
        freeString(path);
      }
    }

    DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);
  }

  return 0;
}
///
///m_Reset_Palettes(cl, obj, msg)
ULONG m_Reset_Palettes(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG entries;

  get(data->obj_table.listview, MUIA_List_Entries, &entries);

  if (entries) {
    struct PaletteItem* p;
    LONG i;

    if (data->edited) {
      if (!MUI_Request(App, obj, NULL, "Warning!", "*_Discard|_Cancel", "You have unsaved changes on the current palette(s)!\nDo you really want to discard them?")) {
        return 1;
      }
    }

    for (i = entries - 1; i >= 0; i--) {
      DoMethod(data->obj_table.listview, MUIM_List_GetEntry, i, &p);
      DoMethod(data->obj_table.listview, MUIM_List_Remove, i);
      freePaletteItem(p);
    }

    DoMethod(data->obj_table.edit_group, MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(data->obj_table.new, MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);

    data->old_palette_item = NULL;
  }

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct {
    Object* new;
    Object* load;
    Object* save;
    Object* undo;
    Object* redo;
    Object* listview;
    Object* list_new;
    Object* list_import;
    Object* list_remove;
    Object* list_move_up;
    Object* list_move_down;
    Object* color_palette;
    Object* color_adjust;
    Object* name;
    Object* size;
    Object* selected;
    Object* edit_group;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_Width, MUIV_Window_Width_Visible(50),
    MUIA_Window_Height, MUIV_Window_Height_Visible(40),
    MUIA_Window_ID, MAKE_ID('S','V','G','4'),
    MUIA_Window_Title, "Palette Editor",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_HorizSpacing, 0,
        MUIA_Group_Child, (objects.new  = MUI_NewImageButton(IMG_SPEC_NEW,  "New palettes file",  MUIA_Disabled)),
        MUIA_Group_Child, (objects.load = MUI_NewImageButton(IMG_SPEC_LOAD, "Load palettes file...", MUIA_Enabled)),
        MUIA_Group_Child, (objects.save = MUI_NewImageButton(IMG_SPEC_SAVEAS, "Save palettes file as...", MUIA_Disabled)),
        MUIA_Group_Child, MUI_NewImageButtonSeparator(),
        MUIA_Group_Child, (objects.undo = MUI_NewImageButton(IMG_SPEC_UNDO, "Undo recent edits\n(NOT IMPLEMENTED)", MUIA_Disabled)),
        MUIA_Group_Child, (objects.redo = MUI_NewImageButton(IMG_SPEC_REDO, "Redo recent edits\n(NOT IMPLEMENTED)", MUIA_Disabled)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
      TAG_END),
      MUIA_Group_Child, MUI_HorizontalSeparator(),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_FrameTitle, "Palettes",
          MUIA_Group_Child, (objects.listview = MUI_NewObject(MUIC_Listview,
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
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_HorizSpacing, 2,
            MUIA_Group_Child, (objects.list_new       = MUI_NewButton("New", NULL, "Create new palette")),
            MUIA_Group_Child, (objects.list_import    = MUI_NewButton("Import", NULL, "Import palette from an ILBM file")),
            MUIA_Group_Child, (objects.list_remove    = MUI_NewButton("Remove", NULL, "Remove selected palette")),
            MUIA_Group_Child, (objects.list_move_up   = MUI_NewButton("Up", NULL, "Move selected palette up")),
            MUIA_Group_Child, (objects.list_move_down = MUI_NewButton("Down", NULL, "Move selected palette down")),
          TAG_END),
        TAG_END),
        MUIA_Group_Child, (objects.edit_group = MUI_NewObject(MUIC_Group,
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_FrameTitle, "Edit Palette",
          MUIA_Group_Horiz, TRUE,
          MUIA_Disabled, TRUE,
          MUIA_Group_Child, (objects.color_palette = NewObject(MUIC_ColorPalette->mcc_Class, NULL,
            MUIA_HorizWeight, 190,
            MUIA_ShortHelp, "Colors on the current palette",
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_HorizWeight, 380,
            MUIA_Group_Child, MUI_NewObject(MUIC_Group,
              MUIA_Frame, MUIV_Frame_ReadList,
              MUIA_Group_Columns, 2,
              MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Name:", MUIA_HorizWeight, 0, TAG_END),
              MUIA_Group_Child, (objects.name = MUI_NewObject(MUIC_String,
                MUIA_Frame, MUIV_Frame_String,
                MUIA_String_Accept, ALPHANUMERIC "_",
                MUIA_ShortHelp, "Name for the UBYTE palette array",
              TAG_END)),
              MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Size:", MUIA_HorizWeight, 0, TAG_END),
              MUIA_Group_Child, (objects.size = NewObject(MUIC_Integer->mcc_Class, NULL,
                MUIA_Integer_PaletteSize, TRUE,
                MUIA_Integer_Value, DEFAULT_PALLETTE_SIZE,
                MUIA_ShortHelp, "Number of colors on current palette",
              TAG_END)),
              MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Selected:", MUIA_HorizWeight, 0, TAG_END),
              MUIA_Group_Child, (objects.selected = NewObject(MUIC_Integer->mcc_Class, NULL,
                MUIA_Integer_Value, 0,
                MUIA_Integer_Min, 0,
                MUIA_Integer_Max, 7,
                MUIA_Integer_Incr, 1,
                MUIA_Integer_Buttons, TRUE,
                MUIA_Integer_Buttons_Inverse, TRUE,
                MUIA_Integer_Button_Inc_Text, ">",
                MUIA_Integer_Button_Dec_Text, "<",
                MUIA_Integer_Input, FALSE,
                MUIA_ShortHelp, "Index of the currently selected color",
              TAG_END)),
            TAG_END),
            MUIA_Group_Child, (objects.color_adjust = MUI_NewObject(MUIC_Coloradjust,
              MUIA_ShortHelp, "Edit selected color",
            TAG_END)),
          TAG_END),
        TAG_END)),
      TAG_END),
    TAG_END),
  TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    //selecting a color on color palette should set the selected integer_gadget
    DoMethod(objects.color_palette, MUIM_Notify, MUIA_ColorPalette_Selected, MUIV_EveryTime, objects.selected, 3,
      MUIM_NoNotifySet, MUIA_Integer_Value, MUIV_TriggerValue);
    //selecting a color on selected integer_gadget should select the color on the color_palette
    DoMethod(objects.selected, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, objects.color_palette, 3,
      MUIM_Set, MUIA_ColorPalette_Selected, MUIV_TriggerValue);

    //palette resize
    DoMethod(objects.size, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, objects.color_palette, 3,
      MUIM_Set, MUIA_ColorPalette_NumColors, MUIV_TriggerValue);
    //setting a new palette into color_palette should update size gadget
    DoMethod(objects.color_palette, MUIM_Notify, MUIA_ColorPalette_NumColors, MUIV_EveryTime, obj, 1,
      MUIM_PaletteEditor_Update_Size);

    //Adjusting the color on color_adjust should update the color on color_palette
    DoMethod(objects.color_adjust, MUIM_Notify, MUIA_Coloradjust_RGB, MUIV_EveryTime, objects.color_palette, 3,
      MUIM_NoNotifySet, MUIA_ColorPalette_RGB, MUIV_TriggerValue);
    //Changing the selected color on color_palette should update the color on color_adjust
    DoMethod(objects.color_palette, MUIM_Notify, MUIA_ColorPalette_RGB, MUIV_EveryTime, objects.color_adjust, 3,
      MUIM_NoNotifySet, MUIA_Coloradjust_RGB, MUIV_TriggerValue);

    //Adding/Removing palettes into palette listview
    DoMethod(objects.list_new, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_PaletteEditor_New_Palette);
    DoMethod(objects.list_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_PaletteEditor_Rem_Palette);
    //Changing the active palette on the listview should update color_palette
    DoMethod(objects.listview, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MUIM_PaletteEditor_Chg_Palette);
    //Changing the name string should update the name on the listview
    DoMethod(objects.name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 1, MUIM_PaletteEditor_Update_Name);
    //Clicking Import button should call m_Import_ILBM() method
    DoMethod(objects.list_import, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_PaletteEditor_Import_ILBM);
    //Load Button
    DoMethod(objects.load, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_PaletteEditor_LoadAs);
    //Save Button
    DoMethod(objects.save, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_PaletteEditor_SaveAs);

    //Move selected palette_item on the listview
    DoMethod(objects.list_move_up, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_PaletteEditor_Move, MUIV_PaletteEditor_Up);
    DoMethod(objects.list_move_down, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2,
      MUIM_PaletteEditor_Move, MUIV_PaletteEditor_Down);

    //NOTE: The following notification is a workaround for a bug in...
    //      ...MUIC_Coloradjust object not respecting MUIM_NoNotifySet
    //Manually adjusting the color on color_adjust should update edited state
    DoMethod(objects.color_palette, MUIM_Notify, MUIA_ColorPalette_Edited, TRUE, obj, 3,
      MUIM_Set, MUIA_PaletteEditor_Edited, TRUE);

    //Changing pallette size manually should update edited state
    DoMethod(objects.size, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_Set, MUIA_PaletteEditor_Edited, TRUE);
    //State of edited should disable/enable save button
    DoMethod(obj, MUIM_Notify, MUIA_PaletteEditor_Edited, MUIV_EveryTime, objects.save, 3,
      MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

    //When there are no palettes on the list new button and edited should get disabled
    DoMethod(objects.listview, MUIM_Notify, MUIA_List_Entries, 0, objects.new, 3,
      MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(objects.listview, MUIM_Notify, MUIA_List_Entries, 0, obj, 3,
      MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);

    //New button
    DoMethod(objects.new, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_PaletteEditor_Reset);

    DoMethod(obj, MUIM_Set, MUIA_PaletteEditor_Edited, FALSE);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    data->obj_table.new = objects.new;
    data->obj_table.load = objects.load;
    data->obj_table.save = objects.save;
    data->obj_table.undo = objects.undo;
    data->obj_table.redo = objects.redo;
    data->obj_table.listview = objects.listview;
    data->obj_table.list_new = objects.list_new;
    data->obj_table.list_import = objects.list_import;
    data->obj_table.list_remove = objects.list_remove;
    data->obj_table.list_move_up = objects.list_move_up;
    data->obj_table.list_move_down = objects.list_move_down;
    data->obj_table.color_palette = objects.color_palette;
    data->obj_table.name = objects.name;
    data->obj_table.size = objects.size;
    data->obj_table.selected = objects.selected;
    data->obj_table.edit_group = objects.edit_group;
    data->old_palette_item = NULL;
    data->edited = FALSE;

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
      case MUIA_PaletteEditor_Edited:
        data->edited = tag->ti_Data;
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
    case MUIA_PaletteEditor_Edited:
      *msg->opg_Storage = data->edited;
    return TRUE;
    case MUIA_PaletteEditor_List:
      *msg->opg_Storage = (ULONG)data->obj_table.listview;
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
    case MUIM_PaletteEditor_Update_Size:
      return m_Update_Size(cl, obj, msg);
    case MUIM_PaletteEditor_Update_Name:
      return m_Update_Name(cl, obj, msg);
    case MUIM_PaletteEditor_New_Palette:
      return m_New_Palette(cl, obj, msg);
    case MUIM_PaletteEditor_Rem_Palette:
      return m_Remove_Palette(cl, obj, msg);
    case MUIM_PaletteEditor_Chg_Palette:
      return m_Change_Palette(cl, obj, msg);
    case MUIM_PaletteEditor_Import_ILBM:
      return m_Import_ILBM(cl, obj, msg);
    case MUIM_PaletteEditor_Move:
      return m_Move_Palette(cl, obj, (struct cl_Msg*)msg);
    case MUIM_PaletteEditor_Load:
      return m_Load(cl, obj, (struct cl_Msg2*)msg);
    case MUIM_PaletteEditor_Save:
      return m_Save(cl, obj, (struct cl_Msg2*)msg);
    case MUIM_PaletteEditor_LoadAs:
      return m_Load_As(cl, obj, msg);
    case MUIM_PaletteEditor_SaveAs:
      return m_Save_As(cl, obj, msg);
    case MUIM_PaletteEditor_Reset:
      return m_Reset_Palettes(cl, obj, msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_PaletteEditor(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
