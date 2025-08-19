///includes
#include <stdio.h>
#include <string.h>

#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/text.h>

#include <proto/exec.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#include "diskio.h"
#include "display.h"
#include "fonts.h"
///
///defines
#define PATH_BUFFER_LENGTH 128
#define ROUND_TO_16(a) ((a + 15) & 0xFFFFFFF0)
///
///globals
struct TextFont* textFonts[NUM_TEXTFONTS] = {NULL};
struct GameFont* gameFonts[NUM_GAMEFONTS] = {NULL};

//private globals
#include "assets.h"
///
///prototypes
struct GameFont* openGameFont(STRPTR);
VOID closeGameFont(struct GameFont*);
///

///openFonts()
BOOL openFonts()
{
  ULONG i;

  for (i = 0; i < NUM_TEXTFONTS; i++) {
    textFonts[i] = OpenDiskFont(&textAttrs[i]);
    if (!textFonts[i]) {
      printf("Disk font %s: %u could not be found!\n", textAttrs[i].ta_Name, textAttrs[i].ta_YSize);
      closeFonts();
      return FALSE;
    }
  }

  for (i = 0; i < NUM_GAMEFONTS; i++) {
    gameFonts[i] = openGameFont(gameFontFiles[i]);
    if (!gameFonts[i]) {
      printf("Game font %s could not be opened!\n", gameFontFiles[i]);
      closeFonts();
      return FALSE;
    }
  }

  return TRUE;
}
///
///closeFonts()
VOID closeFonts()
{
  ULONG i;
  for (i = 0; i < NUM_TEXTFONTS; i++) {
    if (textFonts[i]) {
      CloseFont(textFonts[i]);
    }
  }
  for (i = 0; i < NUM_GAMEFONTS; i++) {
    if (gameFonts[i]) {
      closeGameFont(gameFonts[i]);
    }
  }
}
///
///createBltMasks(bitmap)
/******************************************************************************
 * Creates a single plane mask for bitmaps.                                   *
 ******************************************************************************/
struct BitMap* createBltMasks(struct BitMap* bm)
{
  struct BitMap* mask_bm = NULL;
  ULONG isInterleaved = (GetBitMapAttr(bm, BMA_FLAGS) & BMF_INTERLEAVED);
  ULONG height = bm->Rows;
  ULONG width  = isInterleaved ? bm->BytesPerRow / bm->Depth * 8 : bm->BytesPerRow * 8;

  mask_bm = allocBitMap(width, height, 1, BMF_DISPLAYABLE, bm);
  if (mask_bm) {
    ULONG rs;               // row start (for read)
    ULONG rsw;              // row start (for write)
    UBYTE* wc;              // write cursor
    UBYTE* rd;              // read cursor
    ULONG bpr = width / 8;  // bytes per planeline
    ULONG longs = bpr / 4;
    ULONG word = bpr % 4;
    ULONG r, d, l;

    for (r = 0, rs = 0, rsw = 0; r < height; r++, rs += bm->BytesPerRow, rsw += bpr) {
      for (l = 0; l < longs; l++) {
        ULONG ls = l * 4;
        ULONG mask_l = 0;
        for (d = 0; d < bm->Depth; d++) {
          rd = bm->Planes[d] + ls + rs;
          mask_l |= *(ULONG*)rd;
        }
        wc = mask_bm->Planes[0] + ls + rsw;
        *(ULONG*)wc = mask_l;
      }
      if (word) {
        ULONG ws = longs * 4;
        UWORD mask_w = 0;
        for (d = 0; d < bm->Depth; d++) {
          rd = bm->Planes[d] + ws + rs;
          mask_w |= *(UWORD*)rd;
        }
        wc = mask_bm->Planes[0] + ws + rsw;
        *(WORD*)wc = mask_w;
      }
    }
  }
  else puts("Mask bitmap allocation failed!");

  return mask_bm;
}
///
///openGameFont(file)
struct GameFont* openGameFont(STRPTR file)
{
  struct GameFont* gf = NULL;
  BPTR fh;
  UBYTE path[PATH_BUFFER_LENGTH];
  ULONG path_len = (ULONG)PathPart(file) - (ULONG)file;

