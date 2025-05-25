/******************************************************************************
 * AckString                                                                  *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_AckString_{Attribute} 0x80430001

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_AckString_{Method}    0x80430001

//Public Functions
struct MUI_CustomClass* MUI_Create_AckString(void);

//Some very useful definitions that are omitted in mui.h
#ifndef MUI_OMMITED_H
#define MUI_OMMITED_H
  #define MUIM_GoActive          0x8042491a
  #define MUIM_GoInactive        0x80422c0c
#endif // MUI_OMMITED_H
