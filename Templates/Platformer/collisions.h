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

//Tile identifiers
#define TI_PLATFORM_LEFT_EDGE_1  42
#define TI_PLATFORM_CENTER       43
#define TI_PLATFORM_RIGHT_EDGE_1 44
#define TI_PLATFORM_LEFT_EDGE_2  45
#define TI_PLATFORM_RIGHT_EDGE_2 46
#define TI_PLATFORM_LEFT_EDGE_3  47
#define TI_PLATFORM_RIGHT_EDGE_3 48
#define TI_PLATFORM_SINGLE       49

#define TI_SLOPE_45_UP           54
#define TI_SLOPE_45_UP_SUB_1     64
#define TI_SLOPE_45_UP_SUB_2     66
#define TI_SLOPE_45_DOWN         55
#define TI_SLOPE_45_DOWN_SUB_1   65
#define TI_SLOPE_45_DOWN_SUB_2   67
#define TI_SLOPE_22_1_UP         50
#define TI_SLOPE_22_2_UP         51
#define TI_SLOPE_22_1_UP_SUB_1   56
#define TI_SLOPE_22_2_UP_SUB_1   57
#define TI_SLOPE_22_1_UP_SUB_2   60
#define TI_SLOPE_22_2_UP_SUB_2   61
#define TI_SLOPE_22_1_DOWN       52
#define TI_SLOPE_22_2_DOWN       53
#define TI_SLOPE_22_1_DOWN_SUB_1 58
#define TI_SLOPE_22_2_DOWN_SUB_1 59
#define TI_SLOPE_22_1_DOWN_SUB_2 62
#define TI_SLOPE_22_2_DOWN_SUB_2 63

#define TI_SLOPE_45_UP_SUPER     72
#define TI_SLOPE_45_DOWN_SUPER   73
#define TI_SLOPE_22_1_UP_SUPER   68
#define TI_SLOPE_22_2_UP_SUPER   69
#define TI_SLOPE_22_1_DOWN_SUPER 70
#define TI_SLOPE_22_2_DOWN_SUPER 71
#define TI_SLOPE_END             74

//TO ACCESS COLLISION FUNCTIONS ADD THESE LINES INTO YOUR .c fILE:
//extern VOID (*gobjCollisionFunction[NUM_GOBJ_COLL_FUNCS])(struct GameObject*, struct GameObject*);
//extern VOID (*tileCollisionFunction[NUM_TILE_COLL_FUNCS])(struct GameObject*);

#endif /* COLLISIONS_H */
