/******************************************************************************
 * SpriteBankMerger                                                           *
 ******************************************************************************/

///defines
#define PROGRAMNAME     "SpriteBankMerger"
#define VERSION         0
#define REVISION        5
#define VERSIONSTRING   "0.5"

//define command line syntax and number of options
#define RDARGS_TEMPLATE "BANK1/A, BANK2/A, NEWBANK/A"
#define RDARGS_OPTIONS  3

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

/* The OLD SPRITE TABLE we've used in Amos PlatformGameEngine!
struct  {
  WORD offset;   // Byte offset from WordsOfSpriteData[0] to the the image
  BYTE type;     // Defines the sprite usage of image
  BYTE voffs;    // This is added to the requested Y position
};
*/

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

struct BankInfo {
  BPTR file_handle;
  UWORD num_images;
  UWORD data_size;
  struct Table* table;
  UBYTE* data;
  UBYTE type;
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

enum {
  BANK1,
  BANK2,
  BANK3
};

STRPTR err_TooBig = "Sprite banks are too big to be merged!";
STRPTR err_File = "Could not open %s sprite bank file!";
STRPTR err_Memory = "Not enough memory!";
STRPTR err_Type = "The size type of the two banks has to be identical!";

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

BPTR openSpriteBankFile(STRPTR filename);
BOOL getBankInfo(struct BankInfo* bi);
VOID freeBankInfo(struct BankInfo* bi);

UBYTE determineSaveType(struct BankInfo* bi1, struct BankInfo* bi2);
VOID saveHitbox(struct BankInfo* bi, BPTR fh, UWORD additive, UBYTE save_type);
VOID mergeHitboxes(struct BankInfo* bi1, struct BankInfo* bi2, struct BankInfo* bi3, UBYTE save_type);
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
  struct BankInfo bi[3] = {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}};
  ULONG num_images;
  ULONG offset_of_next;
  ULONG sizeof_struct_Table;
  ULONG i;
  UBYTE save_type = 0;

  bi[BANK1].file_handle = openSpriteBankFile((STRPTR)config->Options[BANK1]);
  if (bi[BANK1].file_handle) {
    bi[BANK2].file_handle = openSpriteBankFile((STRPTR)config->Options[BANK2]);
    if (bi[BANK2].file_handle) {
      bi[BANK3].file_handle = Open((STRPTR)config->Options[BANK3], MODE_NEWFILE);
      if (bi[BANK3].file_handle) {
        if (!getBankInfo(&bi[BANK1])) {puts(err_Memory); rc = 20; goto bailout;}
        if (!getBankInfo(&bi[BANK2])) {puts(err_Memory); rc = 20; goto bailout;}

        if ((bi[BANK1].type & 0x06) == (bi[BANK2].type & 0x06)) {
          save_type = determineSaveType(&bi[BANK1], &bi[BANK2]);

          switch (bi[BANK1].type & 0x06) {
            case 0x04:
              sizeof_struct_Table = sizeof(struct Table_big);
            break;
            case 0x02:
              sizeof_struct_Table = sizeof(struct Table_small);
            break;
            default:
              sizeof_struct_Table = sizeof(struct Table);
          }

          num_images = (ULONG)bi[BANK1].num_images + bi[BANK2].num_images;

          if (num_images <= 0xFFFF) {
            for (i = 0; i < bi[BANK2].num_images; i++) {
              switch (bi[BANK1].type & 0x06) {
                case 0x04:
                  offset_of_next = (ULONG)((struct Table_big*)bi[BANK2].table)[i].offset + bi[BANK1].data_size;
                break;
                case 0x02:
                  offset_of_next = (ULONG)((struct Table_small*)bi[BANK2].table)[i].offset + bi[BANK1].data_size;
                break;
                default:
                  offset_of_next = (ULONG)bi[BANK2].table[i].offset + bi[BANK1].data_size;
              }

              if (offset_of_next > 0xFFFF) {
                puts(err_TooBig);
                rc = 20;
                goto bailout;
              }
              switch (bi[BANK1].type & 0x06) {
                case 0x04:
                  ((struct Table_big*)bi[BANK2].table)[i].offset = offset_of_next;
                break;
                case 0x02:
                  ((struct Table_small*)bi[BANK2].table)[i].offset = offset_of_next;
                break;
                default:
                  bi[BANK2].table[i].offset = offset_of_next;
              }
            }
            offset_of_next = (ULONG)bi[BANK1].data_size + bi[BANK2].data_size;
            if (offset_of_next > 0xFFFF) {
              puts(err_TooBig);
              rc = 20;
              goto bailout;
            }

            bi[BANK3].num_images = num_images;
            bi[BANK3].data_size = offset_of_next;

            Write(bi[BANK3].file_handle, "SPRBANK", 7);
            Write(bi[BANK3].file_handle, &save_type, 1);
            Write(bi[BANK3].file_handle, &bi[BANK3].num_images, sizeof(WORD));
            Write(bi[BANK3].file_handle, bi[BANK1].table, bi[BANK1].num_images * sizeof_struct_Table);
            Write(bi[BANK3].file_handle, bi[BANK2].table, bi[BANK2].num_images * sizeof_struct_Table);
            Write(bi[BANK3].file_handle, &bi[BANK3].data_size, sizeof(WORD));
            Write(bi[BANK3].file_handle, bi[BANK1].data, bi[BANK1].data_size);
            Write(bi[BANK3].file_handle, bi[BANK2].data, bi[BANK2].data_size);

            mergeHitboxes(&bi[BANK1], &bi[BANK2], &bi[BANK3], save_type);
          }
          else {puts(err_TooBig); rc = 20;}
        }
        else {puts(err_Type); rc = 20;}

bailout:
        freeBankInfo(&bi[BANK1]);
        freeBankInfo(&bi[BANK2]);

        Close(bi[BANK3].file_handle);
      }
      else {printf(err_File, "output"); rc = 20;}

      Close(bi[BANK2].file_handle);
    }
    else {printf(err_File, "second"); rc = 20;}

    Close(bi[BANK1].file_handle);
  }
  else {printf(err_File, "first"); rc = 20;}

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

