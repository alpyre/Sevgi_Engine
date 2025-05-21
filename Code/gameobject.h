#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "settings.h"
#include "physics.h"

//GameObject types
#define INVISIBLE_OBJECT 0
#define SPRITE_OBJECT    1
#define BOB_OBJECT       2

//BOBSheet types
#define BST_DISTRUBUTED 0
#define BST_IRREGULAR   1

//Game Object States
#define GOB_DEAD     0x01
#define GOB_INACTIVE 0x02

//Anim directions
#define NONE  0x0
#define LEFT  0x4
#define RIGHT 0x5
#define UP    0x8
#define DOWN  0xA

#define HAS_HITBOXES 0x08 //bit 3 on bobsheet/spritebank file's type

#ifdef BIG_IMAGE_SIZES
  typedef UWORD UIMGS;
  typedef WORD  IMGS;
#else
  #ifdef SMALL_IMAGE_SIZES
    typedef UBYTE UIMGS;
    typedef BYTE  IMGS;
  #else
    typedef UBYTE UIMGS;
    typedef WORD  IMGS;
  #endif
#endif

#define IMAGE_COMMON \
  UIMGS width;\
  UIMGS height;\
  IMGS h_offs; /* offset of hotspot from left edge of the image */\
  IMGS v_offs; /*   "    "     "      "  top    "   "  "    "   */

#ifdef SMALL_HITBOX_SIZES
  #define HITBOX_COMMON \
    BYTE x1, y1;\
    BYTE x2, y2;
#else
  #define HITBOX_COMMON \
    WORD x1, y1;\
    WORD x2, y2;
#endif

struct HitBox {
  HITBOX_COMMON
  struct HitBox* next;
};

struct ImageCommon {
  IMAGE_COMMON
  struct HitBox* hitbox;
  //type specific members follow
};

struct SpriteImage {
  IMAGE_COMMON
  struct HitBox* hitbox;
  struct SpriteBank* sprite_bank;
  UWORD  image_num;     // the index of this image on the sprite bank
  UWORD  hsn;           // hardware sprite number to display this image
};

struct SpriteTable {
  UWORD offset;
  UBYTE type;
  UBYTE pad;
};

struct SpriteBank {
  UWORD num_images;
  UWORD num_hitboxes;
  UWORD dataSize;
  UBYTE attached; //If the bank contains attached sprites this is set to TRUE
  struct HitBox* hitboxes;
  struct SpriteTable* table;
  UBYTE* data;
  struct SpriteImage image[0];
};

struct Sprite {
  struct GameObject* behindList[NUM_SPRITES]; //this sprite is to be displayed behind these sprites
  struct GameObject* rasterList[NUM_SPRITES]; //this sprite shares rasterlines with these sprites
  UBYTE behindList_index;
  UBYTE rasterList_index;
  UBYTE flags;
  UBYTE hsn;
};

struct BOBImage {
  IMAGE_COMMON
  struct HitBox* hitbox;
  struct BitMap* bob_sheet;
  UBYTE* pointer;       //pointer to image start in bob sheet
  UBYTE* mask;          //pointer to image mask in bob sheet
  UWORD  bytesPerRow;   //calculated as ((width-1)/16 + 1) * 2 NOTE: Needs a better name
};

struct BOBSheet {
  UWORD num_images;
  UWORD num_hitboxes;
  struct HitBox* hitboxes;
  struct BitMap* bitmap;
  struct BOBImage image[0];
};

struct BOB {
  UBYTE* background;
  struct GameObject* drawList[NUM_BOBS];  //these BOBs have to be drawn before this BOB to be drawn
  struct GameObject* clearList[NUM_BOBS]; //these BOBs have to be cleared before this BOB to be cleared
  UBYTE drawList_index;
  UBYTE clearList_index;
  UWORD flags;
  struct {
    LONG x;                   // gameobject->x during blitBOB() (but clipped)
    LONG y;                   // gameobject->y during blitBOB() (but clipped)
    LONG y2;                  // gameobject->y2 during blitBOB() (but clipped)
    struct BitMap* bob_sheet; // Bob sheet of the image used in blitBOB()
    UWORD words;              // Width of the blit in words
    UWORD rows;               // Height of the blit in pixellines
    UWORD x_s;                // Shift value to go into BLTCONx (already left shifted 12)
    UWORD bltafwm;
    UWORD bltalwm;
    UWORD bltamod;
    UWORD bltbmod;
    UWORD bltdmod;
    UBYTE* bltapt;
    UBYTE* bltbpt;
  }lastBlt;
};

struct GameObject {
  LONG x, y;          // Map coordinates of the game object (hot spot)
  LONG old_x, old_y;  // Can be used for movement cancellation
  LONG x1, y1;        // Image rectangle on map (top left corner)
  LONG x2, y2;        //   "      "      "   "  (bottom right corner)
  UBYTE type;         // BOB_OBJECT or SPRITE_OBJECT
  UBYTE state;        // GOB_ALIVE, GOB_DEAD or GOB_INACTIVE
  UBYTE me_mask;
  UBYTE hit_mask;
  VOID (*collide)(struct GameObject*, struct GameObject*); // Functions to be called when...
  VOID (*collideTile)(struct GameObject*);                 // ...a collision is detected
  struct Anim {
    UBYTE type;
    UBYTE state;
    UBYTE frame;
    UBYTE direction;
    UBYTE speed;
    UBYTE counter_1;
    UBYTE counter_2;
    UBYTE counter_3;
    LONG storage_1;
    LONG storage_2;
    VOID (*func)(struct GameObject*);
  }anim;
  struct ImageCommon* image; // A pointer to a BOBImage or SpriteImage depending on type
  APTR  medium;    // A pointer to a BOB or to a Sprite depending on type
  BYTE  priority;  // Display priority (higher is in front of lower)
  //struct Physics physics;  // Define your physics struct and functions in physics.c/.h
};

