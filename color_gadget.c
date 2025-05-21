/******************************************************************************
 * ColorGadget                                                                *
 ******************************************************************************/
///Includes
#include <proto/exec.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>       // <-- Required for DoSuperMethod()
#include <clib/alib_stdio_protos.h> // <-- Required for sprintf()
#include <proto/graphics.h>

#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "color_gadget.h"
///
///Structs
struct cl_Data
{
  ULONG red;
  ULONG green;
  ULONG blue;
  ULONG index;
  BOOL selected;
  BOOL is_shown;
};

struct cl_Msg
{
  ULONG MethodID;
  //<SUBCLASS METHOD MESSAGE PAYLOAD HERE>
};
///

//<DEFINE SUBCLASS METHODS HERE>
///m_AskMinMax
static ULONG m_AskMinMax(struct IClass* cl, Object* obj, struct MUIP_AskMinMax* msg)
{
  DoSuperMethodA(cl, obj, (Msg)msg);

  msg->MinMaxInfo->MinWidth  += 1;
  msg->MinMaxInfo->DefWidth  += 16;
  msg->MinMaxInfo->MaxWidth  += 160;

  msg->MinMaxInfo->MinHeight += 1;
  msg->MinMaxInfo->DefHeight += 12;
  msg->MinMaxInfo->MaxHeight += 120;

  return 0;
}
///
///m_Draw
static ULONG m_Draw(struct IClass* cl, Object* obj, struct MUIP_Draw* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct {
    UBYTE A,R,G,B;
  }ARGB;
  BOOL force_redraw = (msg->flags & MADF_DRAWUPDATE) && data->is_shown;
  ULONG box_pen = 1;/* code */

  DoSuperMethodA(cl, obj, (Msg)msg);

  if (!(msg->flags & MADF_DRAWOBJECT || force_redraw)) return(0);

  if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) > 8) {
    ULONG brightness;

    ARGB.A = 0;
    ARGB.R = data->red   >> 24;
    ARGB.G = data->green >> 24;
    ARGB.B = data->blue  >> 24;

    brightness = (ARGB.R + ARGB.R + ARGB.R + ARGB.G + ARGB.G + ARGB.G + ARGB.G + ARGB.B) >> 3;
    if (brightness < 80) box_pen = 2;

    FillPixelArray(_rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), *(ULONG*)&ARGB);
  }
  else {
    UBYTE str[4] = {0};
    struct TextExtent te;
    ULONG len;

    sprintf(str, "%lu", data->index);
    len = strlen(str);

    SetFont(_rp(obj), _font(obj));
    TextExtent(_rp(obj), str, len, &te);

    SetAPen(_rp(obj), 2);
    RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));
    SetAPen(_rp(obj), 1);
    Move(_rp(obj), _mleft(obj) + (_mwidth(obj) - te.te_Width) / 2, _mtop(obj) + _rp(obj)->Font->tf_Baseline + (_mheight(obj) - te.te_Height) / 2);
    Text(_rp(obj), str, len);
  }

  if (data->selected) {
    SetAPen(_rp(obj), box_pen);
    Move(_rp(obj), _mleft(obj), _mtop(obj));
    Draw(_rp(obj), _mright(obj), _mtop(obj));
    Draw(_rp(obj), _mright(obj), _mbottom(obj));
    Draw(_rp(obj), _mleft(obj), _mbottom(obj));
    Draw(_rp(obj), _mleft(obj), _mtop(obj));
  }

  return 0;
}
///
///m_Show
static ULONG m_Show(struct IClass* cl, Object* obj, Msg msg)
{
  ULONG retVal = DoSuperMethodA(cl, obj, msg);

  if (retVal) {
    struct cl_Data* data = INST_DATA(cl, obj);

    data->is_shown = TRUE;
  }

  return retVal;
}
///
///m_Hide
static ULONG m_Hide(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data* data = INST_DATA(cl, obj);

  data->is_shown = FALSE;

  return (DoSuperMethodA(cl, obj, msg));
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct TagItem *tag;
  ULONG red   = 0;
  ULONG green = 0;
  ULONG blue  = 0;
  ULONG index = 0;
  BOOL selected = FALSE;

  if ((tag = FindTagItem(MUIA_ColorGadget_Red, msg->ops_AttrList))) {
    red = tag->ti_Data;
  }

  if ((tag = FindTagItem(MUIA_ColorGadget_Green, msg->ops_AttrList))) {
    green = tag->ti_Data;
  }

  if ((tag = FindTagItem(MUIA_ColorGadget_Blue, msg->ops_AttrList))) {
    blue = tag->ti_Data;
  }

  if ((tag = FindTagItem(MUIA_ColorGadget_RGB, msg->ops_AttrList))) {
    ULONG* RGB = (ULONG*)tag->ti_Data;
    red   = RGB[0];
    green = RGB[1];
    blue  = RGB[2];
  }

  if ((tag = FindTagItem(MUIA_ColorGadget_Index, msg->ops_AttrList))) {
    index = tag->ti_Data;
  }

  if ((tag = FindTagItem(MUIA_ColorGadget_Selected, msg->ops_AttrList))) {
    selected = (BOOL)tag->ti_Data;
  }

  if ((obj = (Object *) DoSuperNew(cl, obj, TAG_MORE, msg->ops_AttrList))) {
    data = (struct cl_Data*) INST_DATA(cl, obj);

    data->red   = red;
    data->green = green;
    data->blue  = blue;
    data->index = index;
    data->selected = selected;
    data->is_shown = FALSE;

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
  struct cl_Data* data = INST_DATA(cl, obj);
  struct TagItem* tags = msg->ops_AttrList;
  struct TagItem* tag  = NextTagItem(&tags);
  BOOL update_needed = FALSE;

  while (tag) {
    switch (tag->ti_Tag) {
      case MUIA_ColorGadget_Red:
        data->red = tag->ti_Data;
        update_needed = TRUE;
      break;
      case MUIA_ColorGadget_Green:
        data->green = tag->ti_Data;
        update_needed = TRUE;
      break;
      case MUIA_ColorGadget_Blue:
        data->blue = tag->ti_Data;
        update_needed = TRUE;
      break;
      case MUIA_ColorGadget_RGB:
      {
        ULONG* RGB = (ULONG*)tag->ti_Data;
        data->red   = RGB[0];
        data->green = RGB[1];
        data->blue  = RGB[2];
        update_needed = TRUE;
      }
      break;
      case MUIA_ColorGadget_Selected:
        data->selected = (BOOL)tag->ti_Data;
        update_needed = TRUE;
      break;
    }

    tag = NextTagItem(&tags);
  }

  if (update_needed) {
    DoMethod(obj, MUIM_Draw, MADF_DRAWUPDATE);
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
    case MUIA_ColorGadget_Red:
      *msg->opg_Storage = data->red;
    return TRUE;
    case MUIA_ColorGadget_Green:
      *msg->opg_Storage = data->green;
    return TRUE;
    case MUIA_ColorGadget_Blue:
      *msg->opg_Storage = data->blue;
    return TRUE;
    case MUIA_ColorGadget_RGB:
      *msg->opg_Storage = (ULONG)&data->red;
    return TRUE;
    case MUIA_ColorGadget_Index:
      *msg->opg_Storage = data->index;
    return TRUE;
    case MUIA_ColorGadget_Selected:
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
    case MUIM_AskMinMax:
      return m_AskMinMax(cl, obj, (struct MUIP_AskMinMax*) msg);
    case MUIM_Draw:
      return m_Draw(cl, obj, (struct MUIP_Draw*) msg);
    case MUIM_Show:
      return m_Show(cl, obj, msg);
    case MUIM_Hide:
      return m_Hide(cl, obj, msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_ColorGadget(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
