#ifndef DISPLAY_H
#define DISPLAY_H

#include "SDI_headers/SDI_compiler.h"

BOOL openNullDisplay(VOID);
VOID closeNullDisplay(VOID);
VOID activateNullCopperList(VOID);
VOID deactivateNullCopperList(VOID);
VOID switchToNullCopperList(VOID);

/*******************************
 * NULL SPRITE                 *
 *******************************/
/* Import as below whereever an empty sprite is required
extern UWORD NULL_SPRITE_ADDRESS_H;
extern UWORD NULL_SPRITE_ADDRESS_L;
*/

/*******************************
 * SOME HELPFUL DEFAULT VALUES *
 *******************************/
#define DEFAULT_DDFSTRT_LORES 0x0038
#define DEFAULT_DDFSTRT_HIRES 0x003C
#define DEFAULT_DDFSTOP_LORES 0x00D0
#define DEFAULT_DIWSTRT 0x2C81
#define DEFAULT_DIWSTOP 0x2CC1

/******************************
 * EXPORTED UTILITY FUNCTIONS *
 ******************************/
#define RPF_LAYER   0x01
#define RPF_BITMAP  0x02
#define RPF_TMPRAS  0x04
#define RPF_AREA    0x08
#define RPF_ALL     0x0F

struct RastPort* allocRastPort(ULONG sizex, ULONG sizey, ULONG depth, ULONG bm_flags, struct BitMap *friend, ULONG rp_flags, LONG max_vectors);
VOID freeRastPort(struct RastPort* rp, ULONG free_flags);

#define CL_SINGLE FALSE
#define CL_DOUBLE TRUE
#define CL_SIZE(a) (sizeof(a))
#define CL_NUM_INSTS(a) (sizeof(a) / 4)

#define allocCopperList(list, access, doubleBuf) copperAllocator(list, CL_NUM_INSTS(list), &access, doubleBuf, 0)
#define freeCopperList(cl) FreeVec(cl);
BOOL copperAllocator(ULONG* cl_insts, ULONG num_insts, UWORD** access_ptr, BOOL double_buffer, ULONG extra_insts);

/***************************************************
 * Ensure BitMap allocations meet AGA requirements *
 ***************************************************/
#define ROUND_TO_64(a) ((a + 63) & 0xFFFFFFC0)
#define allocBitMap(sizex, sizey, depth, flags, friend) AllocBitMap(ROUND_TO_64(sizex), sizey, depth, flags, friend)

#endif /* DISPLAY_H */