struct GameObjectBank {
  ULONG num_gameobjects;
  struct GameObject* gameobjects;
  struct GameObject* gameobjectList[0];
};

// This struct is used to load gameobject initial values from disk
// WARNING: DO NOT add structs. DO NOT add arrays. Standard C storage types only!
struct GameObjectData {
  LONG  x, y;       // Position of the object on the game map
  UWORD width;      // Width to use if the object is of type INVISIBLE_OBJECT
  UWORD height;     // Height to use if the object is of type INVISIBLE_OBJECT
  UBYTE type;       // INVISIBLE_OBJECT, SPRITE_OBJECT or BOB_OBJECT
  UBYTE state;      // either of GOB_ALIVE, GOB_DEAD or GOB_INACTIVE
  UBYTE me_mask;    // A bit mask defining what a gameobject collides with
  UBYTE hit_mask;   // A bit mask defining what a gameobject gets hit by
  UBYTE collide_func;     // The index of the collision func in gobjCollisionFunction[] global
  UBYTE collideTile_func; // The index of the collision func in tileCollisionFunction[] global
  UBYTE anim_type;    // Some values to be used to define the type of animation
  UBYTE anim_state;   // ...a game object will have. The way these will be used
  UBYTE anim_frame;   // ...will be implemented by the developer.
  UBYTE anim_func;    // The index of the animation func in animFunction[] global
  UWORD image;   // The index of the image (in the bank) a gameobject will use
  UBYTE bank;    // The index of the SpriteBank or BOBSheet (on the level)
  BYTE  priority;   // The display priority (front/back) of the gameobject
  //YOU CAN ADD YOUR CUSTOM VARIABLES BELOW
};

VOID initGameObjects(VOID* blitBOBFunc, VOID* unBlitBOBFunc, ULONG max_bob_width, ULONG max_go_height);
VOID updateGameObjects(VOID);
VOID updateBOBs(VOID);
VOID moveGameObject(struct GameObject* go, LONG dx, LONG dy);
VOID moveGameObjectClamped(struct GameObject* go, LONG dx, LONG dy, LONG clampX1, LONG clampY1, LONG clampX2, LONG clampY2);
VOID setGameObjectPos(struct GameObject* go, LONG x, LONG y);
VOID setGameObjectImage(struct GameObject* go, struct ImageCommon* img);

ULONG checkHitBoxCollision(struct GameObject* go1, struct GameObject* go2);
ULONG checkPointHitBoxCollision(struct GameObject* go, LONG x, LONG y);

struct GameObjectBank* newGameObjectBank(ULONG size);
VOID freeGameObjectBank(struct GameObjectBank* bank);
struct GameObjectBank* loadGameObjectBank(STRPTR file);
struct BitMap* allocBOBBackgroundBuffer(UWORD max_bob_width, UWORD max_bob_height, UWORD max_bob_depth);

#if NUM_GAMEOBJECTS > 0
struct GameObject* spawnGameObject(struct GameObject* go);
VOID despawnGameObject(struct GameObject* go);
#endif
VOID destroyGameObject(struct GameObject* go);

#endif /* GAMEOBJECT_H */

/*
STRUCTURE OF A BOB SHEET FILE (in pseudo code)
struct BOBSheetFile {
  UBYTE id[8];   // <-- "BOBSHEET"
  UBYTE ilbmFile[?];
  UBYTE type;
  UWORD num_images;
  // depending on type
  // BST_IRREGULAR
  struct {
    UBYTE or UWORD width;
    UBYTE or UWORD height;
    BYTE or WORD h_offs;
    BYTE or WORD v_offs;
    UBYTE word;  // x coord of image in bob sheet / 16 (NOTE: Bob images MUST be in a WORD boundary)
    UWORD row;   // y coord of image in bob sheet.
  }imageData[num_images];
  // OR
  UBYTE width;
  UBYTE height;
  UBYTE columns;
  UBYTE rows;

  //Hitbox data follows (if type has HAS_HITBOXES)
};
*/

/*
STRUCTURE OF A SPRITE BANK FILE (in pseudo code)
struct SpriteBankFile {
  UBYTE id[7];   // <-- "SPRBANK"
  UBYTE type;
  UWORD num_images;
  struct {
    UWORD offset;
    UBYTE type;
    UBYTE hsn;
    // depending on type
    UBYTE or UWORD width;
    UBYTE or UWORD height;
    BYTE or WORD h_offs;
    BYTE or WORD v_offs;
  }table[num_images];
  UWORD data_size;
  UBYTE data[data_size];

  //Hitbox data follows (if type has HAS_HITBOXES)
};
*/

/*Hitbox data (in pseudo code)
struct HitBoxData {
  UWORD num_hitboxes;
  UWORD index[num_images];  //Index of the first hitbox for each image UWORDMAX (0xFFFF) means none
  struct HitBox {
    //depending on type
    BYTE or WORD x1, y1;
    BYTE or WORD x2, y2;
    UWORD next_hitbox;      //Index of the next hitbox for compound hitboxes UWORDMAX (0xFFFF) means none
  }hitboxes[num_hitboxes];
};
*/

/*
  (data size) type bits:
  0: BST_IRREGULAR
  1: SMALL_IMAGE_SIZES
  2: BIG_IMAGE_SIZES
  3: HAS_HITBOXES
  4: SMALL_HITBOX_SIZES
  5 - 7: {unused}
*/

/*
  (sprite table) type bits:
  0 - 3: num_images
  4:     attached
  5 - 6: sfmode
  7: {unused}
*/
