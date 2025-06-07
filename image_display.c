/******************************************************************************
 * ImageDisplay                                                               *
 ******************************************************************************/
///Defines
#if !defined (__MORPHOS__)
  #define MAX(a, b) (a > b ? a : b)
  #define MIN(a, b) (a > b ? b : a)
#endif
///
///Includes
//standard headers

#include <graphics/gfx.h>
#include <intuition/screens.h>

#include <proto/exec.h>
#include <proto/utility.h>    // <-- Required for tag redirection
#include <proto/graphics.h>

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "image_display.h"
///
///Structs
struct cl_ObjTable
{
  //<SUBCLASS CHILD OBJECT POINTERS HERE>
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  struct {
    struct ILBMImage* image;
    struct LUTPixelArray* lpa;
    struct CLUT* clut;
  }sheet;
  struct ImageSpec* spec;
  ULONG scale;
	int x, y;    //TEMP
  UBYTE axis_pen;
  UBYTE hitbox_pen;
  UBYTE active_hitbox_pen;
};

struct cl_Msg
{
  ULONG MethodID;
  //<SUBCLASS METHOD MESSAGE PAYLOAD HERE>
};
///

//<DEFINE SUBCLASS METHODS HERE>

///mSetSheet(image)
ULONG mSetSheet(struct IClass *cl, Object *obj, struct ILBMImage* img)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->sheet.image) {
    freeLUTPixelArray(data->sheet.lpa);
    freeCLUT(data->sheet.clut);
  }

  if (img) {
    data->sheet.image = img;
    data->sheet.lpa = newLUTPixelArray(img->bitmap);
    data->sheet.clut = newCLUT(img);
  }
  else {
    data->sheet.image = NULL;
    data->spec = NULL;
  }

  return 0;
}
///

