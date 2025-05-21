/******************************************************************************
 * ImageEditor                                                                *
 ******************************************************************************/
///Defines
#define MODE_BLANK      0
#define MODE_BOBSHEET   1
#define MODE_SPRITEBANK 2

#define MUIM_ImageEditor_Load         0x80430601
#define MUIM_ImageEditor_Save         0x80430602
#define MUIM_ImageEditor_SaveAs       0x80430603
#define MUIM_ImageEditor_ChangeSpec   0x80430604
#define MUIM_ImageEditor_UpdateSpec   0x80430605
#define MUIM_ImageEditor_SelectHitBox 0x80430606
#define MUIM_ImageEditor_AddHitBox    0x80430607
#define MUIM_ImageEditor_RemoveHitBox 0x80430608

#define UPDATE_H_OFFS 0x01
#define UPDATE_V_OFFS 0x02
#define UPDATE_HITBOX_X1 0x03
#define UPDATE_HITBOX_Y1 0x04
#define UPDATE_HITBOX_X2 0x05
#define UPDATE_HITBOX_Y2 0x06
///
///Includes
#include <stdio.h>            // <-- For sprintf()
#include <string.h>            // <-- For sprintf()

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <libraries/asl.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "toolbar.h"
#include "integer_gadget.h"
#include "image_display.h"
#include "image_editor.h"
///
///Structs
struct cl_ObjTable
{
  Object* load;
  Object* save;
  Object* saveas;
  Object* undo;
  Object* redo;
  Object* file;
  Object* scale;
  Object* small;
  Object* big;
  Object* hitbox_small;
  Object* display;
  Object* image_group;
  Object* img_num;
  Object* width;
  Object* height;
  Object* h_offs;
  Object* v_offs;
  Object* hitbox_group;
  Object* hitbox_lv;
  Object* hitbox_x1;
  Object* hitbox_y1;
  Object* hitbox_x2;
  Object* hitbox_y2;
  Object* hitbox_add;
  Object* hitbox_rem;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  ULONG mode; //MODE_BOBSHEET or MODE_SPRITEBANK
  struct Sheet* sheet;
  BOOL edited;
};

struct cl_Msg
{
  ULONG MethodID;
  LONG value;
};

struct cl_UpdateMsg
{
  ULONG MethodID;
  ULONG update_mode;
  LONG value;
};
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;

extern Object* App;
extern struct MUI_CustomClass* MUIC_Integer;
extern struct MUI_CustomClass* MUIC_ImageDisplay;

extern struct Project {
  STRPTR directory;
  STRPTR settings_header;
  STRPTR assets_header;
  STRPTR palettes_header;
  STRPTR data_drawer;
  STRPTR assets_drawer;
}g_Project;
///
///Protos
ULONG m_FillHitBoxList(struct IClass* cl, Object* obj);
///
///Hooks
/******************************************************************************
 * Displays the hitbox item as its index on the list.                         *
 ******************************************************************************/
HOOKPROTO(hitbox_dispfunc, LONG, char **array, struct HitBox* p)
{
  if (p) {
    static UBYTE pos[8];
    sprintf(pos, "%ld", (LONG)array[-1] + 1);

    *array = pos;
  }

  return(0);
}
MakeStaticHook(hitbox_disphook, hitbox_dispfunc);
///

///checkFileType(file)
ULONG checkFileType(STRPTR file)
{
  ULONG ret_val = MODE_BLANK;
  BPTR fh = Open(file, MODE_OLDFILE);
  UBYTE id[8];

  if (fh) {
    Read(fh, id, 8);
    if (strncmp(id, "BOBSHEET", 8) == 0) {
      ret_val = MODE_BOBSHEET;
    }
    else if (strncmp(id, "SPRBANK", 7) == 0) {
      ret_val = MODE_SPRITEBANK;
    }

    Close(fh);
  }

  return ret_val;
}
///
///checkDataSizes(sheet)
/******************************************************************************
 * Returns the recommended data size type for the image sheet.                *
 ******************************************************************************/
UBYTE checkDataSizes(struct Sheet* sheet)
{
  UBYTE type = 0;
  ULONG i;
  ULONG i_max = sheet->num_images;
  BOOL small_sizes = TRUE;
  BOOL big_sizes = FALSE;
  BOOL has_hitboxes = FALSE;
  BOOL small_hitboxes = TRUE;
  struct HitBox* hb;

  for (i = 0; i < i_max; i++) {
    if (sheet->spec[i].width > 0xFF || sheet->spec[i].height > 0xFF) {
      big_sizes = TRUE;
      break;
    }
    if (small_sizes && (sheet->spec[i].h_offs > 127 || sheet->spec[i].h_offs < -128 ||
        sheet->spec[i].v_offs > 127 || sheet->spec[i].h_offs < -128)) {
      small_sizes = FALSE;
    }

    for (hb = (struct HitBox*) sheet->spec[i].hitboxes.mlh_Head; hb->node.mln_Succ; hb = (struct HitBox*)hb->node.mln_Succ) {
      has_hitboxes = TRUE;
      if (hb->x1 > 127 || hb->x1 < -128 || hb->x2 > 127 || hb->x2 < -128 || hb->y1 > 127 || hb->y1 < -128 || hb->y2 > 127 || hb->y2 < -128) {
        small_hitboxes = FALSE;
      }
    }
  }

  if (big_sizes) {
    type = 0x04;
  }
  else if (small_sizes) {
    type = 0x02;
  }

  if (has_hitboxes) {
    type |= 0x08;
    if (small_hitboxes) {
      type |= 0x10;
    }
  }

  return type;
}
///

///findOnSaveList(save_list, index)
struct HitBox* findOnSaveList(struct MinList* save_list, UWORD index)
{
  struct HitBox* hb;

  for (hb = (struct HitBox*)save_list->mlh_Head; hb->node.mln_Succ; hb = (struct HitBox*)hb->node.mln_Succ){
    if (hb->index == index) return hb;
  }

