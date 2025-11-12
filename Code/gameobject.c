///includes
#include <exec/exec.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <dos/dos.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include "SDI_headers/SDI_compiler.h"

#include "system.h"
#include "level.h"
#include "anims.h"
#include "collisions.h"

#include "gameobject.h"
///
///defines
//BOB flags
#define BOB_ASSIGNED 0x01
#define BOB_RESIGNED 0x02
#define BOB_DRAWN    0x04
#define BOB_CLEARED  0x08
#define BOB_DYING    0x10
#define BOB_DEAD     0x20

//Sprite flags
#define SPR_ASSIGNED 0x01
#define HSN_NOT_SET  0x09
#define HSN_CANT_SET 0x08
///
///globals
//imported globals
extern struct Level current_level;
extern VOID (*animFunction[NUM_ANIMS])(struct GameObject*);
extern VOID (*gobjCollisionFunction[NUM_GOBJ_COLL_FUNCS])(struct GameObject*, struct GameObject*);
extern VOID (*tileCollisionFunction[NUM_TILE_COLL_FUNCS])(struct GameObject*);
#ifdef DOUBLE_BUFFER
  extern ULONG g_active_buffer; //from display_level.c
#else
  #define g_active_buffer 0
#endif

//exported globals
struct GameObject** gameobjectList; // will be set by initGameObjects()
struct BOB bobs[NUM_BOBS];
struct Sprite sprites[NUM_SPRITES];
struct GameObject* bobList[NUM_BOBS + 1];
struct GameObject* spriteList[NUM_SPRITES + 1];
struct BitMap* BOBsBackBuffer = NULL;

//local globals
//NOTE: These 4 mapPos values are actually unsigned (meaning always positive).
//      But they are accessed by pointer and will be used in comparisons with
//      signed gameobject coordinates.
//      The values these pointers point to are going to be stored in level's map.
//      So, every display that will utilize algorithms from gameobject.o has to
//      have (at least a dummy) tilemap. level.o handles this.
//      If the game changes tilemap during play, the these 5 variables below have
//      to be updated here as well
struct TileMap* map;     // current tilemap on display
LONG* mapPosX  = NULL;   // map's horizontal position in pixels
LONG* mapPosY  = NULL;   // map's vertical position in pixels
LONG* mapPosX2 = NULL;   // defines the square that displayed on
LONG* mapPosY2 = NULL;   // ...the screen.

STATIC UBYTE bob_index = 0;
STATIC UBYTE sprite_index = 0;
STATIC struct BOB* avail_bobs[NUM_BOBS + 1];
STATIC struct BOB** avail_bob;
STATIC struct Sprite* avail_sprites[NUM_SPRITES + 1];
STATIC struct Sprite** avail_sprite;
STATIC ULONG max_gameobject_height = 0;

//The total number of gameobjects at init time
ULONG num_gameobjects = 0;
#if NUM_GAMEOBJECTS > 0
//Spawnable / de-spawnable gameobjects
STATIC struct GameObject gameobjects[NUM_GAMEOBJECTS];
STATIC struct GameObject* avail_gameobjects[NUM_GAMEOBJECTS + 1];
STATIC struct GameObject** avail_gameobject;
#endif
///
///prototypes
  // Displays which want to use gameobject routines has to provide these two...
  // ...functions and pass them here with initGameObjects() function.
VOID (*blitBOB)(struct GameObject*);   //the function to blit an image to the screen
VOID (*unBlitBOB)(struct GameObject*); //the function to restore the background under a blitted image
STATIC VOID checkGameObjectCollisions(VOID);
///

///initGameObjects(blitBOBFunc, unBlitBOBFunc, max_bob_width, max_go_height)
/******************************************************************************
 * Initializes the mediums used in gameobject.o to appropriate initial values *
 * determined by loadLevel().                                                 *
 ******************************************************************************/
