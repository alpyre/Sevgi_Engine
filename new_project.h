/******************************************************************************
 * NewProject                                                                 *
 ******************************************************************************/
//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_NewProject_{Attribute} 0x80430901
#define MUIA_NewProject_Create   0x80430901  //(...) (Listenable)
#define MUIA_NewProject_Drawer   0x80430902  //(..G)
#define MUIA_NewProject_Name     0x80430903  //(..G)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_NewProject_{Method}    0x80430901

//Public Functions
struct MUI_CustomClass* MUI_Create_NewProject(void);
