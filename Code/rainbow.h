#ifndef RAINBOW_H
#define RAINBOW_H

#include "display_level.h"
#include "color.h"

//Gradient flags
#define GRD_SCROLLABLE 0x01
#define GRD_MOVABLE    0x02
#define GRD_BLITABLE   0x03
#define GRD_DITHERED   0x04

struct CopOp {
  UWORD wait;
  UWORD size;     // number of instructions in the CopOp (WARNING: NOT byte size!)
  ULONG* pointer; // pointer to the actual copper list instruction in chipram
};

struct Rainbow {
  UWORD num_ops;      // Number of rainbow CopOps
  UWORD num_insts;    // list_size can be determined with num_insts * 4
  ULONG blitable;     // Has movable and/or scrollable gradients
  ULONG* list;        // In word aligned chipram
  struct Gradient** gradList; // createRainbow fills this as a back pointer
  UBYTE correspondance[SCREEN_HEIGHT];
  struct CopOp copOps[0];
};

struct Rainbow* newRainbow(UWORD num_ops, UWORD op_size, UWORD num_insts, BOOL blitable, UWORD num_end_insts);
struct Rainbow* createEmptyRainbow(VOID);
struct Rainbow* createRainbow(struct Gradient**, ULONG* end_insts);
VOID freeRainbow(struct Rainbow*);
VOID updateRainbow(struct Rainbow*);

struct Gradient* createGradient(UBYTE type, UBYTE col_reg, UWORD num_colors, UWORD scr_pos, UWORD vis, UWORD pos, ULONG flags, UBYTE* huelist);
VOID scrollGradientUp(struct Gradient* grd, UWORD pixels);
VOID scrollGradientDown(struct Gradient* grd, UWORD pixels);
VOID setGradientScrollPos(struct Gradient* grd, UWORD pos);
VOID moveGradient(struct Gradient* grd, UWORD scr_pos);
VOID freeGradient(struct Gradient*);

#endif /* RAINBOW_H */