  strncpy(path, file, path_len);
  path[path_len] = 0;

  fh = Open(file, MODE_OLDFILE);
  if (fh) {
    UBYTE id[8];
    UBYTE strip_file[32];
    struct BitMap* strip = NULL;
    struct BitMap* mask = NULL;
    ULONG extra_width = 0;
    struct {
      UBYTE type;
      UBYTE width;
      UBYTE height;
      UBYTE spacing;
      UBYTE start;
      UBYTE end;
    }properties;

    Read(fh, id, 8);
    if (strncmp(id, "GAMEFONT", 8) == 0) {
      ULONG i = 0;
      Read(fh, &strip_file[i], 1);
      while (strip_file[i]) {
        Read(fh, &strip_file[++i], 1);
      }
      if (i) {
        Read(fh, &properties, sizeof(properties));
        switch (properties.type) {
          case GF_TYPE_FIXED:
            extra_width = ROUND_TO_16(properties.width);
            AddPart(path, strip_file, PATH_BUFFER_LENGTH);
            strip = loadILBMBitMap(path, BM_TYPE_GAMEFONT, extra_width);
            if (strip) {
              if (strip->Depth > 1) {
                mask = createBltMasks(strip);
                if (!mask) {
                  puts("Could not create game font mask!");
                  FreeBitMap(strip);
                  Close(fh);
                  return NULL;
                }
              }
              else mask = strip;

              gf = AllocMem(sizeof(struct GameFont), MEMF_ANY);
              if (gf) {
                InitRastPort(&gf->rastport);
                gf->rastport.BitMap = strip;
                gf->mask    = mask;
                gf->type    = properties.type;
                gf->width   = properties.width;
                gf->height  = properties.height;
                gf->spacing = properties.spacing;
                gf->start   = properties.start;
                gf->end     = properties.end;
                #if BM_TYPE_GAMEFONT & BMF_INTERLEAVED
                gf->tmp_buf_x = (strip->BytesPerRow * 8 / strip->Depth) - extra_width;
                #else
                gf->tmp_buf_x = (strip->BytesPerRow * 8) - extra_width;
                #endif
              }
              else {
                puts("Not enough memory for fixed width game font!");
                FreeBitMap(strip);
              }
            }
            else puts("Game font ilbm could not be loaded!");
          break;
          case GF_TYPE_PROPORTIONAL:
          {
            UBYTE letters = properties.end - properties.start + 2;
            gf = AllocMem(sizeof(struct GameFont) + sizeof(UWORD) * letters, MEMF_ANY);
            if (gf) {
              ULONG i;
              UWORD max_letter_width = 0;

              Read(fh, gf->letter, sizeof(UWORD) * letters);

              max_letter_width = properties.width;
              for (i = 1; i < letters; i++) {
                UWORD letter_width = gf->letter[i].offset - gf->letter[i - 1].offset;
                if (letter_width > max_letter_width) max_letter_width = letter_width;
              }

              extra_width = ROUND_TO_16(max_letter_width);
              AddPart(path, strip_file, PATH_BUFFER_LENGTH);
              strip = loadILBMBitMap(path, BM_TYPE_GAMEFONT, extra_width);
              if (strip) {
                if (strip->Depth > 1) {
                  mask = createBltMasks(strip);
                  if (!mask) {
                    puts("Could not create game font mask!");
                    FreeBitMap(strip);
                    FreeMem(gf, sizeof(struct GameFont) + sizeof(UWORD) * letters);
                    Close(fh);
                    return NULL;
                  }
                }
                else mask = strip;

                InitRastPort(&gf->rastport);
                gf->rastport.BitMap = strip;
                gf->mask    = mask;
                gf->type    = properties.type;
                gf->width   = properties.width;
                gf->height  = properties.height;
                gf->spacing = properties.spacing;
                gf->start   = properties.start;
                gf->end     = properties.end;
                #if BM_TYPE_GAMEFONT & BMF_INTERLEAVED
                gf->tmp_buf_x = (strip->BytesPerRow * 8 / strip->Depth) - extra_width;
                #else
                gf->tmp_buf_x = (strip->BytesPerRow * 8) - extra_width;
                #endif
              }
              else puts("Game font ilbm could not be loaded!");
            }
            else {
              puts("Not enough memory for fixed width game font!");
            }
          }
          break;
          default:
            puts("Invalid game font type!");
            FreeBitMap(strip);
          break;
        }
      }
      else puts("Invalid game font ilbm filename!");
    }
    else puts("Invalid game font file!");

    Close(fh);
  }

