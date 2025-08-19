#ifndef FONTS_H
#define FONTS_H

//TO ACCESS LOADED FONTS ADD THESE LINE INTO YOUR .c FILE:
// extern struct TextFont* textFonts[NUM_TEXTFONTS];
// extern struct GameFont* gameFonts[NUM_GAMEFONTS];

#include "settings.h"

#define GF_TYPE_FIXED        0
#define GF_TYPE_PROPORTIONAL 1

struct GameFont {
  struct RastPort rastport;
  struct BitMap* mask;
  UWORD tmp_buf_x;
  UBYTE type;
  UBYTE width;
  UBYTE height;
  UBYTE spacing;
  UBYTE start;
  UBYTE end;
  struct {
    UWORD offset;
  }letter[0];
};

BOOL openFonts(VOID);
VOID closeFonts(VOID);
ULONG GF_TextLength(struct GameFont* gf, STRPTR str, ULONG count);
ULONG GF_TextFit(struct GameFont* gf, STRPTR str, ULONG count, ULONG width);
VOID GF_Text(struct RastPort* rp, struct GameFont* gf, STRPTR str, ULONG count);
struct BitMap* createBltMasks(struct BitMap* bm);

#endif /* FONTS_H */

/*
  STRIP FONT FILE FORMAT DESIGN:
    "GAMEFONT"      // eight char identifier
    "Stripname.iff" // filename string to strip image (NULL terminated)
    UBYTE type      // fixed or proportional (0/1)
    UBYTE width     // width for fixed (width of space char for proportional)
    UBYTE height    // height of the gamefont
    UBYTE spacing   // space between every letter
    UBYTE start     // how many letters this strip contains beginning from...
    UBYTE end       // ..ascii start to ascii end!
    // file ends here for type fixed
    // for proportional:
    struct {
      UWORD offset;
    }letter[end - start + 2];
*/
