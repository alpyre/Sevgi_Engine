/******************************************************************************
 * Integer Gadget                                                               *
 ******************************************************************************/
//Public Attributes
#define MUIA_Integer_Value           0x80470001 //(ISG) (LONG)
#define MUIA_Integer_Min             0x80470002 //(ISG) (LONG)
#define MUIA_Integer_Max             0x80470003 //(ISG) (LONG)
#define MUIA_Integer_Incr            0x80470004 //(ISG) (LONG)
#define MUIA_Integer_Buttons         0x80470005 //(IS.) (BOOL)
#define MUIA_Integer_Buttons_Inverse 0x80470006 //(IS.) (BOOL)
#define MUIA_Integer_Button_Inc_Text 0x80470007 //(I..) (STRPTR)
#define MUIA_Integer_Button_Dec_Text 0x80470008 //(I..) (STRPTR)
#define MUIA_Integer_Input           0x80470009 //(I..) (BOOL)
#define MUIA_Integer_124             0x8047000A //(I..) (BOOL)
#define MUIA_Integer_ASCII           0x8047000B //(I..) (BOOL)
#define MUIA_Integer_PaletteSize     0x8047000C //(I..) (BOOL)

//Public Methods
#define MUIM_Integer_Increase    0x80470001
#define MUIM_Integer_Decrease    0x80470002
#define MUIM_Integer_Keystroke   0x80470003 //NOTE: Make this private
#define MUIM_Integer_Acknowledge 0x80470004 //NOTE: Make this private

//Public Functions
struct MUI_CustomClass* MUI_Create_Integer(void);