  return gf;
}
///
///closeGameFont(gamefont)
VOID closeGameFont(struct GameFont* gf)
{
  if (gf) {
    FreeBitMap(gf->rastport.BitMap);
    if (gf->mask != gf->rastport.BitMap) FreeBitMap(gf->mask);

    switch (gf->type) {
      case GF_TYPE_FIXED:
        FreeMem(gf, sizeof(struct GameFont));
      break;
      case GF_TYPE_PROPORTIONAL:
        FreeMem(gf, sizeof(struct GameFont) + sizeof(UWORD) * (gf->end - gf->start + 2));
      break;
    }
  }
}
///

///GF_TextLength(gamefont, str, count)
/******************************************************************************
 * Similar to TextLength from the API, this function calculates the estimated *
 * pixel width of a string when rendered with the given gamefont.             *
 ******************************************************************************/
ULONG GF_TextLength(struct GameFont* gf, STRPTR str, ULONG count)
{
  ULONG len = 0;

  switch (gf->type) {
    case GF_TYPE_FIXED:
    {
      ULONG i = 0;
      UBYTE ch = *str;

      while (ch) {
        if (ch == ' ') {
          len += gf->width;
          if (++i >= count) break;
          ch = *str++;
          continue;
        }
        if (ch < gf->start || ch > gf->end) {
          if (++i >= count) break;
          ch = *str++;
          continue;
        }
        len += gf->width;

        if (++i >= count) break;
        if ((ch = *str++)) len += gf->spacing;
      }
    }
    break;
    case GF_TYPE_PROPORTIONAL:
    {
      ULONG i = 0;
      UBYTE ch = *str;

      while (ch) {
        UBYTE l;

        if (ch == ' ') {
          len += gf->width;
          if (++i >= count) break;
          ch = *str++;
          continue;
        }
        if (ch < gf->start || ch > gf->end) {
          if (++i >= count) break;
          ch = *str++;
          continue;
        }
        l = gf->letter[ch - gf->start + 1].offset - gf->letter[ch - gf->start].offset;
        len += l;

        if (++i >= count) break;
        if ((ch = *str++) && l) len += gf->spacing;
      }
    }
    break;
  }

  return len;
}
///
///GF_TextFit(gamefont, str, count, width)
/******************************************************************************
 * Similar to TextFit from the API, this function calculates how many         *
 * characters of the given string would fit into the given width when         *
 * rendered with the given gamefont.                                          *
 ******************************************************************************/
ULONG GF_TextFit(struct GameFont* gf, STRPTR str, ULONG count, ULONG width)
{
  ULONG i = 0;

  switch (gf->type) {
    case GF_TYPE_FIXED:
    {
      ULONG len = 0;
      UBYTE ch = *str;

      while (ch) {
        if (ch == ' ') {
          len += gf->width;
          if (len > width) break;
          if (++i >= count) break;
          ch = *str++;
          continue;
        }
        if (ch < gf->start || ch > gf->end) {
          if (++i >= count) break;
          ch = *str++;
          continue;
        }
        len += gf->width;
        if (len > width) break;

        if (++i >= count) break;
        if ((ch = *str++)) len += gf->spacing;
      }
    }
    break;
    case GF_TYPE_PROPORTIONAL:
    {
      ULONG len = 0;
      UBYTE ch = *str;

      while (ch) {
        UBYTE l;

        if (ch == ' ') {
          len += gf->width;
          if (len > width) break;
          if (++i >= count) break;
          ch = *str++;
          continue;
        }
        if (ch < gf->start || ch > gf->end) {
          if (++i >= count) break;
          ch = *str++;
          continue;
        }
        l = gf->letter[ch - gf->start + 1].offset - gf->letter[ch - gf->start].offset;
        len += l;
        if (len > width) break;

        if (++i >= count) break;
        if ((ch = *str++) && l) len += gf->spacing;
      }
    }
    break;
  }

  return i;
}
///
///GF_Text(rastport, gamefont, str, count)
/******************************************************************************
 * Similar to Text from the API, this function renders the given string onto  *
 * the given destination rastport using the given gamefont. It respects the   *
 * Layer (clipping) and the "DrawMode" of the destination rasport.            *
 * NOTE: For proper rendering the depth of the rasport's bitmap and the depth *
 * of the gamefont should match (as well as their palettes).                  *
 ******************************************************************************/
