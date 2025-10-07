/******************************************************************************
 * SpritebankCreator                                                          *
 ******************************************************************************/
///Defines
//private methods
#define MUIM_SpritebankCreator_CheckValidity     0x8043041A
#define MUIM_SpritebankCreator_SourceAcknowledge 0x8043041B
#define MUIM_SpritebankCreator_Create            0x8043041C
#define MUIM_SpritebankCreator_DisplaySource     0x8043041D
#define MUIM_SpritebankCreator_SetHotspotMargins 0x8043041E
///
///Includes
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <datatypes/pictureclass.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()
#include <clib/alib_stdio_protos.h>

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "popasl_string.h"
#include "integer_gadget.h"
#include "dtpicdisplay.h"
#include "spritebank_creator.h"
///
///Structs
struct cl_ObjTable
{
  Object* name;
  Object* source;
  Object* source_btn;
  Object* margin_x;
  Object* margin_y;
  Object* spacing_x;
  Object* spacing_y;
  Object* columns;
  Object* rows;
  Object* width;
  Object* height;
  Object* hotspot_x;
  Object* hotspot_y;
  Object* reverse_x;
  Object* reverse_y;
  Object* columns_first;
  Object* colors;
  Object* sfmode;
  Object* hsn_auto;
  Object* hsn;
  Object* small;
  Object* big;
  Object* create;
  Object* cancel;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  BOOL source_valid;
};

struct cl_Msg
{
  ULONG MethodID;
  STRPTR source;
};
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;
extern struct {
  STRPTR convert_tiles;
  STRPTR convert_map;
  STRPTR bob_sheeter;
  STRPTR sprite_banker;
  STRPTR gob_banker;
  struct {
    BOOL convert_tiles;
    BOOL convert_map;
    BOOL bob_sheeter;
    BOOL sprite_banker;
    BOOL gob_banker;
  }avail;
}g_Tools;

extern Object* App;
extern Object* g_dtPicDisplay;
extern struct MUI_CustomClass *MUIC_PopASLString;
extern struct MUI_CustomClass *MUIC_Integer;
extern struct MUI_CustomClass *MUIC_AckString;

extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;

static struct {
  STRPTR name;
  STRPTR source;
  STRPTR columns;
  STRPTR rows;
  STRPTR width;
  STRPTR height;
  STRPTR margin;
  STRPTR margin_x;
  STRPTR margin_y;
  STRPTR spacing;
  STRPTR spacing_x;
  STRPTR spacing_y;
  STRPTR hotspot;
  STRPTR hotspot_x;
  STRPTR hotspot_y;
  STRPTR reverse;
  STRPTR reverse_x;
  STRPTR reverse_y;
  STRPTR columns_first;
  STRPTR colors;
  STRPTR sfmode;
  STRPTR hsn_auto;
  STRPTR hsn;
  STRPTR small;
  STRPTR big;
  STRPTR create;
  STRPTR cancel;
}help_string = {
  "Filename for the sprite bank.",
  "Source ILBM sheet.",
  "Columns of images on the sheet.",
  "Rows of images on the sheet.",
  "The width of image columns.",
  "The height of image rows.",
  "Pixel spacings at the top and left side of\nthe ILBM file before columns and rows.",
  "Pixel spacing at the top of\nthe ILBM file before rows.",
  "Pixel spacing at the left side of\nthe ILBM file before columns.",
  "Pixel spacings between columns and rows.",
  "Pixel spacing between columns.",
  "Pixel spacing between rows.",
  "The offset of the hot spot for each image\nfrom the top left corner of the\nhypothetical image rectangle.",
  "Horizontal offset of the hot spot for each image from image column left edge.",
  "Vertical offset of the hot spot for each image from image row top.",
  "Grab images in reverse order.",
  "Grab images from left to right instead.",
  "Grab images from bottom to top instead.",
  "Grab images columns first.",
  "Color depth for the spritebank.",
  "Sprite fetch mode to display this spritebank.",
  "Let engine decide which hardware\nsprite(s) to use during runtime!\nRequires SMART_SPRITES.",
  "Hardware sprite to display the sprites in this bank.",
  "Use BYTEs instead of WORDs to store image offset information.\nRequires compiling the engine with SMALL_IMAGE_SIZES.",
  "Use UWORDs instead of UBYTESs to store image size information.\nRequires compiling the engine with BIG_IMAGE_SIZES.",
  "Grab and save the sprite bank.",
  "Cancel and quit the creator."
};
///

