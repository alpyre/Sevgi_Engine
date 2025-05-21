/******************************************************************************
 * BOBSheeter                                                                 *
 * NOTE: This tool only creates BST_IRREGULAR type bob sheets.                *
 ******************************************************************************/

///defines
#define PROGRAMNAME     "BOBSheeter"
#define VERSION         0
#define REVISION        12
#define VERSIONSTRING   "0.12"

//define command line syntax and number of options
#define RDARGS_TEMPLATE "ILBMFILE/A, SHEETFILE/A, STRTX/N/A, STRTY/N/A, SEPX/N/A, SEPY/N/A, COLUMNS/N/A, ROWS/N/A, WIDTH/N/A, HEIGHT/N/A, HSX/N, HSY/N, X=REVX/S, Y=REVY/S, C=COLFIRST/S, D=NOCOMP/S, S=SMALL/S, B=BIG/S"
#define RDARGS_OPTIONS  18

enum {
  ILBMFILE,
  SHEETFILE,
  STRTX,
  STRTY,
  SEPX,
  SEPY,
  COLUMNS,
  ROWS,
  WIDTH,
  HEIGHT,
  HSX,
  HSY,
  REVX,
  REVY,
  COLFIRST,
  NOCOMP,
  SMALL,
  BIG
};

//#define or #undef GENERATEWBMAIN to enable workbench startup
//#define GENERATEWBMAIN
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
  UBYTE hotSpot_x;
  UBYTE hotSpot_y;
  UWORD num_images;
  UWORD work_bitmap_width;
  BOOL  no_compression;
  BOOL  reverse_x;
  BOOL  reverse_y;
  BOOL  columns_first;
  BOOL  small_sizes;
  BOOL  big_sizes;
};

struct Analyze {
  UWORD v_start;
  UWORD v_end;
  UWORD h_start;
  UWORD h_end;
  UWORD height;
  UWORD width;
  UWORD bytes;
  UWORD empty;
};

struct __attribute__((packed)) Table_big {
  UWORD width;
  UWORD height;
  WORD h_offs;
  WORD v_offs;
  UBYTE word;  // x coord of image in bob sheet / 16 (NOTE: Bob images MUST be in a WORD boundary)
  UWORD row;   // y coord of image in bob sheet.
};

struct __attribute__((packed)) Table_small {
  UBYTE width;
  UBYTE height;
  BYTE h_offs;
  BYTE v_offs;
  UBYTE word;  // x coord of image in bob sheet / 16 (NOTE: Bob images MUST be in a WORD boundary)
  UWORD row;   // y coord of image in bob sheet.
};

struct __attribute__((packed)) Table {
  UBYTE width;
  UBYTE height;
  WORD h_offs;
  WORD v_offs;
  UBYTE word;  // x coord of image in bob sheet / 16 (NOTE: Bob images MUST be in a WORD boundary)
  UWORD row;   // y coord of image in bob sheet.
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

ULONG sizeof_struct_Table;
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

VOID freeString(STRPTR str);
STRPTR changeExtension(STRPTR fileName, STRPTR ext);
VOID saveSheetFile(UWORD num_images, struct Table* table, STRPTR saveFile, struct Parameters *params);
VOID saveWorkBitMap(struct BitMap* wbm, UWORD max_bytes, UWORD rows, STRPTR saveFile, STRPTR origFile, BOOL no_compression);
VOID analyzeImage(struct BitMap* tbm, struct Analyze* analyzed, struct Parameters* params);
struct Parameters* checkParameters(struct Config *config);
struct BitMap* loadILBM(STRPTR fileName);
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

