/******************************************************************************
 * ConvertTiles                                                               *
 * Converts the given ILBM tilesheet file into a tileset the engine can use.  *
 * MARGIN and SPACING values are the values used in Tiled*.                   *
 * If the tilesheet does not have the blank tile at ID=0 (which the engine    *
 * requires) ADDZERO will add one (if you use ADDZERO here you will not need  *
 * to use it on ConvertMap for the maps that use this tileset).               *
 * TILESIZE defaults to 16.                                                   *
 * NUMTILES will default to map's width * height (+1 if you use ADDZERO). You *
 * can limit the size of the the tileset giving a positive value (ADDZERO     *
 * will not add 1 this way). A negative value will set it to:                 *
 * map's width * height - value (+1 if ADDZERO is used).                      *
 * So you can just count the empty tiles at the end of the sheet's final row, *
 * and pass it as a negative value on NUMTILES.                               *
 * Produced tileset will be saved into the current directory with the         *
 * filename passed in OUTPUT.                                                 *
 *                                                                            *
 * (*) Tiled: The infamous tile editing software. (https://www.mapeditor.org) *
 ******************************************************************************/

///defines
#define PROGRAMNAME     "ConvertTiles"
#define VERSION         0
#define REVISION        7
#define VERSIONSTRING   "0.7"

//define command line syntax and number of options
#define RDARGS_TEMPLATE "F=FILE/A, O=OUTPUT/A, N=NUMTILES/N, T=TILESIZE/N, M=MARGIN/N, S=SPACING/N, A=ADDZERO/S, C=COLMAP/S"
#define RDARGS_OPTIONS  8

enum {
  OPT_FILE,
  OPT_OUTPUT,
  OPT_NUMTILES,
  OPT_TILESIZE,
  OPT_MARGIN,
  OPT_SPACING,
  OPT_ADDZERO,
  OPT_COLMAP
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

#include <datatypes/pictureclass.h>
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
  struct BitMap* tile;
  LONG numtiles;
  UWORD width;
  UWORD height;
  UWORD columns;
  UWORD rows;
  UBYTE margin;
  UBYTE spacing;
  UBYTE tilesize;
  #endif

  //<YOUR GLOBAL DATA HERE>

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

BOOL locateStrInFile(BPTR fh, STRPTR str);
struct BitMap* loadILBMBitMap(STRPTR fileName, struct Config* config);
ULONG writeTileSetFile(STRPTR fileName, struct BitMap* bitmap, struct Config* config);
VOID printBitMapHeader(struct BitMapHeader *bmhd);
VOID printColorMap(UBYTE* cm, UWORD numColors);
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
        config->RDArgs = ReadArgs(RDARGS_TEMPLATE, config->Options, NULL);
        if (config->RDArgs)
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
  STRPTR fileName = (STRPTR)config->Options[OPT_FILE];
  STRPTR output = (STRPTR)config->Options[OPT_OUTPUT];
  ULONG tilesize = config->Options[OPT_TILESIZE] ? *(ULONG*)config->Options[OPT_TILESIZE] : 16;
  LONG numtiles = config->Options[OPT_NUMTILES] ? (*(ULONG*)config->Options[OPT_NUMTILES] ? *(ULONG*)config->Options[OPT_NUMTILES] : 0x10000) : 0;
  struct BitMap* bm;

