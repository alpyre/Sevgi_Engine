/******************************************************************************
 * PaletteEditor                                                              *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_PaletteEditor_{Attribute} 0x80430501
#define MUIA_PaletteEditor_Edited 0x80430501 //(.SG)
#define MUIA_PaletteEditor_List   0x80430502 //(..G)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_PaletteEditor_{Method}    0x80430501
#define MUIM_PaletteEditor_Load 0x8043050B
#define MUIM_PaletteEditor_Save 0x8043050C

//Public Functions
struct MUI_CustomClass* MUI_Create_PaletteEditor(void);
