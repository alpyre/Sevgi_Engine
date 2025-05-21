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

#define JUMP_ACCELL_STEPS 16
#define FALL_ACCELL_STEPS 22

BYTE jump_accel[JUMP_ACCELL_STEPS] = {0, 0, -4, -4, -4, -3, -3, -3, -3, -2, -2, -2, -2, -1, -1, 0};
BYTE fall_accel[FALL_ACCELL_STEPS] = {0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8};
///
//prototypes
VOID char_anim(struct GameObject*);
VOID waterfall_anim(struct GameObject*);
//
//exports
VOID (*animFunction[NUM_ANIMS])(struct GameObject*) = {NULL,
                                                       char_anim
                                                      };
//

///char_anim(gameobject)
/******************************************************************************
 * Applies main character movement and image animation regarding state.       *
 * It provides the character to stand on platform tiles!                      *
 ******************************************************************************/
VOID char_anim(struct GameObject* go)
{
  //NOTE: We should be going by the current sprite bank of current level here
  struct SpriteBank* sb = ((struct SpriteImage*)go->image)->sprite_bank;

  switch (go->anim.state) {
    case AS_IDLE:
    {
      switch (go->anim.direction) {
        case 0:
          go->anim.direction = RIGHT;
        break;
        case RIGHT|UP:
        case RIGHT|DOWN:
          go->anim.direction = RIGHT;
        case RIGHT:
          if (go->anim.frame > AF_RIGHT_IDLE_18) {
            go->anim.frame = AF_RIGHT_IDLE_1;
            go->anim.counter_1 = 0;
            go->anim.speed = 2;
          }
        break;
        case LEFT|UP:
        case LEFT|DOWN:
          go->anim.direction = LEFT;
        case LEFT:
          if (go->anim.frame < AF_LEFT_IDLE_1 || go->anim.frame > AF_LEFT_IDLE_18) {
            go->anim.frame = AF_LEFT_IDLE_1;
            go->anim.counter_1 = 0;
            go->anim.speed = 2;
          }
        break;
      }

      setGameObjectImage(go, (struct ImageCommon*) &sb->image[go->anim.frame]);
      if (go->anim.counter_1 > go->anim.speed) {
        go->anim.frame++;
        go->anim.counter_1 = 0;
      }
    }
    break;
    case AS_RUN:
    {
      struct TileMap* map = current_level.tilemap[current_level.current.tilemap];
      LONG x = go->x;
      LONG y = go->y;
      TILEID tileID;

      switch (go->anim.direction) {
        case LEFT:
        case LEFT|UP:
        case LEFT|DOWN:
          if (go->anim.frame < AF_LEFT_RUN_1 || go->anim.frame > AF_LEFT_RUN_24) {
            go->anim.frame = AF_LEFT_RUN_1;
            go->anim.counter_1 = 0;
            go->anim.speed = 1;
          }

          // Check if the player is still standing on a platform tile
          // Assumes character hotspot is at the bottom (1px out of the image) center
          // NOTE: we check one extra pixel below to make down slope runs correct
          tileID = TILEID_AT_COORD(map, x, y);

          switch (tileID) {
            case TI_PLATFORM_LEFT_EDGE_1:
            case TI_PLATFORM_CENTER:
            case TI_PLATFORM_RIGHT_EDGE_1:
            case TI_PLATFORM_LEFT_EDGE_2:
            case TI_PLATFORM_RIGHT_EDGE_2:
            case TI_PLATFORM_LEFT_EDGE_3:
            case TI_PLATFORM_RIGHT_EDGE_3:
            case TI_PLATFORM_SINGLE:
            break;
            case TI_SLOPE_45_DOWN_SUB_1:
            case TI_SLOPE_45_DOWN_SUB_2:
              y = (y & TILESIZE_MASK) - TILESIZE + (x % TILESIZE);
            break;
            case TI_SLOPE_45_DOWN:
              y = (y & TILESIZE_MASK) + (x % TILESIZE); //45 degree slope up correction
            break;

            case TI_SLOPE_22_2_DOWN_SUB_1:
            case TI_SLOPE_22_2_DOWN_SUB_2:
              y = (y & TILESIZE_MASK) - TILESIZE + ((TILESIZE - 1) / 2 + (x % TILESIZE) / 2);
            break;
            case TI_SLOPE_22_1_DOWN_SUB_1:
            case TI_SLOPE_22_1_DOWN_SUB_2:
              y = (y & TILESIZE_MASK) - TILESIZE / 2 + (x % TILESIZE) / 2; //22.5 degree slope up correction
            break;
            case TI_SLOPE_22_1_DOWN:
              y = (y & TILESIZE_MASK) + (x % TILESIZE) / 2;
            break;
            case TI_SLOPE_22_2_DOWN:
              y = (y & TILESIZE_MASK) + TILESIZE / 2 + (x % TILESIZE) / 2;
            break;

            case TI_SLOPE_45_UP_SUPER:
              y = (y & TILESIZE_MASK) + TILESIZE + ((TILESIZE - 1) - (x % TILESIZE));
            break;
            case TI_SLOPE_45_UP:
              y = (y & TILESIZE_MASK) + ((TILESIZE - 1) - (x % TILESIZE));
            break;
            case TI_SLOPE_22_2_UP_SUPER:
              y = (y & TILESIZE_MASK) + TILESIZE + ((TILESIZE - 1) / 2 - (x % TILESIZE) / 2);
            break;
            case TI_SLOPE_22_1_UP:
              y = (y & TILESIZE_MASK) + TILESIZE / 2 + (TILESIZE - (x % TILESIZE) - 1) / 2;
            break;
            case TI_SLOPE_22_2_UP:
              y = (y & TILESIZE_MASK) + ((TILESIZE - 1) / 2 - (x % TILESIZE) / 2);
            break;
            case TI_SLOPE_END:
              y = (y & TILESIZE_MASK) + TILESIZE;
            break;

            case TI_SLOPE_45_UP_SUB_1:
            case TI_SLOPE_45_UP_SUB_2:
            case TI_SLOPE_22_1_UP_SUB_1:
            case TI_SLOPE_22_1_UP_SUB_2:
            case TI_SLOPE_22_2_UP_SUB_1:
            case TI_SLOPE_22_2_UP_SUB_2:
            default:
              go->anim.state = AS_FALL;
              go->anim.direction |= DOWN;
            break;
          }
        break;
        case RIGHT:
        case RIGHT|UP:
        case RIGHT|DOWN:
          if (go->anim.frame < AF_RIGHT_RUN_1 || go->anim.frame > AF_RIGHT_RUN_24) {
            go->anim.frame = AF_RIGHT_RUN_1;
            go->anim.counter_1 = 0;
            go->anim.speed = 1;
          }

          // Check if the player is still standing on a platform tile
          // Assumes character hotspot is at the bottom (1px out of the image) center
          tileID = TILEID_AT_COORD(map, x, y);
          switch (tileID) {
            case TI_PLATFORM_LEFT_EDGE_1:
            case TI_PLATFORM_CENTER:
            case TI_PLATFORM_RIGHT_EDGE_1:
            case TI_PLATFORM_LEFT_EDGE_2:
            case TI_PLATFORM_RIGHT_EDGE_2:
            case TI_PLATFORM_LEFT_EDGE_3:
            case TI_PLATFORM_RIGHT_EDGE_3:
            case TI_PLATFORM_SINGLE:
            break;
            case TI_SLOPE_45_UP_SUB_1:
            case TI_SLOPE_45_UP_SUB_2:
              y = (y & TILESIZE_MASK) - (x % TILESIZE) - 1;
            break;
            case TI_SLOPE_45_UP:
              y = (y & TILESIZE_MASK) + TILESIZE - ((x % TILESIZE) + 1); //45 degree slope up correction
            break;
            case TI_SLOPE_22_1_UP_SUB_1:
            case TI_SLOPE_22_1_UP_SUB_2:
              y = (y & TILESIZE_MASK) - (x % TILESIZE) / 2 - 1; //22.5 degree slope up correction
            break;
            case TI_SLOPE_22_1_UP:
              y = (y & TILESIZE_MASK) + TILESIZE - (x % TILESIZE) / 2 - 1;
            break;
            case TI_SLOPE_22_2_UP:
              y = (y & TILESIZE_MASK) + TILESIZE / 2 - (x % TILESIZE) / 2 - 1;
            break;

            case TI_SLOPE_45_DOWN_SUPER:
              y = (y & TILESIZE_MASK) + TILESIZE + (x % TILESIZE);
            break;
            case TI_SLOPE_45_DOWN:
              y = (y & TILESIZE_MASK) + (x % TILESIZE);
            break;
            case TI_SLOPE_22_1_DOWN_SUPER:
              y = (y & TILESIZE_MASK) + TILESIZE + (x % TILESIZE) / 2;
            break;
            case TI_SLOPE_22_1_DOWN:
              y = (y & TILESIZE_MASK) + (x % TILESIZE) / 2;
            break;
            case TI_SLOPE_22_2_DOWN:
              y = (y & TILESIZE_MASK) + TILESIZE / 2 + (x % TILESIZE) / 2;
            break;
            case TI_SLOPE_END:
              y = (y & TILESIZE_MASK) + TILESIZE;
            break;

            case TI_SLOPE_22_2_UP_SUB_1:
            case TI_SLOPE_22_2_UP_SUB_2:
            case TI_SLOPE_45_DOWN_SUB_1:
            case TI_SLOPE_45_DOWN_SUB_2:
            case TI_SLOPE_22_1_DOWN_SUB_1:
            case TI_SLOPE_22_1_DOWN_SUB_2:
            case TI_SLOPE_22_2_DOWN_SUB_1:
            case TI_SLOPE_22_2_DOWN_SUB_2:
            default:
              go->anim.state = AS_FALL;
              go->anim.direction |= DOWN;
            break;
          }
        break;
      }

      setGameObjectPos(go, x, y);
      setGameObjectImage(go, (struct ImageCommon*) &sb->image[go->anim.frame]);
      if (go->anim.counter_1 > go->anim.speed) {
        go->anim.frame++;
        go->anim.counter_1 = 0;

        if (go->anim.frame == AF_RIGHT_RUN_7 || go->anim.frame == AF_LEFT_RUN_7) {
          PT_PlaySFX(current_level.sound_sample[0]);
        }
        if (go->anim.frame == AF_RIGHT_RUN_19 || go->anim.frame == AF_LEFT_RUN_19) {
          PT_PlaySFX(current_level.sound_sample[1]);
        }
      }
    }
    break;
    case AS_JUMP:
    {
      LONG x = go->x;
      LONG y = go->y;

      switch (go->anim.direction) {
        case RIGHT:
        case RIGHT|UP:
        if (go->anim.frame < AF_RIGHT_JUMP_1 || go->anim.frame > AF_RIGHT_JUMP_8) {
          go->anim.frame = AF_RIGHT_JUMP_1;
          go->anim.counter_1 = 0;
          go->anim.speed = 1;
          PT_PlaySFX(current_level.sound_sample[2]);
        }
        break;
        case LEFT:
        case LEFT|UP:
        if (go->anim.frame < AF_LEFT_JUMP_1 || go->anim.frame > AF_LEFT_JUMP_8) {
          go->anim.frame = AF_LEFT_JUMP_1;
          go->anim.counter_1 = 0;
          go->anim.speed = 1;
          PT_PlaySFX(current_level.sound_sample[2]);
        }
        break;
      }

      y += jump_accel[go->anim.counter_2];
      setGameObjectPos(go, x, y);
      setGameObjectImage(go, (struct ImageCommon*) &sb->image[go->anim.frame]);
      go->anim.counter_2++;
      if (go->anim.counter_2 > JUMP_ACCELL_STEPS) {
        go->anim.counter_2 = 0;
        go->anim.state = AS_FALL;
        go->anim.direction |= DOWN;
        go->anim.frame = AF_LEFT_RUN_1;
      }

      if (go->anim.counter_1 > go->anim.speed) {
        go->anim.frame++;
        go->anim.counter_1 = 0;
      }
    }
    break;
    case AS_FALL:
    {
      struct TileMap* map = current_level.tilemap[current_level.current.tilemap];
      LONG x = go->x;
      LONG y = go->y;
      TILEID tileID;
      LONG platform_height = 0;

      switch (go->anim.direction) {
        case RIGHT:
        case RIGHT|DOWN:
        if (go->anim.frame < AF_RIGHT_FALL_1 || go->anim.frame > AF_RIGHT_FALL_6) {
          go->anim.frame = AF_RIGHT_FALL_1;
          go->anim.counter_1 = 0;
          go->anim.speed = 1;
          go->anim.storage_1 = y; // store the initital fall height for platform check
        }
        break;
        case LEFT:
        case LEFT|DOWN:
        if (go->anim.frame < AF_LEFT_FALL_1 || go->anim.frame > AF_LEFT_FALL_6) {
          go->anim.frame = AF_LEFT_FALL_1;
          go->anim.counter_1 = 0;
          go->anim.speed = 1;
          go->anim.storage_1 = y; // store the initital fall height for platform check
        }
        break;
      }

      y += fall_accel[go->anim.counter_2];
      tileID = TILEID_AT_COORD(map, x, y);
      switch (tileID) {
        case TI_PLATFORM_LEFT_EDGE_1:
        case TI_PLATFORM_CENTER:
        case TI_PLATFORM_RIGHT_EDGE_1:
        case TI_PLATFORM_LEFT_EDGE_2:
        case TI_PLATFORM_RIGHT_EDGE_2:
        case TI_PLATFORM_LEFT_EDGE_3:
        case TI_PLATFORM_RIGHT_EDGE_3:
        case TI_PLATFORM_SINGLE:
          platform_height = (y & TILESIZE_MASK);
        break;
        case TI_SLOPE_45_UP:
          platform_height = (y & TILESIZE_MASK) + ((TILESIZE - 1) - (x % TILESIZE));
        break;
        case TI_SLOPE_45_UP_SUB_1:
        case TI_SLOPE_45_UP_SUB_2:
          platform_height = (y & TILESIZE_MASK) - (x % TILESIZE) - 1;
        break;
        case TI_SLOPE_22_1_UP:
          platform_height = (y & TILESIZE_MASK) + ((TILESIZE - 1) - (x % TILESIZE) / 2);
        break;
        case TI_SLOPE_22_2_UP:
          platform_height = (y & TILESIZE_MASK) + ((TILESIZE - 1) / 2 - (x % TILESIZE) / 2);
        break;
        case TI_SLOPE_22_1_UP_SUB_1:
        case TI_SLOPE_22_1_UP_SUB_2:
          platform_height = (y & TILESIZE_MASK) - (x % TILESIZE) / 2 - 1;
        break;
        case TI_SLOPE_22_2_UP_SUB_1:
        case TI_SLOPE_22_2_UP_SUB_2:
          platform_height = (y & TILESIZE_MASK) - TILESIZE + ((TILESIZE - 1) / 2 - (x % TILESIZE) / 2);
        break;
        case TI_SLOPE_45_DOWN:
          platform_height = (y & TILESIZE_MASK) + (x % TILESIZE);
        break;
        case TI_SLOPE_45_DOWN_SUB_1:
        case TI_SLOPE_45_DOWN_SUB_2:
          platform_height = (y & TILESIZE_MASK) - TILESIZE + (x % TILESIZE);
        break;
        case TI_SLOPE_22_1_DOWN:
          platform_height = (y & TILESIZE_MASK) + (x % TILESIZE) / 2;
        break;
        case TI_SLOPE_22_1_DOWN_SUB_1:
        case TI_SLOPE_22_1_DOWN_SUB_2:
          platform_height = (y & TILESIZE_MASK) - TILESIZE + (x % TILESIZE) / 2;
        break;
        case TI_SLOPE_22_2_DOWN:
          platform_height = (y & TILESIZE_MASK) + TILESIZE / 2 + (x % TILESIZE) / 2;
        break;
        case TI_SLOPE_22_2_DOWN_SUB_1:
        case TI_SLOPE_22_2_DOWN_SUB_2:
          platform_height = (y & TILESIZE_MASK) - TILESIZE + TILESIZE / 2 + (x % TILESIZE) / 2;
        break;
      }

      if (go->anim.storage_1 < platform_height && y >= platform_height) {
        y = platform_height;
        go->anim.state = AS_LAND;
        go->anim.counter_2 = 0;
      }

      setGameObjectImage(go, (struct ImageCommon*) &sb->image[go->anim.frame]);
      setGameObjectPos(go, x, y);
      if (go->anim.counter_2 < FALL_ACCELL_STEPS - 1) {
        go->anim.counter_2++;
      }

      if (go->anim.counter_1 > go->anim.speed) {
        go->anim.frame++;
        if ((go->anim.direction & RIGHT) == RIGHT) {
          if (go->anim.frame > AF_RIGHT_FALL_6) {
            go->anim.frame = AF_RIGHT_FALL_4;
          }
        }
        else {
          if (go->anim.frame > AF_LEFT_FALL_6) {
            go->anim.frame = AF_LEFT_FALL_4;
          }
        }

        go->anim.counter_1 = 0;
      }
    }
    break;
    case AS_LAND:
    {
      switch (go->anim.direction) {
        case LEFT:
        case LEFT|DOWN:
        {
          if (go->anim.frame < AF_LEFT_LAND_1 || go->anim.frame > AF_LEFT_LAND_3) {
            go->anim.frame = AF_LEFT_LAND_1;
            PT_PlaySFX(current_level.sound_sample[1]);
          }

          setGameObjectImage(go, (struct ImageCommon*) &sb->image[go->anim.frame]);
          go->anim.frame++;
          if (go->anim.frame > AF_LEFT_LAND_3) {
            go->anim.state = AS_IDLE;
            go->anim.direction = LEFT;
          }
        }
        break;
        case RIGHT:
        case RIGHT|DOWN:
        {
          if (go->anim.frame < AF_RIGHT_LAND_1 || go->anim.frame > AF_RIGHT_LAND_3) {
            go->anim.frame = AF_RIGHT_LAND_1;
            PT_PlaySFX(current_level.sound_sample[1]);
          }

          setGameObjectImage(go, (struct ImageCommon*) &sb->image[go->anim.frame]);
          go->anim.frame++;
          if (go->anim.frame > AF_RIGHT_LAND_3) {
            go->anim.state = AS_IDLE;
            go->anim.direction = RIGHT;
          }
        }
        break;
      }
    }
    break;
  }

  go->anim.counter_1++;
}
///
