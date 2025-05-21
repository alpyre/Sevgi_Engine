#ifndef PHYSICS_H
#define PHYSICS_H

#define GRAVITY 10

struct Physics {
  BYTE velocity_x;
  BYTE velocity_y;
  WORD mass;
  WORD elasticity;
  WORD friction;
  WORD spin;
  //...etc...
};

//PROTOTYPES FOR YOU PHYSICS FUNCTIONS BELOW

#endif /* PHYSICS_H */
