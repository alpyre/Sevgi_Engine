#ifndef	CHUNKY_IMAGE_H
#define	CHUNKY_IMAGE_H

#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>

#include "ILBM_image.h"

typedef union {
  struct {
    UBYTE A,R,G,B;
  }argb;
  ULONG value;
} ARGB;

struct CLUT {
  ULONG size; //It is an ULONG to long align the table
  ARGB colors[0];
};

struct LUTPixelArray {
  UWORD width;
  UWORD height;
  UBYTE pixels[0];
};

struct CLUT* newCLUT(struct ILBMImage* img);
VOID freeCLUT(struct CLUT* clut);

struct LUTPixelArray* newLUTPixelArray(struct BitMap* bm);
VOID freeLUTPixelArray(struct LUTPixelArray* lparr);
VOID scaleLUTPixelArray(APTR srcRect, UWORD srcW, UWORD srcH, UWORD srcMod, struct RastPort* rp, APTR colTable, UWORD destX, UWORD destY, UWORD scale);

#endif /* CHUNKY_IMAGE_H */


/*
WriteLUTPixelArray(data->sheet.lpa->pixels, 0, 0, data->sheet.lpa->width,
                  _rp(obj), data->sheet.clut->colors, _mleft(obj), _mtop(obj), MIN(_mwidth(obj), data->sheet.lpa->width), MIN(_mheight(obj), data->sheet.lpa->height), CTABFMT_XRGB8);

printf("srcRect: %lu, srcW: %u, srcH: %u, srcMod: %u\nrp: %lu, lut: %lu, destX: %u, destY: %u, scale: %u\n",
(ULONG)srcRect, srcW, srcH, srcMod, (ULONG)rp, (ULONG)colTable, destX, destY, scale);

*/
