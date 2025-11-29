/******************************************************************************
 * DisplayCreator                                                             *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_DisplayCreator_{Attribute} 0x80430C01
#define MUIA_DisplayCreator_PaletteSelector 0x80430C01 //(.S.)
#define MUIA_DisplayCreator_AGACheck        0x80430C02 //(.S.)
#define MUIA_DisplayCreator_CLPCheck        0x80430C03 //(.S.)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_DisplayCreator_{Method}    0x80430C01

//Public Functions
struct MUI_CustomClass* MUI_Create_DisplayCreator(void);
//<YOUR SUBCLASS FUNCTIONS HERE>
