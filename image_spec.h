#ifndef	IMAGE_SPEC_H
#define	IMAGE_SPEC_H

#include "ILBM_image.h"

//BOBSheet types
#define BST_DISTRUBUTED 0
#define BST_IRREGULAR   1

struct HitBox {
  struct MinNode node;
  WORD x1, y1;
  WORD x2, y2;
  UWORD index;
  UWORD next;
};

struct ImageSpec {
  UWORD x;      //x coord of image on the sheet
  UWORD y;      //y coord of image on the sheet
  UWORD width;
  UWORD height;
  WORD h_offs;
  WORD v_offs;
  struct HitBox* active_hitbox;
  struct MinList hitboxes;
};

struct Sheet {
  STRPTR sheet_file;  //The file path for the bobsheet or the spritebank
  STRPTR ilbm_file;   //The file name for the ilbm file of the bobsheet
  APTR sprite_bank;
  ULONG sprite_bank_size;
  UWORD type;
  UWORD num_images;
  UWORD num_hitboxes;
  struct ILBMImage* image;
  struct ImageSpec spec[0];
};

struct Sheet* loadBOBSheet(STRPTR fileName);
struct Sheet* loadSpriteBank(STRPTR fileName);
VOID freeSheet(struct Sheet* bs);

struct HitBox* newHitBox(WORD x1, WORD y1, WORD x2, WORD y2, UWORD index, UWORD next);
VOID deleteHitBox(struct HitBox* hb);

#endif /* IMAGE_SPEC_H */

/*
  for (hb = (struct HitBox*)list->mlh_Head; hb->node.mln_Succ; hb = (struct HitBox*)hb->node.mln_Succ) {

  }
  if (hb_next->node.mln_Succ)
  hb_new = newHitBox(hb->x1, hb->y1, hb->x2, hb->y2, h, UWORD next);

*/
