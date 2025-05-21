/******************************************************************************
 * SpriteBanker                                                               *
 ******************************************************************************/

///defines
#define PROGRAMNAME     "SpriteBanker"
#define VERSION         0
#define REVISION        18
#define VERSIONSTRING   "0.18"

//define command line syntax and number of options
#define RDARGS_TEMPLATE "ILBMFILE/A, SPRITEFILE/A, STRTX/N/A, STRTY/N/A, SEPX/N/A, SEPY/N/A, COLUMNS/N/A, ROWS/N/A, WIDTH/N/A, HEIGHT/N/A, COLORS/N, SFMODE/N, HSN/N, HSX/N, HSY/N, X=REVX/S, Y=REVY/S, C=COLFIRST/S, S=SMALL/S, B=BIG/S"
#define RDARGS_OPTIONS  20

enum {
  ILBMFILE,
  SPRITEFILE,
  STRTX,
  STRTY,
  SEPX,
  SEPY,
  COLUMNS,
  ROWS,
  WIDTH,
  HEIGHT,
  COLORS,
  SFMODE,
  HSN,
  HSX,
  HSY,
  REVX,
  REVY,
  COLFIRST,
  SMALL,
  BIG
};

//#define or #undef GENERATEWBMAIN to enable workbench startup
#define GENERATEWBMAIN
///
///includes
//standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

//Amiga headers
#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <dos/datetime.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/layers.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <datatypes/pictureclass.h>
#include <libraries/asl.h>
#include <libraries/commodities.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/locale.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <utility/hooks.h>

//Amiga protos
#include <clib/alib_protos.h>
#include <proto/asl.h>
#include <proto/commodities.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/locale.h>
#include <proto/rexxsyslib.h>
#include <proto/utility.h>
#include <proto/wb.h>
///
///structs
/***********************************************
* Global configuration struct for this program *
************************************************/
struct Config
{
  struct RDArgs *RDArgs;

  //command line options
  #if RDARGS_OPTIONS
  LONG Options[RDARGS_OPTIONS];
  #endif

  //<YOUR GLOBAL DATA HERE>

};

//<YOUR STRUCTS HERE>
struct Parameters {
  UWORD start_h;
  UWORD start_v;
  UWORD separation_h;
  UWORD separation_v;
  UWORD columns;
  UWORD rows;
  UWORD width;
  UWORD height;
  UWORD num_images;
  UWORD image_size;
  UWORD hard_sprite_size;
  UBYTE hard_sprites_per_image;
  UBYTE num_glued;
  UBYTE colors;
  UBYTE sfmode;
  UBYTE hsn;
  WORD  hotSpot_x;
  WORD  hotSpot_y;
  UBYTE reverse_x;
  UBYTE reverse_y;
  UBYTE columns_first;
  UBYTE small_sizes;
  UBYTE big_sizes;
  UBYTE depth;
  UBYTE type;
  UBYTE bitmap_width;
  UBYTE bitmap_modulo;
};

struct Analyze {
  UWORD v_start;
  UWORD v_end;
  UWORD h_start;
  UWORD h_end;
  UWORD height;
  UWORD width;
  UWORD image_size;
  UWORD hard_sprite_size;
  UWORD hard_sprites_per_image;
  UBYTE num_glued;
  UBYTE type;
  UWORD img_width;
  UWORD img_height;
  WORD  h_offs;
  WORD  v_offs;
  UBYTE empty;
};

struct Table_small {
  UWORD offset;  // Byte offset from WordsOfSpriteData[0] to the the image
  UBYTE type;    // Defines the sprite usage of image
  UBYTE hsn;     // Sprites in this bank will be displayed with (starting from) this hardware sprite
  UBYTE width;
  UBYTE height;
  BYTE  h_offs;  // Defines the distance of the hotspot to image left edge
  BYTE  v_offs;  //    "     "      "    "   "     "    "    "   top edge
};

struct Table_big {
  UWORD offset;  // Byte offset from WordsOfSpriteData[0] to the the image
  UBYTE type;    // Defines the sprite usage of image
  UBYTE hsn;     // Sprites in this bank will be displayed with (starting from) this hardware sprite
  UWORD width;
  UWORD height;
  WORD  h_offs;  // Defines the distance of the hotspot to image left edge
  WORD  v_offs;  //    "     "      "    "   "     "    "    "   top edge
};