///openSpriteBankFile(filename)
BPTR openSpriteBankFile(STRPTR filename)
{
  BPTR fh = Open(filename, MODE_OLDFILE);
  if (fh) {
    UBYTE id[8];
    Read(fh, id, 7);

    if (strncmp(id, "SPRBANK", 7) != 0) {
      Close(fh);
      fh = 0;
    }
  }

  return fh;
}
///
///getBankInfo(bankInfo)
BOOL getBankInfo(struct BankInfo* bi)
{
  ULONG sizeof_struct_Table;

  Read(bi->file_handle, &bi->type, 1);
  switch (bi->type & 0x06) {
    case 0x04:
      sizeof_struct_Table = sizeof(struct Table_big);
    break;
    case 0x02:
      sizeof_struct_Table = sizeof(struct Table_small);
    break;
    default:
      sizeof_struct_Table = sizeof(struct Table);
  }

  Read(bi->file_handle, &bi->num_images, sizeof(WORD));
  bi->table = (struct Table*)AllocMem(bi->num_images * sizeof_struct_Table, MEMF_ANY);
  if (bi->table) {
    Read(bi->file_handle, bi->table, bi->num_images * sizeof_struct_Table);
    Read(bi->file_handle, &bi->data_size, sizeof(WORD));
    bi->data = (UBYTE*)AllocMem(bi->data_size, MEMF_ANY);
    if (bi->data) {
      Read(bi->file_handle, bi->data, bi->data_size);
      return TRUE;
    }
  }

  return FALSE;
}
///
///freeBankInfo(bankInfo)
VOID freeBankInfo(struct BankInfo* bi)
{
  ULONG sizeof_struct_Table;
  switch (bi->type & 0x06) {
    case 0x04:
      sizeof_struct_Table = sizeof(struct Table_big);
    break;
    case 0x02:
      sizeof_struct_Table = sizeof(struct Table_small);
    break;
    default:
      sizeof_struct_Table = sizeof(struct Table);
  }

  if (bi->data) {
    FreeMem(bi->data, bi->data_size);
  }
  if (bi->table) {
    FreeMem(bi->table, bi->num_images * sizeof_struct_Table);
  }
}
///

