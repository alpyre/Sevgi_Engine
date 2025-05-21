/******************************************************************************
 * ColorPalette                                                               *
 ******************************************************************************/
///defines
#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(obj,x,y) (_between(_mleft(obj),(x),_mright(obj)) && _between(_mtop(obj),(y),_mbottom(obj)))
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
#include "color_gadget.h"
#include "color_palette.h"
///
///Structs
struct cl_Data
{
  Object* group;
  ULONG num_fields;
  Object** fields;
  ULONG selected;
  ULONG rows;
  ULONG columns;
  BOOL display_only;
  BOOL edited;
};

struct cl_Msg
{
  ULONG MethodID;
  //<SUBCLASS METHOD MESSAGE PAYLOAD HERE>
};
///
///Globals
extern APTR g_MemoryPool;
extern struct MUI_CustomClass* MUIC_ColorGadget;

UBYTE default_palette[] = {
  255,
  204, 204, 204,
    0,   0,   0,
  255, 255, 255,
  136, 136, 136,
  170,  34,   0,
  221,  85,   0,
  238, 187,   0,
    0, 136,   0,
  102, 187,   0,
   17, 119,   0,
   85, 187, 204,
   51, 102, 119,
    0, 119, 187,
    0,  68, 136,
  238, 136, 136,
  187,  85,  85,
  238, 119,  34,
  204, 102,  68,
  136,  34,   0,
  255, 187, 119,
  153,   0, 255,
  102,   0, 204,
  187, 119,   0,
  153,  85,   0,
  255, 170,  85,
  221, 136,  68,
  153, 187, 153,
  102, 136, 102,
   68, 170, 119,
   17, 119,  85,
   51,  51,  51,
  238, 238, 238,
  102, 102, 102,
    0,   0,   0,
  119, 119, 119,
   34,  68,   0,
  102,  34,   0,
   85,  17,   0,
  119,  85,   0,
   85,  68,   0,
   51,  85,   0,
    0,  51,   0,
   34,  85, 102,
   17,  51,  51,
    0,  51,  85,
    0,  34,  68,
  119,  68,  68,
   85,  34,  34,
  119,  51,  17,
  102,  51,  34,
   68,  17,   0,
  119,  85,  51,
   68,   0, 119,
   51,   0, 102,
   85,  51,   0,
   68,  34,   0,
  119,  85,  34,
  102,  68,  34,
   68,  85,  68,
   51,  68,  51,
   34,  85,  51,
    0,  51,  34,
   17,  17,  17,
  119, 119, 119,
  255, 255,   0,
  238, 238,   0,
  221, 221,   0,
  204, 204,   0,
  187, 187,   0,
  170, 170,   0,
  153, 153,   0,
  136, 136,   0,
  119, 119,   0,
  102, 102,   0,
   85,  85,   0,
   68,  68,   0,
   51,  51,   0,
   34,  34,   0,
   17,  17,   0,
    0,   0,   0,
  255,   0,   0,
  238,   0,   0,
  221,   0,   0,
  204,   0,   0,
  187,   0,   0,
  170,   0,   0,
  153,   0,   0,
  136,   0,   0,
  119,   0,   0,
  102,   0,   0,
   85,   0,   0,
   68,   0,   0,
   51,   0,   0,
   34,   0,   0,
   17,   0,   0,
    0,   0,   0,
    0, 255,   0,
    0, 238,   0,
    0, 221,   0,
    0, 204,   0,
    0, 187,   0,
    0, 170,   0,
    0, 153,   0,
    0, 136,   0,
    0, 119,   0,
    0, 102,   0,
    0,  85,   0,
    0,  68,   0,
    0,  51,   0,
    0,  34,   0,
    0,  17,   0,
    0,   0,   0,
    0,   0, 255,
    0,   0, 238,
    0,   0, 221,
    0,   0, 204,
    0,   0, 187,
    0,   0, 170,
    0,   0, 153,
    0,   0, 136,
    0,   0, 119,
    0,   0, 102,
    0,   0,  85,
    0,   0,  68,
    0,   0,  51,
    0,   0,  34,
    0,   0,  17,
    0,   0,   0,
    0, 255, 255,
    0, 238, 238,
    0, 221, 221,
    0, 204, 204,
    0, 187, 187,
    0, 170, 170,
    0, 153, 153,
    0, 136, 136,
    0, 119, 119,
    0, 102, 102,
    0,  85,  85,
    0,  68,  68,
    0,  51,  51,
    0,  34,  34,
    0,  17,  17,
    0,   0,   0,
  255,   0, 255,
  238,   0, 238,
  221,   0, 221,
  204,   0, 204,
  187,   0, 187,
  170,   0, 170,
  153,   0, 153,
  136,   0, 136,
  119,   0, 119,
  102,   0, 102,
   85,   0,  85,
   68,   0,  68,
   51,   0,  51,
   34,   0,  34,
   17,   0,  17,
    0,   0,   0,
  255, 255, 255,
  255, 255, 238,
  255, 255, 221,
  255, 255, 204,
  255, 255, 187,
  255, 255, 170,
  255, 255, 153,
  255, 255, 136,
  255, 255, 119,
  255, 255, 102,
  255, 255,  85,
  255, 255,  68,
  255, 255,  51,
  255, 255,  34,
  255, 255,  17,
  255, 255,   0,
  255, 255, 255,
  238, 238, 238,
  221, 221, 221,
  204, 204, 204,
  187, 187, 187,
  170, 170, 170,
  153, 153, 153,
  136, 136, 136,
  119, 119, 119,
  102, 102, 102,
   85,  85,  85,
   68,  68,  68,
   51,  51,  51,
   34,  34,  34,
   17,  17,  17,
    0,   0,   0,
  255, 119, 119,
  255, 136, 119,
  255, 170, 119,
  255, 187, 119,
  255, 204, 119,
  255, 221, 119,
  255, 255, 119,
  238, 255, 119,
  221, 255, 119,
  187, 255, 119,
  170, 255, 119,
  153, 255, 119,
  136, 255, 119,
  119, 255, 136,
  119, 255, 153,
  119, 255, 170,
  119, 255, 204,
  119, 255, 221,
  119, 255, 238,
  119, 238, 255,
  119, 221, 255,
  119, 204, 255,
  119, 187, 255,
  119, 153, 255,
  119, 136, 255,
  119, 119, 255,
  153, 119, 255,
  170, 119, 255,
  187, 119, 255,
  204, 119, 255,
  238, 119, 255,
  255, 119, 255,
  255,   0,   0,
  255,  34,   0,
  255,  85,   0,
  255, 119,   0,
  255, 170,   0,
  255, 204,   0,
  255, 238,   0,
  221, 255,   0,
  187, 255,   0,
  136, 255,   0,
  102, 255,   0,
   51, 255,   0,
   17, 255,   0,
    0, 255,  17,
    0, 255,  68,
    0, 255, 102,
    0, 255, 153,
    0, 255, 187,
    0, 255, 221,
    0, 238, 255,
    0, 187, 255,
    0, 153, 255,
    0, 119, 255,
    0,  68, 255,
    0,  34, 255,
    0,   0, 255,
   51,   0, 255,
   85,   0, 255,
  136,   0, 255,
  170,   0, 255,
  204,   0, 255,
  255,   0, 255
};
///

