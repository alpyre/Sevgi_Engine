#ifndef COLLISIONS_H
#define COLLISIONS_H

#include "level.h"
#include "tiles.h"
#include "tilemap.h"
#include "gameobject.h"
#include "audio.h"

/*********************************************
 * NUMBER OF COLLISION FUNCTIONS             *
 * NOTE: One extra for the NULL initializer! *
 *********************************************/
#define NUM_GOBJ_COLL_FUNCS 2
#define NUM_TILE_COLL_FUNCS 2

//TO ACCESS COLLISION FUNCTIONS ADD THESE LINES INTO YOUR .c fILE:
//extern VOID (*gobjCollisionFunction[NUM_GOBJ_COLL_FUNCS])(struct GameObject*, struct GameObject*);
//extern VOID (*tileCollisionFunction[NUM_TILE_COLL_FUNCS])(struct GameObject*);

#endif /* COLLISIONS_H */