VOID initGameObjects(VOID* blitBOBFunc, VOID* unBlitBOBFunc, ULONG max_bob_width, ULONG max_go_height)
{
  map = current_level.tilemap[current_level.current.tilemap];
  mapPosX = &map->mapPosX;
  mapPosY = &map->mapPosY;
  mapPosX2 = &map->mapPosX2;
  mapPosY2 = &map->mapPosY2;

  blitBOB = (VOID (*)(struct GameObject*))blitBOBFunc;
  unBlitBOB = (VOID (*)(struct GameObject*))unBlitBOBFunc;

  bob_index = 0;
  sprite_index = 0;

  bobList[0] = NULL;
  spriteList[0] = NULL;

  if (current_level.gameobject_bank) {
    gameobjectList = current_level.gameobject_bank[current_level.current.gameobject_bank]->gameobjectList;
    max_gameobject_height = max_go_height;
    num_gameobjects = current_level.gameobject_bank[current_level.current.gameobject_bank]->num_gameobjects;

    #if NUM_GAMEOBJECTS > 0
    {
      ULONG i;
      for (i = 0; i < NUM_GAMEOBJECTS; i++) {
        avail_gameobjects[i] = &gameobjects[i];
      }
      avail_gameobjects[NUM_GAMEOBJECTS] = NULL;
      avail_gameobject = &avail_gameobjects[0];
    }
    #endif

    if (NUM_BOBS) {
      LONG i;
      ULONG bob_width = ((max_bob_width + 15) / 16) * 16;

      memset(bobs, 0, sizeof(struct BOB) * NUM_BOBS);

      //Initialize bobs
      for (i = 0; i < NUM_BOBS; i++) {
        bobs[i].flags = BOB_CLEARED;
        bobs[i].background = (UBYTE*)(BOBsBackBuffer->Planes[0] + i * (bob_width / 8)); //WARNING: This depends on BOBsBackBuffer being interleaved!
      }
      //Initialize available bob list and pointer
      for (i = 0; i < NUM_BOBS; i++) {
        avail_bobs[i] = &bobs[i];
      }
      avail_bobs[NUM_BOBS] = NULL;
      avail_bob = &avail_bobs[0];
    }
    if (NUM_SPRITES) {
      LONG i;

      memset(sprites, 0, sizeof(struct Sprite) * NUM_SPRITES);

      //Initialize available sprite list and pointer
      for (i = 0; i < NUM_SPRITES; i++) {
        avail_sprites[i] = &sprites[i];
      }
      avail_sprites[NUM_SPRITES] = NULL;
      avail_sprite = &avail_sprites[0];
    }
  }
}
///

///allocBOBBackgroundBuffer()
/******************************************************************************
 * Allocates the chipmemory required to store bob backgrounds that will be    *
 * used by unBlitBOB().                                                       *
 ******************************************************************************/
struct BitMap* allocBOBBackgroundBuffer(UWORD max_bob_width, UWORD max_bob_height, UWORD max_bob_depth)
{
  struct BitMap* bm = AllocBitMap((((max_bob_width + 15) / 16) * 16) * NUM_BOBS + (NUM_BOBS == 1 ? 16 : 0),
                                  max_bob_height, max_bob_depth,
                                  BMF_DISPLAYABLE | BMF_INTERLEAVED | BMF_CLEAR, NULL);
  if (bm) {
    if ((TypeOfMem(bm->Planes[0]) & MEMF_CHIP) && (GetBitMapAttr(bm, BMA_FLAGS) & BMF_INTERLEAVED)) {
      return bm;
    }
    else {
      FreeBitMap(bm);
      bm = NULL;
    }
  }

  return bm;
}
///

///SpriteListFunctions
#ifdef SMART_SPRITES
INLINE STATIC VOID addToSpriteBehindList(struct GameObject* go1, struct GameObject* go2)
{
  if (go1->priority > go2->priority) {
    ((struct Sprite*)go2->u.medium)->behindList[((struct Sprite*)go2->u.medium)->behindList_index++] = go1;
  }
  else {
    ((struct Sprite*)go1->u.medium)->behindList[((struct Sprite*)go1->u.medium)->behindList_index++] = go2;
  }
}

INLINE STATIC VOID addToSpriteRasterList(struct GameObject* go1, struct GameObject* go2)
{
  ((struct Sprite*)go1->u.medium)->rasterList[((struct Sprite*)go1->u.medium)->rasterList_index++] = go2;
  ((struct Sprite*)go2->u.medium)->rasterList[((struct Sprite*)go2->u.medium)->rasterList_index++] = go1;
}

INLINE STATIC VOID terminateSpriteLists(struct Sprite* spr)
{
  spr->rasterList[spr->rasterList_index] = NULL;
  spr->behindList[spr->behindList_index] = NULL;
  spr->rasterList_index = 0;
  spr->behindList_index = 0;
}
#endif
///
///BOBListFunctions
//NOTE: this could be totally inlined! Or macroed!
//OPTIMIZE: can we fill the draw/clearLists by pointer (and not by index)?
INLINE STATIC VOID addToBOBDrawList(struct GameObject* go1, struct GameObject* go2)
{
  if (go1->priority > go2->priority) {
    struct BOB* bob1 = (struct BOB*)go1->u.mediums[g_active_buffer];
    bob1->drawList[bob1->drawList_index++] = go2;
  }
  else {
    struct BOB* bob2 = (struct BOB*)go2->u.mediums[g_active_buffer];
    bob2->drawList[bob2->drawList_index++] = go1;
  }
}

INLINE STATIC VOID terminateBOBDrawList(struct BOB* bob)
{
  bob->drawList[bob->drawList_index] = NULL;
  bob->drawList_index = 0;
}

INLINE STATIC VOID addToBOBClearList(struct GameObject* go1, struct GameObject* go2)
{
  struct BOB* bob1 = (struct BOB*)go1->u.mediums[g_active_buffer];
  bob1->clearList[bob1->clearList_index++] = go2;
}

INLINE STATIC VOID terminateBOBClearList(struct BOB* bob)
{
  bob->clearList[bob->clearList_index] = NULL;
  bob->clearList_index = 0;
}

