///includes
#define ECS_SPECIFIC

#include <stdio.h>
#include <exec/exec.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/display.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>

#include "system.h"
#include "color.h"
#include "cop_inst_macros.h"

#include "display.h"
///
///defines (private)
#define NULL_SCREEN_WIDTH  320
#define NULL_SCREEN_HEIGHT 256
#define NULL_SCREEN_DEPTH  1

#define NULL_BITMAP_WIDTH  NULL_SCREEN_WIDTH
#define NULL_BITMAP_HEIGHT 2  // we aimed for 1 but OS3.2 graphics library crashes with 1
#define NULL_BITMAP_DEPTH  NULL_SCREEN_DEPTH

#define BPLXMOD_V ((-NULL_BITMAP_WIDTH / 8) & 0xFFFF) // Display the same single line of bitmap

#define DDFSTART_V 0x0038
#define DDFSTOP_V  0x00D0
#define DIWSTART_V 0x2C81
#define DIWSTOP_V  0x2CC1
///
///globals
//imported globals
extern struct Custom custom;
extern volatile LONG new_frame_flag;

//exported globals
UWORD NULL_SPRITE_ADDRESS_H; // Access these via extern to have a null sprite
UWORD NULL_SPRITE_ADDRESS_L; //   "      "    "    "    "    "  "  "     "

// private globals
STATIC struct Screen* null_screen = NULL; // We only call an OpenScreen() for null display
STATIC UWORD* CopperList = (UWORD*)  0;
STATIC UWORD* CL_BPL1PTH = (UWORD*)  0;
STATIC UWORD* CL_SPR0PTH = (UWORD*)  0;

STATIC ULONG copperList_Instructions[] = {
                                              // Access Ptr:  Action:
  MOVE(COLOR00, 0x0),                         //              Set color 0 to black
  MOVE(FMODE,   0),                           //              Set Sprite/Bitplane Fetch Modes
  MOVE(BPLCON0, (NULL_SCREEN_DEPTH * BPLCON0_BPU0)),
  MOVE(BPLCON1, 0),                           // CL_BPLCON1   Set h_scroll register
  MOVE(BPLCON2, 0x264),
  MOVE(BPL1MOD, BPLXMOD_V),                   //              Set bitplane mods
  MOVE(BPL2MOD, BPLXMOD_V),                   //               "     "       "
  MOVE(DIWSTRT, DIWSTART_V),                  //              Set Display Window Start
  MOVE(DIWSTOP, DIWSTOP_V),                   //              Set Display Window Stop
  MOVE(DDFSTRT, DDFSTART_V),                  //              Set Data Fetch Start to fetch early
  MOVE(DDFSTOP, DDFSTOP_V),                   //              Set Data Fetch Stop
  MOVE_PH(BPL1PTH, 0),                        // CL_BPL1PTH   Set bitplane addresses
  MOVE(BPL1PTL, 0),                           // CL_BPL1PTL    "      "       "
  MOVE_PH(SPR0PTH, 0),                        // CL_SPR0PTH   Set sprite pointers
  MOVE(SPR0PTL, 0),                           // CL_SPT0PTL    "     "      "
  MOVE(SPR1PTH, 0),                           // CL_SPT1PTH    "     "      "
  MOVE(SPR1PTL, 0),                           // CL_SPT1PTL    "     "      "
  MOVE(SPR2PTH, 0),                           // CL_SPT2PTH    "     "      "
  MOVE(SPR2PTL, 0),                           // CL_SPT2PTL    "     "      "
  MOVE(SPR3PTH, 0),                           // CL_SPT3PTH    "     "      "
  MOVE(SPR3PTL, 0),                           // CL_SPT3PTL    "     "      "
  MOVE(SPR4PTH, 0),                           // CL_SPT4PTH    "     "      "
  MOVE(SPR4PTL, 0),                           // CL_SPT4PTL    "     "      "
  MOVE(SPR5PTH, 0),                           // CL_SPT5PTH    "     "      "
  MOVE(SPR5PTL, 0),                           // CL_SPT5PTL    "     "      "
  MOVE(SPR6PTH, 0),                           // CL_SPT6PTH    "     "      "
  MOVE(SPR6PTL, 0),                           // CL_SPT6PTL    "     "      "
  MOVE(SPR7PTH, 0),                           // CL_SPT7PTH    "     "      "
  MOVE(SPR7PTL, 0),                           // CL_SPT7PTL    "     "      "
  END
};
///
///prototypes (private)
STATIC struct Screen* openNullScreen(VOID);
STATIC VOID closeNullScreen(VOID);
STATIC UWORD* createNullCopperList(VOID);
STATIC VOID disposeNullCopperList(VOID);
///