struct Table {
  UWORD offset;  // Byte offset from WordsOfSpriteData[0] to the the image
  UBYTE type;    // Defines the sprite usage of image
  UBYTE hsn;     // Sprites in this bank will be displayed with (starting from) this hardware sprite
  UBYTE width;
  UBYTE height;
  WORD  h_offs;  // Defines the distance of the hotspot to image left edge
  WORD  v_offs;  //    "     "      "    "   "     "    "    "   top edge
};
///
///globals
/***********************************************
* Version string for this program              *
************************************************/
#if defined(__SASC)
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " "  __AMIGADATE__ "\n\0";
#elif defined(_DCC)
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __COMMODORE_DATE__ ")\n\0";
#elif defined(__GNUC__)
__attribute__((section(".text"))) volatile static const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";
#else
const UBYTE VersionTag[] = "$VER: " PROGRAMNAME " " VERSIONSTRING " (" __DATE__ ")\n\0";
#endif

ULONG sizeof_struct_Table = NULL;
///
///prototypes
/***********************************************
* Function forward declarations                *
************************************************/
int            main   (int argc, char **argv);
int            wbmain (struct WBStartup *wbs);
struct Config *Init   (void);
int            Main   (struct Config *config);
void           CleanUp(struct Config *config);

struct BitMap* loadILBM(STRPTR fileName);

struct Parameters* checkParameters(struct Config *config);
VOID analyzeFirstPass(struct BitMap* tbm, struct Analyze* analyzed, struct Parameters* params);
VOID analyzeImage(struct BitMap* wbm, struct Analyze* analyzed, struct Parameters* params);
BOOL checkSpriteLine(UBYTE* ptr, struct Parameters* params);
VOID getSprite(struct BitMap* wbm, UBYTE* dest, struct Parameters* params, struct Analyze* analyzed);
VOID clearBitMap(struct BitMap* bm);
//DEBUG
VOID saveWorkBitMap(struct BitMap* wbm, STRPTR origFile);
///
///init
/***********************************************
* Program initialization                       *
* - Allocates the config struct to store the   *
*   global configuration data.                 *
* - Do your other initial allocations here.    *
************************************************/
struct Config *Init()
{
  struct Config *config = (struct Config*)AllocMem(sizeof(struct Config), MEMF_CLEAR);

  if (config)
  {
    //<YOUR INITIALIZATION CODE HERE>
  }

  return(config);
}
///
///entry
/***********************************************
 * Ground level entry point                    *
 * - Branches regarding Shell/WB call.         *
 ***********************************************/
int main(int argc, char **argv)
{
  int rc = 20;

  //argc != 0 identifies call from shell
  if (argc)
  {
    struct Config *config = Init();

    if (config)
    {
      #if RDARGS_OPTIONS
        // parse command line arguments
        if ((config->RDArgs = ReadArgs(RDARGS_TEMPLATE, config->Options, NULL)))
          rc = Main(config);
        else
          PrintFault(IoErr(), PROGRAMNAME);
      #else
        rc = Main(config);
      #endif

      CleanUp(config);
    }
  }
  else
    rc = wbmain((struct WBStartup *)argv);

  return(rc);
}

/***********************************************
 * Workbench main                              *
 * - This executable was called from Workbench *
 ***********************************************/
int wbmain(struct WBStartup *wbs)
{
  int rc = 20;

  #ifdef GENERATEWBMAIN
    struct Config *config = Init();

    if (config)
    {
      //<SET Config->Options[] HERE>

      rc = Main(config);

      CleanUp(config);
    }
  #endif

  return(rc);
}
///
///main
/***********************************************
 * Developer level main                        *
 * - Code your program here.                   *
 ***********************************************/
