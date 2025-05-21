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
extern volatile struct Custom custom;
extern struct Level current_level;
///
//prototypes
// Collisions functions between gameobjects
VOID testGobjColl1(struct GameObject*, struct GameObject*);

// Collisions between tiles and gameobjects
VOID testTileColl1(struct GameObject*);

//exports
VOID (*gobjCollisionFunction[NUM_GOBJ_COLL_FUNCS])(struct GameObject*, struct GameObject*) = {NULL,
                                                                                              testGobjColl1};

VOID (*tileCollisionFunction[NUM_TILE_COLL_FUNCS])(struct GameObject*) = {NULL,
                                                                          testTileColl1};
//

/********************************************
 * Collisions functions between gameobjects *
 ********************************************/
///testGobjColl1(gameobject) //TEMP:
VOID testGobjColl1(struct GameObject* go1, struct GameObject* go2)
{
  // A hitbox collision can be implemented here if required

}
///

/********************************************
 * Collisions between tiles and gameobjects *
 ********************************************/
///testTileColl1(gameobject)
VOID testTileColl1(struct GameObject* go)
{
  // TILEID tileID;

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

  /**********************************
   * Tile collide per image hotspot *
   **********************************/
  // tileID = TILEID_AT_COORD(map, go->x, go->y);
}
///