///locateNullSpritePtr(planeptr)
/*******************************************************************************
 * The bitmap of the null display is a single depth, single raster line of     *
 * empty bytes. This empty buffer can also be used for the null sprite.        *
 * This function returns the first 64bit aligned byte of the given plane       *
 * buffer.                                                                     *
 * NOTE: This function may be unnecessary! Test and remove if possible         *
 *******************************************************************************/
STATIC UBYTE* locateNullSpritePtr(UBYTE* ptr)
{
  if ((ULONG)ptr % 8) {
    return ptr + (8 - ((ULONG)ptr % 8));
  }
  else return ptr;
}
///
///openNullScreen()
/******************************************************************************
 * We open a completely blank, one color, lowres screen (which displays the   *
 * same single one line bitmap at every raster line) to be displayed at the   *
 * start and exit of the program. We need to use the API function OpenScreen()*
 * here to make intuition think that we are a program that opens an intuition *
 * screen so it handles our take over and release back of the system as       *
 * system friendly as possible.                                               *
 * For other displays we don't need to open a screen. We can just allocate a  *
 * bitmap and create a copperlist.                                            *
 * We also set the NULL_SPRITE pointers which will be used to point unused    *
 * sprites to in all our other screens. The same memory for the one line      *
 * blank null_bitmap will be used for this.                                   *
  *****************************************************************************/
STATIC struct Screen* openNullScreen() {
  static ULONG color_table32[] = {1 << 16 | 0, 0,0,0, 0};

  null_screen = OpenScreenTags(0, SA_Width,     NULL_SCREEN_WIDTH,
                                  SA_Height,    NULL_BITMAP_HEIGHT,
                                  SA_Depth,     NULL_SCREEN_DEPTH,
                                  SA_DisplayID, PAL_MONITOR_ID,
                                  SA_Colors32,  color_table32,
                                  SA_Draggable, FALSE,
                                  SA_ShowTitle, FALSE,
                                  SA_Quiet,     TRUE,
                                  TAG_DONE);

  if (null_screen) {
    //locate the first 64bit aligned location to set as null_sprite pointer
    UBYTE* null_sprite = locateNullSpritePtr(null_screen->RastPort.BitMap->Planes[0]);
    NULL_SPRITE_ADDRESS_H = (UWORD)((ULONG)null_sprite >> 16);
    NULL_SPRITE_ADDRESS_L = (UWORD)((ULONG)null_sprite & 0xFFFF);
  }
  else { puts("Couldn't open screen!"); closeNullScreen(); }

  return null_screen;
}
///
///closeNullScreen()
STATIC VOID closeNullScreen()
{
  if (null_screen) {
    CloseScreen(null_screen);
    null_screen = NULL;
  }
}
///

///createNullCopperList()
STATIC UWORD* createNullCopperList()
{
  if (allocCopperList(copperList_Instructions, CopperList, CL_SINGLE)) {
    UWORD* sp;

    //Set copperlist bitplane instructions point to the null_screen's bitmap
    *CL_BPL1PTH = (WORD)((ULONG)null_screen->RastPort.BitMap->Planes[0] >> 16);
    *(CL_BPL1PTH + 2) = (WORD)((ULONG)null_screen->RastPort.BitMap->Planes[0] & 0xFFFF);

    //Set all sprite pointers to null_sprite
    for (sp = CL_SPR0PTH; sp < CL_SPR0PTH + 32; sp += 2) {
      *sp = NULL_SPRITE_ADDRESS_H; sp += 2;
      *sp = NULL_SPRITE_ADDRESS_L;
    }
  }
  else
    puts("Couldn't allocate NULL CopperList!");

  return CopperList;
}
///
///activateNullCopperList()
VOID activateNullCopperList()
{
  WaitVBL();

  custom.dmacon   = CLR_ALL ^ (DMAF_MASTER | DMAF_BLITTER);
  custom.beamcon0 = DISPLAYPAL;
  custom.bplcon3 = BPLCON3_V;

  custom.cop2lc = (ULONG)CopperList;

  busyWaitBlit();
  WaitVBL();

  // Enable Blitter, Copper, Bitplane DMA's.
  custom.copjmp2 = 0;
  custom.dmacon = DMAF_SETCLR | DMAF_BLITTER | DMAF_COPPER | DMAF_RASTER | DMAF_SPRITE | DMAF_DISK | DMAF_MASTER;
  custom.bplcon4 = 0;

  custom.intena = INTF_SETCLR | INTF_VERTB;
}
///
///deactivateNullCopperList()
VOID deactivateNullCopperList()
{
  custom.intena = INTF_VERTB;
}
///
///disposeNullCopperList()
STATIC VOID disposeNullCopperList()
{
  if (CopperList) {
    freeCopperList(CopperList);
  }
}
///

