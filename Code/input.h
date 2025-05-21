#ifndef INPUT_H
#define INPUT_H

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

#endif /* INPUT_H */