int Main(struct Config *config)
{
  int rc = 0;
  struct Parameters *params = checkParameters(config);

  if (params) {   //TableSizeW                 Table               ImageDataSizeW
    ULONG table_size = 2 + (params->num_images * sizeof_struct_Table) + 2;
    ULONG bank_size = table_size + params->num_images * params->image_size;
    UBYTE* bank = AllocMem(bank_size, MEMF_ANY | MEMF_CLEAR);

    if (bank) {
      UWORD* num_images = (UWORD*)bank;
      struct Table* table = (struct Table*)(bank + sizeof(UWORD));
      UBYTE* data = bank + table_size;

      struct BitMap* tbm = AllocBitMap(params->bitmap_width, params->height, params->depth, BMF_CLEAR | BMF_INTERLEAVED, NULL);
      if (tbm) {
        struct BitMap* wbm = AllocBitMap(params->bitmap_width, params->height, params->depth, BMF_CLEAR | BMF_INTERLEAVED, NULL);
        if (wbm) {
          struct BitMap* bm = loadILBM((STRPTR)config->Options[ILBMFILE]);
          if (bm) {
            UWORD img = 0;
            ULONG offset_of_next = 0;
            LONG r, c;
            LONG i1, i1s, i1e, i1d;
            LONG i2, i2s, i2e, i2d;
            BPTR fh;

            if (params->columns_first) {
              if (params->reverse_x) {
                i1s = params->columns - 1;
                i1e = -1;
                i1d = -1;
              }
              else {
                i1s = 0;
                i1e = params->columns;
                i1d = 1;
              }

              if (params->reverse_y) {
                i2s = params->rows - 1;
                i2e = -1;
                i2d = -1;
              }
              else {
                i2s = 0;
                i2e = params->rows;
                i2d = 1;
              }
            }
            else {
              if (params->reverse_y) {
                i1s = params->rows - 1;
                i1e = -1;
                i1d = -1;
              }
              else {
                i1s = 0;
                i1e = params->rows;
                i1d = 1;
              }

              if (params->reverse_x) {
                i2s = params->columns - 1;
                i2e = -1;
                i2d = -1;
              }
              else {
                i2s = 0;
                i2e = params->columns;
                i2d = 1;
              }
            }

            for (i1 = i1s; i1 != i1e; i1 += i1d) {
              for (i2 = i2s; i2 != i2e; i2 += i2d) {
                struct Analyze analyzed;

                if (params->columns_first) {
                  c = i1;
                  r = i2;
                }
                else {
                  c = i2;
                  r = i1;
                }

                BltBitMap(bm,  params->start_h + c * (params->width + params->separation_h),
                               params->start_v + r * (params->height + params->separation_v),
                          tbm, 0, 0, params->width, params->height, 0xC0, 0xFF, NULL);

                analyzeFirstPass(tbm, &analyzed, params);
                if (analyzed.empty) continue;

                clearBitMap(wbm);

                BltBitMap(tbm, analyzed.h_start, analyzed.v_start,
                          wbm, 0, 0, analyzed.img_width, analyzed.img_height, 0xC0, 0xFF, NULL);

                analyzeImage(wbm, &analyzed, params);
                if (analyzed.num_glued) {
                  getSprite(wbm, data + offset_of_next, params, &analyzed);
                  //add fetch mode onto type
                  switch (params->sfmode) {
                    case 2:
                      analyzed.type |= 0x20;
                    break;
                    case 4:
                      analyzed.type |= 0x40;
                    break;
                  }

                  //set table entry
                  if (params->big_sizes) {
                    struct Table_big* table_b = (struct Table_big*) table;

                    table_b[img].offset = offset_of_next;
                    table_b[img].type   = analyzed.type;
                    table_b[img].hsn    = params->hsn;
                    table_b[img].width  = analyzed.img_width;
                    table_b[img].height = analyzed.img_height;
                    table_b[img].h_offs = analyzed.h_offs - params->hotSpot_x;
                    table_b[img].v_offs = analyzed.v_offs - params->hotSpot_y;
                  }
                  else if (params->small_sizes) {
                    struct Table_small* table_s = (struct Table_small*) table;

                    table_s[img].offset = offset_of_next;
                    table_s[img].type   = analyzed.type;
                    table_s[img].hsn    = params->hsn;
                    table_s[img].width  = analyzed.img_width;
                    table_s[img].height = analyzed.img_height;
                    table_s[img].h_offs = analyzed.h_offs - params->hotSpot_x;
                    table_s[img].v_offs = analyzed.v_offs - params->hotSpot_y;
                  }
                  else {
                    table[img].offset = offset_of_next;
                    table[img].type   = analyzed.type;
                    table[img].hsn    = params->hsn;
                    table[img].width  = analyzed.img_width;
                    table[img].height = analyzed.img_height;
                    table[img].h_offs = analyzed.h_offs - params->hotSpot_x;
                    table[img].v_offs = analyzed.v_offs - params->hotSpot_y;
                  }

                  img++;
                  offset_of_next += analyzed.image_size;
                  if (offset_of_next > 0xFFFF) {
                    img--;
                    offset_of_next -= analyzed.image_size;
                    puts("Too many sprites! Some sprites could not be stored in the bank!");
                    goto bailout;
                  }
                }
              }
            }
bailout:
            if (img) {
              //finalize the bank
              *num_images = img;

              //write the file
              fh = Open((STRPTR)config->Options[SPRITEFILE], MODE_READWRITE);
              if (fh) {
                UBYTE size_type = params->big_sizes ? 0x04 : (params->small_sizes ? 0x02 : 0x00);
                Write(fh, "SPRBANK", 7);
                Write(fh, &size_type, 1);
                Write(fh, bank, 2 + (img * sizeof_struct_Table));
                Write(fh, ((UBYTE*)(&offset_of_next)) + 2, 2);
                Write(fh, data, offset_of_next);
                SetFileSize(fh, 0, OFFSET_CURRENT);
                Close(fh);
                printf("%u Sprites has been exported!\n", img);
              }
              else puts("Could not open file for write!");
            }
            else puts("No image data could be found on the ilbm file!");

            FreeBitMap(bm);
          }
          else puts("ILBM file could not be loaded!");

          FreeBitMap(wbm);
        }
        else puts("Not enough memory!");

        FreeBitMap(tbm);
      }
      else puts("Not enough memory!");

      FreeMem(bank, bank_size);
    }
    else puts("Not enough memory!");
  }

  return(rc);
}
///
///cleanup
/***********************************************
 * Clean up before exit                        *
 * - Free allocated resources here.            *
 ***********************************************/