///openNullDisplay()
BOOL openNullDisplay()
{
  if (openNullScreen()) {
    if (createNullCopperList()) {
      return TRUE;
    }
    else closeNullScreen();
  }

  return FALSE;
}
///
///closeNullDisplay()
VOID closeNullDisplay()
{
  disposeNullCopperList();
  closeNullScreen();
}
///
///switchToNullCopperList()
VOID switchToNullCopperList()
{
  removeVBlankEvents();
  WaitVBeam(299);
  custom.cop2lc = (ULONG)CopperList;
  new_frame_flag = 1;
  waitTOF();
  blackOut();
}
///

///setSprite(image, x, y, cl_spr0pth, diwstrt, hsn, fetch_mode)
/******************************************************************************
 * The modularized all-purpose version of setSprite() function which could be *
 * used on displays that do not demand maximum performance like UI screens.   *
 ******************************************************************************/
VOID setSprite(struct SpriteImage* image, LONG x, LONG y, UWORD* cl_spr0pth, UWORD diwstrt, WORD hardware_sprite_num, UBYTE fetch_mode)
{
  struct SpriteTable* entry = &image->sprite_bank->table[image->image_num];
  ULONG hsn = hardware_sprite_num < 0 ? image->hsn : hardware_sprite_num;

  UWORD offset = entry->offset;
  UBYTE type = entry->type;

  UWORD offsetOfNext = *((UWORD*)((UBYTE*)entry + sizeof(struct SpriteTable)));
  UWORD numSprites = type & 0xF;

  UWORD ssize = (offsetOfNext - offset) / numSprites;
  UWORD height = image->height; // (ssize / (4 * fetch_mode)) - 2;

  ULONG X = x + image->h_offs + (diwstrt & 0xFF);
  ULONG Y = y + image->v_offs + (diwstrt >> 8);
  ULONG S = Y + height;
  ULONG glue_offset = fetch_mode << 4; //(16 * fetch_mode)

  UBYTE* saddr = image->sprite_bank->data + offset;
  WORD* haddr = cl_spr0pth + (hsn << 2); // (hsn * 4)

  //Prepare the two sprite control words (in one long) for the static vertical coords.
  ULONG ctl_l0 = (Y << 24) | ((S << 8) & 0xFF00) | ((Y >> 8) << 2) | ((S >> 8) << 1);
  ULONG ctl_l1;
  ULONG ctl_l2;
  if (type & 0x10) {
    while (numSprites) {
      //finalize the control words with the current X value for this glued sprite
      ctl_l1 = ctl_l0 | ((X >> 1) << 16) | 0x80 | (X & 0x1);
      ctl_l2 = ctl_l1 << 16;

      //Handle the first attached sprite:
      //set pos & ctl words on the sprite
      switch (fetch_mode) {
        case 4: *((ULONG*)saddr + 2) = ctl_l2;
        case 2: *((ULONG*)saddr + 1) = ctl_l2;
        case 1: *((ULONG*)saddr) = ctl_l1;
      }

      //set the sprite address on CopperList
      *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
      *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;

      //get to the attached sprite
      saddr += ssize;

      //set pos & ctl words on this sprite structure as well
      switch (fetch_mode) {
        case 4: *((ULONG*)saddr + 2) = ctl_l2;
        case 2: *((ULONG*)saddr + 1) = ctl_l2;
        case 1: *((ULONG*)saddr) = ctl_l1;
      }

      //set this sprite address on CopperList as well
      *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
      *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;

      //get to the next glued sprite (if there is any)
      saddr += ssize;
      X += glue_offset;
      numSprites -= 2;
    }
  }
  else
  {
    while (numSprites) {
      //finalize the control words with the current X value for this glued sprite
      ctl_l1 = ctl_l0 | ((X >> 1) << 16) | (X & 0x1);
      ctl_l2 = ctl_l1 << 16;

      //set pos & ctl words on the sprite
      switch (fetch_mode) {
        case 4: *((ULONG*)saddr + 2) = ctl_l2;
        case 2: *((ULONG*)saddr + 1) = ctl_l2;
        case 1: *((ULONG*)saddr) = ctl_l1;
      }

      //set the sprite address on CopperList
      *haddr = (WORD)((ULONG)saddr >> 16);  haddr += 2;
      *haddr = (WORD)((ULONG)saddr & 0xFFFF); haddr += 2;

      //get to the next glued sprite (if there is any)
      saddr += ssize;
      X += glue_offset;
      numSprites--;
    }
  }
}
///

