///includes
#include <exec/exec.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "anims.h"
///
///defines
///
///globals
extern volatile struct Custom custom;
extern struct Level current_level;
extern volatile ULONG g_frame_counter;
///
//prototypes
VOID testAnim1(struct GameObject*);
VOID testAnim2(struct GameObject*);
//
//exports
VOID (*animFunction[NUM_ANIMS])(struct GameObject*) = {NULL,
                                                       testAnim1,
                                                       testAnim2};
//

///testAnim1(gameobject) //TEMP:
VOID testAnim1(struct GameObject* go)
{
  if (go->anim.frame > 64) {
    go->anim.frame = 0;
    go->anim.state++;
    if (go->anim.state > 3) {
      go->anim.state = 0;
    }
  }

  if (!(go->anim.frame % 3)) {
    go->anim.storage_1++;
    if (go->anim.storage_1 > 7) go->anim.storage_1 = 0;
    go->image = (struct ImageCommon*)&current_level.bob_sheet[0]->image[go->anim.state * 16 + go->anim.storage_1];
  }

  switch (go->anim.state) {
    case 0:
      go->anim.frame++;
      go->y--;
    break;
    case 1:
      go->anim.frame++;
      go->x++;
    break;
    case 2:
      go->anim.frame++;
      go->y++;
    break;
    case 3:
      go->anim.frame++;
      go->x--;
    break;
  }

  go->x1 = go->x + ((struct BOBImage*)go->image)->h_offs;
  go->x2 = go->x1 + ((struct BOBImage*)go->image)->width;
  go->y1 = go->y + ((struct BOBImage*)go->image)->v_offs;
  go->y2 = go->y1 + ((struct BOBImage*)go->image)->height;
}
///
///testAnim2(gameobject)
VOID testAnim2(struct GameObject* go)
{
  #define IMAGE_NUM 1

  static UWORD eighth_frame = 7;
  static UWORD image_num = IMAGE_NUM;

  if (go->anim.frame > 16) {
    go->anim.frame = 0;
    go->anim.state++;
    if (go->anim.state > 3) {
      go->anim.state = 0;
    }
  }

  switch (go->anim.state) {
    case 0:
      go->anim.frame++;
      go->y1--;
      go->y2--;
    break;
    case 1:
      go->anim.frame++;
      go->x1--;
      go->x2--;
    break;
    case 2:
      go->anim.frame++;
      go->y1++;
      go->y2++;
    break;
    case 3:
      go->anim.frame++;
      go->x1++;
      go->x2++;
    break;
  }

  if (eighth_frame >= 7) {
    struct SpriteBank* sb = ((struct SpriteImage*)go->image)->sprite_bank;

    image_num++;
    if (image_num >= 47) {
      image_num = 0;
    }

    go->image = (struct ImageCommon*) &sb->image[image_num];
    go->x2 = go->x1 + go->image->width;
    go->y2 = go->y1 + go->image->height;

    eighth_frame = 0;
  }
  else {
    eighth_frame++;
  }
}
///