void CleanUp(struct Config *config)
{
  if (config)
  {
    //<YOUR CLEAN UP CODE HERE>

    // free command line arguments
    #if RDARGS_OPTIONS
      if (config->RDArgs)
        FreeArgs(config->RDArgs);
    #endif

    FreeMem(config, sizeof(struct Config));
  }
}
///

///checkParameters(config)
struct Parameters* checkParameters(struct Config *config) {
  static struct Parameters params;
  params.start_h = *(ULONG*)config->Options[STRTX];
  params.start_v = *(ULONG*)config->Options[STRTY];
  params.separation_h = *(ULONG*)config->Options[SEPX];
  params.separation_v = *(ULONG*)config->Options[SEPY];
  params.columns = *(ULONG*)config->Options[COLUMNS];
  params.rows    = *(ULONG*)config->Options[ROWS];
  params.width   = *(ULONG*)config->Options[WIDTH];
  params.height  = *(ULONG*)config->Options[HEIGHT];
  params.colors  = config->Options[COLORS] ? *(ULONG*)config->Options[COLORS] : 4;
  params.sfmode  = config->Options[SFMODE] ? *(ULONG*)config->Options[SFMODE] : 1;
  params.hsn     = config->Options[HSN] ? *(LONG*)config->Options[HSN] : 0x09; // HSN_NOT_SET
  params.hotSpot_x = config->Options[HSX] ? *(LONG*)config->Options[HSX] : 0;
  params.hotSpot_y = config->Options[HSY] ? *(LONG*)config->Options[HSY] : 0;
  params.reverse_x = config->Options[REVX] ? TRUE : FALSE;
  params.reverse_y = config->Options[REVY] ? TRUE : FALSE;
  params.columns_first = config->Options[COLFIRST] ? TRUE : FALSE;
  params.small_sizes = config->Options[SMALL] ? TRUE : FALSE;
  params.big_sizes = config->Options[BIG] ? TRUE : FALSE;

  if (!params.columns) {
    puts("Columns cannot be zero!");
    return NULL;
  }
  if (!params.rows) {
    puts("Rows cannot be zero!");
    return NULL;
  }
  if (!params.width) {
    puts("Width can not be zero!");
    return NULL;
  }
  if (!params.height) {
    puts("Height can not be zero!");
    return NULL;
  }
  if (params.sfmode != 1 && params.sfmode != 2 && params.sfmode != 4) {
    puts("Invalid SFMODE!");
    return NULL;
  }
  if (!params.colors) params.colors = 4;
  if (params.colors != 4 && params.colors != 16) {
    puts("Invalid COLORS!");
    return NULL;
  }
  params.depth = params.colors == 4 ? 2 : 4;
  if (params.width > ((128 / (params.depth / 2)) * params.sfmode)) {
    puts("width too big for this SFMODE and/or COLORS!");
    return NULL;
  }
  if (config->Options[HSN]) {
    if (*(LONG*)config->Options[HSN] == -1) {
      params.hsn = 0x09; //HSN_NOT_SET
    }
    else if (params.hsn > 7) {
      puts("A valid hardware sprite number (HSN) is 0-7.");
      return NULL;
    }
  }

  params.bitmap_width = (((params.width - 1) / (params.sfmode * 16)) + 1) * (params.sfmode * 16);
  params.bitmap_modulo = params.bitmap_width / 8;

  params.type = params.bitmap_width / (params.sfmode * 16) * (params.depth / 2);
  if (params.depth == 4) params.type |= 0x10;

                          //ctl_w   color words    term_w
  params.hard_sprite_size = (2 + 2 * params.height + 2) * sizeof(WORD) * params.sfmode;

  params.num_images = params.columns * params.rows;

  params.num_glued = params.bitmap_width / (params.sfmode * 16);

  params.hard_sprites_per_image = params.bitmap_width / (params.sfmode * 16) * (params.depth / 2);