///mAskMinMax()
SAVEDS ULONG mAskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
	/*
	** let our superclass first fill in what it thinks about sizes.
	** this will e.g. add the size of frame and inner spacing.
	*/

	DoSuperMethodA(cl, obj, (Msg)msg);

	/*
	** now add the values specific to our object. note that we
	** indeed need to *add* these values, not just set them!
	*/

	msg->MinMaxInfo->MinWidth  += 100;
	msg->MinMaxInfo->DefWidth  += 120;
	msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;

	msg->MinMaxInfo->MinHeight += 40;
	msg->MinMaxInfo->DefHeight += 90;
	msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;

	return 0;
}
///
///mShow()
SAVEDS ULONG mShow(struct IClass *cl, Object *obj, Msg msg)
{
	struct cl_Data *data = INST_DATA(cl, obj);

  data->axis_pen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap, 255 << 24, 255 << 24, 0, TAG_END);
  data->hitbox_pen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap, 0, 0, 255 << 24, TAG_END);
  data->active_hitbox_pen = ObtainBestPen(_screen(obj)->ViewPort.ColorMap, 255 << 24, 0, 0, TAG_END);

  return DoSuperMethodA(cl, obj, (Msg)msg);
}
///
///mHide()
SAVEDS ULONG mHide(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
	struct cl_Data *data = INST_DATA(cl, obj);

  ReleasePen(_screen(obj)->ViewPort.ColorMap, data->axis_pen);
  ReleasePen(_screen(obj)->ViewPort.ColorMap, data->hitbox_pen);
  ReleasePen(_screen(obj)->ViewPort.ColorMap, data->active_hitbox_pen);

  return DoSuperMethodA(cl, obj, (Msg)msg);
}
///
///mDraw()
VOID drawSpec(struct IClass *cl, Object *obj)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  LONG obj_width = _width(obj) / data->scale;
  LONG obj_height = _height(obj) / data->scale;
  LONG obj_half_w = obj_width / 2;
  LONG obj_half_h = obj_height / 2;
  LONG x_vis;
  LONG x_crop;
  LONG y_vis;
  LONG y_crop;
  APTR src_rect;

  if (data->spec->h_offs >= obj_half_w || data->spec->h_offs <= -(obj_half_w + data->spec->width)) {
      x_vis = 0;
      x_crop = 0;
  }
  else {
      x_vis = MIN(obj_half_w - data->spec->h_offs, data->spec->width);
      if ((-data->spec->h_offs) > obj_half_w) {
        x_crop = (-data->spec->h_offs) - obj_half_w;
        x_vis -= x_crop;
      }
      else
        x_crop = 0;
  }

  if (data->spec->v_offs >= obj_half_h || data->spec->v_offs <= -(obj_half_h + data->spec->height)) {
      y_vis = 0;
      y_crop = 0;
  }
  else {
      y_vis = MIN(obj_half_h - data->spec->v_offs, data->spec->height);
      if ((-data->spec->v_offs) > obj_half_h) {
        y_crop = (-data->spec->v_offs) - obj_half_h;
        y_vis -= y_crop;
      }
      else
        y_crop = 0;
  }

  if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) > 8) {
    src_rect = &data->sheet.lpa->pixels[data->sheet.lpa->width * (data->spec->y + y_crop) + data->spec->x + x_crop];

    scaleLUTPixelArray(src_rect, x_vis, y_vis, data->sheet.lpa->width, _rp(obj), data->sheet.clut->colors,
                      _left(obj) + (obj_half_w + data->spec->h_offs + x_crop) * data->scale,
                      _top(obj) + (obj_half_h + data->spec->v_offs + y_crop) * data->scale,
                      data->scale);
  }
  else {
    //WARNING: The blit below directly hits screen bitmap without any layering!
    struct BitScaleArgs bsa;
    bsa.bsa_SrcX = data->spec->x + x_crop;
    bsa.bsa_SrcY = data->spec->y + y_crop;
    bsa.bsa_SrcWidth = x_vis;
    bsa.bsa_SrcHeight = y_vis;
    bsa.bsa_XSrcFactor = 1;
    bsa.bsa_YSrcFactor = 1;
    bsa.bsa_DestX = _window(obj)->LeftEdge + _left(obj) + (obj_half_w + data->spec->h_offs + x_crop) * data->scale;
    bsa.bsa_DestY = _window(obj)->TopEdge + _top(obj) + (obj_half_h + data->spec->v_offs + y_crop) * data->scale;
    bsa.bsa_XDestFactor = data->scale;
    bsa.bsa_YDestFactor = data->scale;
    bsa.bsa_SrcBitMap = data->sheet.image->bitmap;
    bsa.bsa_DestBitMap = _rp(obj)->BitMap;
    bsa.bsa_Flags = 0;

    BitMapScale(&bsa);
  }
}

VOID drawAxis(struct IClass *cl, Object *obj)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  LONG obj_width = _width(obj) / data->scale;
  LONG obj_height = _height(obj) / data->scale;
  LONG obj_half_w = obj_width / 2;
  LONG obj_half_h = obj_height / 2;

  SetAPen(_rp(obj), data->axis_pen);
  Move(_rp(obj), _left(obj) + obj_half_w * data->scale, _top(obj));
  Draw(_rp(obj), _rp(obj)->cp_x, _bottom(obj));
  Move(_rp(obj), _left(obj), _top(obj) + obj_half_h * data->scale);
  Draw(_rp(obj), _right(obj), _rp(obj)->cp_y);
}

