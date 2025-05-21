///includes
#include <exec/exec.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "collisions.h"
///
///defines
///
///globals
extern struct Level current_level;
///
//prototypes
// Collisions between gameobjects
VOID blockObjectCollision(struct GameObject*, struct GameObject*);

// Collisions between tiles and gameobjects
VOID testTileColl1(struct GameObject*);

//exports
VOID (*gobjCollisionFunction[NUM_GOBJ_COLL_FUNCS])(struct GameObject*, struct GameObject*) = {NULL,
                                                                                              blockObjectCollision};

VOID (*tileCollisionFunction[NUM_TILE_COLL_FUNCS])(struct GameObject*) = {NULL,
                                                                          testTileColl1};
//

/********************************************
 * Collisions functions between gameobjects *
 ********************************************/
///blockObjectCollision(gameobject)
/******************************************************************************
 * Prevents the image rectangle of a gameobject entering in.                  *
 * NOTE: This implementation prioritizes pushing go2s out vertical first in   *
 * the case of entry at corners. This is in accordance with block tiles       *
 * pushing out horizontal first. Otherwise there will be leakages in blocking.*
 * NOTE: You must overlap block objects at least one pixel if you need        *
 * compound shapes which use more than one gameobject. Otherwise there will   *
 * be leakages in blocking. Colliding block objects won't push eachother out  *
 * as long as they don't have an anim.direction.                              *
 ******************************************************************************/
VOID blockObjectCollision(struct GameObject* go1, struct GameObject* go2)
{
  //Collisions fire up by image rectangle
  switch (go2->anim.direction) {
    case LEFT:
      setGameObjectPos(go2, go1->x2 - go2->image->h_offs, go2->y);
    break;
    case RIGHT:
      setGameObjectPos(go2, go1->x1 - go2->image->width - go2->image->h_offs - 1, go2->y);
    break;
    case DOWN:
      setGameObjectPos(go2, go2->x, go1->y1 - go2->image->height - go2->image->v_offs - 1);
    break;
    case UP:
      setGameObjectPos(go2, go2->x, go1->y2 - go2->image->h_offs);
    break;
    case LEFT|UP:
      if (go1->x2 - go2->x1 >= go1->y2 - go2->y1) {
        setGameObjectPos(go2, go2->x, go1->y2 - go2->image->h_offs);
      }
      else {
        setGameObjectPos(go2, go1->x2 - go2->image->h_offs, go2->y);
      }
    break;
    case LEFT|DOWN:
      if (go1->x2 - 1 - go2->x1 >= go2->y2 - go1->y1) {
        setGameObjectPos(go2, go2->x, go1->y1 - go2->image->height - go2->image->v_offs - 1);
      }
      else {
        setGameObjectPos(go2, go1->x2 - go2->image->h_offs, go2->y);
      }
    break;
    case RIGHT|UP:
      if (go2->x2 - go1->x1 >= go1->y2 - 1 - go2->y1) {
        setGameObjectPos(go2, go2->x, go1->y2 - go2->image->h_offs);
      }
      else {
        setGameObjectPos(go2, go1->x1 - go2->image->width - go2->image->h_offs - 1, go2->y);
      }
    break;
    case RIGHT|DOWN:
      if (go2->x2 - go1->x1 >= go2->y2 - go1->y1) {
        setGameObjectPos(go2, go2->x, go1->y1 - go2->image->height - go2->image->v_offs - 1);
      }
      else {
        setGameObjectPos(go2, go1->x1 - go2->image->width - go2->image->h_offs - 1, go2->y);
      }
    break;
  }
}
///

/********************************************
 * Collisions between tiles and gameobjects *
 ********************************************/
///testTileColl1(gameobject)
VOID testTileColl1(struct GameObject* go)
{
  /*
  LONG x = go->x;
  LONG y = go->y;
  struct TileMap* map = current_level.tilemap[current_level.current.tilemap];
  */

  /**********************************
   * Tile collide per image hotspot *
   **********************************/
  /*
  TILEID tileID = TILEID_AT_COORD(map, x, y);
  */

  /************************************
   * Tile collide per image rectangle *
   ************************************/
  // x1, y1 collision
  // tileID = TILEID_AT_COORD(map, go->x1, go->y1);

  // x2, y1 collision
  // tileID = TILEID_AT_COORD(map, go->x2, go->y1);

  // x2, y2 collision
  // tileID = TILEID_AT_COORD(map, go->x2, go->y2);

  // x1, y2 collision
  // tileID = TILEID_AT_COORD(map, go->x1, go->y2);
}
///