  params.image_size = params.hard_sprite_size * params.hard_sprites_per_image;

  if (params.big_sizes) {
    params.small_sizes = FALSE;
    sizeof_struct_Table = sizeof(struct Table_big);
  }
  else {
    if (params.small_sizes)
      sizeof_struct_Table = sizeof(struct Table_small);
    else
      sizeof_struct_Table = sizeof(struct Table);
  }

  return &params;
}
///
///checkSpriteLine(ptr, params)
BOOL checkSpriteLine(UBYTE* ptr, struct Parameters* params)
{
  LONG d;
  ULONG test = 0;

  for (d = 0; d < params->depth; d++) {
    switch (params->sfmode) {
      case 1:
        test = *((UWORD*)ptr);
      break;
      case 2:
        test = *((ULONG*)ptr);
      break;
      case 4:
        test = *((ULONG*)ptr);
        test |= *((ULONG*)(ptr + 4));
      break;
    }

    if (test) return TRUE;

    ptr += params->bitmap_modulo; //WARNING: This assumes work BitMap is interleaved
  }

  return FALSE;
}
///
///analyzeFirstPass(tempBitMap, analyzed, params)
VOID analyzeFirstPass(struct BitMap* tbm, struct Analyze* analyzed, struct Parameters* params)
{
  LONG i, d, w, b = 0;
  BOOL empty = TRUE;
  UWORD wppl = params->bitmap_width / 16; // words per plane line
  UWORD* test;
  UWORD mask;

  analyzed->empty = FALSE;

  // Analyze top space
  for (i = 0; i < params->height; i++) {
    for (d = 0; d < tbm->Depth; d++) {
      for (w = 0; w < wppl; w++) {
        test = (UWORD*)(tbm->Planes[d] + w * 2 + i * tbm->BytesPerRow);
        if (*test) {
          empty = FALSE;
          break;
        }
      }
      if (!empty) break;
    }
    if (!empty) break;
  }
  if (empty) {
    analyzed->empty = TRUE;
    return;
  }
  analyzed->v_start = i;

  // Analyze bottom space
  empty = TRUE;
  for (i = params->height - 1; i > -1; i--) {
    for (d = 0; d < tbm->Depth; d++) {
      for (w = 0; w < wppl; w++) {
        test = (UWORD*)(tbm->Planes[d] + w * 2 + i * tbm->BytesPerRow);
        if (*test) {
          empty = FALSE;
          break;
        }
      }
      if (!empty) break;
    }
    if (!empty) break;
  }
  analyzed->v_end = i;

  // Analyze left space
  empty = TRUE;
  for (w = 0; w < wppl; w++) {
    for (b = 0, mask = 1 << 15; b < 16; b++, mask >>= 1) {
      for (i = analyzed->v_start; i <= analyzed->v_end; i++) {
        for (d = 0; d < tbm->Depth; d++) {
          test = (UWORD*)(tbm->Planes[d] + w * 2 + i * tbm->BytesPerRow);
          if (*test & mask) {
            empty = FALSE;
            break;
          }
        }
        if (!empty) break;
      }
      if (!empty) break;
    }
    if (!empty) break;
  }
  analyzed->h_start = w * 16 + b;

  // Analyze right space
  empty = TRUE;
  for (w = wppl - 1; w > -1; w--) {
    for (b = 15, mask = 1; b > -1; b--, mask <<= 1) {
      for (i = analyzed->v_start; i <= analyzed->v_end; i++) {
        for (d = 0; d < tbm->Depth; d++) {
          test = (UWORD*)(tbm->Planes[d] + w * 2 + i * tbm->BytesPerRow);
          if (*test & mask) {
            empty = FALSE;
            break;
          }
        }
        if (!empty) break;
      }
      if (!empty) break;
    }
    if (!empty) break;
  }
  analyzed->h_end = w * 16 + b;

  analyzed->img_height = analyzed->v_end - analyzed->v_start + 1;
  analyzed->img_width  = analyzed->h_end - analyzed->h_start + 1;
  analyzed->h_offs = analyzed->h_start;
  analyzed->v_offs = analyzed->v_start;
}
///
///analyzeImage(workBitMap, analyzed, params)
VOID analyzeImage(struct BitMap* wbm, struct Analyze* analyzed, struct Parameters* params)
{
  LONG g, l;
  BOOL empty;

  analyzed->v_start = 0;
  analyzed->v_end = params->height - 1;
  analyzed->height = params->height;
  analyzed->width = params->width;
  analyzed->image_size = params->image_size;
  analyzed->hard_sprites_per_image = params->hard_sprites_per_image;
  analyzed->num_glued = params->num_glued;
  analyzed->type = params->type;

  //Test right glued sprites for being empty
  for (g = params->num_glued - 1; g > -1; g--) {
    empty = TRUE;
    for (l = 0; l < params->height; l++) {
      if (checkSpriteLine(wbm->Planes[0] + (l * wbm->BytesPerRow) + (g * params->sfmode * sizeof(WORD)), params)) {
        empty = FALSE;
        break;
      }
    }
    if (empty == TRUE) {
      analyzed->width -= (params->sfmode * 16);
      analyzed->num_glued--;
    }
  }

  if (analyzed->num_glued == 0) return;

  //recalculate some values:
  analyzed->hard_sprites_per_image = analyzed->num_glued * (params->depth / 2);

  //Test top lines for being empty
  for (l = 0; l < params->height; l++) {
    BOOL test = FALSE;
    for (g = 0; g < analyzed->num_glued; g++) {
      test |= checkSpriteLine(wbm->Planes[0] + (l * wbm->BytesPerRow) + (g * params->sfmode * sizeof(WORD)), params);
    }
    if (test) {
      analyzed->v_start = l;
      break;
    }
  }

  //Test bottom lines for being empty
  for (l = params->height - 1; l > -1; l--) {
    BOOL test = FALSE;
    for (g = 0; g < analyzed->num_glued; g++) {
      test |= checkSpriteLine(wbm->Planes[0] + (l * wbm->BytesPerRow) + (g * params->sfmode * sizeof(WORD)), params);
    }
    if (test) {
      analyzed->v_end = l;
      break;
    }
  }

  //recalculate some values
  analyzed->height = analyzed->v_end - analyzed->v_start + 1;
  analyzed->hard_sprite_size = (2 + 2 * analyzed->height + 2) * sizeof(WORD) * params->sfmode;
  analyzed->image_size = analyzed->hard_sprites_per_image * analyzed->hard_sprite_size;
  analyzed->type = analyzed->num_glued * (params->depth / 2);
  if (params->depth == 4) analyzed->type |= 0x10;
}
///
///getSpritePlaneLine(dest, src, analyzed)
VOID getSpritePlaneLine(UBYTE* dest, UBYTE* src, UBYTE sfmode)
{
  switch (sfmode) {
    case 1:
      *(UWORD*)dest = *(UWORD*)src;
    break;
    case 2:
      *(ULONG*)dest = *(ULONG*)src;
    break;
    case 4:
      *(ULONG*)dest = *(ULONG*)src;
      *(ULONG*)(dest + sizeof(LONG)) = *(ULONG*)(src + sizeof(LONG));
    break;
  }
}
///
///getSprite(workBitMap, dest, params, analyzed)
VOID getSprite(struct BitMap* wbm, UBYTE* dest, struct Parameters* params, struct Analyze* analyzed)
{
  LONG g, lr, lw;
  UBYTE* wa = dest + (2 * sizeof(WORD) * params->sfmode);
  UBYTE* ra = wbm->Planes[0];

  for (g = 0; g < analyzed->num_glued; g++) {
    for (lr = analyzed->v_start, lw = 0; lr <= analyzed->v_end; lr++, lw++) {
      getSpritePlaneLine(wa + (lw * 2 * sizeof(WORD) * params->sfmode), ra + (lr * wbm->BytesPerRow) + (g * params->sfmode * sizeof(WORD)), params->sfmode);
      getSpritePlaneLine(wa + ((lw * 2 + 1) * sizeof(WORD) * params->sfmode), ra + (lr * wbm->BytesPerRow) + params->bitmap_modulo + (g * params->sfmode * sizeof(WORD)), params->sfmode);
      if (params->depth == 4) {
        getSpritePlaneLine(wa + analyzed->hard_sprite_size + (lw * 2 * sizeof(WORD) * params->sfmode), ra + (lr * wbm->BytesPerRow) + (2 * params->bitmap_modulo) + (g * params->sfmode * sizeof(WORD)), params->sfmode);
        getSpritePlaneLine(wa + analyzed->hard_sprite_size + ((lw * 2 + 1) * sizeof(WORD) * params->sfmode), ra + (lr * wbm->BytesPerRow) + (3 * params->bitmap_modulo) + (g * params->sfmode * sizeof(WORD)), params->sfmode);
      }
    }
    wa += analyzed->hard_sprite_size * (params->depth / 2);
  }
}
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
///loadILBM(fileName)
struct BitMap* loadILBM(STRPTR fileName)
{
  struct BitMapHeader bmhd;
  struct BitMap* bm = NULL;
  BPTR fh;