#ifndef DOUBLE_BUFFER
/******************************************************************************
 * For a BOB which has moved to a new location on screen to be able to draw   *
 * itself, there must not be any bobs that has not cleared themselves. This   *
 * function fills up a list of such bobs to be cleared.                       *
 * NOTE: This function is called at a state where bobList holds the bobs      *
 * drawn the previous frame.                                                  *
 * OPTIMIZE: This function introduces a performance hit! It is a traversion   *
 * in traversion. Imagine NUM_BOBS is set to 10 and all 10 are visible on a   *
 * frame and the next frame all 10 has moved. This means the collision        *
 * comparisons below will have to be made 10*9 times.                         *
 ******************************************************************************/
INLINE STATIC VOID fillBOBCl2DrList(struct GameObject* go1)
{
  struct GameObject** go_i = bobList;
  struct GameObject* go2;
  struct BOB* go1_bob = (struct BOB*)go1->u.medium;
  ULONG index = 0; //index for cl2drList

  while ((go2 = *go_i++)) {
    if (go2 != go1) {
      struct BOB* go2_bob = (struct BOB*)go2->u.medium;

      if (go1->y2 < go2_bob->lastBlt.y2 - max_gameobject_height) break;
      if (go1->y2 > go2_bob->lastBlt.y1 && go2_bob->lastBlt.y2 >= go1->y1 &&
          go1->x2 > go2_bob->lastBlt.x1 && go2_bob->lastBlt.x2 >= go1->x1) {
        //Add this bob to "clear to draw" list of the first gameobject
        go1_bob->cl2drList[index++] = go2;
      }
    }
  }
  go1_bob->cl2drList[index] = NULL;
}
#endif
///
///assignSprite(gameobject)
/******************************************************************************
 * Assigns a sprite medium to the game object. This is an optimized algorithm.*
 * avail_sprite always points to a pointer to the next available sprite medium.*                                                        *
 ******************************************************************************/
INLINE STATIC VOID assignSprite(struct GameObject* go)
{
  if (*avail_sprite) {
    struct Sprite* spr = *avail_sprite;
    spr->flags |= SPR_ASSIGNED;
    spr->hsn = ((struct SpriteImage*)go->image)->hsn;
    go->u.medium = (APTR)spr;
    avail_sprite++;
  }
}
///
///resignSprite(gameobject)
INLINE STATIC VOID resignSprite(struct GameObject* go)
{
  struct Sprite* spr = (struct Sprite*)go->u.medium;
  avail_sprite--;
  *avail_sprite = spr;
  spr->flags &= ~SPR_ASSIGNED;
  go->u.medium = NULL;
}
///
///assignBOB(gameobject)
/******************************************************************************
 * Assigns a BOB medium to the game object. This is an optimized algorithm.   *
 * avail_bob always points to a pointer to the next available BOB medium.     *
 ******************************************************************************/
INLINE STATIC VOID assignBOB(struct GameObject* go)
{
  if (*avail_bob) {
    struct BOB* bob = *avail_bob;
    bob->flags |= BOB_ASSIGNED;
    go->u.mediums[g_active_buffer] = (APTR)bob;
    avail_bob++;
  }
}
///
///resignBOB(gameobject)
INLINE STATIC VOID resignBOB(struct GameObject* go)
{
  struct BOB* bob = (struct BOB*)go->u.mediums[g_active_buffer];
  avail_bob--;
  *avail_bob = bob;
  bob->flags |= BOB_RESIGNED;
}
///
///checkGameObjectCollisions()
/******************************************************************************
 * Checks all GameObjects for collisions. It also checks out of display       *
 * collisions. For linear time collision check the algorithm depends on the   *
 * gameobjectList being pre-sorted.                                           *
 * Colliding game objects' mediums get eachothers pointers on their           *
 * respective lists according to their display priorities.                    *
 * OPTIMIZE: Refactor for SPRITE_OBJECT / BOB_OBJECT cross checks!            *
 ******************************************************************************/