//Utility Functions
///isValidBitMap(file)
STATIC BOOL isValidBitMap(STRPTR file)
{
  BPTR fh;
  struct BitMapHeader bmhd;
  BOOL result = FALSE;

  fh = Open(file, MODE_OLDFILE);
  if (fh) {
    if (locateStrInFile(fh, "FORM")) {
      if (locateStrInFile(fh, "ILBM")) {
        if (locateStrInFile(fh, "BMHD")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct BitMapHeader) == Read(fh, &bmhd, sizeof(struct BitMapHeader))) {
            if (bmhd.bmh_Width && bmhd.bmh_Height && bmhd.bmh_Depth) result = TRUE;
          }
        }
      }
    }
    Close(fh);
  }

  return result;
}
///

///m_CheckValidity()
STATIC ULONG m_CheckValidity(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  STRPTR name;
  ULONG columns;
  ULONG rows;
  ULONG width;
  ULONG height;
  ULONG validity = FALSE;

  GetAttr(MUIA_String_Contents, data->obj_table.name, (ULONG*)&name);
  GetAttr(MUIA_Integer_Value, data->obj_table.columns, &columns);
  GetAttr(MUIA_Integer_Value, data->obj_table.rows, &rows);
  GetAttr(MUIA_Integer_Value, data->obj_table.width, &width);
  GetAttr(MUIA_Integer_Value, data->obj_table.height, &height);

  if (*name && data->source_valid && columns && rows && width && height) {
    validity = TRUE;
  }
  else validity = FALSE;

  DoMethod(data->obj_table.create, MUIM_Set, MUIA_Disabled, !validity);

  return 0;
}
///
///m_SourceAcknowledge()
STATIC ULONG m_SourceAcknowledge(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (isValidBitMap(msg->source)) {
    data->source_valid = TRUE;

    DoMethod(obj, MUIM_SpritebankCreator_CheckValidity);
  }
  else {
    data->source_valid = FALSE;
  }

  return 0;
}
///
///m_Create()
STATIC ULONG m_Create(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct ReturnInfo rtrn;
  UBYTE command[MAX_CMD_LENGTH];
  STRPTR name;
  STRPTR source;
  ULONG columns;
  ULONG rows;
  ULONG width;
  ULONG height;
  ULONG margin_x;
  ULONG margin_y;
  ULONG spacing_x;
  ULONG spacing_y;
  LONG hotspot_x;
  LONG hotspot_y;
  ULONG reverse_x;
  ULONG reverse_y;
  ULONG columns_first;
  ULONG colors;
  ULONG sfmode;
  ULONG hsn_auto;
  LONG hsn;
  ULONG small;
  ULONG big;
  ULONG name_len;
  STRPTR filename = NULL;
  STRPTR filepath = NULL;
  STRPTR error_string = NULL;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);
  GetAttr(MUIA_String_Contents, data->obj_table.name, (ULONG*)&name);
  GetAttr(MUIA_String_Contents, data->obj_table.source, (ULONG*)&source);
  GetAttr(MUIA_Integer_Value, data->obj_table.columns, &columns);
  GetAttr(MUIA_Integer_Value, data->obj_table.rows, &rows);
  GetAttr(MUIA_Integer_Value, data->obj_table.width, &width);
  GetAttr(MUIA_Integer_Value, data->obj_table.height, &height);
  GetAttr(MUIA_Integer_Value, data->obj_table.margin_x, &margin_x);
  GetAttr(MUIA_Integer_Value, data->obj_table.margin_y, &margin_y);
  GetAttr(MUIA_Integer_Value, data->obj_table.spacing_x, &spacing_x);
  GetAttr(MUIA_Integer_Value, data->obj_table.spacing_y, &spacing_y);
  GetAttr(MUIA_Integer_Value, data->obj_table.hotspot_x, &hotspot_x);
  GetAttr(MUIA_Integer_Value, data->obj_table.hotspot_y, &hotspot_y);

  GetAttr(MUIA_Selected, data->obj_table.reverse_x, &reverse_x);
  GetAttr(MUIA_Selected, data->obj_table.reverse_y, &reverse_y);
  GetAttr(MUIA_Selected, data->obj_table.columns_first, &columns_first);
  GetAttr(MUIA_Integer_Value, data->obj_table.colors, &colors);
  GetAttr(MUIA_Integer_Value, data->obj_table.sfmode, &sfmode);
  GetAttr(MUIA_Selected, data->obj_table.hsn_auto, &hsn_auto);
  GetAttr(MUIA_Integer_Value, data->obj_table.hsn, &hsn);
  GetAttr(MUIA_Selected, data->obj_table.small, &small);
  GetAttr(MUIA_Selected, data->obj_table.big, &big);

  if (hsn_auto) {
    hsn = -1;
  }

  name_len = strlen(name);
  if (name_len >= 4) {
    if (strcmp(&name[name_len - 4], ".spr")) {
      filename = makePath(NULL, name, ".spr");
    }
    else {
      filename = makeString(name);
    }
  }
  else {
    filename = makePath(NULL, name, ".spr");
  }

  if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Save SpriteBank",
                                    ASLFR_Window, window,
                                    ASLFR_PositiveText, "Create",
                                    ASLFR_DrawersOnly, FALSE,
                                    ASLFR_DoSaveMode, TRUE,
                                    ASLFR_DoPatterns, TRUE,
                                    ASLFR_InitialPattern, "#?.spr",
                                    g_Project.data_drawer ? ASLFR_InitialDrawer : TAG_IGNORE, g_Project.data_drawer,
                                    ASLFR_InitialFile, filename,
                                    TAG_END) && strlen(g_FileReq->fr_File)) {

    filepath = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);

    if (filepath) {
      if (Exists(filepath)) {
        if (!MUI_Request(App, obj, NULL, "SpriteBank Creator", "*_Overwrite|_Cancel", "SpriteBank file already exists!")) {
          goto error;
        }
      }

      sprintf(command, "%s \"%s\" \"%s\" %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %s %s %s %s %s",
        g_Tools.sprite_banker, source, filepath, margin_x, margin_y, spacing_x, spacing_y, columns, rows, width, height, colors, sfmode, hsn, hotspot_x, hotspot_y,
        reverse_x ? "REVX" : "",
        reverse_y ? "REVY" : "",
        columns_first ? "COLFIRST" : "",
        small ? "SMALL" : "",
        big ? "BIG" : "");

      if (execute(&rtrn, command))
        error_string = rtrn.string;
      else
        error_string = "Tools/SpriteBanker could not be found!";
    }
  }

