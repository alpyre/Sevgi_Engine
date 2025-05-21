/******************************************************************************
 * Integer Gadget                                                           *
 ******************************************************************************/
///Includes
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>          // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>       // <-- Required for DoSuperMethod()
#include <clib/alib_stdio_protos.h> // <-- Required for sprintf()

#include <SDI_compiler.h>           //     Required for
#include <SDI_hook.h>               // <-- multi platform
#include <SDI_stdarg.h>             //     compatibility

#include "dosupernew.h"
#include "integer_gadget.h"
///
///Structs
struct cl_ObjTable
{
  Object* text;
  Object* plus;
  Object* minus;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  LONG value;
  LONG min;
  LONG max;
  LONG incr;
  BOOL buttons;
  BOOL input;
  UWORD type;
};

enum {
  TYPE_REGULAR,
  TYPE_124,
  TYPE_ASCII,
  TYPE_PLTSZ
};

struct cl_Msg
{
  ULONG MethodID;
  //<SUBCLASS METHOD MESSAGE PAYLOAD HERE>
};
///
///Globals
extern struct MUI_CustomClass *MUIC_AckString;
///

//<SUBCLASS METHODS>
///m_SetValue()
ULONG m_SetValue(struct IClass* cl, Object* obj, LONG value) {
  struct cl_Data *data = INST_DATA(cl, obj);
  UBYTE buf[32];

  if (value > data->max) value = data->max;
  if (value < data->min) value = data->min;

  if (data->type == TYPE_PLTSZ) {
    UWORD valid[] = {2, 4, 8, 16, 32, 64, 128, 256};
    UWORD s_valid = sizeof(valid) / sizeof(UWORD);
    ULONG i;

    for (i = 0; i < s_valid; i++) {
      if (value == valid[i]) break;
    }
    if (i == s_valid) return 0;
  }

  if (data->type == TYPE_ASCII) {
    *buf = value;
    buf[1] = 0;
  }
  else {
    sprintf(buf, "%ld", value);
  }

  if (data->input)
    DoMethod(data->obj_table.text, MUIM_NoNotifySet, MUIA_String_Contents, buf);
  else
    DoMethod(data->obj_table.text, MUIM_NoNotifySet, MUIA_Text_Contents, buf);

  data->value = value;

  return 0;
}
///
///m_Kestroke()
ULONG m_Kestroke(struct IClass* cl, Object* obj) {
  struct cl_Data *data = INST_DATA(cl, obj);
  LONG newValue;

  if (data->type == TYPE_ASCII) {
    STRPTR newString;
    GetAttr(MUIA_String_Contents, data->obj_table.text, (ULONG*)&newString);
    newValue = (LONG)newString[0];
  }
  else
    GetAttr(MUIA_String_Integer, data->obj_table.text, &newValue);

  if (newValue > data->max) newValue = data->max;
  if (newValue < data->min) newValue = data->min;

  data->value = newValue;

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct TagItem *tags, *tag;
  STRPTR accept = "-0123456789";
  STRPTR inc_txt = "+";
  STRPTR dec_txt = "-";
  LONG value = 0;
  LONG min  = MININT;
  LONG max  = MAXINT;
  LONG incr = 1;
  BOOL buttons = FALSE;
  BOOL input = FALSE;
  BOOL inverse = FALSE;
  UWORD type = TYPE_REGULAR;
  Object* text;
  Object* plus;
  Object* minus;

  for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
    switch (tag->ti_Tag) {
      case MUIA_Integer_Value:
        value = tag->ti_Data;
      break;
      case MUIA_Integer_Min:
        min = tag->ti_Data;
        if (min > max) min = max;
        if (min >= 0) accept = "0123456789";
      break;
      case MUIA_Integer_Max:
        max = tag->ti_Data;
        if (max < min) max = min;
      break;
      case MUIA_Integer_Incr:
        incr = tag->ti_Data;
      break;
      case MUIA_Integer_Buttons:
        buttons = tag->ti_Data;
      break;
      case MUIA_Integer_Buttons_Inverse:
        inverse = tag->ti_Data;
      break;
      case MUIA_Integer_Button_Inc_Text:
        inc_txt = (STRPTR)tag->ti_Data;
      break;
      case MUIA_Integer_Button_Dec_Text:
        dec_txt = (STRPTR)tag->ti_Data;
      break;
      case MUIA_Integer_Input:
        input = tag->ti_Data;
      break;
      case MUIA_Integer_124:
        accept = "124";
        type = TYPE_124;
        value = 1;
        incr = 1;
        min = 1;
        max = 4;
        buttons = TRUE;
      break;
      case MUIA_Integer_ASCII:
        accept = NULL;
        type = TYPE_ASCII;
        incr = 1;
        min = 33;
        max = 254;
        buttons = TRUE;
      break;
      case MUIA_Integer_PaletteSize:
        accept = NULL;
        type = TYPE_PLTSZ;
        incr = 2;
        min = 2;
        max = 256;
        input = FALSE;
        buttons = TRUE;
      break;
    }
  }

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Group_Horiz, TRUE,
    MUIA_Group_HorizSpacing, 1,
    MUIA_Group_Child, input ? (text = NewObject(MUIC_AckString->mcc_Class, NULL,
      MUIA_Frame, MUIV_Frame_String,
      accept ? MUIA_String_Accept : TAG_IGNORE, accept,
      type == TYPE_ASCII ? MUIA_String_MaxLen : TAG_IGNORE, 2,
    TAG_MORE, msg->ops_AttrList)) : (text = MUI_NewObject(MUIC_Text,
      MUIA_Frame, MUIV_Frame_Text,
    TAG_MORE, msg->ops_AttrList)),
    MUIA_Group_Child, (plus = MUI_NewObject(MUIC_Text,
      MUIA_InputMode, MUIV_InputMode_RelVerify,
      MUIA_Frame, MUIV_Frame_Button,
      MUIA_Background, MUII_ButtonBack,
      MUIA_Font, MUIV_Font_Button,
      MUIA_Text_Contents, inverse ? dec_txt : inc_txt,
      MUIA_HorizWeight, 0,
      MUIA_ShowMe, buttons,
    TAG_END)),
    MUIA_Group_Child, (minus = MUI_NewObject(MUIC_Text,
      MUIA_InputMode, MUIV_InputMode_RelVerify,
      MUIA_Frame, MUIV_Frame_Button,
      MUIA_Background, MUII_ButtonBack,
      MUIA_Font, MUIV_Font_Button,
      MUIA_Text_Contents, inverse ? inc_txt : dec_txt,
      MUIA_HorizWeight, 0,
      MUIA_ShowMe, buttons,
    TAG_END)),
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);
    data->obj_table.text = text;
    data->obj_table.plus = inverse ? minus : plus;
    data->obj_table.minus = inverse ? plus : minus;
    data->value = value;
    data->min   = min;
    data->max   = max;
    data->incr  = incr;
    data->buttons = buttons;
    data->input = input;
    data->type  = type;

    m_SetValue(cl, obj, value);
    DoMethod(data->obj_table.plus, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_Integer_Increase);
    DoMethod(data->obj_table.minus, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_Integer_Decrease);
    if (input) {
      DoMethod(text, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MUIM_Integer_Keystroke);
      DoMethod(text, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 1, MUIM_Integer_Acknowledge);
    }

    //<SUBCLASS INITIALIZATION HERE>
    if (/*<Success of your initializations>*/ TRUE) {

      return((ULONG) obj);
    }
    else CoerceMethod(cl, obj, OM_DISPOSE);
  }

  return NULL;
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
      case MUIA_Integer_Value:
      {
        LONG value = tag->ti_Data;
        m_SetValue(cl, obj, value);
        tag->ti_Data = data->value;
      }
      break;
      case MUIA_Integer_Buttons:
      {
        BOOL buttons = tag->ti_Data;
        if (buttons != data->buttons) {
          data->buttons = buttons;
          DoMethod(data->obj_table.plus,  MUIM_Set, MUIA_ShowMe, buttons);
          DoMethod(data->obj_table.minus, MUIM_Set, MUIA_ShowMe, buttons);
        }
      }
      break;
      case MUIA_Integer_Min:
        data->min = tag->ti_Data;
        if (data->min > data->max)
          DoMethod(obj, MUIM_NoNotifySet, MUIA_Integer_Max, data->min);
        if (data->value < data->min)
          DoMethod(obj, MUIM_NoNotifySet, MUIA_Integer_Value, data->min);
      break;
      case MUIA_Integer_Max:
        data->max = tag->ti_Data;
        if (data->max < data->min)
          DoMethod(obj, MUIM_NoNotifySet, MUIA_Integer_Min, data->max);
        if (data->value > data->max) {
          DoMethod(obj, MUIM_NoNotifySet, MUIA_Integer_Value, data->max);
        }
      break;
      case MUIA_Integer_Incr:
        data->incr = tag->ti_Data;
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
    case MUIA_Integer_Value:
      *msg->opg_Storage = data->value;
    return TRUE;
    case MUIA_Integer_Min:
      *msg->opg_Storage = data->min;
    return TRUE;
    case MUIA_Integer_Max:
      *msg->opg_Storage = data->max;
    return TRUE;
    case MUIA_Integer_Incr:
      *msg->opg_Storage = data->incr;
    return TRUE;
  }

  return (DoSuperMethodA(cl, obj, (Msg) msg));
}
///
///Dispatcher
SDISPATCHER(cl_Dispatcher)
{
  struct cl_Data *data = NULL;
  if (! (msg->MethodID == OM_NEW)) data = INST_DATA(cl, obj);

  switch(msg->MethodID)
  {
    case OM_NEW:
      return m_New(cl, obj, (struct opSet*) msg);
    case OM_SET:
      return m_Set(cl, obj, (struct opSet*) msg);
    case OM_GET:
      return m_Get(cl, obj, (struct opGet*) msg);
    case MUIM_Integer_Increase:
      if (data->type == TYPE_PLTSZ) data->incr = data->value;
      if (data->type == TYPE_124 && data->value == 2)
        return DoMethod(obj, MUIM_Set, MUIA_Integer_Value, 4);
      else
        return DoMethod(obj, MUIM_Set, MUIA_Integer_Value, data->value + data->incr);
    case MUIM_Integer_Decrease:
      if (data->type == TYPE_PLTSZ) data->incr = data->value / 2;
      if (data->type == TYPE_124 && data->value == 4)
        return DoMethod(obj, MUIM_Set, MUIA_Integer_Value, 2);
      else
        return DoMethod(obj, MUIM_Set, MUIA_Integer_Value, data->value - data->incr);
    case MUIM_Integer_Keystroke:
      return m_Kestroke(cl, obj);
    case MUIM_Integer_Acknowledge:
    {
      if (data->type == TYPE_ASCII) {
        STRPTR newString = NULL;
        LONG newValue;
        GetAttr(MUIA_String_Contents, data->obj_table.text, (ULONG*)&newString);
        newValue = (LONG)newString[0];
        return DoMethod(obj, MUIM_Set, MUIA_Integer_Value, newValue);
      }
      else {
        LONG newValue;
        GetAttr(MUIA_String_Integer, data->obj_table.text, &newValue);
        return DoMethod(obj, MUIM_Set, MUIA_Integer_Value, newValue);
      }
    }

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_Integer(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
