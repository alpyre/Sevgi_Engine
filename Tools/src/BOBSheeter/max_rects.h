#ifndef	MAX_RECTS_H
#define	MAX_RECTS_H

// Image Descriptor (specifies an image on the BOB sheet)
struct ImageDesc {
  UWORD original_index;
  UWORD width, height;
  UWORD x_pos, y_pos;
};

struct ImageDesc* PackImagesMaxRects(struct ImageDesc* input, ULONG count, UWORD* out_w, UWORD* out_h);

#endif // MAX_RECTS_H
