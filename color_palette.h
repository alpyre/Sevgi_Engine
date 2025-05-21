/******************************************************************************
 * ColorPalette                                                               *
 ******************************************************************************/
#define DEFAULT_PALLETTE_SIZE 8

//Public Attributes
#define MUIA_ColorPalette_NumColors   0x80430201  // (ISG)
#define MUIA_ColorPalette_Selected    0x80430202  // (ISG)
#define MUIA_ColorPalette_Palette     0x80430203  // (ISG)
#define MUIA_ColorPalette_RGB         0x80430204  // (.SG)
#define MUIA_ColorPalette_DisplayOnly 0x80430205  // (I..)
#define MUIA_ColorPalette_Edited      0x80430206  // (.SG)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_ColorPalette_{Method}    0x80430001

//Public Functions
struct MUI_CustomClass* MUI_Create_ColorPalette(void);
VOID freePalette(UBYTE* palette);
