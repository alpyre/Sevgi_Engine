///includes
#include <exec/exec.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "anims.h"
///
///defines
//Anim Frames
#define AF_LEFT_IDLE_1   6
#define AF_LEFT_IDLE_2   7
#define AF_LEFT_IDLE_3   8

#define AF_RIGHT_IDLE_1 15
#define AF_RIGHT_IDLE_2 16
#define AF_RIGHT_IDLE_3 17

#define AF_DOWN_IDLE_1  24
#define AF_DOWN_IDLE_2  25
#define AF_DOWN_IDLE_3  26

#define AF_UP_IDLE_1    33
#define AF_UP_IDLE_2    34
#define AF_UP_IDLE_3    35

#define AF_LEFT_WALK_1   0
#define AF_LEFT_WALK_2   1
#define AF_LEFT_WALK_3   2
#define AF_LEFT_WALK_4   3
#define AF_LEFT_WALK_5   4
#define AF_LEFT_WALK_6   5

#define AF_RIGHT_WALK_1  9
#define AF_RIGHT_WALK_2 10
#define AF_RIGHT_WALK_3 11
#define AF_RIGHT_WALK_4 12
#define AF_RIGHT_WALK_5 13
#define AF_RIGHT_WALK_6 14

#define AF_DOWN_WALK_1  18
#define AF_DOWN_WALK_2  19
#define AF_DOWN_WALK_3  20
#define AF_DOWN_WALK_4  21
#define AF_DOWN_WALK_5  22
#define AF_DOWN_WALK_6  23

#define AF_UP_WALK_1    27
#define AF_UP_WALK_2    28
#define AF_UP_WALK_3    29
#define AF_UP_WALK_4    30
#define AF_UP_WALK_5    31
#define AF_UP_WALK_6    32

//Anim Values
#define AV_BLINK_DELAY 250 //frames = every 5 seconds
///
///globals
extern struct Level current_level;
extern volatile ULONG g_frame_counter;
///
//prototypes
VOID char_anim(struct GameObject*);
VOID waterfall_anim(struct GameObject*);
//
//exports
VOID (*animFunction[NUM_ANIMS])(struct GameObject*) = {NULL,
                                                       char_anim,
                                                       waterfall_anim};
//

///char_anim(gameobject)
/******************************************************************************
 * Applies main character movement and image animation regarding state.       *
 * It prevents the character hotspot to traverse block tiles!                 *
 ******************************************************************************/
