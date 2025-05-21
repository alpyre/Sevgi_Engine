///defines
#define MATRIX_SIZE 16UL
///
///includes
#include <exec/exec.h>
#include <dos/dos.h>
#include <devices/keyboard.h>
#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>

#include "keyboard.h"
///
///globals
static struct MsgPort *keyMP = NULL;
static struct IORequest* keyIO = NULL;
static UBYTE keyMatrix[MATRIX_SIZE] = {0};
static BOOL device_ok = FALSE;
///

/// setKeyboardAccess()
/****************************************************
 * Creates an IORequest to query the keyboard state *
 ****************************************************/
BOOL setKeyboardAccess(VOID)
{
  struct IOStdReq* ioStdReq = NULL;

  if ((keyMP = CreatePort(NULL, NULL))) {
    if ((ioStdReq = (struct IOStdReq*)CreateExtIO(keyMP, sizeof(struct IOStdReq)))) {
      keyIO = (struct IORequest*)ioStdReq;
      if (!OpenDevice("keyboard.device", NULL, keyIO, NULL)) {
        device_ok = TRUE;
        ioStdReq->io_Command = KBD_READMATRIX;
        ioStdReq->io_Data    = (APTR)keyMatrix;
        ioStdReq->io_Length  = MATRIX_SIZE;
        return TRUE;
      }
      else
        puts("Error: Could not open keyboard.device!");
    }
    else
      puts("Error: Could not create I/O request!");
  }
  else
    puts("Error: Could not create message port!");

  endKeyboardAccess();
  return FALSE;
}
///
/// endKeyboardAccess()
VOID endKeyboardAccess()
{
  if (device_ok) CloseDevice(keyIO);
  if (keyIO) DeleteExtIO(keyIO);
  if (keyMP) DeletePort(keyMP);
}
///
/// doKeyboardIO()
VOID doKeyboardIO()
{
  DoIO(keyIO);
}
///
/// keyState(rawkey)
BOOL keyState(UBYTE rawkey)
{
  UBYTE bit = 0x01;
                       // rawkey /  8             rawkey % 8
  return (BOOL)(keyMatrix[rawkey >> 3] & (bit << (rawkey & 0x7)));
}
///