  if (params) {
    struct Table* table = AllocMem(sizeof_struct_Table * params->num_images, MEMF_ANY | MEMF_CLEAR);
    if (table) {
      struct BitMap* bm = loadILBM((STRPTR)config->Options[ILBMFILE]);
      if (bm) {
        struct BitMap* wbm = AllocBitMap(bm->BytesPerRow / bm->Depth * 8, bm->Rows, bm->Depth, BMF_INTERLEAVED | BMF_CLEAR, NULL);
        if (wbm) {
          struct BitMap* tbm = AllocBitMap(params->width, params->height, bm->Depth, BMF_INTERLEAVED | BMF_CLEAR, NULL);
          if (tbm) {
            ULONG img = 0;
            LONG r, c;
            LONG cs, ce, cd;
            LONG rs, re, rd;

            UBYTE byte; // byte offset of current image on wbm (x coord / 8)
            UWORD row;  // row of current image on wbm (y coord)
            UWORD max_rows;  // maximum column height in rows
            UWORD max_bytes;  // maximum row width in bytes

            struct Analyze analyzed;

            if (params->reverse_x) {
              cs = params->columns - 1;
              ce = -1;
              cd = -1;
            }
            else {
              cs = 0;
              ce = params->columns;
              cd = 1;
            }

            if (params->reverse_y) {
              rs = params->rows - 1;
              re = -1;
              rd = -1;
            }
            else {
              rs = 0;
              re = params->rows;
              rd = 1;
            }

            if (params->columns_first) {
              UWORD col_width; // maximum image width on the current column (WARNING: in bytes)

              byte = 0;
              max_rows = 0;
              for (c = cs; c != ce; c += cd) {
                row = 0;
                col_width = 0;

                for (r = rs; r != re; r += rd) {
                  BltBitMap(bm,  params->start_h + c * (params->width + params->separation_h),
                                 params->start_v + r * (params->height + params->separation_v),
                            tbm, 0, 0, params->width, params->height, 0xC0, 0xFF, NULL);

                  analyzeImage(tbm, &analyzed, params);
                  if (analyzed.empty) {continue;}

                  // Set image properties to image table
                  if (params->big_sizes) {
                    struct Table_big* table_big = (struct Table_big*)table;
                    table_big[img].width  = analyzed.width;
                    table_big[img].height = analyzed.height;
                    table_big[img].h_offs = analyzed.h_start - params->hotSpot_x;
                    table_big[img].v_offs = analyzed.v_start - params->hotSpot_y;
                    table_big[img].word   = byte / 2;
                    table_big[img].row    = row;
                  }
                  else if (params->small_sizes) {
                    struct Table_small* table_sml = (struct Table_small*)table;
                    table_sml[img].width  = analyzed.width;
                    table_sml[img].height = analyzed.height;
                    table_sml[img].h_offs = analyzed.h_start - params->hotSpot_x;
                    table_sml[img].v_offs = analyzed.v_start - params->hotSpot_y;
                    table_sml[img].word   = byte / 2;
                    table_sml[img].row    = row;
                  }
                  else {
                    table[img].width  = analyzed.width;
                    table[img].height = analyzed.height;
                    table[img].h_offs = analyzed.h_start - params->hotSpot_x;
                    table[img].v_offs = analyzed.v_start - params->hotSpot_y;
                    table[img].word   = byte / 2;
                    table[img].row    = row;
                  }

                  // Copy the analyzed image to work bitmap (top left justified)
                  BltBitMap(tbm, analyzed.h_start, analyzed.v_start, wbm, byte * 8, row, analyzed.width, analyzed.height, 0xC0, 0xFF, NULL);

                  if (analyzed.bytes > col_width) col_width = analyzed.bytes;
                  row += analyzed.height;

                  img++;
                }
                byte += col_width;
                if (row > max_rows) max_rows = row;
              }
              max_bytes = byte;
            }
            else {
              UWORD row_height; // maximum image height of the current row

              row = 0;
              max_bytes = 0;
              for (r = rs; r != re; r += rd) {
                byte = 0;
                row_height = 0;

                for (c = cs; c != ce; c += cd) {
                  BltBitMap(bm,  params->start_h + c * (params->width + params->separation_h),
                                 params->start_v + r * (params->height + params->separation_v),
                            tbm, 0, 0, params->width, params->height, 0xC0, 0xFF, NULL);

                  analyzeImage(tbm, &analyzed, params);
                  if (analyzed.empty) {continue;}

                  // Set image properties to image table
                  // Set image properties to image table
                  if (params->big_sizes) {
                    struct Table_big* table_big = (struct Table_big*)table;
                    table_big[img].width  = analyzed.width;
                    table_big[img].height = analyzed.height;
                    table_big[img].h_offs = analyzed.h_start - params->hotSpot_x;
                    table_big[img].v_offs = analyzed.v_start - params->hotSpot_y;
                    table_big[img].word   = byte / 2;
                    table_big[img].row    = row;
                  }
                  else if (params->small_sizes) {
                    struct Table_small* table_sml = (struct Table_small*)table;
                    table_sml[img].width  = analyzed.width;
                    table_sml[img].height = analyzed.height;
                    table_sml[img].h_offs = analyzed.h_start - params->hotSpot_x;
                    table_sml[img].v_offs = analyzed.v_start - params->hotSpot_y;
                    table_sml[img].word   = byte / 2;
                    table_sml[img].row    = row;
                  }
                  else {
                    table[img].width  = analyzed.width;
                    table[img].height = analyzed.height;
                    table[img].h_offs = analyzed.h_start - params->hotSpot_x;
                    table[img].v_offs = analyzed.v_start - params->hotSpot_y;
                    table[img].word   = byte / 2;
                    table[img].row    = row;
                  }

                  // Copy the analyzed image to work bitmap (top left justified)
                  BltBitMap(tbm, analyzed.h_start, analyzed.v_start, wbm, byte * 8, row, analyzed.width, analyzed.height, 0xC0, 0xFF, NULL);

                  if (analyzed.height > row_height) row_height = analyzed.height;
                  byte += analyzed.bytes;
                  img++;
                }
                row += row_height;
                if (byte > max_bytes) max_bytes = byte;
              }
              max_rows = row;
            }

            if (img) {
              // Save the work bitmap
              saveWorkBitMap(wbm, max_bytes, max_rows, (STRPTR)config->Options[SHEETFILE], (STRPTR)config->Options[ILBMFILE], params->no_compression);
              // Save the sheet file
              saveSheetFile(img, table, (STRPTR)config->Options[SHEETFILE], params);
            }
            FreeBitMap(tbm);
          }
          FreeBitMap(wbm);
        }
        FreeBitMap(bm);
      }
      else printf("Cannot open ILBM file: %s\n", (STRPTR)config->Options[ILBMFILE]);

      FreeMem(table, sizeof_struct_Table * params->num_images);
    }
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
struct Parameters* checkParameters(struct Config *config)
{
  static struct Parameters params;
  params.start_h = *(ULONG*)config->Options[STRTX];
  params.start_v = *(ULONG*)config->Options[STRTY];
  params.separation_h = *(ULONG*)config->Options[SEPX];
  params.separation_v = *(ULONG*)config->Options[SEPY];
  params.columns   = *(ULONG*)config->Options[COLUMNS];
  params.rows      = *(ULONG*)config->Options[ROWS];
  params.width     = *(ULONG*)config->Options[WIDTH];
  params.height    = *(ULONG*)config->Options[HEIGHT];
  params.hotSpot_x = config->Options[HSX] ? *(ULONG*)config->Options[HSX] : 0;
  params.hotSpot_y = config->Options[HSY] ? *(ULONG*)config->Options[HSY] : 0;
  params.reverse_x = config->Options[REVX] ? TRUE : FALSE;
  params.reverse_y = config->Options[REVY] ? TRUE : FALSE;
  params.columns_first = config->Options[COLFIRST] ? TRUE : FALSE;
  params.no_compression = config->Options[NOCOMP] ? TRUE : FALSE;
  params.small_sizes = config->Options[SMALL] ? TRUE : FALSE;

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

  params.work_bitmap_width  = (((params.width - 1) / 16) + 1) * 16;

  params.num_images = params.columns * params.rows;

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
              if (bm && (GetBitMapAttr(bm, BMA_FLAGS) & BMF_INTERLEAVED)) { // WARNING! Depth:1 BitMaps are non-interleaved by default
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
                printf("Could not allocate an interleaved bitmap\n");
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

///analyzeImage(tempBitMap, analyzed, params)
VOID analyzeImage(struct BitMap* tbm, struct Analyze* analyzed, struct Parameters* params)
{
  LONG i, d, w, b = 0;
  BOOL empty = TRUE;
  UWORD wppl = params->work_bitmap_width / 16; // words per plane line
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

  analyzed->height = analyzed->v_end - analyzed->v_start + 1;
  analyzed->width  = analyzed->h_end - analyzed->h_start + 1;
  analyzed->bytes  = (((analyzed->width - 1) / 16) + 1) * 2;
}
///
///RLE_CompressILBMBody(bitmap, max_bytes, rows, depth, filehandle)
/******************************************************************************
 * When called with fh = NULL, returns the expected size of body. When called *
 * with a valid fh, saves the body to the file. It will add a pad byte if the *
 * expected body size calculates to be odd.                                   *
 ******************************************************************************/
ULONG RLE_CompressILBMBody(struct BitMap* bm, UWORD max_bytes, UWORD rows, UBYTE depth, BPTR fh)
{
  ULONG bodySize = 0;
  ULONG r;        // iterator for current row
  ULONG d;        // iterator for current plane
  ULONG b;        // iterator for current byte
  UBYTE m = 128;  // bytecode for compression (128 is NOP)
  UBYTE* r_c;     // read cursor on bitmap bytes
  UBYTE c_byte;   // the value of the byte pointed by r_c
  UBYTE p_byte;   // the value of the previous byte

  for (r = 0; r < rows; r++) {
    for (d = 0; d < bm->Depth; d++) {
      r_c = bm->Planes[d] + r * bm->BytesPerRow;
      p_byte = *r_c;
      r_c++;

      for (b = 1; b < max_bytes; b++) {
        c_byte = *r_c;

        if (c_byte == p_byte) {
          if (m == 128) m = 255;
          else if (m > 128) m--;
          else if (*(r_c + 1) == c_byte) { // finalize literal
            //write m, write m + 1 bytes from r_c - (m + 2)
            if (fh) {
              Write(fh, &m, 1);
              Write(fh, r_c - (m + 2), m + 1);
            }
            bodySize += m + 2;
            m = 255;
          }
          else { // keep on literal
            m++;
          }
        }
        else { // c_byte != p_byte
          if (m == 128) m = 0;
          else if (m < 128) m++;
          else { // finalize replicate
            //write m, write p_byte
            if (fh) {
              Write(fh, &m, 1);
              Write(fh, &p_byte, 1);
            }
            bodySize += 2;
            m = 128;
          }
        }

        p_byte = *r_c;
        r_c++;
      }

      if (m == 128) {
        m = 0;
        Write(fh, &m, 1);
        Write(fh, &p_byte, 1);
        bodySize += 2;
      }
      else if (m < 128) { // finalize literal
        //write m + 1, write m + 2 bytes from r_c - (m + 2)
        if (fh) {
          m++;
          Write(fh, &m, 1);
          m--;
          Write(fh, r_c - (m + 2), m + 2);
        }
        bodySize += m + 3;
      }
      else { // finalize replicate
        //write m, write p_byte
        if (fh) {
          Write(fh, &m, 1);
          Write(fh, &p_byte, 1);
        }
        bodySize += 2;
      }
      m = 128;
    }
  }

  // Append a pad byte if bodySize is calculated to be odd.
  if (bodySize % 2) {
    if (fh) {
      UBYTE pad_byte = 0;
      Write(fh, &pad_byte, 1);
    }
    bodySize++;
  }

  return bodySize;
}
///
///saveWorkBitMap(wbm, max_bytes, rows, saveFile, origFile, no_compression)
VOID saveWorkBitMap(struct BitMap* wbm, UWORD max_bytes, UWORD rows, STRPTR saveFile, STRPTR origFile, BOOL no_compression)
{
  struct BitMapHeader bmhd;
  LONG cmapSize = 0;
  LONG formSize = 0;
  LONG bodySize = 0;
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

  bmhd.bmh_Width = max_bytes * 8;
  bmhd.bmh_Height = rows;
  bmhd.bmh_Left = 0;
  bmhd.bmh_Top = 0;
  bmhd.bmh_Depth = wbm->Depth;
  bmhd.bmh_Masking = 0;
  if (no_compression)
    bmhd.bmh_Compression = 0;
  else
    bmhd.bmh_Compression = 1;
  bmhd.bmh_Pad = 0;
  bmhd.bmh_Transparent = 0;
  bmhd.bmh_XAspect = 0;
  bmhd.bmh_YAspect = 0;
  bmhd.bmh_PageWidth = 0;
  bmhd.bmh_PageHeight = 0;

  if (no_compression) {
    bodySize = max_bytes * wbm->Depth * rows;
  }
  else {
    bodySize = RLE_CompressILBMBody(wbm, max_bytes, rows, wbm->Depth, NULL);
  }

  formSize = strlen("ILBM") + strlen("BMHD") + 4 + bmhdSize + strlen("CMAP") + 4 + cmapSize + strlen("BODY") + 4 + bodySize;

  fh = Open(saveFile, MODE_READWRITE);
  if (fh) {
    ULONG r, d;

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

    if (no_compression) {
      for (r = 0; r < rows; r++) {
        for (d = 0; d < wbm->Depth; d++) {
          Write(fh, wbm->Planes[d] + r * wbm->BytesPerRow, max_bytes);
        }
      }
    }
    else {
      RLE_CompressILBMBody(wbm, max_bytes, rows, wbm->Depth, fh);
    }

    SetFileSize(fh, 0, OFFSET_CURRENT);
    Close(fh);
  }

  if (cmap) {
    FreeMem(cmap, cmapSize);
  }
}
///
///saveSheetFile(num_images, table, saveFile, params)
VOID saveSheetFile(UWORD num_images, struct Table* table, STRPTR saveFile, struct Parameters *params)
{
  STRPTR sheetFile = changeExtension(saveFile, "sht");
  STRPTR sheetILBM = FilePart(saveFile);
  UBYTE type = params->big_sizes ? 5 : (params->small_sizes ? 3 : 1);

  BPTR fh = Open(sheetFile, MODE_READWRITE);
  if (fh) {
    Write(fh, "BOBSHEET", 8);
    Write(fh, sheetILBM, strlen(sheetILBM) + 1);
    Write(fh, &type, 1);
    Write(fh, &num_images, 2);
    Write(fh, table, num_images * sizeof_struct_Table);
    printf("%u BOBs has been saved!\n", num_images);
    SetFileSize(fh, 0, OFFSET_CURRENT);
    Close(fh);
  }

  freeString(sheetFile);
}
///

///changeExtension(fileName, ext)
STRPTR changeExtension(STRPTR fileName, STRPTR ext)
{
  STRPTR result = NULL;
  ULONG filename_len = strlen(fileName);
  ULONG len = filename_len;
  UBYTE* c = &fileName[len];

  while (--c >= fileName) {
    len--;
    if (*c == '.') break;
  }

  if (len) {
    result = AllocMem((len + strlen(ext) + 2), MEMF_ANY);
    strncpy(result, fileName, len);
    result[len] = '.';
    result[len + 1] = 0;
    strcat(result, ext);
    return result;
  }
  else {
    len = filename_len;
    result = AllocMem(len + strlen(ext) + 2, MEMF_ANY);
    strcpy(result, fileName);
    strcat(result, ".");
    strcat(result, ext);
  }

  return result;
}
///
///freeString(string)
/******************************************************************************
 * Frees a string allocated on memory.                                        *
 ******************************************************************************/
VOID freeString(STRPTR str)
{
  if (str) FreeMem(str, strlen(str) + 1);
}
///