STATIC VOID checkGameObjectCollisions()
{
  struct GameObject** go_i = gameobjectList;
  struct GameObject** go_l;
  struct GameObject* go1;
  struct GameObject* go2;

  while ((go1 = *go_i++)) {
    go_l = go_i;

    while ((go2 = *go_l++) && go1->y2 > go2->y2 - max_gameobject_height) {
      if (go1->y2 > go2->y1) {
        if (go1->x2 > go2->x1 && go2->x2 >= go1->x1) { //Objects do collide per image rectangle
          if (go1->type == BOB_OBJECT && go1->u.mediums[g_active_buffer] && go2->type == BOB_OBJECT && go2->u.mediums[g_active_buffer]) {
            addToBOBDrawList(go1, go2);
          }
          #ifdef SMART_SPRITES
          if (go1->type == SPRITE_OBJECT && go2->type == SPRITE_OBJECT) {
            addToSpriteBehindList(go1, go2);
          }
          #endif

          if (go1->collide) go1->collide(go1, go2);
          if (go2->collide) go2->collide(go2, go1);
        }
        #ifdef SMART_SPRITES
        else if (go1->type == SPRITE_OBJECT && go2->type == SPRITE_OBJECT) {
          addToSpriteRasterList(go1, go2);
        }
        #endif
      }
    }

    switch (go1->type) {
      case BOB_OBJECT:
      if (go1->u.mediums[g_active_buffer]) {
        bobList[bob_index++] = go1;
        terminateBOBDrawList((struct BOB*)go1->u.mediums[g_active_buffer]);
      }
      break;
      case SPRITE_OBJECT:
      if (go1->u.medium) {
        spriteList[sprite_index++] = go1;
        #ifdef SMART_SPRITES
        terminateSpriteLists((struct Sprite*)go1->u.medium);
        #endif
      }
      break;
    }
  }

  bobList[bob_index] = NULL; bob_index = 0;
  spriteList[sprite_index] = NULL; sprite_index = 0;
}
///
///clearBOB(gameobject)
VOID clearBOB(struct GameObject* self)
{
  struct BOB* bob = (struct BOB*)self->u.mediums[g_active_buffer];

  if (bob && !(bob->flags & BOB_CLEARED)) {
    struct GameObject** go_ptr = bob->clearList;
    struct GameObject* go;

    while ((go = *go_ptr++)) {
      ((struct BOB*)go->u.mediums[g_active_buffer])->flags &= ~BOB_DEAD; //revive overlaying dead bobs!
      clearBOB(go);
    }

    // double-check the medium because it may be resigned in another call
    // by another bob during the recursion in the above loop!
    if (self->u.mediums[g_active_buffer]) {
      if (!(bob->flags & BOB_CLEARED)) unBlitBOB(self);

      if (bob->flags & BOB_RESIGNED) {
        bob->flags &= ~(BOB_ASSIGNED | BOB_RESIGNED | BOB_DEAD);
        bob->clearList_index = 0;
        self->u.mediums[g_active_buffer] = NULL;
      }
    }

    bob->flags |= BOB_CLEARED;
    bob->flags &= ~BOB_DRAWN; // this is only here for dead bobs!
  }
}
///
///drawBOB(gameobject)
VOID drawBOB(struct GameObject* self)
{
  struct BOB* bob = (struct BOB*)self->u.mediums[g_active_buffer];

  if (bob && !(bob->flags & BOB_DRAWN)) {
    struct GameObject** go_ptr;
    struct GameObject* go;

    if (bob->flags & BOB_DEAD)
      bob->flags |= BOB_DRAWN;
    else
      clearBOB(self);

    #ifndef DOUBLE_BUFFER
    go_ptr = bob->cl2drList;
    while ((go = *go_ptr++)) {
      if (go->u.medium) clearBOB(go);
    }
    #endif

    go_ptr = bob->drawList;
    while ((go = *go_ptr++)) {
      if (go->u.mediums[g_active_buffer]) {
        drawBOB(go);
        addToBOBClearList(go, self);
      }
    }

    // double-check the medium because it may be resigned in another call
    // by another bob during the recursion in the above loop!
    if (self->u.mediums[g_active_buffer] && !(bob->flags & BOB_DRAWN)) {
      blitBOB(self);

      if (bob->flags & BOB_DYING) {
        bob->flags &= ~BOB_DYING;
        bob->flags |=  BOB_DEAD;
      }
      bob->flags |= BOB_DRAWN;
    }
  }
}
///
///setSpriteHSN(gameobject)
/******************************************************************************
 * Finds an available hardware sprite number for this sprite. It respects     *
 * priorities of the colliding sprites and rasterline occupancy.              *
 * There is only 8 hardware sprites available for one raster line on the      *
 * Amiga so it is not guaranteed that this function will always allocate a    *
 * hardware sprite number. It will set HSN_CANT_SET in that case.             *
 * WARNING: Only used with SMART_SPRITES.                                     *
 * NOTE: Could be optimized by storing 'used' in struct Sprite                *
 ******************************************************************************/
#ifdef SMART_SPRITES
UWORD getSpriteHSN(struct GameObject* go)
{
  struct Sprite* sprite = (struct Sprite*)go->u.medium;
  struct SpriteImage* si= (struct SpriteImage*)go->image;

  if (sprite->hsn < 8) {
    UWORD num_images = (si->sprite_bank->table[si->image_num].type) & 0xEF;

    return (UWORD)((0x00FF >> (8 - num_images)) << sprite->hsn);
  }

  return (UWORD)0x0000;
}