VOID char_anim(struct GameObject* go)
{
  //NOTE: We should be going by the current sprite bank of current level here
  struct SpriteBank* sb = ((struct SpriteImage*)go->image)->sprite_bank;
  static LONG blink_delay = 0;
  static LONG skip_frames = 0;

  switch (go->anim.state) {
    case AS_IDLE:
    {
      switch (go->anim.direction) {
        case 0:
          go->anim.direction = DOWN;
        case DOWN:
        case DOWN|LEFT:
        case DOWN|RIGHT:
          if (go->anim.frame < AF_DOWN_IDLE_1 || go->anim.frame > AF_DOWN_IDLE_3)
            go->anim.frame = AF_DOWN_IDLE_1;
        break;
        case UP:
        case UP|LEFT:
        case UP|RIGHT:
            go->anim.frame = AF_UP_IDLE_1;
        break;
        case LEFT:
          if (go->anim.frame < AF_LEFT_IDLE_1 || go->anim.frame > AF_LEFT_IDLE_3)
            go->anim.frame = AF_LEFT_IDLE_1;
        break;
        case RIGHT:
          if (go->anim.frame < AF_RIGHT_IDLE_1 || go->anim.frame > AF_RIGHT_IDLE_3)
            go->anim.frame = AF_RIGHT_IDLE_1;
        break;
      }

      setGameObjectImage(go, (struct ImageCommon*) &sb->image[go->anim.frame]);
      if (blink_delay > AV_BLINK_DELAY) {
        if (skip_frames > 3) {
          go->anim.frame++;
          skip_frames = 0;
        }
        if (blink_delay > AV_BLINK_DELAY + 8) blink_delay = 0;
      }
      blink_delay++;
    }
    break;
    case AS_WALK:
    {
      struct TileMap* map = current_level.tilemap[current_level.current.tilemap];
      LONG x = go->x;
      LONG y = go->y;
      TILEID tileID;

      switch (go->anim.direction) {
        case DOWN:
          if (go->anim.frame < AF_DOWN_WALK_1 || go->anim.frame > AF_DOWN_WALK_6)
            go->anim.frame = AF_DOWN_WALK_1;

          y++; //NOTE: below is implemented to implement a walk speed here!
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            y = (y & TILESIZE_MASK) - 1;
          }
        break;
        case DOWN|LEFT:
          if (go->anim.frame < AF_DOWN_WALK_1 || go->anim.frame > AF_DOWN_WALK_6)
            go->anim.frame = AF_DOWN_WALK_1;

          x--;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            x = (x & TILESIZE_MASK) + TILESIZE;
          }

          y++;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            y = (y & TILESIZE_MASK) - 1;
          }
        break;
        case DOWN|RIGHT:
          if (go->anim.frame < AF_DOWN_WALK_1 || go->anim.frame > AF_DOWN_WALK_6)
            go->anim.frame = AF_DOWN_WALK_1;

          x++;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            x = (x & TILESIZE_MASK) - 1;
          }

          y++;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            y = (y & TILESIZE_MASK) - 1;
          }
        break;
        case UP:
          if (go->anim.frame < AF_UP_WALK_1 || go->anim.frame > AF_UP_WALK_6)
            go->anim.frame = AF_UP_WALK_1;

          y--;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            y = (y & TILESIZE_MASK) + TILESIZE;
          }
        break;
        case UP|LEFT:
          if (go->anim.frame < AF_UP_WALK_1 || go->anim.frame > AF_UP_WALK_6)
            go->anim.frame = AF_UP_WALK_1;

          x--;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            x = (x & TILESIZE_MASK) + TILESIZE;
          }

          y--;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            y = (y & TILESIZE_MASK) + TILESIZE;
          }
        break;
        case UP|RIGHT:
          if (go->anim.frame < AF_UP_WALK_1 || go->anim.frame > AF_UP_WALK_6)
            go->anim.frame = AF_UP_WALK_1;

          x++;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            x = (x & TILESIZE_MASK) - 1;
          }

          y--;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            y = (y & TILESIZE_MASK) + TILESIZE;
          }
        break;
        case LEFT:
          if (go->anim.frame < AF_LEFT_WALK_1 || go->anim.frame > AF_LEFT_WALK_6)
            go->anim.frame = AF_LEFT_WALK_1;

          x--;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            x = (x & TILESIZE_MASK) + TILESIZE;
          }
        break;
        case RIGHT:
          if (go->anim.frame < AF_RIGHT_WALK_1 || go->anim.frame > AF_RIGHT_WALK_6)
            go->anim.frame = AF_RIGHT_WALK_1;

          x++;
          tileID = TILEID_AT_COORD(map, x, y);
          if (is_block_tile(tileID)) {
            x = (x & TILESIZE_MASK) - 1;
          }
        break;
        default:
          go->anim.state = AS_IDLE; return;
      }

      setGameObjectImage(go, (struct ImageCommon*) &sb->image[go->anim.frame]);
      setGameObjectPos(go, x, y);
      if (skip_frames > 3) {
        go->anim.frame++;
        skip_frames = 0;
      }
    }
    break;
  }

  skip_frames++;
}
///
///waterfall_anim(gameobject)
/******************************************************************************
 * Plays the waterfall bob animation.                                         *
 ******************************************************************************/
VOID waterfall_anim(struct GameObject* go)
{
  //NOTE: We should be going by the current sprite bank of current level here
  struct BOBSheet* bs = current_level.bob_sheet[0];
  static LONG skip_frames = 1;

  if (skip_frames > 4) {
    go->anim.frame++;
    if (go->anim.frame > 2) go->anim.frame = 0;

    go->image = (struct ImageCommon*)&bs->image[go->anim.frame];

    skip_frames = 0;
  }

  skip_frames++;
}
///