  fh = Open(fileName, MODE_OLDFILE);
  if (fh) {
    if (locateIFFChunkInFile(fh, "FORM")) {
      if (locateIFFChunkInFile(fh, "ILBM")) {
        if (locateIFFChunkInFile(fh, "BMHD")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct BitMapHeader) == Read(fh, &bmhd, sizeof(struct BitMapHeader))) {
            if (locateIFFChunkInFile(fh, "BODY")) {
              Seek(fh, 4, OFFSET_CURRENT);
              bm = AllocBitMap(bmhd.bmh_Width, bmhd.bmh_Height, bmhd.bmh_Depth, BMF_INTERLEAVED, NULL);
              if (bm && (GetBitMapAttr(bm, BMA_FLAGS) & BMF_INTERLEAVED)) {
                if (bmhd.bmh_Compression) { // NOTE: consider other possible compression methods
                  UBYTE *w = (UBYTE*)bm->Planes[0]; // write cursor
                  ULONG bpr = bmhd.bmh_Width / 8;   // bytes per row
                  ULONG row;
                  ULONG plane;
                  UBYTE b = 0;                      // read byte
                  UBYTE b2 = 0;

                  for (row = 0; row < bmhd.bmh_Height; row++) {
                    for (plane = 0; plane < bmhd.bmh_Depth; plane++) {
                      ULONG bytesWritten = 0;
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
                  ULONG bpr = bmhd.bmh_Width / 8;   // bytes per row
                  ULONG row;
                  UBYTE *w = (UBYTE*)bm->Planes[0]; // write cursor
                  for (row = 0; row < bmhd.bmh_Height; row++) {
                    Read(fh, w, bmhd.bmh_Depth * bpr);
                    w += (bmhd.bmh_Depth * bpr);
                    if (bmhd.bmh_Masking == 1) {
                      Seek(fh, bpr, OFFSET_CURRENT);
                    }
                  }
                }
              }
              else if (bm){
                FreeBitMap(bm);
                bm = NULL;
              }
            }

          }
        }
      }
    }
    Close(fh);
  }