UWORD setSpriteHSN(struct GameObject* go)
{
  struct Sprite* sprite = (struct Sprite*)go->u.medium;
  struct SpriteImage* si= (struct SpriteImage*)go->image;
  struct GameObject** go_i = sprite->behindList;
  struct GameObject* bh;
  struct GameObject* rs;
  UWORD availHardSprites = 0;

  if (sprite->hsn == HSN_CANT_SET) return 0x0000;

  if (sprite->hsn == HSN_NOT_SET) {
    UBYTE type = si->sprite_bank->table[si->image_num].type;
    UBYTE num_images = type & 0xEF;
    UBYTE incr = type & 0x10 ? 2 : 1;
    UWORD used = 0x00FF >> (8 - num_images);
    UBYTE hsn = 0;

    // check hsn of the the gameobjects on the behind list
    while ((bh = *go_i++)) {
      availHardSprites |= setSpriteHSN(bh);
    }

    // check hsn of the gameobjects on the raster list
    go_i = sprite->rasterList;
    while ((rs = *go_i++)) {
      availHardSprites |= getSpriteHSN(rs);
    }

    // try to find an available hardware sprite slot to set hsn
    while (hsn < 8) {
      if (used & availHardSprites) {
        hsn += incr;
        used <<= incr;
      }
      else {
        sprite->hsn = hsn;
        return used;
      }
    }
    sprite->hsn = HSN_CANT_SET;
    return (UWORD)0x0000;
  }
  else {
    UBYTE num_images = (si->sprite_bank->table[si->image_num].type) & 0xEF;
    return (UWORD)((0x00FF >> (8 - num_images)) << sprite->hsn);
  }
}
#endif
///
///updateSmartSprites()
#ifdef SMART_SPRITES
VOID updateSmartSprites(VOID)
{
  struct GameObject** go_ptr = spriteList;
  struct GameObject* go = *go_ptr;

  while (go) {
    setSpriteHSN(go);

    go = *(++go_ptr);
  }
}
#endif
///
///updateBOBs()
VOID updateBOBs()
{
  struct GameObject** go_ptr = bobList;
  struct GameObject* go;

#ifdef DOUBLE_BUFFER
  while ((go = *go_ptr++)) {
    clearBOB(go);
  }

  go_ptr = bobList;
#endif

  while ((go = *go_ptr++)) {
    drawBOB(go);
  }
}
///
///updateGameObject(gameobject)
/******************************************************************************
 * Applies the animation functions on each GameObject on themselves.          *
 * Later it assign/resigns the available display mediums for GameObjects that *
 * go into/out of the visible display area.                                   *
 ******************************************************************************/
VOID updateGameObject(struct GameObject* go)
{
  // Apply its animation on the game object itself
  if (go->anim.func) go->anim.func(go);

  // Best place to check and fire up events for GameObject tile collisions
  if (go->collideTile) go->collideTile(go);

  switch (go->type) {
    case SPRITE_OBJECT:
      // Check if the GameObject is inside the visible display
      if (go->u.medium) {
        if (go->state & GOB_INACTIVE) {
          resignSprite(go);
          return;
        }

        if (go->x2 <= *mapPosX || go->x1 >= *mapPosX2 || go->y2 <= *mapPosY || go->y1 >= *mapPosY2) {
          //GameObject has moved out of visible screen so remove the medium from this GameObject
          resignSprite(go);
        }
      }
      else if (go->x2 > *mapPosX && go->x1 < *mapPosX2 && go->y2 > *mapPosY && go->y1 < *mapPosY2) {
        //GameObject has moved into the visible screen so attach a medium to this GameObject
        assignSprite(go);
      }
    break;
    case BOB_OBJECT:
      // Check if the GameObject is inside the visible display
      if (go->u.mediums[g_active_buffer]) {
        if (go->state & GOB_INACTIVE) {
          resignBOB(go);
          return;
        }

        #ifndef DOUBLE_BUFFER
        if (go->x != go->old_x || go->y != go->old_y) {
          fillBOBCl2DrList(go);
        }
        #endif

        //Reset some bob members for the next draw
        ((struct BOB*)go->u.mediums[g_active_buffer])->flags &= (~(BOB_DRAWN | BOB_CLEARED));
        terminateBOBClearList((struct BOB*)go->u.mediums[g_active_buffer]);

        if (go->x2 <= *mapPosX || go->x1 >= *mapPosX2 || go->y2 <= *mapPosY || go->y1 >= *mapPosY2) {
          //GameObject has moved out of visible screen so remove the medium from this GameObject
          resignBOB(go);
        }
        else if (go->state & GOB_DEAD) {
          if (go->x1 >= *mapPosX && go->x2 <= *mapPosX2 && go->y1 >= *mapPosY && go->y2 <= *mapPosY2) {
            if (!(((struct BOB*)go->u.mediums[g_active_buffer])->flags & BOB_DEAD)) {
              ((struct BOB*)go->u.mediums[g_active_buffer])->flags |= BOB_DYING;
            }
          }
          else {
            ((struct BOB*)go->u.mediums[g_active_buffer])->flags &= ~BOB_DEAD;
          }
        }
      }
      else if (go->x2 > *mapPosX && go->x1 < *mapPosX2 && go->y2 > *mapPosY && go->y1 < *mapPosY2) {
        //GameObject has moved into the visible screen so attach a medium to this GameObject
        assignBOB(go);
      }
    break;
  }
}
///
///updateGameObjects()
/******************************************************************************
 * Not only calls updateGameObject() on each GameObject but also does an      *
 * insertion sort on them (against their y2) in the same traversion, so that  *
 * checkGameObjectCollisions() gets a sorted list of GameObjects.             *
 ******************************************************************************/
