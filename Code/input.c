/******************************************************************************
 * NOTE: We need to write a better implementation that gathers all the data   *
 * from both joystick ports in one go.                                        *
 * NOTE: Also imagine a game genre like Street Fighter in which you have to   *
 * read joystick events multiple times each frame and also buffer them. This  *
 * would require a completely different implementation.                       *
 ******************************************************************************/
#include <exec/exec.h>

#include "input.h"

extern struct Custom custom;
extern struct CIA ciaa, ciab;

///readMouse(port)
/******************************************************************************
 * Reads the hardware registers of the given joystick port for mouse state.   *
 * Returns the state packed in an ULONG value. Individual state values can be *
 * accessed by casting the returned ULONG to a struct MouseState.             *
 * NOTE: Reported to not work on some emulators.                              *
 ******************************************************************************/
ULONG readMouse(ULONG port)
{
  static BYTE old_x = 0;
  static BYTE old_y = 0;
  BYTE x;
  BYTE y;
  UWORD joydat;
  UWORD pot;
  ULONG retval = 0;
  struct MouseState* mouseState = (struct MouseState*)&retval;

  custom.potgo = 0xFF00;
  pot = custom.potinp;

  if (port) {
    //Read the second port (the joystick port) of the Amiga
    joydat = custom.joy1dat;
    mouseState->buttons |= (ciaa.ciapra & 0x0080) ? 0 : LEFT_MOUSE_BUTTON;
    mouseState->buttons |= (pot & 0x1000) ? 0 : MIDDLE_MOUSE_BUTTON;
    mouseState->buttons |= (pot & 0x4000) ? 0 : RIGHT_MOUSE_BUTTON;
  }
  else {
    joydat = custom.joy0dat;
    mouseState->buttons |= (ciaa.ciapra & 0x0040) ? 0 : LEFT_MOUSE_BUTTON;
    mouseState->buttons |= (pot & 0x0100) ? 0 : MIDDLE_MOUSE_BUTTON;
    mouseState->buttons |= (pot & 0x0400) ? 0 : RIGHT_MOUSE_BUTTON;
  }

  x = joydat & 0x00FF;
  mouseState->deltaX = x - old_x;
  old_x = x;

  y = joydat >> 8;
  mouseState->deltaY = y - old_y;
  old_y = y;

  return retval;
}
///