///determineSaveType(bankInfo1, bankInfo2)
UBYTE determineSaveType(struct BankInfo* bi1, struct BankInfo* bi2)
{
  UBYTE type = 0;

  if (bi1->type == bi2->type) {
    type = bi1->type;
  }
  else if ((bi1->type & 0x08) == 0) {
    type = bi2->type;
  }
  else if ((bi2->type & 0x08) == 0) {
    type = bi1->type;
  }
  else {
    type = bi1->type |= 0x18;
  }

  return type;
}
///
///mergeHitboxes(bankInfo1, bankInfo2, bankInfo1, bankInfo3, save_type)
VOID mergeHitboxes(struct BankInfo* bi1, struct BankInfo* bi2, struct BankInfo* bi3, UBYTE save_type)
{
  UWORD num_hitboxes_1 = 0;
  UWORD num_hitboxes_2 = 0;
  UWORD num_hitboxes;
  ULONG i;

  if (bi1->type & 0x08) {
    Read(bi1->file_handle, &num_hitboxes_1, sizeof(UWORD));
  }
  if (bi2->type & 0x08) {
    Read(bi2->file_handle, &num_hitboxes_2, sizeof(UWORD));
  }

  num_hitboxes = num_hitboxes_1 + num_hitboxes_2;
  if (num_hitboxes) {
    //save num_hitboxes
    Write(bi3->file_handle, &num_hitboxes, sizeof(UWORD));

    //save first index
    if (bi1->type & 0x08) {
      for (i = 0; i < bi1->num_images; i++) {
        UWORD index;
        Read(bi1->file_handle, &index, sizeof(UWORD));
        Write(bi3->file_handle, &index, sizeof(UWORD));
      }
    }
    else {
      UWORD index = 0xFFFF;
      for (i = 0; i < bi1->num_images; i++) {
        Write(bi3->file_handle, &index, sizeof(UWORD));
      }
    }

    //save second index
    if (bi2->type & 0x08) {
      for (i = 0; i < bi2->num_images; i++) {
        UWORD index;
        Read(bi2->file_handle, &index, sizeof(UWORD));
        if (index != 0xFFFF) {
          index += num_hitboxes_1;
        }
        Write(bi3->file_handle, &index, sizeof(UWORD));
      }
    }
    else {
      UWORD index = 0xFFFF;
      for (i = 0; i < bi2->num_images; i++) {
        Write(bi3->file_handle, &index, sizeof(UWORD));
      }
    }

    //save first hitboxes
    for (i = 0; i < num_hitboxes_1; i++) {
      saveHitbox(bi1, bi3->file_handle, 0, save_type);
    }

    //save second hitboxes
    for (i = 0; i < num_hitboxes_2; i++) {
      saveHitbox(bi2, bi3->file_handle, num_hitboxes_1, save_type);
    }
  }
}
///
///saveHitbox(bankInfo, file_handle, additive, save_type)
VOID saveHitbox(struct BankInfo* bi, BPTR fh, UWORD additive, UBYTE save_type)
{
  struct __attribute__((packed)) HitBox_small {
    BYTE x1, y1;
    BYTE x2, y2;
    UWORD next;
  }hb_small;

  struct __attribute__((packed)) HitBox_big {
    WORD x1, y1;
    WORD x2, y2;
    UWORD next;
  }hb_big;

  if (bi->type & 0x10) {
    if (save_type & 0x10) { //read small, save small
      Read(bi->file_handle, &hb_small, sizeof(hb_small));
      if (hb_small.next != 0xFFFF) {
        hb_small.next += additive;
      }
      Write(fh, &hb_small, sizeof(hb_small));
    }
    else { //read small, save big
      Read(bi->file_handle, &hb_small, sizeof(hb_small));
      hb_big.x1 = hb_small.x1;
      hb_big.y1 = hb_small.y1;
      hb_big.x2 = hb_small.x2;
      hb_big.y2 = hb_small.y2;
      if (hb_small.next != 0xFFFF) {
        hb_big.next += hb_small.next + additive;
      }
      else {
        hb_big.next = hb_small.next;
      }
      Write(fh, &hb_big, sizeof(hb_big));
    }
  }
  else { //read big, save big
    Read(bi->file_handle, &hb_big, sizeof(hb_big));
    if (hb_big.next != 0xFFFF) {
      hb_big.next += additive;
    }
    Write(fh, &hb_big, sizeof(hb_big));
  }
}
///
