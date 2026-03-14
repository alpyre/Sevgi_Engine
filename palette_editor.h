/******************************************************************************
 * PaletteEditor                                                              *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_PaletteEditor_{Attribute} 0x80430501
#define MUIA_PaletteEditor_Edited 0x80430501 //(.SG)
#define MUIA_PaletteEditor_List   0x80430502 //(..G)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_PaletteEditor_{Method}    0x80430501
#define MUIM_PaletteEditor_Import_ILBM 0x80430507
#define MUIM_PaletteEditor_Export_ILBM 0x80430508
#define MUIM_PaletteEditor_Append_ILBM 0x80430509
#define MUIM_PaletteEditor_Merge_ILBM  0x8043050A
#define MUIM_PaletteEditor_LoadAs      0x8043050B
#define MUIM_PaletteEditor_SaveAs      0x8043050C
#define MUIM_PaletteEditor_Reset       0x8043050D
#define MUIM_PaletteEditor_Load        0x8043050E
#define MUIM_PaletteEditor_Save        0x8043050F

//Public Functions
struct MUI_CustomClass* MUI_Create_PaletteEditor(void);