  return bm;
}
///
///allocBitMapFast()
struct BitMap* allocBitMapFast(ULONG sizex, ULONG sizey, ULONG depth, ULONG flags, CONST struct BitMap *friend)
{
  struct BitMap* bm = NULL;
  ULONG allocSize = sizeof(struct BitMap) - (flags & BMF_MINPLANES ? (8 - depth) * sizeof(PLANEPTR) : 0);

  if (sizex && sizey && depth) {
    bm = AllocMem(allocSize, MEMF_ANY);

    if (bm) {
      UWORD bytesPerPlaneLine = ((sizex - 1) / 8) + 1;
      if (flags & BMF_INTERLEAVED) {
        PLANEPTR raster = AllocMem(bytesPerPlaneLine * sizey * depth, MEMF_ANY | (flags & BMF_CLEAR ? MEMF_CLEAR : 0));
        if (raster) {
          ULONG d;
          for (d = 0; d < depth; d++) {
            bm->Planes[d] = raster + d * bytesPerPlaneLine;
          }
        }
        else {
          flags &= ~BMF_INTERLEAVED;
          goto fallback;
        }
      }
      else {
        ULONG d;
fallback:
        for (d = 0; d < depth; d++) {
          bm->Planes[d] = AllocMem(bytesPerPlaneLine * sizey, MEMF_ANY | (flags & BMF_CLEAR ? MEMF_CLEAR : 0));
          if (!bm->Planes[d]) {
            while (d) {
              d--;
              FreeMem(bm->Planes[d], bytesPerPlaneLine * sizey);
            }
            FreeMem(bm, allocSize);
            return NULL;
          }
        }
      }

      bm->BytesPerRow = flags & BMF_INTERLEAVED ? bytesPerPlaneLine * depth : bytesPerPlaneLine;
      bm->Rows = sizey;
      bm->Flags = flags & ~BMF_DISPLAYABLE;
      bm->Depth = depth;
    }
  }