  if (numtiles < 0xFFFF) {
    config->numtiles = numtiles;
    if (!(!tilesize || tilesize % 16 || tilesize > 64)) {
      config->tilesize = tilesize;
      config->margin = config->Options[OPT_MARGIN] ? (UBYTE)*(ULONG*)config->Options[OPT_MARGIN] : 0;
      config->spacing = config->Options[OPT_SPACING] ? (UBYTE)*(ULONG*)config->Options[OPT_SPACING] : 0;
      bm = loadILBMBitMap(fileName, config);
      if (bm) {
        BOOL valid_sizes = FALSE;

        if (!((config->width - config->margin) % (tilesize + config->spacing) | (config->height - config->margin) % (tilesize + config->spacing))) {
          valid_sizes = TRUE;
          config->columns = (config->width - config->margin) / (tilesize + config->spacing);
          config->rows    = (config->height - config->margin) / (tilesize + config->spacing);
        }
        else if (!((config->width - config->margin + config->spacing) % (tilesize + config->spacing) | (config->height - config->margin + config->spacing) % (tilesize + config->spacing))) {
          valid_sizes = TRUE;
          config->columns = (config->width - config->margin + config->spacing) / (tilesize + config->spacing);
          config->rows    = (config->height - config->margin + config->spacing) / (tilesize + config->spacing);
        }

        if (valid_sizes) {
          if (config->columns * config->rows) {
            if (config->numtiles <= (config->columns * config->rows)) {
              if ((config->tile = AllocBitMap(tilesize, tilesize, bm->Depth, BMF_INTERLEAVED | BMF_CLEAR, NULL))) {
                if ((writeTileSetFile(output, bm, config))) {
                  puts("Could not write tileset file");
                }
                else {
                  puts("tileset created successfully");
                }

                FreeBitMap(config->tile);
              }
              else puts("Not enough memory!");
            }
            else puts("NUMTILES too big for tilesheet!");
          }
          else puts("Invalid MARGIN and/or SPACING values!");
        }
        else puts("Invalid MARGIN and/or SPACING values!");

        FreeBitMap(bm);
      }
    }
    else puts("Invalid TILESIZE!");
  }
  else puts("Invalid NUMTILES!");

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

///writeTileSetFile(fileName, bitmap, bmhd)
ULONG writeTileSetFile(STRPTR fileName, struct BitMap* bitmap, struct Config* config)
{
  ULONG rc = 0;
  BOOL addZero = (BOOL)config->Options[OPT_ADDZERO];
  ULONG numTiles = config->numtiles ? config->numtiles < 0 ? config->columns * config->rows + config->numtiles + (addZero ? 1 : 0) : config->numtiles : config->columns * config->rows + (addZero ? 1 : 0);
  BPTR fh;

  printf("Tilesheet: %s\nColumns: %u\nRows: %u\n", (STRPTR)config->Options[OPT_FILE], config->columns, config->rows);

  if (addZero && !config->numtiles) printf("NumTiles: %lu (%lu + 1 because of ADDZERO)\n", numTiles, numTiles - 1);
  else printf("NumTiles: %lu\n", numTiles);

  fh = Open(fileName, MODE_NEWFILE);
  if (fh)
  {
    ULONG c;
    ULONG r;
    ULONG count = 0;
    ULONG tileBytes = (config->tilesize / 8) * config->tilesize * bitmap->Depth;

    if (addZero) {
      Write(fh, config->tile->Planes[0], tileBytes);
      count++;
    }

    for (r = 0; r < config->rows; r++) {
      for (c = 0; c < config->columns; c++) {
        if (count++ == numTiles) goto done;
        BltBitMap(bitmap, config->margin + c * (config->tilesize + config->spacing), config->margin + r * (config->tilesize + config->spacing), config->tile, 0, 0, config->tilesize, config->tilesize, 0x0C0, 0xFF, NULL);
        Write(fh, config->tile->Planes[0], tileBytes);
      }
    }

done:
    Close(fh);
  }
  else rc = 20;

  return rc;
}
///
///locateStrInFile(fileHandle, str)
BOOL locateStrInFile(BPTR fh, STRPTR str)
{
  BOOL found = FALSE;
  ULONG M = strlen(str);
  UBYTE* lps = AllocMem(M, MEMF_ANY | MEMF_CLEAR);

  if (lps) {
    LONG i;
    ULONG j = 0;
    UBYTE len = 0; // length of the previous longest prefix suffix
    UBYTE ch;

    i = 1;
    while (i < M) {
      if (str[i] == str[len]) {
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
      if (str[j] == ch) {
        j++;
      }

      if (j == M) {
        found = TRUE;
        break;
      }
      else {
        i = Read(fh, &ch, 1);
        if (i > 0 && str[j] != ch) {
          if (j != 0)
          j = lps[j - 1];
          else
          i = Read(fh, &ch, 1);
        }
      }
    }

    FreeMem(lps, M);
  }

  return found;
}
///
///loadILBMBitMap(fileName, config)
struct BitMap* loadILBMBitMap(STRPTR fileName, struct Config* config)
{
  struct BitMapHeader bmhd;
  struct BitMap* bm = NULL;
  UWORD numColors;
  UBYTE* colorMap;
  BPTR fh;

  fh = Open(fileName, MODE_OLDFILE);
  if (fh) {
    if (locateStrInFile(fh, "FORM")) {
      if (locateStrInFile(fh, "ILBM")) {
        if (locateStrInFile(fh, "BMHD")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct BitMapHeader) == Read(fh, &bmhd, sizeof(struct BitMapHeader))) {
            config->width = bmhd.bmh_Width;
            config->height = bmhd.bmh_Height;
            if (locateStrInFile(fh, "CMAP")) {
              Seek(fh, 4, OFFSET_CURRENT);
              numColors = 1 << bmhd.bmh_Depth;
              colorMap = AllocMem(numColors * 3, MEMF_ANY);
              if (colorMap) {
                Read(fh, colorMap, numColors * 3);
                if (config->Options[OPT_COLMAP]) {
                  printColorMap(colorMap, numColors);
                }

                if (locateStrInFile(fh, "BODY")) {
                  Seek(fh, 4, OFFSET_CURRENT);
                  bm = AllocBitMap(bmhd.bmh_Width, bmhd.bmh_Height, bmhd.bmh_Depth, BMF_INTERLEAVED, NULL);
                  if (bm && (GetBitMapAttr(bm, BMA_FLAGS) & BMF_INTERLEAVED)) {
                    if (bmhd.bmh_Compression) { // NOTE: consider possible compression methods
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
                  else {
                    puts("Not enough memory!");
                    if (bm){
                      FreeBitMap(bm);
                      bm = NULL;
                    }
                  }
                }
                else puts("Corrupt ILBM file!");

                FreeMem(colorMap, numColors * 3);
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

  return bm;
}
///

///printBitMapHeader(bmhd)
VOID printBitMapHeader(struct BitMapHeader *bmhd)
{
  printf("Bitmap properties:\nWidth: %lu\nHeight: %lu\nLeft: %ld\nTop: %ld\nDepth: %lu\nMasking: %lu\nCompression: %lu\n",
         (LONG)bmhd->bmh_Width, (LONG)bmhd->bmh_Height,
         (LONG)bmhd->bmh_Left,  (LONG)bmhd->bmh_Top,
         (LONG)bmhd->bmh_Depth, (LONG)bmhd->bmh_Masking,
         (LONG)bmhd->bmh_Compression);
}
///
///printColorMap(colormap)
VOID printColorMap(UBYTE* cm, UWORD numColors)
{
  ULONG i;

  printf("ColorTable:\n");
  for (i = 0; i < numColors; i++) {
    printf("%3u, %3u, %3u,\n", *cm, *(cm + 1), *(cm + 2));
    cm += 3;
  }
}
///
