/******************************************************************************
 * ColorGadget                                                                *
 ******************************************************************************/
//Public Attributes
#define MUIA_ColorGadget_Red      MUIA_Colorfield_Red     //(ISG)
#define MUIA_ColorGadget_Green    MUIA_Colorfield_Green   //(ISG)
#define MUIA_ColorGadget_Blue     MUIA_Colorfield_Blue    //(ISG)
#define MUIA_ColorGadget_RGB      MUIA_Colorfield_RGB     //(ISG)
#define MUIA_ColorGadget_Index    0x80430301              //(I.G)
#define MUIA_ColorGadget_Selected 0x80430302              //(ISG)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_ColorGadget_{Method}    0x80430001

//Public Functions
struct MUI_CustomClass* MUI_Create_ColorGadget(void);
