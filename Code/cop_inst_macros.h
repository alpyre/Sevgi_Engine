#ifndef COPPERLIST_INSTRUCTION_MACROS_H
#define COPPERLIST_INSTRUCTION_MACROS_H

// Storage type for a copperlist instruction
typedef ULONG CLINST;

#define CI_BFDB_MASK  0x00008000L // Blitter finished disable bit
#define CI_WAIT_MASK  0x00017FFEL
#define CI_SKIP_MASK  0x00017FFFL
#define CI_MOVE_MASK  0x01FEFFFFL

// WARNING: These macros are meant to be evaluated by the precompiler!
//          So you MUST use them with constant (at compile time) values only if
//          you are using them in copperList_Instructions array initialization.
#define WAIT(x,y) (((ULONG)(y) << 24) | ((ULONG)(x) << 16) | CI_WAIT_MASK | CI_BFDB_MASK)
#define SKIP(x,y) (((ULONG)(y) << 24) | ((ULONG)(x) << 16) | CI_SKIP_MASK | CI_BFDB_MASK)
#define MOVE(r,v) ((((ULONG)(r) << 16) | (ULONG)(v)) & CI_MOVE_MASK)
#define END 0xFFFFFFFEL
#define NOOP WAIT(0,0)

// These two makes copper to also wait for blitter to finish
#define WAITB(x,y) (((ULONG)(y) << 24) | ((ULONG)(x) << 16) | CI_WAIT_MASK)
#define SKIPB(x,y) (((ULONG)(y) << 24) | ((ULONG)(x) << 16) | CI_SKIP_MASK)

// Place holder instructions
#define WAIT_PH(x,y) (((ULONG)(y) << 24) | ((ULONG)(x) << 16) | 0x10000 | CI_BFDB_MASK)
#define WAITB_PH(x,y) (((ULONG)(y) << 24) | ((ULONG)(x) << 16) | 0x10000)
#define SKIP_PH(x,y) (((ULONG)(y) << 24) | ((ULONG)(x) << 16) | 0x10001 | CI_BFDB_MASK)
#define SKIPB_PH(x,y) (((ULONG)(y) << 24) | ((ULONG)(x) << 16) | 0x10001)
#define MOVE_PH(r,v) (((((ULONG)(r) << 16) | (ULONG)(v)) & CI_MOVE_MASK) | 0x80000000)
#define END_PH 0xFFFF8000L
#define NOOP_PH WAIT_PH(0,0)