VOID updateGameObjects()
{
  struct GameObject** i = gameobjectList;

  if (*i) {
    updateGameObject(*i);

    for (++i; *i != NULL; i++) {
      struct GameObject** l;
      struct GameObject* v = *(i);

      updateGameObject(v);

      for (l = i - 1; l >= gameobjectList; l--) {
        if ((*l)->y2 <= v->y2) break;
        else *(l + 1) = *l;
      }
      *(l + 1) = v;
    }
    #if NUM_GAMEOBJECTS > 0
    //Removes despawned/destroyed gameobjects from gameobjectList
    for (--i; (*i)->y2 == MAXINT && num_gameobjects; i--) {
      *i = NULL;
      num_gameobjects--;
    }
    #endif

    checkGameObjectCollisions();
    #ifdef SMART_SPRITES
    updateSmartSprites();
    #endif
  }
}
///

///moveGameObject(gameobject, dx, dy)
VOID moveGameObject(struct GameObject* go, LONG dx, LONG dy)
{
  go->old_x = go->x;
  go->old_y = go->y;

  go->x += dx;
  go->y += dy;

  go->x1 += dx;
  go->y1 += dy;
  go->x2 += dx;
  go->y2 += dy;
}
///
///moveGameObjectClamped(gameobject, dx, dy, clampX, clampY)
VOID moveGameObjectClamped(struct GameObject* go, LONG dx, LONG dy, LONG clampX1, LONG clampY1, LONG clampX2, LONG clampY2)
{
  LONG x = go->x + dx;
  LONG y = go->y + dy;

  go->old_x = go->x;
  go->old_y = go->y;

  if (x < clampX1) { dx = clampX1 - go->x; go->x = clampX1; }
  else if (x > clampX2) { dx = clampX2 - go->x; go->x = clampX2; }
  else go->x = x;

  if (y < clampY1) { dy = clampY1 - go->y; go->y = clampY1; }
  else if (go->y > clampY2) { dy = clampY2 - go->y; go->y = clampY2; }
  else go->y = y;

  go->x1 += dx;
  go->y1 += dy;
  go->x2 += dx;
  go->y2 += dy;
}
///
///setGameObjectPos(gameobject, x, y)
/******************************************************************************
 * NOTE: To cover INVISIBLE_OBJECTs with width & height and no image, we      *
 * should calculate deltas from new values then add them all to the rect.     *
 ******************************************************************************/
VOID setGameObjectPos(struct GameObject* go, LONG x, LONG y)
{
  LONG dx = x - go->x;
  LONG dy = y - go->y;

  go->old_x = go->x;
  go->old_y = go->y;

  go->x = x;
  go->y = y;

  go->x1 += dx;
  go->y1 += dy;
  go->x2 += dx;
  go->y2 += dy;
}
///
///setGameObjectImage(gameobject, image)
VOID setGameObjectImage(struct GameObject* go, struct ImageCommon* img)
{
  go->image = img;

  go->x1 = go->x + img->h_offs;
  go->y1 = go->y + img->v_offs;
  go->x2 = go->x1 + img->width;
  go->y2 = go->y1 + img->height;
}
///

///newGameObjectBank(size)
/******************************************************************************
 * A gameobject bank is a struct that holds (in order) the:                   *
 * - number of gameobjects (num_gameobjects)                                  *
 * - a pointer to actual 'num_gameobjects' gameobjects in memory              *
 * - (gameobjectList) an array of 'num_gameobjects + NUM_GAMEOBJECTS + 1'     *
 *   gameobject pointers (NUM_GAMEOBJECTS is for spawnable gameobjects and +1 *
 *   is for the NULL terminator)                                              *
 * - actual 'num_gameobjects' gameobjects                                     *
 ******************************************************************************/
struct GameObjectBank* newGameObjectBank(ULONG size)
{
  struct GameObjectBank* bank = NULL;

  if (size) {
    bank = AllocMem(sizeof(struct GameObjectBank)
                    + size * sizeof(struct GameObject)
                    + (size + NUM_GAMEOBJECTS + 1) * sizeof(struct GameObject*),
                    MEMF_ANY | MEMF_CLEAR);
    if (bank) {
      ULONG i;

      bank->num_gameobjects = size;
      bank->gameobjects = (struct GameObject*)((ULONG)bank + sizeof(struct GameObjectBank) + (size + NUM_GAMEOBJECTS + 1) * sizeof(struct GameObject*));

      for (i = 0; i < size; i++) {
        bank->gameobjectList[i] = &bank->gameobjects[i];
      }
      bank->gameobjectList[i] = NULL;
    }
    else printf("Not enough memory for GameObjectBank. Size: %lu!\n", size);
  }
  else printf("Invalid GameObjectBank size. Size: %lu\n", size);

