///includes
//Amiga headers
#include <stdio.h> //TEMP

#include <exec/exec.h>
#include <graphics/gfx.h>

//Amiga protos
#include <proto/exec.h>
#include <proto/graphics.h>

#include "chunky_image.h"
///
///globals
extern APTR g_MemoryPool;
///

///newCLUT(ilbm_img)
struct CLUT* newCLUT(struct ILBMImage* img)
{
  ULONG size = img->num_colors;
  struct CLUT* clut = AllocPooled(g_MemoryPool, sizeof(struct CLUT) + size * sizeof(ARGB));

  if (clut) {
    ULONG c;

    clut->size = size;

    for (c = 0; c < size; c++) {
      clut->colors[c].argb.A = 0;
      clut->colors[c].argb.R = img->cmap[c].R;
      clut->colors[c].argb.G = img->cmap[c].G;
      clut->colors[c].argb.B = img->cmap[c].B;
    }
  }

  return clut;
}
///
///freeCLUT(clut)
VOID freeCLUT(struct CLUT* clut)
{
  if (clut) {
    FreePooled(g_MemoryPool, clut, sizeof(struct CLUT) + clut->size * sizeof(ARGB));
  }
}
///
///newLUTPixelArray(bitmap)
/******************************************************************************
 * TODO: This implementation is terribly slow! Find a way to make it faster!  *
 ******************************************************************************/
struct LUTPixelArray* newLUTPixelArray(struct BitMap* bm)
{
  struct LUTPixelArray* lparr = NULL;

  if (bm) {
    UWORD width = GetBitMapAttr(bm, BMA_WIDTH);
    UWORD height = GetBitMapAttr(bm, BMA_HEIGHT);
    //UWORD depth = GetBitMapAttr(bm, BMA_DEPTH);

    lparr = AllocPooled(g_MemoryPool, sizeof(struct LUTPixelArray) + width * height);

    if (lparr) {
      lparr->width = width;
      lparr->height = height;

      //Planar to Chunky conversion!
      {
        ULONG x, y, r;
        struct RastPort rp; //a fake rastport to be able to call ReadPixel()
        InitRastPort(&rp);
        rp.BitMap = bm;

        for (y = 0; y < height; y++) {
          r = y * width;
          for (x = 0; x < width; x++) {
            LONG index = ReadPixel(&rp, x, y); //WARNING: Probably this is slow!
            lparr->pixels[r + x] = (UBYTE)index;
          }
        }
      }
    }
  }

  return lparr;
}
///
///freeLUTPixelArray(LUTPixelArray)
VOID freeLUTPixelArray(struct LUTPixelArray* lparr)
{
  if (lparr) {
    FreePooled(g_MemoryPool, lparr, sizeof(struct LUTPixelArray) + lparr->width * lparr->height);
  }
}
///

///scaleLUTPixelArray(srcRect, srcW, srcH, srcMod, rastPort, colTable, destX, destY, scale)
VOID scaleLUTPixelArray(APTR srcRect, UWORD srcW, UWORD srcH, UWORD srcMod, struct RastPort* rp, APTR colTable, UWORD destX, UWORD destY, UWORD scale)
{
  switch (scale) {
    case 1:
      WriteLUTPixelArray(srcRect, 0, 0, srcMod, rp, colTable, destX, destY, srcW, srcH, CTABFMT_XRGB8);
    break;
    default:
    {
      UBYTE* src = (UBYTE*)srcRect;
      ULONG* lut = (ULONG*)colTable;
      ULONG r, c; //source row and column
      ULONG x, y; //dest x and y on the rastPort
      LONG next_row = srcMod - srcW;

      for (r = 0, y = destY; r < srcH; r++, y += scale) {
        for (c = 0, x = destX; c < srcW; c++, x += scale) {
          FillPixelArray(rp, x, y, scale, scale, lut[*src]);
          src++;
        }
        src += next_row;
      }
    }
    break;
  }
}
///
