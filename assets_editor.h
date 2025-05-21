/******************************************************************************
 * AssetsEditor                                                               *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_AssetsEditor_{Attribute} 0x80430001
#define MUIA_AssetsEditor_NumLevels       0x80430701 //(..G)
#define MUIA_AssetsEditor_NumTextfonts    0x80430702 //(..G)
#define MUIA_AssetsEditor_NumGamefonts    0x80430703 //(..G)
#define MUIA_AssetsEditor_PaletteSelector 0x80430704 //(.S.)
#define MUIA_AssetsEditor_BitmapSelector  0x80430705 //(.S.)
#define MUIA_AssetsEditor_Edited          0x80430706 //(.SG)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_AssetsEditor_{Method}    0x80430001
#define MUIM_AssetsEditor_Reset        0x80430700
#define MUIM_AssetsEditor_Load         0x8043070A
#define MUIM_AssetsEditor_Save         0x8043070B

//Public Functions
struct MUI_CustomClass* MUI_Create_AssetsEditor(void);
