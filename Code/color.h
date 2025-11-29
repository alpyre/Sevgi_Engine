#ifndef COLOR_H
#define COLOR_H

#include "settings.h"

#define BPLCON3_V BPLCON3_BRDNBLNK | 0xC00

//Precision bits for fixed point arithmetic used in ColorTables (8 or 20)
// 20 bits is highly precise and fast.
// 8 bits is precise enough (no noticable visual difference with 20) yet it cuts
// the size of ColorTables in half at the cost of one additional operation per
// color (it is two additional operations in AGA).
#define CT_PRECISION 8

//Data structure of a palette in pseudo code
// struct Palette {
//   UBYTE num_colors;      // actually the number of colors in the palette -1
//   struct {
//     UBYTE R, G, B;
//   }hues[num_colors + 1];
// };
//
// NOTE: num_colors is actually the the number of colors in the palette minus 1.
//       When it is 0 there is 1 color in the palette.
//       When it is 255 there are 256 colors in the palette.
//       This way we can store the size of a 256 color (full AGA palette) in a
//       single UBYTE.
//       So that the data type for a palette can just be an UBYTE array of size:
//       num_colors * 3 + 1

//Gradient types
#define GRD_TYPE_OCS 1  // WARNING: Do not change these values. They not only...
#define GRD_TYPE_AGA 2  // ...define type, but also are used in calculations!

//Color table states
#define CT_IDLE 0
#define CT_FADE_IN 1
#define CT_FADE_OUT 2

#if CT_PRECISION == 8
  struct ColorState {
    union {
      struct {
        UWORD R,G,B;
      }value;
      struct {
        UBYTE R,RP, G,GP, B,BP;
      }bytes;
    }color;
    struct {
      UWORD R,G,B;
    }increment;
  };
#elif CT_PRECISION == 20
  struct ColorState {
    union {
      struct {
        ULONG R,G,B;
      }value;
      struct {
        UBYTE RH,RL;
        UWORD RP;
        UBYTE GH,GL;
        UWORD GP;
        UBYTE BH,BL;
        UWORD BP;
      }bytes;
    }color;
    struct {
      ULONG R,G,B;  //NOTE: we can make this LONG to cover negative increments!
    }increment;
  };
#endif

struct ColorTable {
  UBYTE* table;              // palette, copperlist or the gradient this ColorTable is for
  struct ColorState* states; // holds the current states and increment values for all the colors
  UWORD fade_steps;          // how many steps a complete fade in/out takes
  UWORD fade_step;           // on which step are we on the fade/in out
  UWORD state;               // CT_IDLE, CT_FADE_IN or CT_FADE_OUT
  UWORD colors;              // number of colors this ColorTable fades
};

struct Gradient {
  UWORD num_colors; // number of "all" colors on the gradient
  UBYTE type;       // AGA or OCS gradient
  UBYTE col_reg;    // On which color register this gradient will apply
  UBYTE scrollable: 1;
  UBYTE movable   : 1;
  UBYTE blitable  : 1;
  UBYTE modulus;    // blitter modulus for the color instructions on the rainbow              |
  UBYTE index;      // the position of the color instruction on the CopOp                     -> These three are to be set by createRainbow()
  UBYTE loct_offs;  // the byte offset between the color instruction and it's AGA counterpart |
  UWORD pos;        // the position of the visible part on "all" (num_colors) for scrollable gradients
  UWORD pos_max;    // precalculated maximum scroll position for pos
  UWORD vis;        // the visible size of the gradient on screen (in rasterlines) for scrollable gradients
  UBYTE scr_pos;    // the rasterline this gradient will start (screen coordinate)
  UBYTE scr_pos_max;// precalculated maximum position for scr_pos
  UWORD* colors;    // In word aligned chipram
  struct ColorTable* color_table;
};
struct ColorTable* newColorTable(UBYTE* table, UWORD fade_steps, ULONG step);
struct ColorTable* newColorTable_CL(UWORD* copperlist, UWORD fade_steps, ULONG step);
struct ColorTable* newColorTable_GRD(struct Gradient* grd, UWORD fade_steps, ULONG step);
VOID changeFadeSteps(struct ColorTable* ct, ULONG steps);
VOID freeColorTable(struct ColorTable* ct);
#define updateColorTable(ct) updateColorTable_Partial(ct, 0, ct->colors)
VOID updateColorTable_Partial(struct ColorTable* ct, ULONG start, ULONG end);
#define setColorTable(ct) setColorTable_REG(ct, 0, ct->colors)
#define setColorTable_Partial setColorTable_REG
VOID setColorTable_REG(struct ColorTable* ct, ULONG start, ULONG num_colors);
VOID setColorTable_CL(struct ColorTable* ct);
VOID setColorTable_GRD(struct ColorTable* ct);
VOID setColorTable_CLP(struct ColorTable* ct, UWORD* address, ULONG start, ULONG end);
VOID blackOut(VOID);
VOID setColorToAll(UBYTE R, UBYTE G, UBYTE B);
VOID setColorToAll_CLP(UWORD* address, ULONG size, UBYTE R, UBYTE G, UBYTE B);
VOID setColor(ULONG index, UBYTE R, UBYTE G, UBYTE B);
VOID setColor_CLP(ULONG index, UWORD* address, ULONG size, UBYTE R, UBYTE G, UBYTE B);
VOID setColors(UBYTE* palette);
VOID updateColor(struct ColorTable* ct, ULONG index, UBYTE R, UBYTE G, UBYTE B);

#endif /* COLOR_H, */