  return NULL;
}
///
///compareHitBoxes(save_list, hb1, hb2)
BOOL compareHitBoxes(struct MinList* save_list, struct HitBox* hb1, /*WARNING: must be a save list hitbox*/ struct HitBox* hb2)
{
  if (hb2->next == 0xFFFE) return FALSE;
  if (hb1->x1 != hb2->x1) return FALSE;
  if (hb1->y1 != hb2->y1) return FALSE;
  if (hb1->x2 != hb2->x2) return FALSE;
  if (hb1->y2 != hb2->y2) return FALSE;

  if (hb1->node.mln_Succ->mln_Succ) {
    if (hb2->next == 0xFFFF) return FALSE;
    else {
      return compareHitBoxes(save_list, (struct HitBox*)hb1->node.mln_Succ, findOnSaveList(save_list, hb2->next));
    }
  }
  else {
    if (hb2->next == 0xFFFF) return TRUE;
    else return FALSE;
  }

  return TRUE;
}
///
///createSaveHitBox(save_list, current_index, hitbox)
UWORD createSaveHitBox(struct MinList* save_list, UWORD* current_index, struct HitBox* hitbox)
{
  UWORD ret_val = *current_index;
  struct HitBox* hb_test;
  struct HitBox* hb_new;
  struct HitBox* hb_next;

  if (hitbox->node.mln_Succ) {
    for (hb_test = (struct HitBox*)save_list->mlh_Head; hb_test->node.mln_Succ; hb_test = (struct HitBox*)hb_test->node.mln_Succ) {
      if (compareHitBoxes(save_list, hitbox, hb_test)) {
        return hb_test->index;
      }
    }

    if (hitbox->node.mln_Succ->mln_Succ)
      hb_next = (struct HitBox*)hitbox->node.mln_Succ;
    else
      hb_next = NULL;

    hb_new = newHitBox(hitbox->x1, hitbox->y1, hitbox->x2, hitbox->y2, *current_index, 0xFFFE);
    if (hb_new) {
      AddTail((struct List*)save_list, (struct Node*)hb_new);
      *current_index = *current_index + 1;
      if (hb_next)
        hb_new->next = createSaveHitBox(save_list, current_index, hb_next);
      else
        hb_new->next = 0xFFFF;
    }

    return ret_val;
  }
  else
    return 0xFFFF;
}
///
///saveHitBoxes(fh, sheet, type)
VOID saveHitBoxes(BPTR fh, struct Sheet* sheet, UBYTE type)
{
  if (type & 0x08) {
    ULONG i;
    struct HitBox* hb_save;
    struct HitBox* hb_next;
    struct MinList save_list;
    UWORD* index = AllocPooled(g_MemoryPool, sizeof(UWORD)* sheet->num_images);
    UWORD num_hitboxes = 0;

    if (index) {
      NewList((struct List*)&save_list);

      //populate the save list
      for (i = 0; i < sheet->num_images; i++) {
        index[i] = createSaveHitBox(&save_list, &num_hitboxes, (struct HitBox*)sheet->spec[i].hitboxes.mlh_Head);
      }

      Write(fh, &num_hitboxes, sizeof(UWORD));
      Write(fh, index, sizeof(UWORD) * sheet->num_images);

      if (type & 0x10) {
        for (hb_save = (struct HitBox*)save_list.mlh_Head; hb_save->node.mln_Succ; hb_save = (struct HitBox*)hb_save->node.mln_Succ) {
          struct HitBox_small {
            BYTE x1, y1;
            BYTE x2, y2;
            UWORD next_hitbox;
          }hb;
          hb.x1 = hb_save->x1;
          hb.y1 = hb_save->y1;
          hb.x2 = hb_save->x2;
          hb.y2 = hb_save->y2;
          hb.next_hitbox = hb_save->next;

          Write(fh, &hb, sizeof(hb));
        }
      }
      else
      {
        for (hb_save = (struct HitBox*)save_list.mlh_Head; hb_save->node.mln_Succ; hb_save = (struct HitBox*)hb_save->node.mln_Succ) {
          struct HitBox_big {
            WORD x1, y1;
            WORD x2, y2;
            UWORD next_hitbox;
          }hb;
          hb.x1 = hb_save->x1;
          hb.y1 = hb_save->y1;
          hb.x2 = hb_save->x2;
          hb.y2 = hb_save->y2;
          hb.next_hitbox = hb_save->next;

          Write(fh, &hb, sizeof(hb));
        }
      }
      //Free the memory used
      FreePooled(g_MemoryPool, index, sizeof(UWORD) * sheet->num_images);
      for (hb_save = (struct HitBox*)save_list.mlh_Head; (hb_next = (struct HitBox*)hb_save->node.mln_Succ); hb_save = hb_next) {
        deleteHitBox(hb_save);
      }
    }
  }
}
///

//<DEFINE SUBCLASS METHODS HERE>

