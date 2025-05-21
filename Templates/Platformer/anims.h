#ifndef ANIMS_H
#define ANIMS_H

#include "level.h"
#include "tiles.h"
#include "tilemap.h"
#include "collisions.h"
#include "gameobject.h"
#include "audio.h"

/*********************************************
 * NUMBER OF ANIMATION FUNCTIONS             *
 * NOTE: One extra for the NULL initializer! *
 *********************************************/
#define NUM_ANIMS 2

//Anim States
#define AS_IDLE 0x00
#define AS_RUN  0x01
#define AS_JUMP 0x02
#define AS_FALL 0x03
#define AS_LAND 0x04

//Anim Frames
#define AF_RIGHT_IDLE_1    0
#define AF_RIGHT_IDLE_2    1
#define AF_RIGHT_IDLE_3    2
#define AF_RIGHT_IDLE_4    3
#define AF_RIGHT_IDLE_5    4
#define AF_RIGHT_IDLE_6    5
#define AF_RIGHT_IDLE_7    6
#define AF_RIGHT_IDLE_8    7
#define AF_RIGHT_IDLE_9    8
#define AF_RIGHT_IDLE_10   9
#define AF_RIGHT_IDLE_11  10
#define AF_RIGHT_IDLE_12  11
#define AF_RIGHT_IDLE_13  12
#define AF_RIGHT_IDLE_14  13
#define AF_RIGHT_IDLE_15  14
#define AF_RIGHT_IDLE_16  15
#define AF_RIGHT_IDLE_17  16
#define AF_RIGHT_IDLE_18  17

#define AF_LEFT_IDLE_1    59
#define AF_LEFT_IDLE_2    60
#define AF_LEFT_IDLE_3    61
#define AF_LEFT_IDLE_4    62
#define AF_LEFT_IDLE_5    63
#define AF_LEFT_IDLE_6    64
#define AF_LEFT_IDLE_7    65
#define AF_LEFT_IDLE_8    66
#define AF_LEFT_IDLE_9    67
#define AF_LEFT_IDLE_10   68
#define AF_LEFT_IDLE_11   69
#define AF_LEFT_IDLE_12   70
#define AF_LEFT_IDLE_13   71
#define AF_LEFT_IDLE_14   72
#define AF_LEFT_IDLE_15   73
#define AF_LEFT_IDLE_16   74
#define AF_LEFT_IDLE_17   75
#define AF_LEFT_IDLE_18   76

#define AF_RIGHT_RUN_1    18
#define AF_RIGHT_RUN_2    19
#define AF_RIGHT_RUN_3    20
#define AF_RIGHT_RUN_4    21
#define AF_RIGHT_RUN_5    22
#define AF_RIGHT_RUN_6    23
#define AF_RIGHT_RUN_7    24
#define AF_RIGHT_RUN_8    25
#define AF_RIGHT_RUN_9    26
#define AF_RIGHT_RUN_10   27
#define AF_RIGHT_RUN_11   28
#define AF_RIGHT_RUN_12   29
#define AF_RIGHT_RUN_13   30
#define AF_RIGHT_RUN_14   31
#define AF_RIGHT_RUN_15   32
#define AF_RIGHT_RUN_16   33
#define AF_RIGHT_RUN_17   34
#define AF_RIGHT_RUN_18   35
#define AF_RIGHT_RUN_19   36
#define AF_RIGHT_RUN_20   37
#define AF_RIGHT_RUN_21   38
#define AF_RIGHT_RUN_22   39
#define AF_RIGHT_RUN_23   30
#define AF_RIGHT_RUN_24   41

#define AF_LEFT_RUN_1     77
#define AF_LEFT_RUN_2     78
#define AF_LEFT_RUN_3     79
#define AF_LEFT_RUN_4     80
#define AF_LEFT_RUN_5     81
#define AF_LEFT_RUN_6     82
#define AF_LEFT_RUN_7     83
#define AF_LEFT_RUN_8     84
#define AF_LEFT_RUN_9     85
#define AF_LEFT_RUN_10    86
#define AF_LEFT_RUN_11    87
#define AF_LEFT_RUN_12    88
#define AF_LEFT_RUN_13    89
#define AF_LEFT_RUN_14    90
#define AF_LEFT_RUN_15    91
#define AF_LEFT_RUN_16    92
#define AF_LEFT_RUN_17    93
#define AF_LEFT_RUN_18    94
#define AF_LEFT_RUN_19    95
#define AF_LEFT_RUN_20    96
#define AF_LEFT_RUN_21    97
#define AF_LEFT_RUN_22    98
#define AF_LEFT_RUN_23    99
#define AF_LEFT_RUN_24   100

#define AF_RIGHT_JUMP_1   42
#define AF_RIGHT_JUMP_2   43
#define AF_RIGHT_JUMP_3   44
#define AF_RIGHT_JUMP_4   45
#define AF_RIGHT_JUMP_5   46
#define AF_RIGHT_JUMP_6   47
#define AF_RIGHT_JUMP_7   48
#define AF_RIGHT_JUMP_8   49
#define AF_RIGHT_FALL_1   50
#define AF_RIGHT_FALL_2   51
#define AF_RIGHT_FALL_3   52
#define AF_RIGHT_FALL_4   53
#define AF_RIGHT_FALL_5   54
#define AF_RIGHT_FALL_6   55
#define AF_RIGHT_LAND_1   56
#define AF_RIGHT_LAND_2   57
#define AF_RIGHT_LAND_3   58

#define AF_LEFT_JUMP_1   101
#define AF_LEFT_JUMP_2   102
#define AF_LEFT_JUMP_3   103
#define AF_LEFT_JUMP_4   104
#define AF_LEFT_JUMP_5   105
#define AF_LEFT_JUMP_6   106
#define AF_LEFT_JUMP_7   107
#define AF_LEFT_JUMP_8   108
#define AF_LEFT_FALL_1   109
#define AF_LEFT_FALL_2   110
#define AF_LEFT_FALL_3   111
#define AF_LEFT_FALL_4   112
#define AF_LEFT_FALL_5   113
#define AF_LEFT_FALL_6   114
#define AF_LEFT_LAND_1   115
#define AF_LEFT_LAND_2   116
#define AF_LEFT_LAND_3   117

//TO ACCESS ANIMS ADD THIS LINE INTO YOUR .c fILE:
//extern VOID (*animFunction[NUM_ANIMS])(struct GameObject*);

#endif /* ANIMS_H */