/******************************
 * EXPORTED UTILITY FUNCTIONS *
 ******************************/
///allocRastPort(sizex, sizey, depth, bm_flags, friend, rp_flags, max_vectors)
/******************************************************************************
 * Allocates a BitMap alongside with a RastPort which is required to do draws *
 * on it. It can also create and initialize the Layer, TmpRas and AreaInfo    *
 * structs alogside with it.                                                  *
 * A function which should have been implemented in the API I guess.          *
 * NOTE: To have a layered rastport it is obligatory to also allocate a       *
 * bitmap using the RPF_BITMAP flag.                                          *
  *****************************************************************************/
struct RastPort* allocRastPort(ULONG sizex, ULONG sizey, ULONG depth, ULONG bm_flags, struct BitMap *friend, ULONG rp_flags, LONG max_vectors)
{
  struct RastPort* rp = NULL;

  if ((rp_flags & RPF_BITMAP) && (rp_flags & RPF_LAYER)) {
    struct BitMap* bm = allocBitMap(sizex, sizey, depth, bm_flags, friend);

    if (bm) {
      struct Layer_Info* li = NewLayerInfo();

      if (li) {
        struct Layer* layer = CreateUpfrontLayer(li, bm, 0, 0, sizex - 1, sizey - 1, LAYERSIMPLE, NULL);

        if (layer) {
          rp = layer->rp;
        }
        else {
          DisposeLayerInfo(li);
          FreeBitMap(bm);
          return NULL;
        }
      }
      else {
        FreeBitMap(bm);
        return NULL;
      }
    }
  }
  else {
    rp = AllocMem(sizeof(struct RastPort), MEMF_ANY);

    if (rp) {
      InitRastPort(rp);
      if (rp_flags & RPF_BITMAP) {
        rp->BitMap = allocBitMap(sizex, sizey, depth, bm_flags, friend);
        if (!rp->BitMap) {
          FreeMem(rp, sizeof(struct RastPort));
          return NULL;
        }
      }
    }
  }

  if (rp_flags & RPF_TMPRAS) {
    rp->TmpRas = AllocMem(sizeof(struct TmpRas), MEMF_ANY);
    if (rp->TmpRas) {
      UBYTE* tmpRasBuff = AllocMem(RASSIZE(sizex, sizey), MEMF_CHIP | MEMF_CLEAR);
      if (tmpRasBuff) {
        InitTmpRas(rp->TmpRas, tmpRasBuff, RASSIZE(sizex, sizey));
      }
      else {
        FreeMem(rp->TmpRas, sizeof(struct TmpRas));
        rp->TmpRas = NULL;
      }
    }
  }
  if (rp_flags & RPF_AREA) {
    rp->AreaInfo = AllocMem(sizeof(struct AreaInfo), MEMF_ANY);
    if (rp->AreaInfo) {
      UBYTE* areaInfoBuff = AllocMem(max_vectors * 5, MEMF_ANY | MEMF_CLEAR);
      if (areaInfoBuff) {
        InitArea(rp->AreaInfo, areaInfoBuff, max_vectors);
      }
      else {
        FreeMem(rp->AreaInfo, sizeof(struct AreaInfo));
        rp->AreaInfo = NULL;
      }
    }
  }

  return rp;
}
///
///freeRastPort(rastPort)
/******************************************************************************
 * Only to free RastPorts created by allocRastPort().                         *
 * free_flags determine which components to be freed along with the rastport. *
 * ie. you can create a rastport with RPF_ALL, create graphics on its BitMap  *
 * using rasport funtions, then free it with (RPF_ALL & ~RPF_BITMAP) so that  *
 * the bitmap allocated remains but the Layer, TmpRas and AreaInfo are freed. *
 * WARNING: Do not forget to take pointers to components excluded from        *
 * freeing beforehand. Otherwise you'll have no way to free them and so a     *
 * memory leak!                                                               *
 * NOTE: The layer of a layered rastport will be freed alongside with the     *
 * rastport and can't be excluded from being freed.                           *
 ******************************************************************************/
