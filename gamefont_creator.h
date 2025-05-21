/******************************************************************************
 * GamefontCreator                                                            *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_GamefontCreator_{Attribute} 0x80430401
#define MUIA_GamefontCreator_AssetsDrawer 0x80430401

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_GamefontCreator_{Method}    0x80430401

//Public Functions
struct MUI_CustomClass* MUI_Create_GamefontCreator(void);

/*
  STRIP FONT FILE FORMAT DESIGN:
    "GAMEFONT"      // eight char identifier
    "Stripname.iff" // filename string to strip image (NULL terminated)
    UBYTE type      // fixed or proportional (0/1)
    UBYTE width     // width for fixed (width of space char for proportional)
    UBYTE height    // height of the gamefont
    UBYTE spacing   // space between every letter
    UBYTE start     // how many letters this strip contains beginning from...
    UBYTE end       // ..ascii start to ascii end!
    // file ends here for type fixed
    // for proportional:
    struct {
      UWORD offset;
    }letter[end - start + 2];
*/