///m_Reset(cl, obj, msg)
ULONG m_Reset(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->sheet) {
    freeSheet(data->sheet);
    data->sheet = NULL;
    data->mode = MODE_BLANK;
    data->edited = FALSE;

    DoMethod(data->obj_table.file, MUIM_Set, MUIA_Text_Contents, "");
    DoMethod(data->obj_table.small,        MUIM_NoNotifySet, MUIA_Selected, TRUE);
    DoMethod(data->obj_table.big,          MUIM_NoNotifySet, MUIA_Selected, FALSE);
    DoMethod(data->obj_table.hitbox_small, MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(data->obj_table.hitbox_small, MUIM_Set, MUIA_Selected, TRUE);
    DoMethod(data->obj_table.display, MUIM_Set, MUIA_ImageDisplay_Sheet, NULL);
    DoMethod(data->obj_table.img_num, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
    DoMethod(data->obj_table.width,  MUIM_Set, MUIA_Integer_Value, 0);
    DoMethod(data->obj_table.height, MUIM_Set, MUIA_Integer_Value, 0);
    DoMethod(data->obj_table.h_offs, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
    DoMethod(data->obj_table.v_offs, MUIM_NoNotifySet, MUIA_Integer_Value, 0);

    DoMethod(data->obj_table.hitbox_lv, MUIM_List_Clear);
    DoMethod(data->obj_table.hitbox_x1, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
    DoMethod(data->obj_table.hitbox_y1, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
    DoMethod(data->obj_table.hitbox_x2, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
    DoMethod(data->obj_table.hitbox_y2, MUIM_NoNotifySet, MUIA_Integer_Value, 0);

    DoMethod(data->obj_table.image_group,  MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(data->obj_table.hitbox_group, MUIM_Set, MUIA_Disabled, TRUE);

    DoMethod(data->obj_table.hitbox_add, MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(data->obj_table.hitbox_rem, MUIM_Set, MUIA_Disabled, TRUE);

    DoMethod(data->obj_table.save, MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(data->obj_table.saveas, MUIM_Set, MUIA_Disabled, TRUE);
  }

  return 0;
}
///
///m_SetSheet(cl, obj, sheet, mode, filename)
ULONG m_SetSheet(struct IClass* cl, Object* obj, struct Sheet* sheet, ULONG mode, STRPTR filename)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  data->sheet = sheet;
  data->mode = mode;

  DoMethod(data->obj_table.file, MUIM_Set, MUIA_Text_Contents, filename);
  DoMethod(data->obj_table.small,        MUIM_Set, MUIA_Selected, sheet->type & 0x02);
  DoMethod(data->obj_table.big,          MUIM_Set, MUIA_Selected, sheet->type & 0x04);
  DoMethod(data->obj_table.hitbox_small, MUIM_Set, MUIA_Disabled, sheet->type &0x08 ? FALSE : TRUE);
  DoMethod(data->obj_table.hitbox_small, MUIM_Set, MUIA_Selected, sheet->type &0x08 ? sheet->type & 0x10 : TRUE);
  DoMethod(data->obj_table.display, MUIM_Set, MUIA_ImageDisplay_Sheet, sheet->image);
  DoMethod(data->obj_table.display, MUIM_Set, MUIA_ImageDisplay_ImageSpec, &sheet->spec[0]);

  DoMethod(data->obj_table.img_num, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.img_num, MUIM_Set, MUIA_Integer_Max, sheet->num_images - 1);
  DoMethod(data->obj_table.width,  MUIM_Set, MUIA_Integer_Value, sheet->spec[0].width);
  DoMethod(data->obj_table.height, MUIM_Set, MUIA_Integer_Value, sheet->spec[0].height);
  DoMethod(data->obj_table.h_offs, MUIM_NoNotifySet, MUIA_Integer_Value, sheet->spec[0].h_offs);
  DoMethod(data->obj_table.v_offs, MUIM_NoNotifySet, MUIA_Integer_Value, sheet->spec[0].v_offs);

  DoMethod(data->obj_table.hitbox_x1, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.hitbox_y1, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.hitbox_x2, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
  DoMethod(data->obj_table.hitbox_y2, MUIM_NoNotifySet, MUIA_Integer_Value, 0);

  DoMethod(data->obj_table.image_group, MUIM_Set, MUIA_Disabled, FALSE);
  DoMethod(data->obj_table.hitbox_add, MUIM_Set, MUIA_Disabled, FALSE);
  DoMethod(data->obj_table.saveas, MUIM_Set, MUIA_Disabled, FALSE);
  m_FillHitBoxList(cl, obj);

  return 0;
}
///
///m_LoadSheet(cl, obj, msg)
ULONG m_LoadSheet(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR path;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);

  if (data->edited) {
    if (!MUI_Request(App, obj, NULL, "Sheet Editor", "*_Discard|_Cancel", "Discard the changes made on the current sheet?")) {
      return 0;
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

  if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Load a bob sheet or sprite bank",
                                    ASLFR_Window, window,
                                    ASLFR_PositiveText, "Load",
                                    ASLFR_DrawersOnly, FALSE,
                                    ASLFR_DoSaveMode, FALSE,
                                    ASLFR_DoPatterns, TRUE,
                                    ASLFR_InitialPattern, "#?.(sht|spr)",
                                    g_Project.data_drawer ? ASLFR_InitialDrawer : TAG_IGNORE, g_Project.data_drawer,
                                    ASLFR_InitialFile, "",
                                    TAG_END) && strlen(g_FileReq->fr_File)) {
    path = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);
    if (path) {
      struct Sheet* sheet = NULL;
      ULONG mode = checkFileType(path);

      switch (mode) {
        case MODE_BOBSHEET:
          sheet = loadBOBSheet(path);
        break;
        case MODE_SPRITEBANK:
          sheet = loadSpriteBank(path);
        break;
        default:
          MUI_Request(App, obj, NULL, "Sheet Editor", "*_Ok", "File is not a Sevgi Engine Sprite Bank or Bob Sheet!");
      }

      if (sheet) {
        m_Reset(cl, obj, msg);
        m_SetSheet(cl, obj, sheet, mode, g_FileReq->fr_File);
      }

      freeString(path);
    }
  }

  DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);

  return 0;
}
///
///m_SaveSheet(cl, obj, msg)
ULONG m_SaveSheet(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  BPTR fh;
  ULONG i;

  if (data->sheet) {
    //put user selected size type settings onto the sheet
    ULONG small_sizes;
    ULONG big_sizes;
    ULONG small_hitbox_sizes;
    UBYTE new_type = 0;

    get(data->obj_table.small, MUIA_Selected, &small_sizes);
    get(data->obj_table.big, MUIA_Selected, &big_sizes);
    get(data->obj_table.hitbox_small, MUIA_Selected, &small_hitbox_sizes);

    new_type = small_sizes ? 0x02 : (big_sizes ? 0x04 : 0x00);
    if (data->sheet->num_hitboxes) new_type |= (small_hitbox_sizes ? 0x18 : 0x08);
    data->sheet->type = new_type;

    switch (data->mode) {
      case MODE_BOBSHEET:
      {
        fh = Open(data->sheet->sheet_file, MODE_READWRITE);

        if (fh) {
          UBYTE type = data->sheet->type & 0x06; //Let's temporarily turn off bit 0
          UBYTE hbx_type = data->sheet->type & 0x18;
          UBYTE recommended_type = checkDataSizes(data->sheet);
          UBYTE recommended_img_type = recommended_type & 0x06;
          UBYTE recommended_hbx_type = recommended_type & 0x18;

          if (type == 0x04) {
            MUI_Request(App, obj, NULL, "Sheet Editor", "*_Ok", "This bob sheet requires compiling the engine with BIG_IMAGE_SIZES!");
          }
          else if (type != recommended_img_type) {
            if (recommended_img_type == 0x02) {
              if (MUI_Request(App, obj, NULL, "Sheet Editor", "Use _small sizes|*Use _default sizes", "This bob sheet uses small size offsets and can be saved using small data size.\nWhich also suggests compiling the engine with SMALL_IMAGE_SIZES.\nWould you want to save with small data sizes?")) {
                type = recommended_img_type;
              }
            }
            else {
              if (MUI_Request(App, obj, NULL, "Sheet Editor", "Continue|*_Cancel", "This bob sheet was created with %s size offsets.\nAfter the edits made it now requires %s data sizes.\nIf you click 'Continue' the sheet will be saved using %s data sizes.%s\n\nWhat would you want to do?",
                (ULONG)(type == 0x02 ? "small" : "regular"), (ULONG)(recommended_img_type == 0x02 ? "small" : "regular"), (ULONG)(recommended_img_type == 0x02 ? "small" : "regular"), (ULONG)(recommended_img_type == 0x02 ? "\nThis will require compiling the engine with SMALL_IMAGE_SIZES for this sheet to work." : ""))) {
                type = recommended_img_type;
              }
              else {
                Close(fh);
                return 0;
              }
            }
          }
          else if (type == 0x02) {
            MUI_Request(App, obj, NULL, "Sheet Editor", "*_Ok", "This bob sheet requires compiling the engine with SMALL_IMAGE_SIZES!");
          }

          if (recommended_hbx_type & 0x8) {
            if (hbx_type & 0x8) {
              if (hbx_type != recommended_hbx_type) {
                if (recommended_hbx_type == 0x18) {
                  //was big, now small
                  if (MUI_Request(App, obj, NULL, "Sheet Editor", "Use _small sizes|*Use _default sizes", "The hitboxes on this bob sheet used to have big size offsets.\nAfter the edits made it now can be saved with small data sizes.\nWhich also suggests compiling the engine with SMALL_IMAGE_SIZES.\n\nWhat would you want to do?")) {
                    type |= recommended_hbx_type;
                  }
                  else {
                    type |= hbx_type;
                  }
                }
                else {
                  //was small, now big
                  if (MUI_Request(App, obj, NULL, "Sheet Editor", "Continue|*_Cancel", "The hitboxes on this bob sheet used to have small size offsets.\nAfter the edits made it now requires big data sizes.\nIf you click 'Continue' the sheet will be saved using big data sizes.\nThis will require compiling the engine withOUT SMALL_IMAGE_SIZES for this sheet to work.\n\nWhat would you want to do?")) {
                    type |= recommended_hbx_type;
                  }
                  else {
                    Close(fh);
                    return 0;
                  }
                }
              }
              else {
                type |= recommended_hbx_type;
              }
            }
            else {
              type |= recommended_hbx_type;
            }
          }

          type |= 0x01; //This editor can only save BST_IRREGULAR sheets

          Write(fh, "BOBSHEET", 8);
          Write(fh, data->sheet->ilbm_file, strlen(data->sheet->ilbm_file) + 1);
          Write(fh, &type, 1);
          Write(fh, &data->sheet->num_images, 2);

          switch (type & 0x06) {
            case 0x02: //SMALL_SIZES
              for (i = 0; i < data->sheet->num_images; i++) {
                UBYTE width = data->sheet->spec[i].width;
                UBYTE height = data->sheet->spec[i].height;
                BYTE h_offs = data->sheet->spec[i].h_offs;
                BYTE v_offs = data->sheet->spec[i].v_offs;
                UBYTE word = data->sheet->spec[i].x / 16;
                UWORD row = data->sheet->spec[i].y;

                Write(fh, &width, sizeof(width));
                Write(fh, &height, sizeof(height));
                Write(fh, &h_offs, sizeof(h_offs));
                Write(fh, &v_offs, sizeof(v_offs));
                Write(fh, &word, sizeof(word));
                Write(fh, &row, sizeof(row));
              }
            break;
            case 0x04: //BIG_SIZES
              for (i = 0; i < data->sheet->num_images; i++) {
                UWORD width = data->sheet->spec[i].width;
                UWORD height = data->sheet->spec[i].height;
                WORD h_offs = data->sheet->spec[i].h_offs;
                WORD v_offs = data->sheet->spec[i].v_offs;
                UBYTE word = data->sheet->spec[i].x / 16;
                UWORD row = data->sheet->spec[i].y;

                Write(fh, &width, sizeof(width));
                Write(fh, &height, sizeof(height));
                Write(fh, &h_offs, sizeof(h_offs));
                Write(fh, &v_offs, sizeof(v_offs));
                Write(fh, &word, sizeof(word));
                Write(fh, &row, sizeof(row));
              }
            break;
            default:
              for (i = 0; i < data->sheet->num_images; i++) {
                UBYTE width = data->sheet->spec[i].width;
                UBYTE height = data->sheet->spec[i].height;
                WORD h_offs = data->sheet->spec[i].h_offs;
                WORD v_offs = data->sheet->spec[i].v_offs;
                UBYTE word = data->sheet->spec[i].x / 16;
                UWORD row = data->sheet->spec[i].y;

                Write(fh, &width, sizeof(width));
                Write(fh, &height, sizeof(height));
                Write(fh, &h_offs, sizeof(h_offs));
                Write(fh, &v_offs, sizeof(v_offs));
                Write(fh, &word, sizeof(word));
                Write(fh, &row, sizeof(row));
              }
          }

          saveHitBoxes(fh, data->sheet, type);

          SetFileSize(fh, 0, OFFSET_CURRENT);
          Close(fh);
        }
      }
      break;
      case MODE_SPRITEBANK:
      {
        UBYTE type = data->sheet->type & 0x06;
        UBYTE hbx_type = data->sheet->type & 0x18;
        UBYTE* table = (UBYTE*)data->sheet->sprite_bank + 10;
        ULONG table_size = type == 0x02 ? 8 : (type == 0x04 ? 12 : 10);
        UBYTE* spr_data = table + (table_size * data->sheet->num_images) + 2;
        UWORD spr_data_size = *(UWORD*)(spr_data - 2);

        fh = Open(data->sheet->sheet_file, MODE_READWRITE);

        if (fh) {
          UBYTE recommended_type = checkDataSizes(data->sheet);
          UBYTE recommended_img_type = recommended_type & 0x06;
          UBYTE recommended_hbx_type = recommended_type & 0x18;

          if (type == 0x04) {
            MUI_Request(App, obj, NULL, "Sheet Editor", "*_Ok", "This spritebank requires compiling the engine with BIG_IMAGE_SIZES!");
          }
          else if (type != recommended_img_type) {
            if (recommended_img_type == 0x02) {
              if (MUI_Request(App, obj, NULL, "Sheet Editor", "Use _small sizes|*Use _default sizes", "This spritebank uses small size offsets and can be saved using small data size.\nWhich also suggests compiling the engine with SMALL_IMAGE_SIZES.\nWould you want to save with small data sizes?")) {
                type = recommended_img_type;
              }
            }
            else {
              if (MUI_Request(App, obj, NULL, "Sheet Editor", "Continue|*_Cancel", "This spritebank was created with %s size offsets.\nAfter the edits made it now requires %s data sizes.\nIf you click 'Continue' the bank will be saved using %s data sizes.%s\n\nWhat would you want to do?",
                (ULONG)(type == 0x02 ? "small" : "regular"), (ULONG)(recommended_img_type == 0x02 ? "small" : "regular"), (ULONG)(recommended_img_type == 0x02 ? "small" : "regular"), (ULONG)(recommended_img_type == 0x02 ? "\nThis will require compiling the engine with SMALL_IMAGE_SIZES for this spritebank to work." : ""))) {
                type = recommended_img_type;
              }
              else {
                Close(fh);
                return 0;
              }
            }
          }
          else if (type == 0x02) {
            MUI_Request(App, obj, NULL, "Sheet Editor", "*_Ok", "This spritebank requires compiling the engine with SMALL_IMAGE_SIZES!");
          }

          if (recommended_hbx_type & 0x8) {
            if (hbx_type & 0x8) {
              if (hbx_type != recommended_hbx_type) {
                if (recommended_hbx_type == 0x18) {
                  //was big, now small
                  if (MUI_Request(App, obj, NULL, "Sheet Editor", "Use _small sizes|*Use _default sizes", "The hitboxes on this bob sheet used to have big size offsets.\nAfter the edits made it now can be saved with small data sizes.\nWhich also suggests compiling the engine with SMALL_IMAGE_SIZES.\n\nWhat would you want to do?")) {
                    type |= recommended_hbx_type;
                  }
                  else {
                    type |= hbx_type;
                  }
                }
                else {
                  //was small, now big
                  if (MUI_Request(App, obj, NULL, "Sheet Editor", "Continue|*_Cancel", "The hitboxes on this bob sheet used to have small size offsets.\nAfter the edits made it now requires big data sizes.\nIf you click 'Continue' the sheet will be saved using big data sizes.\nThis will require compiling the engine withOUT SMALL_IMAGE_SIZES for this sheet to work.\n\nWhat would you want to do?")) {
                    type |= recommended_hbx_type;
                  }
                  else {
                    Close(fh);
                    return 0;
                  }
                }
              }
              else {
                type |= recommended_hbx_type;
              }
            }
            else {
              type |= recommended_hbx_type;
            }
          }

          Write(fh, "SPRBANK", 7);
          Write(fh, &type, 1);
          Write(fh, &data->sheet->num_images, 2);

          switch (type & 0x06) {
            case 0x02: //SMALL_SIZES
              for (i = 0; i < data->sheet->num_images; i++) {
                UBYTE width = data->sheet->spec[i].width;
                UBYTE height = data->sheet->spec[i].height;
                BYTE h_offs = data->sheet->spec[i].h_offs;
                BYTE v_offs = data->sheet->spec[i].v_offs;

                Write(fh, table + table_size * i, 4); // Writes offset, type and hsn

                Write(fh, &width, sizeof(width));
                Write(fh, &height, sizeof(height));
                Write(fh, &h_offs, sizeof(h_offs));
                Write(fh, &v_offs, sizeof(v_offs));
              }
            break;
            case 0x04: //BIG_SIZES
              for (i = 0; i < data->sheet->num_images; i++) {
                UWORD width = data->sheet->spec[i].width;
                UWORD height = data->sheet->spec[i].height;
                WORD h_offs = data->sheet->spec[i].h_offs;
                WORD v_offs = data->sheet->spec[i].v_offs;

                Write(fh, table + table_size * i, 4); // Writes offset, type and hsn

                Write(fh, &width, sizeof(width));
                Write(fh, &height, sizeof(height));
                Write(fh, &h_offs, sizeof(h_offs));
                Write(fh, &v_offs, sizeof(v_offs));
              }
            break;
            default:
              for (i = 0; i < data->sheet->num_images; i++) {
                UBYTE width = data->sheet->spec[i].width;
                UBYTE height = data->sheet->spec[i].height;
                WORD h_offs = data->sheet->spec[i].h_offs;
                WORD v_offs = data->sheet->spec[i].v_offs;

                Write(fh, table + table_size * i, 4); // Writes offset, type and hsn

                Write(fh, &width, sizeof(width));
                Write(fh, &height, sizeof(height));
                Write(fh, &h_offs, sizeof(h_offs));
                Write(fh, &v_offs, sizeof(v_offs));
              }
          }

          // Write sprite data size
          Write(fh, &spr_data_size, 2);
          // Write sprite data
          Write(fh, spr_data, spr_data_size);

          saveHitBoxes(fh, data->sheet, type);

          SetFileSize(fh, 0, OFFSET_CURRENT);
          Close(fh);
        }
      }
      break;
    }

    data->edited = FALSE;
    DoMethod(data->obj_table.save, MUIM_Set, MUIA_Disabled, TRUE);
  }

  return 0;
}
///
///m_SaveSheetAs(cl, obj, msg)
ULONG m_SaveSheetAs(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  STRPTR path;
  struct Window* window;

  get(obj, MUIA_Window_Window, &window);

  if (data->sheet) {
    DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, TRUE);

    if (MUI_AslRequestTags(g_FileReq, ASLFR_TitleText, "Save bob sheet or sprite bank as",
                                      ASLFR_Window, window,
                                      ASLFR_PositiveText, "Save",
                                      ASLFR_DrawersOnly, FALSE,
                                      ASLFR_DoSaveMode, TRUE,
                                      ASLFR_DoPatterns, TRUE,
                                      ASLFR_InitialPattern, "#?.(sht|spr)",
                                      g_Project.data_drawer ? ASLFR_InitialDrawer : TAG_IGNORE, g_Project.data_drawer,
                                      ASLFR_InitialFile, FilePart(data->sheet->sheet_file),
                                      TAG_END) && strlen(g_FileReq->fr_File)) {
      path = makePath(g_FileReq->fr_Drawer, g_FileReq->fr_File, NULL);
      if (path) {
        freeString(data->sheet->sheet_file);
        data->sheet->sheet_file = path;

        m_SaveSheet(cl, obj, msg);

        DoMethod(data->obj_table.file, MUIM_Set, MUIA_Text_Contents, g_FileReq->fr_File);
      }
    }

    DoMethod(obj, MUIM_Set, MUIA_Window_Sleep, FALSE);
  }

  return 0;
}
///
///m_ChangeSpec(cl, obj, msg)
ULONG m_ChangeSpec(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->sheet) {
    struct ImageSpec* spec = &data->sheet->spec[msg->value];

    DoMethod(data->obj_table.width,  MUIM_Set, MUIA_Integer_Value, spec->width);
    DoMethod(data->obj_table.height, MUIM_Set, MUIA_Integer_Value, spec->height);
    DoMethod(data->obj_table.h_offs, MUIM_NoNotifySet, MUIA_Integer_Value, spec->h_offs);
    DoMethod(data->obj_table.v_offs, MUIM_NoNotifySet, MUIA_Integer_Value, spec->v_offs);

    m_FillHitBoxList(cl, obj);

    DoMethod(data->obj_table.display, MUIM_Set, MUIA_ImageDisplay_ImageSpec, spec);
  }

  return 0;
}
///
///m_UpdateSpec(cl, obj, msg)
ULONG m_UpdateSpec(struct IClass* cl, Object* obj, struct cl_UpdateMsg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->sheet) {
    ULONG img_num;
    struct HitBox* hb;

    get(data->obj_table.img_num, MUIA_Integer_Value, &img_num);
    DoMethod(data->obj_table.hitbox_lv, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &hb);

    switch (msg->update_mode) {
      case UPDATE_H_OFFS:
        data->sheet->spec[img_num].h_offs = msg->value;
      break;
      case UPDATE_V_OFFS:
        data->sheet->spec[img_num].v_offs = msg->value;
      break;
      case UPDATE_HITBOX_X1:
        if (hb) hb->x1 = msg->value;
      break;
      case UPDATE_HITBOX_Y1:
        if (hb) hb->y1 = msg->value;
      break;
      case UPDATE_HITBOX_X2:
        if (hb) hb->x2 = msg->value;
      break;
      case UPDATE_HITBOX_Y2:
        if (hb) hb->y2 = msg->value;
      break;
    }

    if (data->edited == FALSE) {
      data->edited = TRUE;
      DoMethod(data->obj_table.save, MUIM_Set, MUIA_Disabled, FALSE);
    }

    DoMethod(data->obj_table.display, MUIM_ImageDisplay_Update);
  }

  return 0;
}
///

///m_FillHitBoxList(cl, obj)
ULONG m_FillHitBoxList(struct IClass* cl, Object* obj)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->sheet) {
    ULONG img_num;
    ULONG i;
    struct MinList* hbs;
    struct HitBox* hb;
    ULONG entries = 0;

    get(data->obj_table.img_num, MUIA_Integer_Value, &img_num);
    hbs = &data->sheet->spec[img_num].hitboxes;

    DoMethod(data->obj_table.hitbox_lv, MUIM_List_Clear);

    for (hb = (struct HitBox*) hbs->mlh_Head, i = 0; hb->node.mln_Succ; hb = (struct HitBox*) hb->node.mln_Succ, i++) {
      DoMethod(data->obj_table.hitbox_lv, MUIM_List_InsertSingle, hb, MUIV_List_Insert_Bottom);
      entries++;
    }

    if (entries) {
      DoMethod(data->obj_table.hitbox_lv, MUIM_Set, MUIA_List_Active, 0);
      DoMethod(data->obj_table.hitbox_group, MUIM_Set, MUIA_Disabled, FALSE);
      DoMethod(data->obj_table.hitbox_rem, MUIM_Set, MUIA_Disabled, FALSE);
    }
    else {
      DoMethod(data->obj_table.hitbox_group, MUIM_Set, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.hitbox_rem, MUIM_Set, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.hitbox_x1, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
      DoMethod(data->obj_table.hitbox_y1, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
      DoMethod(data->obj_table.hitbox_x2, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
      DoMethod(data->obj_table.hitbox_y2, MUIM_NoNotifySet, MUIA_Integer_Value, 0);
    }

  }

  return 0;
}
///
///m_SelectHitBox(cl, obj, msg)
ULONG m_SelectHitBox(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct HitBox* hb = NULL;
  struct ImageSpec* spec;
  ULONG img_num;

  if (data->sheet) {
    get(data->obj_table.img_num, MUIA_Integer_Value, &img_num);
    spec = &data->sheet->spec[img_num];
    DoMethod(data->obj_table.hitbox_lv, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &hb);

    if (hb) {
      spec->active_hitbox = hb;

      DoMethod(data->obj_table.hitbox_x1, MUIM_NoNotifySet, MUIA_Integer_Max, hb->x2);
      DoMethod(data->obj_table.hitbox_y1, MUIM_NoNotifySet, MUIA_Integer_Max, hb->y2);
      DoMethod(data->obj_table.hitbox_x2, MUIM_NoNotifySet, MUIA_Integer_Min, hb->x1);
      DoMethod(data->obj_table.hitbox_y2, MUIM_NoNotifySet, MUIA_Integer_Min, hb->y1);
      DoMethod(data->obj_table.hitbox_x1, MUIM_NoNotifySet, MUIA_Integer_Value, hb->x1);
      DoMethod(data->obj_table.hitbox_y1, MUIM_NoNotifySet, MUIA_Integer_Value, hb->y1);
      DoMethod(data->obj_table.hitbox_x2, MUIM_NoNotifySet, MUIA_Integer_Value, hb->x2);
      DoMethod(data->obj_table.hitbox_y2, MUIM_NoNotifySet, MUIA_Integer_Value, hb->y2);

      DoMethod(data->obj_table.display, MUIM_ImageDisplay_Update);
    }
    else {
      spec->active_hitbox = NULL;
    }
  }

  return 0;
}
///
///m_AddHitBox(cl, obj, msg)
ULONG m_AddHitBox(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct ImageSpec* spec;
  struct HitBox* current_hb;
  struct HitBox* new_hb;
  ULONG num_entries;
  ULONG current_pos;
  ULONG img_num;

  DoMethod(data->obj_table.hitbox_lv, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &current_hb);
  get(data->obj_table.hitbox_lv, MUIA_List_Active, &current_pos);
  get(data->obj_table.hitbox_lv, MUIA_List_Entries, &num_entries);
  get(data->obj_table.img_num, MUIA_Integer_Value, &img_num);
  spec = &data->sheet->spec[img_num];

  new_hb = newHitBox(spec->h_offs, spec->v_offs, spec->h_offs + spec->width, spec->v_offs + spec->height, 0, 0);
  if (new_hb) {
    if (current_hb) {
      Insert((struct List*)&spec->hitboxes, (struct Node*)new_hb, (struct Node*)current_hb);
      if (current_pos == MUIV_List_Active_Off || (current_pos + 1) == num_entries) {
        DoMethod(data->obj_table.hitbox_lv, MUIM_List_InsertSingle, new_hb, MUIV_List_Insert_Bottom);
        DoMethod(data->obj_table.hitbox_lv, MUIM_Set, MUIA_List_Active, num_entries);
      }
      else {
        DoMethod(data->obj_table.hitbox_lv, MUIM_List_InsertSingle, new_hb, current_pos + 1);
        DoMethod(data->obj_table.hitbox_lv, MUIM_Set, MUIA_List_Active, current_pos + 1);
      }
    }
    else {
      AddTail((struct List*)&spec->hitboxes, (struct Node*)new_hb);
      DoMethod(data->obj_table.hitbox_lv, MUIM_List_InsertSingle, new_hb, MUIV_List_Insert_Bottom);
      DoMethod(data->obj_table.hitbox_lv, MUIM_Set, MUIA_List_Active, 0);
    }

    DoMethod(data->obj_table.hitbox_group, MUIM_Set, MUIA_Disabled, FALSE);
    DoMethod(data->obj_table.hitbox_rem, MUIM_Set, MUIA_Disabled, FALSE);

    if (data->edited == FALSE) {
      data->edited = TRUE;
      DoMethod(data->obj_table.save, MUIM_Set, MUIA_Disabled, FALSE);
    }

    DoMethod(data->obj_table.hitbox_small, MUIM_Set, MUIA_Disabled, FALSE);
    data->sheet->num_hitboxes++;
  }

  return 0;
}
///
///m_RemoveHitBox(cl, obj, msg)
ULONG m_RemoveHitBox(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct HitBox* current_hb;
  ULONG num_entries;

  DoMethod(data->obj_table.hitbox_lv, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &current_hb);

  if (current_hb) {
    Remove((struct Node*)current_hb);
    DoMethod(data->obj_table.hitbox_lv, MUIM_List_Remove, MUIV_List_Remove_Selected);
    deleteHitBox(current_hb);
    data->sheet->num_hitboxes--;
    if (data->sheet->num_hitboxes == 0) {
      DoMethod(data->obj_table.hitbox_small, MUIM_Set, MUIA_Disabled, TRUE);
    }

    get(data->obj_table.hitbox_lv, MUIA_List_Entries, &num_entries);
    if (!num_entries) {
      DoMethod(data->obj_table.hitbox_rem, MUIM_Set, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.hitbox_group, MUIM_Set, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.display, MUIM_ImageDisplay_Update);
    }

    if (data->edited == FALSE) {
      data->edited = TRUE;
      DoMethod(data->obj_table.save, MUIM_Set, MUIA_Disabled, FALSE);
    }
  }

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct {
    Object* load;
    Object* save;
    Object* saveas;
    Object* undo;
    Object* redo;
    Object* file;
    Object* scale;
    Object* small;
    Object* big;
    Object* hitbox_small;
    Object* display;
    Object* image_group;
    Object* img_num;
    Object* width;
    Object* height;
    Object* h_offs;
    Object* v_offs;
    Object* hitbox_group;
    Object* hitbox_lv;
    Object* hitbox_list;
    Object* hitbox_x1;
    Object* hitbox_y1;
    Object* hitbox_x2;
    Object* hitbox_y2;
    Object* hitbox_add;
    Object* hitbox_rem;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_Width, MUIV_Window_Width_Visible(56),
    MUIA_Window_Height, MUIV_Window_Height_Visible(52),
    MUIA_Window_ID, MAKE_ID('S','V','G','A'),
    MUIA_Window_Title, "Image Editor",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_HorizSpacing, 0,
        MUIA_Group_Child, (objects.load = MUI_NewImageButton(IMG_SPEC_LOAD, "Load sheet file...", MUIA_Enabled)),
        MUIA_Group_Child, (objects.save = MUI_NewImageButton(IMG_SPEC_SAVE, "Save sheet file", MUIA_Disabled)),
        MUIA_Group_Child, (objects.saveas = MUI_NewImageButton(IMG_SPEC_SAVEAS, "Save sheet file as...", MUIA_Disabled)),
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
          MUIA_FrameTitle, "Bobsheet/SpriteBank",
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Columns, 4,
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "File:", MUIA_HorizWeight, 0, TAG_END),
            MUIA_Group_Child, (objects.file = MUI_NewObject(MUIC_Text,
              MUIA_Frame, MUIV_Frame_Text,
              MUIA_Text_Contents, "",
              MUIA_ShortHelp, "Opened bob sheet or sprite bank file.",
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Scale:", MUIA_HorizWeight, 0, TAG_END),
            MUIA_Group_Child, (objects.scale = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_FixWidthTxt, "000",
              MUIA_String_MaxLen, 3,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 1,
              MUIA_Integer_Min, 1,
              MUIA_Integer_Max, 10,
              MUIA_Integer_Buttons, TRUE,
              MUIA_ShortHelp, "Pixel size for the display below.",
            TAG_END)),
          TAG_END),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Columns, 2,
            MUIA_Group_Child, MUI_NewCheckMark(&objects.small, TRUE, "Small sizes", 0, NULL),
            MUIA_Group_Child, MUI_NewCheckMark(&objects.hitbox_small, TRUE, "Small hitbox sizes", 0, NULL),
            MUIA_Group_Child, MUI_NewCheckMark(&objects.big, FALSE, "Big sizes", 0, NULL),
            MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
          TAG_END),
          MUIA_Group_Child, (objects.display = NewObject(MUIC_ImageDisplay->mcc_Class, NULL,
          TAG_END)),
        TAG_END),

        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_FrameTitle, "Image",
          MUIA_Group_Child, (objects.image_group = MUI_NewObject(MUIC_Group,
            MUIA_Disabled, TRUE,
            MUIA_Group_Columns, 2,
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "Image:", MUIA_HorizWeight, 0, TAG_END),
            MUIA_Group_Child, (objects.img_num = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 0,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Max, 0xFFFF,
              MUIA_Integer_Buttons, TRUE,
              MUIA_ShortHelp, "Currently displayed image.",
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "width:", MUIA_HorizWeight, 0, TAG_END),
            MUIA_Group_Child, (objects.width = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, FALSE,
              MUIA_Integer_Value, 0,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Max, 0xFFFF,
              MUIA_ShortHelp, "Width of the current image.",
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "height:", MUIA_HorizWeight, 0, TAG_END),
            MUIA_Group_Child, (objects.height = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, FALSE,
              MUIA_Integer_Value, 0,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Max, 0xFFFF,
              MUIA_ShortHelp, "height of the current image.",
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "h_offs:", MUIA_HorizWeight, 0, TAG_END),
            MUIA_Group_Child, (objects.h_offs = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 0,
              MUIA_Integer_Min, -32768,
              MUIA_Integer_Max, 32767,
              MUIA_Integer_Buttons, TRUE,
              MUIA_ShortHelp, "Horizontal offset of the image from hot spot.",
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "v_offs:", MUIA_HorizWeight, 0, TAG_END),
            MUIA_Group_Child, (objects.v_offs = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 0,
              MUIA_Integer_Min, -32768,
              MUIA_Integer_Max, 32767,
              MUIA_Integer_Buttons, TRUE,
              MUIA_ShortHelp, "Vertical offset of the image from hot spot.",
            TAG_END)),
          TAG_END)),
          MUIA_Group_Child, MUI_HorizontalTitle("Hitbox"),
          MUIA_Group_Child, (objects.hitbox_group = MUI_NewObject(MUIC_Group,
            MUIA_Disabled, TRUE,
            MUIA_Group_Child, MUI_NewObject(MUIC_Group,
              MUIA_Group_Child, (objects.hitbox_lv = MUI_NewObject(MUIC_Listview,
                MUIA_Listview_List, (objects.hitbox_list = MUI_NewObject(MUIC_List,
                  MUIA_Frame, MUIV_Frame_InputList,
                  MUIA_List_Active, 0,
                  MUIA_List_AutoVisible, TRUE,
                  MUIA_List_DisplayHook, &hitbox_disphook,
                TAG_END)),
              TAG_END)),
            TAG_END),
            MUIA_Group_Child, MUI_NewObject(MUIC_Group,
              MUIA_Group_Columns, 4,
              MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "x1:", MUIA_HorizWeight, 0, TAG_END),
              MUIA_Group_Child, (objects.hitbox_x1 = NewObject(MUIC_Integer->mcc_Class, NULL,
                MUIA_Integer_Input, TRUE,
                MUIA_Integer_Value, 0,
                MUIA_Integer_Min, -32768,
                MUIA_Integer_Max, 32767,
                MUIA_Integer_Buttons, TRUE,
                MUIA_ShortHelp, "Offset of the left edge of the hitbox from image hot spot.",
              TAG_END)),
              MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "y1:", MUIA_HorizWeight, 0, TAG_END),
              MUIA_Group_Child, (objects.hitbox_y1 = NewObject(MUIC_Integer->mcc_Class, NULL,
                MUIA_Integer_Input, TRUE,
                MUIA_Integer_Value, 0,
                MUIA_Integer_Min, -32768,
                MUIA_Integer_Max, 32767,
                MUIA_Integer_Buttons, TRUE,
                MUIA_ShortHelp, "Offset of the top edge of the hitbox from image hot spot.",
              TAG_END)),
              MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "x2:", MUIA_HorizWeight, 0, TAG_END),
              MUIA_Group_Child, (objects.hitbox_x2 = NewObject(MUIC_Integer->mcc_Class, NULL,
                MUIA_Integer_Input, TRUE,
                MUIA_Integer_Value, 0,
                MUIA_Integer_Min, -32768,
                MUIA_Integer_Max, 32767,
                MUIA_Integer_Buttons, TRUE,
                MUIA_ShortHelp, "Offset of the right edge of the hitbox from image hot spot.",
              TAG_END)),
              MUIA_Group_Child, MUI_NewObject(MUIC_Text, MUIA_Text_Contents, "y2:", MUIA_HorizWeight, 0, TAG_END),
              MUIA_Group_Child, (objects.hitbox_y2 = NewObject(MUIC_Integer->mcc_Class, NULL,
                MUIA_Integer_Input, TRUE,
                MUIA_Integer_Value, 0,
                MUIA_Integer_Min, -32768,
                MUIA_Integer_Max, 32767,
                MUIA_Integer_Buttons, TRUE,
                MUIA_ShortHelp, "Offset of the bottom edge of the hitbox from image hot spot.",
              TAG_END)),
            TAG_END),
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Horiz, TRUE,
            MUIA_Group_Child, (objects.hitbox_add = MUI_NewButton("Add", NULL, "Create new hitbox")),
            MUIA_Group_Child, (objects.hitbox_rem = MUI_NewButton("Remove", NULL, "Remove current hitbox")),
          TAG_END),
        TAG_END),
      TAG_END),
    TAG_END),
  TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.load, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_ImageEditor_Load);

    DoMethod(objects.save, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_ImageEditor_Save);

    DoMethod(objects.saveas, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_ImageEditor_SaveAs);

    DoMethod(objects.img_num, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 2,
      MUIM_ImageEditor_ChangeSpec, MUIV_TriggerValue);

    DoMethod(objects.scale, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, objects.display, 3,
      MUIM_Set, MUIA_ImageDisplay_Scale, MUIV_TriggerValue);

    DoMethod(objects.small, MUIM_Notify, MUIA_Selected, TRUE, objects.big, 3,
      MUIM_Set, MUIA_Selected, FALSE);

    DoMethod(objects.big, MUIM_Notify, MUIA_Selected, TRUE, objects.small, 3,
      MUIM_Set, MUIA_Selected, FALSE);

    DoMethod(objects.h_offs, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_ImageEditor_UpdateSpec, UPDATE_H_OFFS, MUIV_TriggerValue);

    DoMethod(objects.v_offs, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_ImageEditor_UpdateSpec, UPDATE_V_OFFS, MUIV_TriggerValue);

    DoMethod(objects.hitbox_list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 2,
      MUIM_ImageEditor_SelectHitBox, MUIV_TriggerValue);

    DoMethod(objects.hitbox_x1, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_ImageEditor_UpdateSpec, UPDATE_HITBOX_X1, MUIV_TriggerValue);

    DoMethod(objects.hitbox_y1, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_ImageEditor_UpdateSpec, UPDATE_HITBOX_Y1, MUIV_TriggerValue);

    DoMethod(objects.hitbox_x2, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_ImageEditor_UpdateSpec, UPDATE_HITBOX_X2, MUIV_TriggerValue);

    DoMethod(objects.hitbox_y2, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
      MUIM_ImageEditor_UpdateSpec, UPDATE_HITBOX_Y2, MUIV_TriggerValue);

    DoMethod(objects.hitbox_x1, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, objects.hitbox_x2, 3,
      MUIM_Set, MUIA_Integer_Min, MUIV_TriggerValue);

    DoMethod(objects.hitbox_x2, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, objects.hitbox_x1, 3,
      MUIM_Set, MUIA_Integer_Max, MUIV_TriggerValue);

    DoMethod(objects.hitbox_y1, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, objects.hitbox_y2, 3,
      MUIM_Set, MUIA_Integer_Min, MUIV_TriggerValue);

    DoMethod(objects.hitbox_y2, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, objects.hitbox_y1, 3,
      MUIM_Set, MUIA_Integer_Max, MUIV_TriggerValue);

    DoMethod(objects.hitbox_add, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_ImageEditor_AddHitBox);

    DoMethod(objects.hitbox_rem, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1,
      MUIM_ImageEditor_RemoveHitBox);

    DoMethod(objects.hitbox_add, MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(objects.hitbox_rem, MUIM_Set, MUIA_Disabled, TRUE);
    DoMethod(objects.hitbox_small, MUIM_Set, MUIA_Disabled, TRUE);

    data->obj_table.load = objects.load;
    data->obj_table.save = objects.save;
    data->obj_table.saveas = objects.saveas;
    data->obj_table.undo = objects.undo;
    data->obj_table.redo = objects.redo;
    data->obj_table.file    = objects.file;
    data->obj_table.scale   = objects.scale;
    data->obj_table.small        = objects.small;
    data->obj_table.big          = objects.big;
    data->obj_table.hitbox_small = objects.hitbox_small;
    data->obj_table.display = objects.display;
    data->obj_table.image_group = objects.image_group;
    data->obj_table.img_num = objects.img_num;
    data->obj_table.width = objects.width;
    data->obj_table.height = objects.height;
    data->obj_table.h_offs = objects.h_offs;
    data->obj_table.v_offs = objects.v_offs;
    data->obj_table.hitbox_group = objects.hitbox_group;
    data->obj_table.hitbox_lv    = objects.hitbox_lv;
    data->obj_table.hitbox_x1    = objects.hitbox_x1;
    data->obj_table.hitbox_y1    = objects.hitbox_y1;
    data->obj_table.hitbox_x2    = objects.hitbox_x2;
    data->obj_table.hitbox_y2    = objects.hitbox_y2;
    data->obj_table.hitbox_add   = objects.hitbox_add;
    data->obj_table.hitbox_rem   = objects.hitbox_rem;

    data->sheet = NULL;
    data->mode = 0;
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
  struct cl_Data *data = INST_DATA(cl, obj);

  if (data->sheet) {
    freeSheet(data->sheet);
  }

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
      /*
      case MUIA_ImageEditor_{Attribute}:
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
  //struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->opg_AttrID)
  {
    //<SUBCLASS ATTRIBUTES HERE>
    /*
    case MUIA_ImageEditor_{Attribute}:
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
    case MUIM_ImageEditor_Load:
      return m_LoadSheet(cl, obj, msg);
    case MUIM_ImageEditor_Save:
      return m_SaveSheet(cl, obj, msg);
    case MUIM_ImageEditor_SaveAs:
      return m_SaveSheetAs(cl, obj, msg);
    case MUIM_ImageEditor_ChangeSpec:
      return m_ChangeSpec(cl, obj, (struct cl_Msg*) msg);
    case MUIM_ImageEditor_UpdateSpec:
      return m_UpdateSpec(cl, obj, (struct cl_UpdateMsg*) msg);
    case MUIM_ImageEditor_SelectHitBox:
      return m_SelectHitBox(cl, obj, msg);
    case MUIM_ImageEditor_AddHitBox:
      return m_AddHitBox(cl, obj, msg);
    case MUIM_ImageEditor_RemoveHitBox:
      return m_RemoveHitBox(cl, obj, msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_ImageEditor(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