VOID drawHitBoxes(struct IClass *cl, Object *obj)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct MinList* list = &data->spec->hitboxes;
  LONG obj_width = _width(obj) / data->scale;
  LONG obj_height = _height(obj) / data->scale;
  LONG obj_half_w = obj_width / 2;
  LONG obj_half_h = obj_height / 2;
  LONG obj_center_x = _left(obj) + obj_half_w * data->scale;
  LONG obj_center_y = _top(obj) + obj_half_h * data->scale;
  struct HitBox* hb;
  WORD x1, y1, x2, y2;

  for (hb = (struct HitBox*) list->mlh_Head; hb->node.mln_Succ; hb = (struct HitBox*)hb->node.mln_Succ) {
    if (hb == data->spec->active_hitbox) {
      SetAPen(_rp(obj), data->active_hitbox_pen);
    }
    else {
      SetAPen(_rp(obj), data->hitbox_pen);
    }

    x1 = obj_center_x + hb->x1 * data->scale;
    y1 = obj_center_y + hb->y1 * data->scale;
    x2 = obj_center_x + hb->x2 * data->scale;
    y2 = obj_center_y + hb->y2 * data->scale;

    if (x1 < _right(obj) && x2 >= _left(obj)) {
      WORD xs = MAX(x1, _left(obj));
      WORD xe = MIN(x2, _right(obj));

      if (y1 >= _top(obj) && y1 < _bottom(obj)) {
         Move(_rp(obj), xs, y1);
         Draw(_rp(obj), xe, y1);
      }
      if (y2 >= _top(obj) && y2 < _bottom(obj)) {
         Move(_rp(obj), xs, y2);
         Draw(_rp(obj), xe, y2);
      }
    }
    if (y1 < _bottom(obj) && y2 >= _top(obj)) {
      WORD ys = MAX(y1, _top(obj));
      WORD ye = MIN(y2, _bottom(obj));
      if (x1 >= _left(obj) && x1 < _right(obj)) {
        Move(_rp(obj), x1, ys);
        Draw(_rp(obj), x1, ye);
      }
      if (x2 >= _left(obj) && x2 < _right(obj)) {
        Move(_rp(obj), x2, ys);
        Draw(_rp(obj), x2, ye);
      }
    }
  }
}

VOID drawClear(struct IClass *cl, Object *obj)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) > 8) {
    FillPixelArray(_rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), data->sheet.clut->colors[0].value);
  }
  else {
    SetAPen(_rp(obj),_dri(obj)->dri_Pens[SHINEPEN]);
    RectFill(_rp(obj),_mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
  }
}

VOID draw(struct IClass *cl, Object *obj)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->spec) {
    drawClear(cl, obj);
    drawSpec(cl, obj);
    drawAxis(cl, obj);
    drawHitBoxes(cl, obj);
  }
  else {
    drawClear(cl, obj);
    drawAxis(cl, obj);
  }
}