  return bank;
}
///
///freeGameObjectBank(gameobject_bank)
VOID freeGameObjectBank(struct GameObjectBank* bank)
{
  if (bank) {
    FreeMem(bank, sizeof(struct GameObjectBank)
                  + bank->num_gameobjects * sizeof(struct GameObject)
                  + (bank->num_gameobjects + NUM_GAMEOBJECTS + 1) * sizeof(struct GameObject*));
  }
}
///
///loadGameObjectBank(file)
struct GameObjectBank* loadGameObjectBank(STRPTR file)
{
  struct GameObjectBank* bank = NULL;

  if (file) {
    BPTR fh = Open(file, MODE_OLDFILE);
    if (fh) {
      ULONG i;
      UBYTE id[8];
      ULONG num_gameobjects = 0;
      struct GameObjectData data;

      Read(fh, id, 8);
      if (strcmp(id, "GAMEOBJ")) {
        Close(fh);
        printf("Invalid GameObject file: %s\n", file);
        return NULL;
      }
      Read(fh, &num_gameobjects, sizeof(num_gameobjects));

      bank = newGameObjectBank(num_gameobjects);
      if (bank) {
        for (i = 0; i < num_gameobjects; i++) {
          Read(fh, &data, sizeof(data));
          bank->gameobjects[i].x = data.x;
          bank->gameobjects[i].y = data.y;
          bank->gameobjects[i].type = data.type;
          bank->gameobjects[i].state = data.state;
          bank->gameobjects[i].me_mask = data.me_mask;
          bank->gameobjects[i].hit_mask = data.hit_mask;
          bank->gameobjects[i].collide = gobjCollisionFunction[data.collide_func];
          bank->gameobjects[i].collideTile = tileCollisionFunction[data.collideTile_func];
          bank->gameobjects[i].anim.type = data.anim_type;
          bank->gameobjects[i].anim.state = data.anim_state;
          bank->gameobjects[i].anim.frame = data.anim_frame;
          bank->gameobjects[i].anim.direction = NONE;
          bank->gameobjects[i].anim.storage_1 = 0;
          bank->gameobjects[i].anim.storage_2 = 0;
          bank->gameobjects[i].anim.func = animFunction[data.anim_func];
          switch (data.type) {
            case SPRITE_OBJECT:
            bank->gameobjects[i].image = (struct ImageCommon*) &current_level.sprite_bank[data.bank]->image[data.image];
            break;
            case BOB_OBJECT:
            bank->gameobjects[i].image = (struct ImageCommon*) &current_level.bob_sheet[data.bank]->image[data.image];
            break;
            default:
            bank->gameobjects[i].image = NULL; //TODO: Implement other Game Object types!
            break;
          }
          if (bank->gameobjects[i].image) {
            bank->gameobjects[i].x1 = bank->gameobjects[i].x + bank->gameobjects[i].image->h_offs;
            bank->gameobjects[i].y1 = bank->gameobjects[i].y + bank->gameobjects[i].image->v_offs;
            bank->gameobjects[i].x2 = bank->gameobjects[i].x1 + bank->gameobjects[i].image->width;
            bank->gameobjects[i].y2 = bank->gameobjects[i].y1 + bank->gameobjects[i].image->height;
          }
          else {
            bank->gameobjects[i].x1 = data.x;
            bank->gameobjects[i].y1 = data.y;
            bank->gameobjects[i].x2 = data.x + data.width;
            bank->gameobjects[i].y2 = data.y + data.height;
          }
          bank->gameobjects[i].priority = data.priority;
        }
      }

      Close(fh);
    }
  }

  return bank;
}
///

///checkHitBoxCollision(go1, go2)
/******************************************************************************
 * Checks the collisions of hitboxes on both gameobjects. Return value has    *
 * the first gameobject's (go1) hitbox index in the highword and the second   *
 * gameobject's hitbox index in the lowword. If either one of the gameobjects *
 * do not have hitboxes, collision will be tested for its image rectangle and *
 * its word on the return value will be 0. If both do not have hitboxes will  *
 * return 0. So for each specific collision you will have a unique return     *
 * value you can test against and initiate events.                            *
 * NOTE: The collision of the first encountered hitboxes of both GameObjects  *
 * will be on the return value. So order your hitboxes with this in mind.     *
 ******************************************************************************/