//<DEFINE SUBCLASS METHODS HERE>
///isNumColorsValid(num)
static ULONG isNumColorsValid(ULONG num)
{
  UWORD arr[8] = {2, 4, 8, 16, 32, 64, 128, 256};
  ULONG i;

  for (i = 0; i < 8; i++) {
    if (arr[i] == num) {
      return i + 1;
    }
  }

  return 0;
}
///
///newColorPaletteGroup(palette, &fields, &rows, &columns)
static Object* newColorPaletteGroup(UBYTE* palette, Object*** r_fields, ULONG* r_rows, ULONG* r_columns) {
  Object* group = NULL;
  ULONG num_colors = palette[0] + 1;
  ULONG num_colors_index = 0;
  ULONG columns = 0;
  ULONG rows = 0;
  Object** fields = NULL;
  struct TagItem tag_list[9];
  struct TagItem tag_list_inner[33];
  struct {
    UBYTE columns;
    UBYTE rows;
  }sizes[] = {{0, 0}, {1, 2}, {1, 4}, {2, 4}, {2, 8}, {4, 8}, {4, 16}, {8, 16}, {8, 32}};

  if (num_colors && (num_colors_index = isNumColorsValid(num_colors))) {
    columns = sizes[num_colors_index].columns;
    rows = sizes[num_colors_index].rows;
    fields = AllocPooled(g_MemoryPool, num_colors * sizeof(Object*));

    if (fields) {
      LONG i, r, c;
      ULONG rgb_color[3] = {0};

      for (i = 0; i < num_colors; i++) {
        if (palette) {
          rgb_color[0] = (ULONG)palette[i * 3 + 1] << 24;
          rgb_color[1] = (ULONG)palette[i * 3 + 2] << 24;
          rgb_color[2] = (ULONG)palette[i * 3 + 3] << 24;
        }

        fields[i] = NewObject(MUIC_ColorGadget->mcc_Class, NULL, MUIA_ColorGadget_RGB, rgb_color,
                                                                 MUIA_ColorGadget_Index, i,
                                                                 TAG_END);
        if (!fields[i]) {
          for (--i; i >= 0 ; i--) MUI_DisposeObject(fields[i]);
          FreePooled(g_MemoryPool, fields, num_colors * sizeof(Object*));
          return NULL;
        }
      }

      for (c = 0, i = 0; c < columns; c++) {
        for (r = 0; r < rows; r++) {
          tag_list_inner[r].ti_Tag = MUIA_Group_Child;
          tag_list_inner[r].ti_Data = (ULONG)fields[i++];
        }
        tag_list_inner[r].ti_Tag = TAG_END;

        tag_list[c].ti_Tag = MUIA_Group_Child;
        tag_list[c].ti_Data = (ULONG)MUI_NewObject(MUIC_Group, MUIA_Group_Spacing, 2, TAG_MORE, tag_list_inner);
      }
      tag_list[c].ti_Tag = TAG_END;
      tag_list[c].ti_Data = NULL;

      group = MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE,
                                        MUIA_Group_Spacing, 2,
                                        TAG_MORE, tag_list);
      if (!group) {
        FreePooled(g_MemoryPool, fields, num_colors * sizeof(Object*));
      }

      *r_rows = rows;
      *r_columns = columns;
      *r_fields = fields;
    }
  }

  return group;
}
///
///getPalette(cl, obj)
static UBYTE* getPalette(struct IClass* cl, Object* obj)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG num_colors = data->num_fields;
  UBYTE* palette = AllocPooled(g_MemoryPool, (num_colors * 3) + 1);

  if (palette) {
    ULONG f, c;

    for (f = 0, c = 1; f < num_colors; f++) {
      ULONG* RGB;
      get(data->fields[f], MUIA_ColorGadget_RGB, &RGB);

      palette[c++] = RGB[0] >> 24;
      palette[c++] = RGB[1] >> 24;
      palette[c++] = RGB[2] >> 24;
    }

    palette[0] = num_colors - 1;
  }

  return palette;
}
///
///freePalette(palette)
VOID freePalette(UBYTE* palette)
{
  if (palette) FreePooled(g_MemoryPool, palette, ((palette[0] + 1) * 3) + 1);
}
///
///setPalette(cl, obj, palette)
static ULONG setPalette(struct IClass* cl, Object* obj, UBYTE* palette)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  UBYTE def_palette_size = default_palette[0];
  ULONG num_colors;

  if (!palette) {
    palette = default_palette;
    palette[0] = DEFAULT_PALLETTE_SIZE - 1;
  }

  num_colors = palette[0] + 1;

  if (num_colors == data->num_fields) {
    ULONG f, c;

    for (f = 0, c = 1; f < data->num_fields; f++) {
      ULONG RGB[3];

      RGB[0] = (ULONG)palette[c++] << 24;
      RGB[1] = (ULONG)palette[c++] << 24;
      RGB[2] = (ULONG)palette[c++] << 24;

      DoMethod(data->fields[f], MUIM_Set, MUIA_ColorGadget_RGB, &RGB);
      //NOTE: This is to notify the color adjust object about palette change!
      if (f == data->selected) {
        DoMethod(obj, MUIM_Set, MUIA_ColorPalette_RGB, &RGB);
      }
    }
  }
  else {
    ULONG *RGB;

    if (data->fields) {
      FreePooled(g_MemoryPool, data->fields, data->num_fields * sizeof(Object*));
      data->fields = NULL;
      data->num_fields = 0;
    }

    DoMethod(obj, MUIM_Group_InitChange);
    DoMethod(obj, OM_REMMEMBER, data->group);
    MUI_DisposeObject(data->group);
    data->group = newColorPaletteGroup(palette, &data->fields, &data->rows, &data->columns);
    if (data->group) {
      DoMethod(obj, OM_ADDMEMBER, data->group);
      if (data->selected >= num_colors) {
        data->selected = num_colors - 1;
      }

      data->num_fields = num_colors;
      DoMethod(obj, MUIM_Set, MUIA_ColorPalette_NumColors, num_colors); //To issue a notification
      if (!data->display_only) {
        DoMethod(data->fields[data->selected], MUIM_Set, MUIA_ColorGadget_Selected, TRUE);
      }
    }
    else data->selected = 0;

    DoMethod(obj, MUIM_Group_ExitChange);

    //NOTE: This is to notify the color adjust object about palette change!
    get(data->fields[data->selected], MUIA_ColorGadget_RGB, &RGB);
    DoMethod(obj, MUIM_Set, MUIA_ColorPalette_RGB, RGB);
  }

  default_palette[0] = def_palette_size;

  return 0;
}
///
///setNumColors(cl, obj, num_colors)
ULONG setNumColors(struct IClass* cl, Object* obj, ULONG num_colors)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (isNumColorsValid(num_colors) && num_colors != data->num_fields) {
    if (num_colors > data->num_fields) {
      UBYTE* new_palette = AllocPooled(g_MemoryPool, (num_colors * 3) + 1);
      new_palette[0] = num_colors - 1;

      if (new_palette) {
        UBYTE* old_palette = getPalette(cl, obj);

        if (old_palette) {
          ULONG old_num_colors = old_palette[0] + 1;
          ULONG old_size = old_num_colors * 3;

          memcpy(new_palette + 1, old_palette + 1, old_size);
          memcpy(new_palette + 1 + old_size, default_palette + 1 + old_size, (num_colors - old_num_colors) * 3);

          freePalette(old_palette);

          setPalette(cl, obj, new_palette);
        }
        freePalette(new_palette);
      }
    }
    else { // num_colors < data->num_fields
      UBYTE* old_palette = getPalette(cl, obj);
      if (old_palette) {
        UBYTE old_plt_0 = old_palette[0];

        old_palette[0] = num_colors - 1;
        setPalette(cl, obj, old_palette);
        old_palette[0] = old_plt_0;

        freePalette(old_palette);
      }
    }
  }


  return 0;
}
///

