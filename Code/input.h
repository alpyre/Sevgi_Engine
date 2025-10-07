#ifndef INPUT_H
#define INPUT_H

#include <hardware/custom.h>
#include <hardware/cia.h>

//for joystick on port 0 (mouse port) a = 0
//for joystick on port 1 (joystick port) a = 1
#define JOY_LEFT(a)    (custom.joy##a##dat & 512)
#define JOY_RIGHT(a)   (custom.joy##a##dat & 2)
#define JOY_UP(a)      (((custom.joy##a##dat << 1) ^ custom.joy##a##dat) & 512)
#define JOY_DOWN(a)    (((custom.joy##a##dat << 1) ^ custom.joy##a##dat) & 2)
#define JOY_BUTTON1(a) !(ciaa.ciapra & (a ? 0x0080 : 0x0040))
#define JOY_BUTTON2(a) !(custom.potinp & (a ? 0x4000 : 0x0400))

#define LEFT_MOUSE_BUTTON   1
#define MIDDLE_MOUSE_BUTTON 2
#define RIGHT_MOUSE_BUTTON  4

#define UL_VALUE(a) (*(ULONG*)&a)

struct MouseState {
  UBYTE buttons;
  UBYTE pad;
  BYTE deltaX;
  BYTE deltaY;
};

ULONG readMouse(ULONG);

//CD32 Joypad Buttons
#define CD32_BLUE      0x01
#define CD32_RED       0x02
#define CD32_YELLOW    0x04
#define CD32_GREEN     0x08
#define CD32_RSHOULDER 0x10 // Fast Forward
#define CD32_LSHOULDER 0x20 // Rewind
#define CD32_PAUSE     0x40

ULONG readCD32JoyPadButtons(ULONG port);

#endif /* INPUT_H */