ULONG checkHitBoxCollision(struct GameObject* go1, struct GameObject* go2)
{
  ULONG hb_count1 = 0;
  ULONG hb_count2 = 0;

  if (go1->image && go1->image->hitbox) {
    struct HitBox* hb1 = go1->image->hitbox;

    do {
      LONG x1_1, y1_1;
      LONG x2_1, y2_1;

      hb_count1++;
      x1_1 = go1->x + hb1->x1;
      y1_1 = go1->y + hb1->y1;
      x2_1 = go1->x + hb1->x2;
      y2_1 = go1->y + hb1->y2;

      if (go2->image && go2->image->hitbox) {
        struct HitBox* hb2 = go2->image->hitbox;

        do {
          LONG x1_2, y1_2;
          LONG x2_2, y2_2;

          hb_count2++;
          x1_2 = go2->x + hb2->x1;
          y1_2 = go2->y + hb2->y1;
          x2_2 = go2->x + hb2->x2;
          y2_2 = go2->y + hb2->y2;

          if (x2_2 > x1_1 && x2_1 > x1_2 && y2_2 > y1_1 && y2_1 > y1_2) {
            return ((hb_count1 << 16) & hb_count2);
          }

        } while((hb2 = hb2->next));
      }
      else {
        if (go2->x2 > x1_1 && x2_1 > go2->x1 && go2->y2 > y1_1 && y2_1 > go2->y1) {
          return (hb_count1 << 16);
        }
      }

    }while((hb1 = hb1->next));
  }
  else {
    if (go2->image && go2->image->hitbox) {
      struct HitBox* hb2 = go2->image->hitbox;

      do {
        LONG x1_2, y1_2;
        LONG x2_2, y2_2;

        hb_count2++;
        x1_2 = go2->x + hb2->x1;
        y1_2 = go2->y + hb2->y1;
        x2_2 = go2->x + hb2->x2;
        y2_2 = go2->y + hb2->y2;

        if (x2_2 > go1->x1 && go1->x2 > x1_2 && y2_2 > go1->y1 && go1->y2 > y1_2) {
          return (hb_count2);
        }

      } while((hb2 = hb2->next));
    }
    else {
      return 0;
    }
  }

  return 0;
}
///
///checkPointHitBoxCollision(go, x, y)
/******************************************************************************
 * Checks if a map coordinate collides with one of the hitboxes on the        *
 * gameobject. Return value will be the index of the hitbox that collides     *
 * with this point. Returns 0 if there are no hitboxes on the gameobject or   *
 * none of them collides with the point.                                      *
 * NOTE: The collision of the first encountered hitbox will be returned.      *
 *       So order your hitboxes with this in mind.                            *
 ******************************************************************************/
ULONG checkPointHitBoxCollision(struct GameObject* go, LONG x, LONG y)
{
  ULONG hb_count = 0;

  if (go->image && go->image->hitbox) {
    struct HitBox* hb = go->image->hitbox;

    do {
      LONG x1, y1;
      LONG x2, y2;

      hb_count++;
      x1 = go->x + hb->x1;
      y1 = go->y + hb->y1;
      x2 = go->x + hb->x2;
      y2 = go->y + hb->y2;

      if (x >= x1 && x < x2 && y >= y1 && y < y2) {
        return hb_count;
      }

    }while((hb = hb->next));
  }

  return 0;
}
///

///spawnGameObject(go)
/******************************************************************************
 * Add a new gameobject to gameObjectList if there is any spawnable           *
 * gameobjects available. The properties on the passed gameobject pointer     *
 * will be copied to the spawned gameobject.                                  *
 * Uses the same very optimized algorithm as used in assign medium functions  *
 * to find a free spawnable gameobject.                                       *
 * NOTE: If no free spawnable gameobject is available nothing will be done!   *
 ******************************************************************************/
#if NUM_GAMEOBJECTS > 0
struct GameObject* spawnGameObject(struct GameObject* go)
{
  struct GameObject* spawn = NULL;

  if (*avail_gameobject) {
    spawn = *avail_gameobject;
    gameobjectList[num_gameobjects++] = spawn;
    memcpy(spawn, go, sizeof(struct GameObject));
    avail_gameobject++;
  }

  return spawn;
}
#endif
///
///despawnGameObject(go)
/******************************************************************************
 * Marks a spawned gameobject to be sorted to out and eventually removed from *
 * gameObjectList, and makes it available for another spawnGameObject() call. *
 * WARNING: NEVER CALL THIS ON A NON-SPAWNED GAMEOBJECT FROM A BANK!          *
 * Use destroyGameObject() instead!                                           *
 ******************************************************************************/
#if NUM_GAMEOBJECTS > 0
VOID despawnGameObject(struct GameObject* go)
{
  go->state = GOB_INACTIVE; //Will resign its mediums
  avail_gameobject--;
  *avail_gameobject = go;
  go->y2 = MAXINT; //Will mark this game object to be removed from gameObjectList
}
#endif
///
///destroyGameObject(go)
/******************************************************************************
 * Removes a bank gameobject from the gameObjectList.                         *
 * WARNING: DO NOT CALL THIS ON A SPAWNED GAME OBJECT!                        *
 * Use despawnGameObject() instead!                                           *
 ******************************************************************************/
VOID destroyGameObject(struct GameObject* go)
{
  go->state = GOB_INACTIVE; //Will resign its mediums
  go->y2 = MAXINT; //Will mark this game object to be removed from gameObjectList
}
///