error:
  freeString(filename);
  freeString(filepath);

  if (error_string) {
    MUI_Request(App, obj, NULL, "SpriteBank Creator", "*_OK", error_string);
  }

  return 0;
}
///
///m_DisplaySource()
STATIC ULONG m_DisplaySource(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->source_valid) {
    STRPTR picture;

    GetAttr(MUIA_String_Contents, data->obj_table.source, (ULONG*)&picture);

    DoMethod(g_dtPicDisplay, MUIM_Set, MUIA_DtPicDisplay_Picture, (ULONG)picture);
  }

  return 0;
}
///
///m_SetHotspotMargins()
STATIC ULONG m_SetHotspotMargins(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG small_sizes;

  get(data->obj_table.small, MUIA_Selected, &small_sizes);

  if (small_sizes) {
    DoMethod(data->obj_table.hotspot_x, MUIM_Set, MUIA_Integer_Max, 127);
    DoMethod(data->obj_table.hotspot_x, MUIM_Set, MUIA_Integer_Min, -128);
    DoMethod(data->obj_table.hotspot_y, MUIM_Set, MUIA_Integer_Max, 127);
    DoMethod(data->obj_table.hotspot_y, MUIM_Set, MUIA_Integer_Min, -128);
  }
  else {
    DoMethod(data->obj_table.hotspot_x, MUIM_Set, MUIA_Integer_Max, 32767);
    DoMethod(data->obj_table.hotspot_x, MUIM_Set, MUIA_Integer_Min, -32768);
    DoMethod(data->obj_table.hotspot_y, MUIM_Set, MUIA_Integer_Max, 32767);
    DoMethod(data->obj_table.hotspot_y, MUIM_Set, MUIA_Integer_Min, -32768);
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
    Object* name;
    Object* source;
    Object* source_string;
    Object* source_pop;
    Object* source_btn;
    Object* margin_x;
    Object* margin_y;
    Object* spacing_x;
    Object* spacing_y;
    Object* columns;
    Object* rows;
    Object* width;
    Object* height;
    Object* hotspot_x;
    Object* hotspot_y;
    Object* reverse_x;
    Object* reverse_y;
    Object* columns_first;
    Object* colors;
    Object* sfmode;
    Object* hsn_auto;
    Object* hsn;
    Object* small;
    Object* big;
    Object* create;
    Object* cancel;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','9'),
    MUIA_Window_Title, "Sprite Bank Creator",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Columns, 2,
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Name:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.name, TAG_END),
        MUIA_Group_Child, (objects.name = MUI_NewObject(MUIC_String,
          MUIA_ShortHelp, help_string.name,
          MUIA_Frame, MUIV_Frame_String,
          MUIA_String_Reject, DOS_RESERVED DOS_UNRECOMMENDED,
        TAG_END)),
        MUIA_Group_Child, (objects.source_btn = MUI_NewObject(MUIC_Text,
          MUIA_Text_Contents, "Source:",
          MUIA_HorizWeight, 0,
          MUIA_ShortHelp, help_string.source,
          MUIA_InputMode, MUIV_InputMode_RelVerify,
          MUIA_Frame, MUIV_Frame_Button,
        TAG_END)),
        MUIA_Group_Child, (objects.source = NewObject(MUIC_PopASLString->mcc_Class, NULL,
          MUIA_PopASLString_Requester, g_FileReq,
          MUIA_PopASLString_IgnoreContents, TRUE,
          MUIA_ShortHelp, help_string.source,
          ASLFR_TitleText, "Please select ilbm bob sheet file...",
          ASLFR_PositiveText, "Open",
          ASLFR_DoSaveMode, FALSE,
          ASLFR_DrawersOnly, FALSE,
          ASLFR_DoPatterns, TRUE,
          ASLFR_InitialPattern, "#?.(iff|ilbm)",
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Columns:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.columns, TAG_END),
        MUIA_Group_Child, (objects.columns = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.columns,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 8192,
          MUIA_String_MaxLen, 5,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Rows:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.rows, TAG_END),
        MUIA_Group_Child, (objects.rows = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.rows,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 8192,
          MUIA_String_MaxLen, 5,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Width:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.width, TAG_END),
        MUIA_Group_Child, (objects.width = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.width,
          MUIA_Integer_Input, TRUE,
          MUIA_Integer_Value, 0,
          MUIA_Integer_Incr, 1,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 0,
          MUIA_Integer_Max, 512,
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
          MUIA_Integer_Max, 8192,
          MUIA_String_MaxLen, 5,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Margin:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.margin, TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Horiz, TRUE,
          MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "X:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.margin_x, TAG_END),
          MUIA_Group_Child, (objects.margin_x = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_ShortHelp, help_string.margin_x,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 0,
            MUIA_Integer_Max, 8192,
            MUIA_String_MaxLen, 5,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Y:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.margin_y, TAG_END),
          MUIA_Group_Child, (objects.margin_y = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_ShortHelp, help_string.margin_y,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 0,
            MUIA_Integer_Max, 8192,
            MUIA_String_MaxLen, 5,
          TAG_END)),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Spacing:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.spacing, TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Horiz, TRUE,
          MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "X:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.spacing_x, TAG_END),
          MUIA_Group_Child, (objects.spacing_x = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_ShortHelp, help_string.spacing_x,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 0,
            MUIA_Integer_Max, 8192,
            MUIA_String_MaxLen, 5,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Y:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.spacing_y, TAG_END),
          MUIA_Group_Child, (objects.spacing_y = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_ShortHelp, help_string.spacing_y,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 0,
            MUIA_Integer_Max, 8192,
            MUIA_String_MaxLen, 5,
          TAG_END)),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Hotspot:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.hotspot, TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Horiz, TRUE,
          MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "X:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.hotspot_x, TAG_END),
          MUIA_Group_Child, (objects.hotspot_x = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_ShortHelp, help_string.hotspot_x,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, -128,
            MUIA_Integer_Max, 127,
            MUIA_String_MaxLen, 7,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Y:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.hotspot_y, TAG_END),
          MUIA_Group_Child, (objects.hotspot_y = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_ShortHelp, help_string.hotspot_y,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, -128,
            MUIA_Integer_Max, 127,
            MUIA_String_MaxLen, 7,
          TAG_END)),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Reverse:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.reverse, TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Horiz, TRUE,
          MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "X:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.reverse_x, TAG_END),
          MUIA_Group_Child, MUI_NewCheckMark(&objects.reverse_x, FALSE, NULL, 0, help_string.reverse_x),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Y:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.reverse_y, TAG_END),
          MUIA_Group_Child, MUI_NewCheckMark(&objects.reverse_y, FALSE, NULL, 0, help_string.reverse_y),
          MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, MUIA_HorizWeight, 4000, TAG_END),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Columns first:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.columns_first, TAG_END),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.columns_first, FALSE, NULL, 0, help_string.columns_first),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Colors:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.colors, TAG_END),
        MUIA_Group_Child, (objects.colors = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.colors,
          MUIA_Integer_Input, FALSE,
          MUIA_Integer_Value, 4,
          MUIA_Integer_Incr, 12,
          MUIA_Integer_Buttons, TRUE,
          MUIA_Integer_Min, 4,
          MUIA_Integer_Max, 16,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Fetch Mode:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.sfmode, TAG_END),
        MUIA_Group_Child, (objects.sfmode = NewObject(MUIC_Integer->mcc_Class, NULL,
          MUIA_ShortHelp, help_string.sfmode,
          MUIA_Integer_124, TRUE,
        TAG_END)),
        MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Hardware Sprite:", MUIA_HorizWeight, 0, MUIA_ShortHelp, help_string.hsn_auto, TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Horiz, TRUE,
          MUIA_Group_Child, MUI_NewCheckMark(&objects.hsn_auto, TRUE, NULL, 0, help_string.hsn_auto),
          MUIA_Group_Child, (objects.hsn = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_ShortHelp, help_string.hsn,
            MUIA_Disabled, TRUE,
            MUIA_HorizWeight, 4000,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 0,
            MUIA_Integer_Max, 7,
          TAG_END)),
        TAG_END),
      TAG_END),
      MUIA_Group_Child, MUI_NewCheckMark(&objects.small, FALSE, "Small sizes", 0, help_string.small),
      MUIA_Group_Child, MUI_NewCheckMark(&objects.big, FALSE, "Big sizes", 0, help_string.big),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Child, (objects.create = MUI_NewButton("Create", NULL, help_string.create)),
        MUIA_Group_Child, (objects.cancel = MUI_NewButton("Cancel", NULL, help_string.cancel)),
      TAG_END),
    TAG_END),
  TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    get(objects.source, MUIA_PopASLString_StringObject, &objects.source_string);
    get(objects.source, MUIA_PopASLString_PopButton, &objects.source_pop);
    data->obj_table.name = objects.name;
    data->obj_table.source = objects.source;
    data->obj_table.source_btn = objects.source_btn;
    data->obj_table.margin_x = objects.margin_x;
    data->obj_table.margin_y = objects.margin_y;
    data->obj_table.spacing_x = objects.spacing_x;
    data->obj_table.spacing_y = objects.spacing_y;
    data->obj_table.columns = objects.columns;
    data->obj_table.rows = objects.rows;
    data->obj_table.width = objects.width;
    data->obj_table.height = objects.height;
    data->obj_table.hotspot_x = objects.hotspot_x;
    data->obj_table.hotspot_y = objects.hotspot_y;
    data->obj_table.reverse_x = objects.reverse_x;
    data->obj_table.reverse_y = objects.reverse_y;
    data->obj_table.columns_first = objects.columns_first;
    data->obj_table.colors = objects.colors;
    data->obj_table.sfmode = objects.sfmode;
    data->obj_table.hsn_auto = objects.hsn_auto;
    data->obj_table.hsn = objects.hsn;
    data->obj_table.small = objects.small;
    data->obj_table.big = objects.big;
    data->obj_table.create = objects.create;
    data->obj_table.cancel = objects.cancel;

    data->source_valid = FALSE;

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.name,
                                             objects.source_btn,
                                             objects.source_string,
                                             objects.source_pop,
                                             MUI_GetChild(objects.columns, 1),
                                             MUI_GetChild(objects.rows, 1),
                                             MUI_GetChild(objects.width, 1),
                                             MUI_GetChild(objects.height, 1),
                                             MUI_GetChild(objects.margin_x, 1),
                                             MUI_GetChild(objects.margin_y, 1),
                                             MUI_GetChild(objects.spacing_x, 1),
                                             MUI_GetChild(objects.spacing_y, 1),
                                             MUI_GetChild(objects.hotspot_x, 1),
                                             MUI_GetChild(objects.hotspot_y, 1),
                                             objects.reverse_x,
                                             objects.reverse_y,
                                             objects.columns_first,
                                             MUI_GetChild(objects.colors, 2),
                                             MUI_GetChild(objects.colors, 3),
                                             MUI_GetChild(objects.sfmode, 2),
                                             MUI_GetChild(objects.sfmode, 3),
                                             objects.hsn_auto,
                                             MUI_GetChild(objects.hsn, 1),
                                             objects.small,
                                             objects.big,
                                             objects.create,
                                             objects.cancel,
                                             NULL);

    DoMethod(objects.create, MUIM_Set, MUIA_Disabled, TRUE);

    DoMethod(data->obj_table.small, MUIM_Set, MUIA_Selected, TRUE);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.name, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1,
      MUIM_SpritebankCreator_CheckValidity);

    DoMethod(objects.source, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 2,
      MUIM_SpritebankCreator_SourceAcknowledge, MUIV_TriggerValue);

    DoMethod(objects.columns, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_SpritebankCreator_CheckValidity);

    DoMethod(objects.rows, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_SpritebankCreator_CheckValidity);

    DoMethod(objects.width, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_SpritebankCreator_CheckValidity);

    DoMethod(objects.height, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_SpritebankCreator_CheckValidity);

    DoMethod(objects.hsn_auto, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, objects.hsn, 3,
      MUIM_Set, MUIA_Disabled, MUIV_TriggerValue);

    DoMethod(data->obj_table.small, MUIM_Notify, MUIA_Selected, TRUE, data->obj_table.big, 3,
      MUIM_Set, MUIA_Selected, FALSE);

    DoMethod(data->obj_table.big, MUIM_Notify, MUIA_Selected, TRUE, data->obj_table.small, 3,
      MUIM_Set, MUIA_Selected, FALSE);

    DoMethod(objects.create, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_SpritebankCreator_Create);

    DoMethod(objects.cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.source_btn, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_SpritebankCreator_DisplaySource);

    DoMethod(objects.small, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1,
      MUIM_SpritebankCreator_SetHotspotMargins);

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
      case MUIA_SpritebankCreator_AssetsDrawer:
        DoMethod(data->obj_table.source, MUIM_Set, ASLFR_InitialDrawer, tag->ti_Data);
      break;
    }
  }

  return (DoSuperMethodA(cl, obj, (Msg) msg));
}
///
///Overridden OM_GET
static ULONG m_Get(struct IClass* cl, Object* obj, struct opGet* msg)
{
  //struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->opg_AttrID)
  {
    //<SUBCLASS ATTRIBUTES HERE>
    /*
    case MUIA_SpritebankCreator_{Attribute}:
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
    //<DISPATCH SUBCLASS METHODS HERE>
    case MUIM_SpritebankCreator_CheckValidity:
      return m_CheckValidity(cl, obj, (struct cl_Msg*) msg);
    case MUIM_SpritebankCreator_SourceAcknowledge:
      return m_SourceAcknowledge(cl, obj, (struct cl_Msg*) msg);
    case MUIM_SpritebankCreator_Create:
      return m_Create(cl, obj, (struct cl_Msg*) msg);
    case MUIM_SpritebankCreator_DisplaySource:
      return m_DisplaySource(cl, obj, (struct cl_Msg*) msg);
    case MUIM_SpritebankCreator_SetHotspotMargins:
      return m_SetHotspotMargins(cl, obj, (struct cl_Msg*) msg);

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_SpritebankCreator(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