VOID freeRastPort(struct RastPort* rp, ULONG free_flags)
{
  if (rp) {
    struct BitMap* bm = rp->BitMap;

    if (free_flags & RPF_AREA && rp->AreaInfo) {
      FreeMem(rp->AreaInfo->VctrTbl, rp->AreaInfo->MaxCount * 5);
      FreeMem(rp->AreaInfo, sizeof(struct AreaInfo));
    }
    if (free_flags & RPF_TMPRAS && rp->TmpRas) {
      FreeMem(rp->TmpRas->RasPtr, rp->TmpRas->Size);
      FreeMem(rp->TmpRas, sizeof(struct TmpRas));
    }
    if (rp->Layer) {
      struct Layer_Info* li = rp->Layer->LayerInfo;
      struct Region* old_region = InstallClipRegion(rp->Layer, NULL);
      if (old_region) DisposeRegion(old_region);
      DeleteLayer(0L, rp->Layer);
      DisposeLayerInfo(li);
      if (free_flags & RPF_BITMAP && bm) FreeBitMap(bm);
    }
    else {
      if (free_flags & RPF_BITMAP && bm) FreeBitMap(bm);
      FreeMem(rp, sizeof(struct RastPort));
    }
  }
}
///

///copperAllocator(cl_insts, num_insts, access_ptr, double_buffer, extra_insts)
/******************************************************************************
 * Allocates the memory required for a copperlist in CHIP_MEM and sets the    *
 * access pointers to its placeholder instructions (marked with _PH).         *
 * Declarations must be in this form:                                         *
 *                                                                            *
 *  STATIC UWORD* CopperList    = (UWORD*) 0;                                 *
 *  STATIC UWORD* CL_BPLCON1    = (UWORD*) 0;                                 *
 *  ...                                                                       *
 *  STATIC ULONG copperList_Instructions[] = {                                *
 *    ...                                                                     *
 *    MOVE_PH(BPLCON1, 0),                                                    *
 *    ...                                                                     *
 *    END                                                                     *
 *  };                                                                        *
 *                                                                            *
 * Pass copperList_Instructions in cl_insts, sizeof(copperList_Instructions)  *
 * in num_insts, &CopperList in access_ptr.                                   *
 * double_buffer allocates and initializes two copperlists. When              *
 * double_buffer is set to TRUE, all access pointers MUST be declared in      *
 * pairs. Examine the example below:                                          *
 *                                                                            *
 *  STATIC UWORD* CopperList   = (UWORD*) 0;                                  *
 *  STATIC UWORD* CopperList1  = (UWORD*) 0;                                  *
 *  STATIC UWORD* CopperList2  = (UWORD*) 0;                                  *
 *  STATIC UWORD* CL_BPLCON1_1 = (UWORD*) 0;                                  *
 *  STATIC UWORD* CL_BPLCON1_2 = (UWORD*) 0;                                  *
 *  STATIC UWORD* CL_BPL1PTH_1 = (UWORD*) 0;                                  *
 *  STATIC UWORD* CL_BPL1PTH_2 = (UWORD*) 0;                                  *
 *  ...                                                                       *
 *  STATIC ULONG copperList_Instructions[] = {                                *
 *  ...                                                                       *
 *  MOVE_PH(BPLCON1, 0),                                                      *
 *  ...                                                                       *
 *  MOVE_PH(BPL1PTH, 0),                                                      *
 *  ...                                                                       *
 *                                                                            *
 * You shall use the macro variant allocCopperList(list, access, doubleBuf)   *
 * for convenience where:                                                     *
 * list is copperList_Instructions,                                           *
 * access is CopperList (NOT &CopperList).                                    *
 * doubleBuf should be set to either of CL_SINGLE or CL_DOUBLE                *
 ******************************************************************************/