//Writable Amiga Display Hardware Register addresses
#define VPOSW    0xDFF02A
#define VHPOSW   0xDFF02C
#define STREQU   0xDFF038
#define STRVBL   0xDFF03A
#define STRHOR   0xDFF03C
#define STRLONG  0xDFF03E
#define BLTCON0  0xDFF040
#define BLTCON1  0xDFF042
#define BLTAFWM  0xDFF044
#define BLTALWM  0xDFF046
#define BLTCPTH  0xDFF048
#define BLTCPTL  0xDFF04A
#define BLTBPTH  0xDFF04C
#define BLTBPTL  0xDFF04E
#define BLTAPTH  0xDFF050
#define BLTAPTL  0xDFF052
#define BLTDPTH  0xDFF054
#define BLTDPTL  0xDFF056
#define BLTSIZE  0xDFF058
#define BLTCON0L 0xDFF05A
#define BLTSIZV  0xDFF05C
#define BLTSIZH  0xDFF05E
#define BLTCMOD  0xDFF060
#define BLTBMOD  0xDFF062
#define BLTAMOD  0xDFF064
#define BLTDMOD  0xDFF066
#define BLTCDAT  0xDFF070
#define BLTBDAT  0xDFF072
#define BLTADAT  0xDFF074
#define SPRHDAT  0xDFF078
#define BPLHDAT  0xDFF07A
#define COP1LCH  0xDFF080
#define COP1LCL  0xDFF082
#define COP2LCH  0xDFF084
#define COP2LCL  0xDFF086
#define COPJMP1  0xDFF088
#define COPJMP2  0xDFF08A
#define COPINS   0xDFF08C
#define DIWSTRT  0xDFF08E
#define DIWSTOP  0xDFF090
#define DDFSTRT  0xDFF092
#define DDFSTOP  0xDFF094
#define DMACON   0xDFF096
#define CLXCON   0xDFF098
#define INTENA   0xDFF09A
#define INTREQ   0xDFF09C
#define BPL1PTH  0xDFF0E0
#define BPL1PTL  0xDFF0E2
#define BPL2PTH  0xDFF0E4
#define BPL2PTL  0xDFF0E6
#define BPL3PTH  0xDFF0E8
#define BPL3PTL  0xDFF0EA
#define BPL4PTH  0xDFF0EC
#define BPL4PTL  0xDFF0EE
#define BPL5PTH  0xDFF0F0
#define BPL5PTL  0xDFF0F2
#define BPL6PTH  0xDFF0F4
#define BPL6PTL  0xDFF0F6
#define BPL7PTH  0xDFF0F8
#define BPL7PTL  0xDFF0FA
#define BPL8PTH  0xDFF0FC
#define BPL8PTL  0xDFF0FE
#define BPLCON0  0xDFF100
#define BPLCON1  0xDFF102
#define BPLCON2  0xDFF104
#define BPLCON3  0xDFF106
#define BPL1MOD  0xDFF108
#define BPL2MOD  0xDFF10A
#define BPLCON4  0xDFF10C
#define CLXCON2  0xDFF10E
#define BPL1DAT  0xDFF110
#define BPL2DAT  0xDFF112
#define BPL3DAT  0xDFF114
#define BPL4DAT  0xDFF116
#define BPL5DAT  0xDFF118
#define BPL6DAT  0xDFF11A
#define BPL7DAT  0xDFF11C
#define BPL8DAT  0xDFF11E
#define SPR0PTH  0xDFF120
#define SPR0PTL  0xDFF122
#define SPR1PTH  0xDFF124
#define SPR1PTL  0xDFF126
#define SPR2PTH  0xDFF128
#define SPR2PTL  0xDFF12A
#define SPR3PTH  0xDFF12C
#define SPR3PTL  0xDFF12E
#define SPR4PTH  0xDFF130
#define SPR4PTL  0xDFF132
#define SPR5PTH  0xDFF134
#define SPR5PTL  0xDFF136
#define SPR6PTH  0xDFF138
#define SPR6PTL  0xDFF13A
#define SPR7PTH  0xDFF13C
#define SPR7PTL  0xDFF13E
#define SPR0POS  0xDFF140
#define SPR0CTL  0xDFF142
#define SPR0DATA 0xDFF144
#define SPR0DATB 0xDFF146
#define SPR1POS  0xDFF148
#define SPR1CTL  0xDFF14A
#define SPR1DATA 0xDFF14C
#define SPR1DATB 0xDFF14E
#define SPR2POS  0xDFF150
#define SPR2CTL  0xDFF152
#define SPR2DATA 0xDFF154
#define SPR2DATB 0xDFF156
#define SPR3POS  0xDFF158
#define SPR3CTL  0xDFF15A
#define SPR3DATA 0xDFF15C
#define SPR3DATB 0xDFF15E
#define SPR4POS  0xDFF160
#define SPR4CTL  0xDFF162
#define SPR4DATA 0xDFF164
#define SPR4DATB 0xDFF166
#define SPR5POS  0xDFF168
#define SPR5CTL  0xDFF16A
#define SPR5DATA 0xDFF16C
#define SPR5DATB 0xDFF16E
#define SPR6POS  0xDFF170
#define SPR6CTL  0xDFF172
#define SPR6DATA 0xDFF174
#define SPR6DATB 0xDFF176
#define SPR7POS  0xDFF178
#define SPR7CTL  0xDFF17A
#define SPR7DATA 0xDFF17C
#define SPR7DATB 0xDFF17E
#define COLOR00  0xDFF180
#define COLOR01  0xDFF182
#define COLOR02  0xDFF184
#define COLOR03  0xDFF186
#define COLOR04  0xDFF188
#define COLOR05  0xDFF18A
#define COLOR06  0xDFF18C
#define COLOR07  0xDFF18E
#define COLOR08  0xDFF190
#define COLOR09  0xDFF192
#define COLOR10  0xDFF194
#define COLOR11  0xDFF196
#define COLOR12  0xDFF198
#define COLOR13  0xDFF19A
#define COLOR14  0xDFF19C
#define COLOR15  0xDFF19E
#define COLOR16  0xDFF1A0
#define COLOR17  0xDFF1A2
#define COLOR18  0xDFF1A4
#define COLOR19  0xDFF1A6
#define COLOR20  0xDFF1A8
#define COLOR21  0xDFF1AA
#define COLOR22  0xDFF1AC
#define COLOR23  0xDFF1AE
#define COLOR24  0xDFF1B0
#define COLOR25  0xDFF1B2
#define COLOR26  0xDFF1B4
#define COLOR27  0xDFF1B6
#define COLOR28  0xDFF1B8
#define COLOR29  0xDFF1BA
#define COLOR30  0xDFF1BC
#define COLOR31  0xDFF1BE
#define HTOTAL   0xDFF1C0
#define HSSTOP   0xDFF1C2
#define HBSTRT   0xDFF1C4
#define HBSTOP   0xDFF1C6
#define VTOTAL   0xDFF1C8
#define VSSTOP   0xDFF1CA
#define VBSTRT   0xDFF1CC
#define VBSTOP   0xDFF1CE
#define SPRHSTRT 0xDFF1D0
#define SPRHSTOP 0xDFF1D2
#define BPLHSTRT 0xDFF1D4
#define BPLHSTOP 0xDFF1D6
#define HHPOSW   0xDFF1D8
#define HHPOSR   0xDFF1DA
#define BEAMCON0 0xDFF1DC
#define HSSTRT   0xDFF1DE
#define VSSTRT   0xDFF1E0
#define HCENTER  0xDFF1E2
#define DIWHIGH  0xDFF1E4
#define BPLHMOD  0xDFF1E6
#define SPRHPTH  0xDFF1E8
#define SPRHPTL  0xDFF1EA
#define BPLHPTH  0xDFF1EC
#define BPLHPTL  0xDFF1EE
#define FMODE    0xDFF1FC

#endif /* COPPERLIST_INSTRUCTION_MACROS_H */
