#ifndef KEYBOARD_H
#define KEYBOARD_H

//Amiga Raw Key Codes
#define RAW_GRAVE     0x00
#define RAW_1         0x01
#define RAW_2         0x02
#define RAW_3         0x03
#define RAW_4         0x04
#define RAW_5         0x05
#define RAW_6         0x06
#define RAW_7         0x07
#define RAW_8         0x08
#define RAW_9         0x09
#define RAW_0         0x0A
#define RAW_HYPHEN    0x0B
#define RAW_EQUALS    0x0C
#define RAW_OR        0x0D
#define RAW_NUM_0     0x0F
#define RAW_Q         0x10
#define RAW_W         0x11
#define RAW_E         0x12
#define RAW_R         0x13
#define RAW_T         0x14
#define RAW_Y         0x15
#define RAW_U         0x16
#define RAW_I         0x17
#define RAW_O         0x18
#define RAW_P         0x19
#define RAW_BRACKET_L 0x1A
#define RAW_BRACKET_R 0x1B
#define RAW_NUM_1     0x1D
#define RAW_NUM_2     0x1E
#define RAW_NUM_3     0x1F
#define RAW_A         0x20
#define RAW_S         0x21
#define RAW_D         0x22
#define RAW_F         0x23
#define RAW_G         0x24
#define RAW_H         0x25
#define RAW_J         0x26
#define RAW_K         0x27
#define RAW_L         0x28
#define RAW_SEMICOLON 0x29
#define RAW_QUOTE     0x2A
#define RAW_NUM_4     0x2D
#define RAW_NUM_5     0x2E
#define RAW_NUM_6     0x2F
#define RAW_Z         0x31
#define RAW_X         0x32
#define RAW_C         0x33
#define RAW_V         0x34
#define RAW_B         0x35
#define RAW_N         0x36
#define RAW_M         0x37
#define RAW_COMMA     0x38
#define RAW_PERIOD    0x39
#define RAW_SLASH     0x3A
#define RAW_NUM_DOT   0x3C
#define RAW_NUM_7     0x3D
#define RAW_NUM_8     0x3E
#define RAW_NUM_9     0x3F
#define RAW_SPACE     0x40
#define RAW_BACKSPACE 0x41
#define RAW_TAB       0x42
#define RAW_NUM_ENTER 0x43
#define RAW_RETURN    0x44
#define RAW_ESC       0x45
#define RAW_DEL       0x46
#define RAW_NUM_MINUS 0x4A
#define RAW_UP        0x4C
#define RAW_DOWN      0x4D
#define RAW_RIGHT     0x4E
#define RAW_LEFT      0x4F
#define RAW_F1        0x50
#define RAW_F2        0x51
#define RAW_F3        0x52
#define RAW_F4        0x53
#define RAW_F5        0x54
#define RAW_F6        0x55
#define RAW_F7        0x56
#define RAW_F8        0x57
#define RAW_F9        0x58
#define RAW_F10       0x59
#define RAW_NUM_PAR_L 0x5A
#define RAW_NUM_PAR_R 0x5B
#define RAW_NUM_DIV   0x5C
#define RAW_NUM_MUL   0x5D
#define RAW_NUM_PLUS  0x5E
#define RAW_HELP      0x5F
#define RAW_SHIFT_L   0x60
#define RAW_SHIFT_R   0x61
#define RAW_CAPSLOCK  0x62
#define RAW_CTRL      0x63
#define RAW_ALT_L     0x64
#define RAW_ALT_R     0x65
#define RAW_AMIGA_L   0x66
#define RAW_AMIGA_R   0x67

BOOL setKeyboardAccess(VOID);
VOID endKeyboardAccess(VOID);
VOID doKeyboardIO(VOID);
BOOL keyState(UBYTE);

#endif /* KEYBOARD_H */
