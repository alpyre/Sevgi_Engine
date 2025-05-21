/******************************************************************************
 * GameSettings                                                               *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_GameSettings_{Attribute} 0x80430801
#define MUIA_GameSettings_Edited   0x80430801 //(.SG)
#define MUIA_GameSettings_AGACheck 0x80430802 //(..G)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_GameSettings_{Method}    0x80430801
#define MUIM_GameSettings_Load    0x80430801
#define MUIM_GameSettings_Save    0x80430802

//Public Functions
struct MUI_CustomClass* MUI_Create_GameSettings(void);