  return bm;
}
///
///freeBitMapFast(bitmap)
VOID freeBitMapFast(struct BitMap* bm)
{
  if (bm) {
    if (bm->Flags & BMF_INTERLEAVED) {
      FreeMem(bm->Planes[0], bm->BytesPerRow * bm->Rows);
    }
    else {
      ULONG d;
      for (d = 0; d < bm->Depth; d++) {
        FreeMem(bm->Planes[d], bm->BytesPerRow * bm->Rows);
      }
    }

    FreeMem(bm, sizeof(struct BitMap) - (bm->Flags & BMF_MINPLANES ? (8 - bm->Depth) * sizeof(PLANEPTR) : 0));
  }
}
///
///clearBitMap(bitmap)
VOID clearBitMap(struct BitMap* bm)
{
  if (GetBitMapAttr(bm, BMA_FLAGS) & BMF_INTERLEAVED) {
    if (TypeOfMem(bm->Planes[0]) & MEMF_CHIP) {
      BltClear(bm->Planes[0], bm->BytesPerRow * bm->Rows, 1);
    }
    else {
      memset(bm->Planes[0], 0, bm->BytesPerRow * bm->Rows);
    }
  }
  else {
    ULONG d;

    if (TypeOfMem(bm->Planes[0]) & MEMF_CHIP) {
      for (d = 0; d < bm->Depth; d++) {
        BltClear(bm->Planes[d], bm->BytesPerRow * bm->Rows, 1);
      }
    }
    else {
      for (d = 0; d < bm->Depth; d++) {
        memset(bm->Planes[d], 0, bm->BytesPerRow * bm->Rows);
      }
    }
  }
}
///

///DEBUG saveWorkBitMap(bitmap)
VOID saveWorkBitMap(struct BitMap* wbm, STRPTR origFile) {
  struct BitMapHeader bmhd;
  LONG cmapSize = 0;
  LONG formSize = 0;
  LONG bodySize = wbm->BytesPerRow * wbm->Rows;
  LONG bmhdSize = sizeof(struct BitMapHeader);
  UBYTE* cmap = NULL;
  BPTR fh = Open(origFile, MODE_OLDFILE);
  if (fh) {
    if (locateIFFChunkInFile(fh, "CMAP")) {
      Read(fh, &cmapSize, 4);
      cmap = AllocMem(cmapSize, MEMF_ANY);
      if (cmap) {
        Read(fh, cmap, cmapSize);
      }
    }
    Close(fh);
  }

  bmhd.bmh_Width = wbm->BytesPerRow * 8 / wbm->Depth; //WARNING: This assumes the wbm to be BMF_INTERLEAVED
  bmhd.bmh_Height = wbm->Rows;
  bmhd.bmh_Left = 0;
  bmhd.bmh_Top = 0;
  bmhd.bmh_Depth = wbm->Depth;
  bmhd.bmh_Masking = 0;
  bmhd.bmh_Compression = 0;
  bmhd.bmh_Pad = 0;
  bmhd.bmh_Transparent = 0;
  bmhd.bmh_XAspect = 0;
  bmhd.bmh_YAspect = 0;
  bmhd.bmh_PageWidth = 0;
  bmhd.bmh_PageHeight = 0;

  formSize = strlen("ILBM") + strlen("BMHD") + 4 + bmhdSize + strlen("CMAP") + 4 + cmapSize + strlen("BODY") + 4 + bodySize;

  fh = Open("RAM:wbm.iff", MODE_NEWFILE);
  if (fh) {
    Write(fh, "FORM", 4);
    Write(fh, &formSize, 4);
    Write(fh, "ILBMBMHD", 8);
    Write(fh, &bmhdSize, 4);
    Write(fh, &bmhd, bmhdSize);
    Write(fh, "CMAP", 4);
    Write(fh, &cmapSize, 4);
    Write(fh, cmap, cmapSize);
    Write(fh, "BODY", 4);
    Write(fh, &bodySize, 4);
    Write(fh, wbm->Planes[0], bodySize); //WARNING: This assumes the wbm to be BMF_INTERLEAVED
    Close(fh);
  }

  if (cmap) {
    FreeMem(cmap, cmapSize);
  }
}
///

/* DEBUG
printf("start_h : %u\n"
       "start_v : %u\n"
       "separation_h : %u\n"
       "separation_v : %u\n"
       "columns : %u\n"
       "rows : %u\n"
       "width : %u\n"
       "height : %u\n"
       "num_images : %u\n"
       "image_size : %u\n"
       "hard_sprite_size : %u\n"
       "colors : %u\n"
       "sfmode : %u\n"
       "voffs : %u\n"
       "reverse : %u\n"
       "depth : %u\n"
       "type: %u\n"
       "bitmap_width: %u\n"
       "bitmap_modulo: %u\n",
       params->start_h,
       params->start_v,
       params->separation_h,
       params->separation_v,
       params->columns,
       params->rows,
       params->width,
       params->height,
       params->num_images,
       params->image_size,
       params->hard_sprite_size,
       (UWORD)params->colors,
       (UWORD)params->sfmode,
       (UWORD)params->voffs,
       (UWORD)params->reverse,
       (UWORD)params->depth,
       (UWORD)params->type,
       (UWORD)params->bitmap_width,
       (UWORD)params->bitmap_modulo);
*/
