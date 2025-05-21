/******************************************************************************
 * PaletteSelector                                                            *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_PaletteSelector_{Attribute} 0x80430B01
#define MUIA_PaletteSelector_SourceList 0x80430B01 //(.S.)
#define MUIA_PaletteSelector_Selected   0x80430B02 //(..G) (Listenable)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_PaletteSelector_{Method}    0x80430B03

//Public Functions
struct MUI_CustomClass* MUI_Create_PaletteSelector(void);
//<YOUR SUBCLASS FUNCTIONS HERE>