///Overridden HandleInput
static ULONG m_HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleInput *msg)
{
  struct cl_Data* data = INST_DATA(cl, obj);

  if (msg->imsg) {
    switch (msg->imsg->Class) {
      case IDCMP_MOUSEBUTTONS:
        switch (msg->imsg->Code) {
          case SELECTDOWN:
            if (_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY)) {
              ULONG field_column;
              ULONG field_row;
              ULONG selected_field;
              ULONG elapsed;
              ULONG mX = msg->imsg->MouseX - _mleft(obj);
              ULONG mY = msg->imsg->MouseY - _mtop(obj);
              for (field_column = 0, elapsed = _mwidth(data->fields[0]); field_column < data->columns; field_column++, elapsed += _mwidth(data->fields[field_column * data->rows]) + 2) {
                 if (elapsed >= mX) break;
              }
              for (field_row = 0, elapsed = _mheight(data->fields[0]); field_row < data->rows; field_row++, elapsed += _mheight(data->fields[field_row]) + 2) {
                 if (elapsed >= mY) break;
              }
              selected_field = field_row + field_column * data->rows;

              if (selected_field != data->selected) {
                DoMethod(obj, MUIM_Set, MUIA_ColorPalette_Selected, selected_field);
              }
            }
          break;
        }
      break;
    }
  }

  return 0;
}
///
///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct TagItem *tag;
  UBYTE* palette = default_palette;
  ULONG num_colors = DEFAULT_PALLETTE_SIZE;
  ULONG rows = 2;
  ULONG columns = 1;
  ULONG selected = 0;
  BOOL display_only = FALSE;
  Object** fields;
  Object* group;

  if ((tag = FindTagItem(MUIA_ColorPalette_NumColors, msg->ops_AttrList)) &&
     isNumColorsValid(tag->ti_Data)) {
    num_colors = tag->ti_Data;
  }

  if ((tag = FindTagItem(MUIA_ColorPalette_Palette, msg->ops_AttrList))) {
    palette = (UBYTE*)tag->ti_Data;
  }

  if ((tag = FindTagItem(MUIA_ColorPalette_Selected, msg->ops_AttrList))) {
    selected = tag->ti_Data;
  }

  if ((tag = FindTagItem(MUIA_ColorPalette_DisplayOnly, msg->ops_AttrList))) {
    display_only = (BOOL)tag->ti_Data;
  }

  palette[0] = num_colors - 1;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Group_Child, (group = newColorPaletteGroup(palette, &fields, &rows, &columns)),
    TAG_MORE, (ULONG)msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    data->group = group;
    data->fields = fields;
    data->num_fields = num_colors;
    data->selected = selected;
    data->rows = rows;
    data->columns = columns;
    data->display_only = display_only;
    data->edited = FALSE;

    if (!display_only) {
      DoMethod(fields[selected], MUIM_Set, MUIA_ColorGadget_Selected, TRUE);
      MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS);
    }

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

  if (!data->display_only) {
    MUI_RejectIDCMP(obj, IDCMP_MOUSEBUTTONS);
  }

  if (data->fields) FreePooled(g_MemoryPool, data->fields, sizeof(Object*) * data->num_fields);

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
      case MUIA_ColorPalette_NumColors:
        setNumColors(cl, obj, tag->ti_Data);
      break;
      case MUIA_ColorPalette_Palette:
        setPalette(cl, obj, (UBYTE*)tag->ti_Data);
      break;
      case MUIA_ColorPalette_Selected:
        if (data->display_only) {
          data->selected = tag->ti_Data;
        }
        else {
          ULONG *RGB;

          if (tag->ti_Data > data->num_fields - 1) tag->ti_Data = data->num_fields - 1;
          if (tag->ti_Data != data->selected) {
            DoMethod(data->fields[data->selected], MUIM_Set, MUIA_ColorGadget_Selected, FALSE);
            data->selected = tag->ti_Data;
            DoMethod(data->fields[data->selected], MUIM_Set, MUIA_ColorGadget_Selected, TRUE);

            //Create a notification on MUIA_ColorGadget_RGB
            get(data->fields[data->selected], MUIA_ColorGadget_RGB, &RGB);
            DoMethod(obj, MUIM_Set, MUIA_ColorPalette_RGB, RGB);
          }
        }
      break;
      case MUIA_ColorPalette_RGB:
      {
        //NOTE: The following implementation is a workaround for a bug in...
        //      ...MUIC_Coloradjust object not respecting MUIM_NoNotifySet
        ULONG* RGB_prospect = (ULONG*)tag->ti_Data;
        ULONG* RGB_current;
        get(data->fields[data->selected], MUIA_ColorGadget_RGB, &RGB_current);
        if (!((RGB_current[0] >> 24) == (RGB_prospect[0] >> 24) &&
              (RGB_current[1] >> 24) == (RGB_prospect[1] >> 24) &&
              (RGB_current[2] >> 24) == (RGB_prospect[2] >> 24))) {
          DoMethod(data->fields[data->selected], MUIM_Set, MUIA_ColorGadget_RGB, tag->ti_Data);
          DoMethod(obj, MUIM_Set, MUIA_ColorPalette_Edited, TRUE);
        }
      }
      break;
      case MUIA_ColorPalette_Edited:
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
    case MUIA_ColorPalette_NumColors:
      *msg->opg_Storage = data->num_fields;
    return TRUE;
    case MUIA_ColorPalette_Selected:
      *msg->opg_Storage = data->selected;
    return TRUE;
    case MUIA_ColorPalette_Palette:
      *msg->opg_Storage = (ULONG)getPalette(cl, obj);
    return TRUE;
    case MUIA_ColorPalette_RGB:
      get(data->fields[data->selected], MUIA_ColorGadget_RGB, msg->opg_Storage);
    return TRUE;
    case MUIA_ColorPalette_Edited:
      *msg->opg_Storage = (ULONG)data->edited;
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
    case MUIM_HandleInput:
      return m_HandleInput(cl, obj, (struct MUIP_HandleInput*) msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_ColorPalette(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