SAVEDS ULONG mDraw(struct IClass *cl, Object *obj, struct MUIP_Draw* msg)
{
	struct cl_Data *data = INST_DATA(cl, obj);

  DoSuperMethodA(cl, obj, (Msg)msg);

	if (msg->flags & MADF_DRAWUPDATE)
	{
    if (data->sheet.image) {
      draw(cl, obj);
    }
    else {
      SetAPen(_rp(obj),_dri(obj)->dri_Pens[SHINEPEN]);
      RectFill(_rp(obj),_mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
    }
	}
	else if (msg->flags & MADF_DRAWOBJECT)
	{
    if (data->sheet.image) {
      draw(cl, obj);
    }
    else {
      SetAPen(_rp(obj),_dri(obj)->dri_Pens[SHINEPEN]);
      RectFill(_rp(obj),_mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
    }
	}

	return 0;
}
///
///mSetup()
SAVEDS ULONG mSetup(struct IClass *cl,Object *obj, struct MUIP_HandleInput *msg)
{
	if (!(DoSuperMethodA(cl, obj, (Msg)msg)))
		return FALSE;

	MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY);

	return TRUE;
}
///
///mCleanup
SAVEDS ULONG mCleanup(struct IClass *cl,Object *obj, struct MUIP_HandleInput *msg)
{
	MUI_RejectIDCMP(obj, IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY);
	return DoSuperMethodA(cl, obj, (Msg)msg);
}
///
///mHandleInput()
SAVEDS ULONG mHandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleInput *msg)
{
	#define _between(a, x, b) ((x)>=(a) && (x)<=(b))
	#define _isinobject(x,y) (_between(_mleft(obj), (x), _mright(obj)) && _between(_mtop(obj), (y), _mbottom(obj)))

	struct cl_Data *data = INST_DATA(cl, obj);

	if (msg->imsg)
	{
		switch (msg->imsg->Class)
		{
			case IDCMP_MOUSEBUTTONS:
			{
				if (msg->imsg->Code == SELECTDOWN)
				{
					if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
					{
						data->x = msg->imsg->MouseX;
						data->y = msg->imsg->MouseY;
						MUI_Redraw(obj, MADF_DRAWUPDATE);
						MUI_RequestIDCMP(obj, IDCMP_MOUSEMOVE);
					}
				}
				else
					MUI_RejectIDCMP(obj, IDCMP_MOUSEMOVE);
			}
			break;

			case IDCMP_MOUSEMOVE:
			{
				if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
				{
					data->x = msg->imsg->MouseX;
					data->y = msg->imsg->MouseY;
					MUI_Redraw(obj, MADF_DRAWUPDATE);
				}
			}
			break;
		}
	}

	return DoSuperMethodA(cl, obj, (Msg)msg);
}
///
///mUpdate()
SAVEDS ULONG mUpdate(struct IClass *cl, Object *obj, Msg msg)
{
	struct cl_Data *data = INST_DATA(cl, obj);

  if (data->sheet.image) {
    draw(cl, obj);
  }

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct TagItem *tags, *tag;
  struct ILBMImage* img = NULL;
  struct ImageSpec* spec = NULL;
  ULONG scale = 1;

  for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
    switch (tag->ti_Tag) {
      case MUIA_ImageDisplay_Sheet:
        img = (struct ILBMImage*)tag->ti_Data;
      break;
      case MUIA_ImageDisplay_ImageSpec:
        spec = (struct ImageSpec*)tag->ti_Data;
      break;
      case MUIA_ImageDisplay_Scale:
        scale = tag->ti_Data;
      break;
    }
  }

  if ((obj = (Object *) DoSuperNew(cl, obj,
    //<SUPERCLASS TAGS HERE>
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    data->sheet.image = NULL;
    data->sheet.lpa = NULL;
    data->sheet.clut = NULL;
    data->spec = img ? spec : NULL;
    data->scale = scale;
    data->axis_pen = 0;

    if (img) {
      mSetSheet(cl, obj, img);
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

  //<FREE SUBCLASS INITIALIZATIONS HERE>
  if (data->sheet.image) {
    freeLUTPixelArray(data->sheet.lpa);
    freeCLUT(data->sheet.clut);
  }

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
      //<SUBCLASS ATTRIBUTES HERE>
      case MUIA_ImageDisplay_Sheet:
        mSetSheet(cl, obj, (struct ILBMImage*)tag->ti_Data);
      break;
      case MUIA_ImageDisplay_ImageSpec:
        if (data->sheet.image) {
          data->spec = (struct ImageSpec*)tag->ti_Data;
          draw(cl, obj);
        }
      break;
      case MUIA_ImageDisplay_Scale:
        if (data->scale != tag->ti_Data) {
          data->scale = tag->ti_Data;
          if (data->sheet.image && data->spec) {
            draw(cl, obj);
          }
        }
      break;
    }
  }

  return DoSuperMethodA(cl, obj, (Msg)msg);
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
    case MUIA_ImageDisplay_Sheet:
      *msg->opg_Storage = (ULONG)data->sheet.image;
    return TRUE;
    case MUIA_ImageDisplay_ImageSpec:
      *msg->opg_Storage = (ULONG)data->spec;
    return TRUE;
    case MUIA_ImageDisplay_Scale:
      *msg->opg_Storage = (ULONG)data->scale;
    return TRUE;
  }

  return DoSuperMethodA(cl, obj, (Msg)msg);
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
      return mAskMinMax(cl, obj, (APTR)msg);
    case MUIM_Show:
      return mShow(cl, obj, (APTR)msg);
    case MUIM_Hide:
      return mHide(cl, obj, (APTR)msg);
		case MUIM_Draw:
      return mDraw(cl, obj, (APTR)msg);
		case MUIM_HandleInput:
      return mHandleInput(cl, obj, (APTR)msg);
		case MUIM_Setup:
      return mSetup(cl, obj, (APTR)msg);
		case MUIM_Cleanup:
      return mCleanup(cl, obj, (APTR)msg);
    case MUIM_ImageDisplay_Update:
      return mUpdate(cl, obj, (APTR)msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_ImageDisplay(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
