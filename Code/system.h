#ifndef SYSTEM_H
#define SYSTEM_H

#include "missing_hardware_defines.h"

/******************************************************************************
 * The global variable "chipset" will hold one of the values below.           *
 * To access from other .c files, declare as:                                 *
 * extern ULONG chipset;                                                      *
 ******************************************************************************/
#define CHIPSET_OCS 0
#define CHIPSET_ECS 1
#define CHIPSET_AGA 2

VOID detectChipset(VOID);
BOOL takeOverSystem(VOID);
VOID giveBackSystem(VOID);
VOID WaitVBL(VOID);
VOID waitVBeam(ULONG);
VOID WaitVBeam(ULONG);
VOID waitTOF(VOID);
VOID busyWaitBlit(VOID);
VOID setVBlankEvents(VOID (*function)(VOID));
VOID removeVBlankEvents(VOID);

/******************************************************************************
 * The macros below provide a non-blocking time delay check using the         *
 * g_frame_counter. To use them you should declare a static ULONG variable to *
 * hold the rendezvous frame in your anim function and set it with one of the *
 * initDelay macros below. Then to test if the rendezvous time has come use   *
 * testDelay() macro with your static declared variable. Example:             *
 *                                                                            *
 * animFunc(struct GameObject* go)                                            *
 * {                                                                          *
 *    static ULONG anim_wait = 0;                                             *
 *    if (testDelay(anim_wait)) {                                             *
 *      // your animation routines                                            *
 *      anim_wait = initDelaySeconds(1);                                      *
 *    }                                                                       *
 * }                                                                          *
 *                                                                            *
 * Your animation routines will be executed with one second intervals.        *
 * NOTE: don't forget to import the frame_counter global in your .c file. as: *
 * extern volatile ULONG g_frame_counter;                                     *
 ******************************************************************************/
#define initDelayFrames(f)  (g_frame_counter + f)
#define initDelaySeconds(s) (g_frame_counter + s * 50)  //WARNING: This assumes a PAL system
#define initDelayMinutes(m) (g_frame_counter + m * 300) //WARNING: This assumes a PAL system
#define testDelay(r) (g_frame_counter >= r)

#endif /* SYSTEM_H */