BOOL copperAllocator(ULONG* cl_insts, ULONG num_insts, UWORD** access_ptr, BOOL double_buffer, ULONG extra_insts)
{
  // Get sizes:
  ULONG num_access_ptrs = (((ULONG)cl_insts - (ULONG)access_ptr) / sizeof(void*)) - 1;
  ULONG copperlist_size = num_insts * sizeof(CLINST);
  ULONG extra_size = extra_insts * sizeof(CLINST);
  ULONG* CopperList;

  if (double_buffer) {
    //WARNING: The 1st UWORD pointer in access_ptr list MUST be the CopperList
    //         The 2nd UWORD pointer in access_ptr list MUST be the CopperList1
    //         The 2nd UWORD pointer in access_ptr list MUST be the CopperList2
    ULONG* CopperList2;

    if (num_access_ptrs % 2 || !num_access_ptrs) {
      //WARNING: Every access pointer in a double buffered copperlist must be in duplicate
      printf("Incorrect number of access pointers in a double buffered copperlist\n");
      return FALSE;
    }

    num_access_ptrs -= 2;

    CopperList = AllocVec(copperlist_size * 2 + extra_size, MEMF_CHIP);
    *access_ptr = (UWORD*)CopperList; access_ptr++;
    *access_ptr = (UWORD*)CopperList; access_ptr++;
    CopperList2 = (ULONG*)((ULONG)CopperList + copperlist_size);
    *access_ptr = (UWORD*)CopperList2; access_ptr++;

    if (CopperList) {
      ULONG i;

      CopyMem(cl_insts, CopperList, copperlist_size);

      for (i = 0; i < num_insts; i++) {
        ULONG instruction = CopperList[i];

        if (instruction & 0x10000) {         //is the instruction a WAIT/SKIP
          if (!(instruction & 0x7FFE)) {     //is the instruction a placeholder
            if (num_access_ptrs) {
              //set the access pointer for CopperList1
              *access_ptr = (UWORD*)&CopperList[i]; access_ptr++;
              //set the access pointer for CopperList2
              *access_ptr = (UWORD*)((ULONG)&CopperList[i] + copperlist_size);
              CopperList[i] |= CI_WAIT_MASK; //fix the instruction to a real one
              access_ptr++;
              num_access_ptrs -= 2;
            }
            else goto fail_num;
          }
        }
        else {                               //is the instruction a MOVE inst.
          if (instruction & 0x80000000) {    //is the instruction a placeholder
            if (num_access_ptrs) {
              //set the access pointer for CopperList1
              *access_ptr = ((UWORD*)&CopperList[i]) + 1; access_ptr++;
              *access_ptr = ((UWORD*)((ULONG)&CopperList[i] + copperlist_size)) + 1;
              CopperList[i] &= CI_MOVE_MASK; //fix the instruction to a real one
              access_ptr++;
              num_access_ptrs -= 2;
            }
            else goto fail_num;
          }
        }
      }

      //Copy the fixed CopperList1 to its double buffer CopperList2
      CopyMem(CopperList, CopperList2, copperlist_size);
    }
    else goto fail_mem;
  }
  else {
    //WARNING: The first UWORD pointer in access_ptr list MUST be the CopperList

    CopperList = AllocVec(copperlist_size + extra_size, MEMF_CHIP);
    *access_ptr = (UWORD*)CopperList; access_ptr++;

    if (CopperList) {
      ULONG i;

      CopyMem(cl_insts, CopperList, copperlist_size);

      for (i = 0; i < num_insts; i++) {
        ULONG instruction = CopperList[i];

        if (instruction & 0x10000) {         //is the instruction a WAIT/SKIP
          if (!(instruction & 0x7FFE)) {     //is the instruction a placeholder
            if (num_access_ptrs) {
              *access_ptr = (UWORD*)&CopperList[i];   //set the access pointer
              CopperList[i] |= CI_WAIT_MASK; //fix the instruction to a real one
              access_ptr++;
              num_access_ptrs--;
            }
            else goto fail_num;
          }
        }
        else {                               //is the instruction a MOVE inst.
          if (instruction & 0x80000000) {    //is the instruction a placeholder
            if (num_access_ptrs) {
              *access_ptr = ((UWORD*)&CopperList[i]) + 1; //set the access pointer
              CopperList[i] &= CI_MOVE_MASK; //fix the instruction to a real one
              access_ptr++;
              num_access_ptrs--;
            }
            else goto fail_num;
          }
        }
      }
    }
    else goto fail_mem;
  }

  if (num_access_ptrs) {
    printf("WARNING: There are %lu un-initialized access_ptrs!\n", num_access_ptrs);
  }

  return TRUE;
fail_mem:
  printf("Not enough memory to create copperlist\n");
  return FALSE;
fail_num:
  printf("There are more instruction place holders than the access pointers declared!\n");
  FreeVec(CopperList);
  return FALSE;
}
///