VOID GF_Text(struct RastPort* rp, struct GameFont* gf, STRPTR str, ULONG count)
{
  ULONG minterm;
  ULONG i;
  ULONG jam2 = rp->DrawMode & JAM2;
  UBYTE ch = *str;
  minterm = 0xC0;

  //if (jam2) minterm = 0xC0;
  //else      minterm = 0x60;

  for (i = 0; i < count; i++) {
    UWORD letter_offset;
    UWORD letter_width;
    UBYTE letter_index;

    if (!ch) break;
    if (ch == ' ') {
      WaitBlit();
      if (jam2)
        ClipBlit(&gf->rastport, 0, 0, rp, rp->cp_x, rp->cp_y, gf->width, gf->height, minterm);
      else {
        BltBitMap(rp->BitMap, rp->cp_x, rp->cp_y, gf->rastport.BitMap, gf->tmp_buf_x, 0, gf->width, gf->height, minterm, 0xFF, NULL);
        BltMaskBitMapRastPort(gf->rastport.BitMap, 0, 0, &gf->rastport, gf->tmp_buf_x, 0, gf->width, gf->height, minterm, gf->mask->Planes[0]);
        ClipBlit(&gf->rastport, gf->tmp_buf_x, 0, rp, rp->cp_x, rp->cp_y, gf->width, gf->height, minterm);
      }

      rp->cp_x += gf->width;
    }
    if (ch < gf->start || ch > gf->end) {
      ch = *(++str);
      continue;
    }
    letter_index = ch - gf->start;

    switch (gf->type) {
      case GF_TYPE_FIXED:
        letter_offset = (letter_index + 1) * gf->width;
        letter_width  = gf->width;
      break;
      case GF_TYPE_PROPORTIONAL:
        letter_offset = gf->letter[letter_index].offset;
        letter_width  = gf->letter[letter_index + 1].offset - letter_offset;
      break;
      default:
        return;
      break;
    }

    WaitBlit();
    if (jam2)
      ClipBlit(&gf->rastport, letter_offset, 0, rp, rp->cp_x, rp->cp_y, letter_width, gf->height, minterm);
    else  {
      BltBitMap(rp->BitMap, rp->cp_x, rp->cp_y, gf->rastport.BitMap, gf->tmp_buf_x, 0, letter_width, gf->height, minterm, 0xFF, NULL);
      BltMaskBitMapRastPort(gf->rastport.BitMap, 0, 0, &gf->rastport, gf->tmp_buf_x, 0, letter_width, gf->height, minterm, gf->mask->Planes[0]);
      ClipBlit(&gf->rastport, gf->tmp_buf_x, 0, rp, rp->cp_x, rp->cp_y, letter_width, gf->height, minterm);
    }

    rp->cp_x += letter_width;

    ch = *(++str);

    if (jam2 && i < count - 1 && ch) {
      WaitBlit();
      ClipBlit(&gf->rastport, 0, 0, rp, rp->cp_x, rp->cp_y, gf->spacing, gf->height, minterm);
    }
    rp->cp_x += gf->spacing;
  }
  rp->cp_y += gf->height + gf->spacing;
}
///
