/******************************************************************************
 * TODO: We need to write a better implementation that gathers all the data   *
 * from both joystick ports in one go.                                        *
 * TODO: Also imagine a game genre like Street Fighter in which you have to   *
 * read joystick events multiple times each frame and also buffer them. This  *
 * would require a completely different implementation.                       *
 * WARNING: This source file *MUST* be compiled *WITHOUT* any compiler        *
 * optimizations to prevent the compiler from optimizing out the microsecond  *
 * busy-wait delays necessary in the CD32 JoyPad protocol.                    *
 ******************************************************************************/
///includes
#include <exec/exec.h>
#include "input.h"
///
///defines
#define ECLOCK_DELAY(d) {ULONG i; for (i = 0; i < d; i++) if (ciaa.ciapra);}
///
///globals
extern struct Custom custom;
extern struct CIA ciaa, ciab;
///

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

  custom.potgo = 0xFF00; //Set pin5 and pin9 to output (raise voltage on them)
  pot = custom.potinp;   //Read the state of pin5 and pin9 being grounded

  if (port) {
    //Read the second port (the joystick port) of the Amiga
    joydat = custom.joy1dat;
    mouseState->buttons |= (ciaa.ciapra & 0x0080) ? 0 : LEFT_MOUSE_BUTTON;
    mouseState->buttons |= (pot & 0x1000) ? 0 : MIDDLE_MOUSE_BUTTON;
    mouseState->buttons |= (pot & 0x4000) ? 0 : RIGHT_MOUSE_BUTTON;
  }
  else {
    //Read the first port (the mouse port) of the Amiga
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
///readCD32JoyPadButtons(port)
/******************************************************************************
 * Reads the states of the CD32 joypad buttons by putting the given joystick  *
 * port into serial data exchange mode of the CD32 joypad protocol.           *
 * Returns a bit field of the pressed states of the seven CD32 buttons.       *
 * Will default to a 2 button "dumb" joystick if no CD32 JoyPad is detected   *
 * on the given port (bit CD32_RED represents button 1 and bit CD32_BLUE      *
 * represents button 2).                                                      *
 * Use the usual JOY_LEFT/JOY_RIGHT/JOY_UP/JOY_DOWN macros for directions.    *
 ******************************************************************************/
ULONG readCD32JoyPadButtons(ULONG port)
{
  ULONG buttons = 0;
  ULONG dumb_button1_state;
  ULONG i;

  if (port) {
    //Read the second port (the joystick port) of the Amiga
    //*****************************************************
    //Store the normal (dumb) joystick button one state
    dumb_button1_state = !(ciaa.ciapra & 0x0080);

    //Put port into CD32 JoyPad protocol mode
    ciaa.ciaddra |= CIAF_GAMEPORT1; //Put pin6 on the port to output mode...
    ciaa.ciapra &= ~CIAF_GAMEPORT1; //...and pull it low
    custom.potgo = 0x2F00;          //Pull pin5 low

    //Read the 9 serial bits pulsing a clock signal (7 buttons + 2 ID bits)
    for (i = 0; i < 9; i++) {
      ECLOCK_DELAY(6);
      if (!(custom.potinp & 0x4000)) buttons |= (1U << i);

      //Generate a clock pulse on pin6
      ciaa.ciapra |= CIAF_GAMEPORT1;  //pull it high...
      ciaa.ciapra &= ~CIAF_GAMEPORT1; //...and pull it back low
    }

    custom.potgo = 0xFF00;           //Put pin5 back to joystick mode
    ciaa.ciaddra &= ~CIAF_GAMEPORT1; //Put pin6 back to joystick mode

    //If the signal doesn't have the CD32 JoyPad signature discard extra buttons
    if ((buttons & 0x180) != 0x100) {
      buttons &= 0x01;
      if (dumb_button1_state) buttons |= 0x02;
    }
  }
  else {
    //Read the first port (the mouse port) of the Amiga
    //*************************************************
    //Store the normal (dumb) joystick button one state
    dumb_button1_state = !(ciaa.ciapra & 0x0040);

    //Put port into CD32 JoyPad protocol mode
    ciaa.ciaddra |= CIAF_GAMEPORT0; //Put pin6 on the port to output mode...
    ciaa.ciapra &= ~CIAF_GAMEPORT0; //...and pull it low
    custom.potgo = 0xF200;          //Pull pin5 low

    //Read the 9 serial bits pulsing a clock signal (7 buttons + 2 ID bits)
    for (i = 0; i < 9; i++) {
      ECLOCK_DELAY(6);
      if (!(custom.potinp & 0x0400)) buttons |= (1U << i);

      //Generate a clock pulse on pin6
      ciaa.ciapra |= CIAF_GAMEPORT0;  //pull it high...
      ciaa.ciapra &= ~CIAF_GAMEPORT0; //...and pull it back low
    }

    custom.potgo = 0xFF00;           //Put pin5 back to joystick mode
    ciaa.ciaddra &= ~CIAF_GAMEPORT0; //Put pin6 back to joystick mode

    //If the signal doesn't have the CD32 JoyPad signature discard extra buttons
    if ((buttons & 0x180) != 0x100) {
      buttons &= 0x01;
      if (dumb_button1_state) buttons |= 0x02;
    }
  }

  return buttons;
}
///
