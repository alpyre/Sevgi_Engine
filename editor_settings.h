/******************************************************************************
 * EditorSettings                                                             *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_EditorSettings_{Attribute} 0x80430A01
#define MUIA_EditorSettings_IDE      0x80430A01
#define MUIA_EditorSettings_Compiler 0x80430A02
#define MUIA_EditorSettings_Output   0x80430A03

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_EditorSettings_{Method}    0x80430A01

//Public Functions
struct MUI_CustomClass* MUI_Create_EditorSettings(void);
//<YOUR SUBCLASS FUNCTIONS HERE>

//getting MUIA_EditorSettings_Compiler will return one of these below:
enum {
  COMPILER_GCC,
  COMPILER_SAS_C
};
