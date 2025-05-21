#ifndef	ILBM_IMAGE_H
#define	ILBM_IMAGE_H

struct ILBMImage {
  UWORD width;
  UWORD height;
  ULONG num_colors;
  struct BitMap* bitmap;
  struct {
    UBYTE R, G, B;
  }cmap[0];
};

struct ILBMImage* loadILBMImage(STRPTR fileName, ULONG bm_flags);
VOID freeILBMImage(struct ILBMImage* img);

#endif /* IMAGE_H */
