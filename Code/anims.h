#ifndef ANIMS_H
#define ANIMS_H

#include "level.h"
#include "tiles.h"
#include "tilemap.h"
#include "collisions.h"
#include "gameobject.h"
#include "audio.h"

/*********************************************
 * NUMBER OF ANIMATION FUNCTIONS             *
 * NOTE: One extra for the NULL initializer! *
 *********************************************/
#define NUM_ANIMS 3

//TO ACCESS ANIMS ADD THIS LINE INTO YOUR .c fILE:
//extern VOID (*animFunction[NUM_ANIMS])(struct GameObject*);

#endif /* ANIMS_H */
