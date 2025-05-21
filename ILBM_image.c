///includes
//standard headers
#include <stdio.h>

//Amiga headers
#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>

//Amiga protos
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include "ILBM_image.h"
///
///globals
extern APTR g_MemoryPool;
///

///locateIFFChunkInFile(fileHandle, chunkID)
/******************************************************************************
 * A KMP routine to locate the given 4 character chunkID in an open iff file. *
 ******************************************************************************/
BOOL locateIFFChunkInFile(BPTR fh, STRPTR chunkID)
{
  BOOL found = FALSE;
  UBYTE lps[4] = {0};
  LONG i;
  ULONG j = 0;
  UBYTE len = 0; // length of the previous longest prefix suffix
  UBYTE ch;

  i = 1;
  while (i < 4) {
    if (chunkID[i] == chunkID[len]) {
      len++;
      lps[i] = len;
      i++;
    }
    else {
      if (len != 0) {
        len = lps[len - 1];
      }
      else {
        lps[i] = 0;
        i++;
      }
    }
  }

  i = Read(fh, &ch, 1);
  while (i > 0) {
    if (chunkID[j] == ch) {
      j++;
    }

    if (j == 4) {
      found = TRUE;
      break;
    }
    else {
      i = Read(fh, &ch, 1);
      if (i > 0 && chunkID[j] != ch) {
        if (j != 0)
        j = lps[j - 1];
        else
        i = Read(fh, &ch, 1);
      }
    }
  }

  return found;
}
///
///loadILBMImage(fileName, bm_flags)
struct ILBMImage* loadILBMImage(STRPTR fileName, ULONG bm_flags)
{
  struct BitMapHeader bmhd;
  struct ILBMImage* img = NULL;
  UWORD num_colors;
  BPTR fh;

  fh = Open(fileName, MODE_OLDFILE);
  if (fh) {
    if (locateIFFChunkInFile(fh, "FORM")) {
      if (locateIFFChunkInFile(fh, "ILBM")) {
        if (locateIFFChunkInFile(fh, "BMHD")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct BitMapHeader) == Read(fh, &bmhd, sizeof(struct BitMapHeader))) {
            if (locateIFFChunkInFile(fh, "CMAP")) {
              Seek(fh, 4, OFFSET_CURRENT);
              num_colors = 1 << bmhd.bmh_Depth;

              img = AllocPooled(g_MemoryPool, sizeof(struct ILBMImage) + num_colors * 3);
              if (img) {
                img->width = bmhd.bmh_Width;
                img->height = bmhd.bmh_Height;
                img->num_colors = num_colors;
                img->bitmap = NULL;
                Read(fh, img->cmap, num_colors * 3);

                if (locateIFFChunkInFile(fh, "BODY")) {
                  Seek(fh, 4, OFFSET_CURRENT);
                  img->bitmap = AllocBitMap(bmhd.bmh_Width, bmhd.bmh_Height, bmhd.bmh_Depth, bm_flags, NULL);
                  if (img->bitmap && (bm_flags ? (GetBitMapAttr(img->bitmap, BMA_FLAGS) & bm_flags) : TRUE)) {
                    if (bmhd.bmh_Compression) { // NOTE: consider possible compression methods
                      UBYTE *w = (UBYTE*)img->bitmap->Planes[0]; // write cursor
                      ULONG bpr = (bm_flags & BMF_INTERLEAVED) ? img->bitmap->BytesPerRow / bmhd.bmh_Depth : img->bitmap->BytesPerRow; // real bytes per row
                      ULONG row;
                      ULONG plane;
                      UBYTE b = 0;                      // read byte
                      UBYTE b2 = 0;

                      for (row = 0; row < bmhd.bmh_Height; row++) {
                        for (plane = 0; plane < bmhd.bmh_Depth; plane++) {
                          ULONG bytesWritten = 0;
                          w = (UBYTE*)img->bitmap->Planes[plane] + (row * img->bitmap->BytesPerRow); // update write cursor

                          while (bytesWritten < bpr) {
                            //read a byte from body
                            Read(fh, &b, 1);
                            if (b < 128) {
                              Read(fh, w, b + 1);
                              w += b + 1;
                              bytesWritten += b + 1;
                            }
                            else if (b > 128) {
                              Read(fh, &b2, 1);
                              memset(w, b2, 257 - b);
                              w += (257 - b);
                              bytesWritten += (257 - b);
                            }
                          }
                        }
                        if (bmhd.bmh_Masking == 1) {
                          ULONG bytesSkipped = 0;
                          while (bytesSkipped < bpr) {
                            //read a byte from body
                            Read(fh, &b, 1);
                            if (b < 128) {
                              Seek(fh, b + 1, OFFSET_CURRENT);
                              bytesSkipped += b + 1;
                            }
                            else if (b > 128) {
                              Seek(fh, 1, OFFSET_CURRENT);
                              bytesSkipped += (257 - b);
                            }
                          }
                        }
                      }
                    }
                    else // uncompressed ilbm
                    {
                      if (bm_flags & BMF_INTERLEAVED) {
                        ULONG bpr = (bm_flags & BMF_INTERLEAVED) ? img->bitmap->BytesPerRow / bmhd.bmh_Depth : img->bitmap->BytesPerRow; // real bytes per row
                        ULONG row;
                        ULONG plane;
                        UBYTE *w;
                        for (row = 0; row < bmhd.bmh_Height; row++) {
                          for (plane = 0; plane < bmhd.bmh_Depth; plane++) {
                            w = (UBYTE*)img->bitmap->Planes[plane] + (row * img->bitmap->BytesPerRow); // update write cursor
                            Read(fh, w, bpr);
                          }
                          if (bmhd.bmh_Masking == 1) {
                            Seek(fh, bpr, OFFSET_CURRENT);
                          }
                        }
                      }
                      else {
                        ULONG bpr = (bm_flags & BMF_INTERLEAVED) ? img->bitmap->BytesPerRow / bmhd.bmh_Depth : img->bitmap->BytesPerRow; // real bytes per row
                        ULONG row;
                        UBYTE *w = (UBYTE*)img->bitmap->Planes[0]; // write cursor
                        for (row = 0; row < bmhd.bmh_Height; row++) {
                          Read(fh, w, bmhd.bmh_Depth * bpr);
                          w += (bmhd.bmh_Depth * bpr);
                          if (bmhd.bmh_Masking == 1) {
                            Seek(fh, bpr, OFFSET_CURRENT);
                          }
                        }
                      }
                    }
                  }
                  else { freeILBMImage(img); img = NULL; puts("Not enough memory!"); }
                }
                else { freeILBMImage(img); img = NULL; puts("Corrupt ILBM file!"); }
              }
              else puts("Not enough memory!");
            }
            else puts("Cannot find color map in file!");
          }
          else puts("Corrupt ILBM file!");
        }
        else puts("Corrupt ILBM file!");
      }
      else puts("Not an ILBM file!");
    }
    else puts("Not an IFF file!");


    Close(fh);
  }
  else puts("Cannot open ILBM file!");

  return img;
}
///
///freeILBMImage(image)
VOID freeILBMImage(struct ILBMImage* img)
{
  if (img) {
    if (img->bitmap) FreeBitMap(img->bitmap);
    FreePooled(g_MemoryPool, img, sizeof(struct ILBMImage) + img->num_colors * 3);
  }
}
///
