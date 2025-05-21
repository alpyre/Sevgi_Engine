/******************************************************************************
 * PopASLString                                                               *
 ******************************************************************************/
#include <libraries/asl.h>

//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_PopASLString_{Attribute} 0x80430FF1
#define MUIA_PopASLString_Requester      0x80430FF1 //(I..) (struct FileRequester*) // !!MANDATORY!!
#define MUIA_PopASLString_StringFunc     0x80430FF2 //(ISG) (VOID (*)(struct FileRequester*, Object*))
#define MUIA_PopASLString_StringObject   0x80430FF3 //(..G) (Object*)
#define MUIA_PopASLString_PopButton      0x80430FF4 //(..G) (Object*)
#define MUIA_PopASLString_IgnoreContents 0x80430FF5 //(ISG) (BOOL)
//All ASLFR_{*} tags are recognized
//MUIA_Image_Spec will set the image on the pop button (defaults to MUII_PopFile)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_PopASLString_{Method}    0x80430FF1

//Public Functions
struct MUI_CustomClass* MUI_Create_PopASLString(void);
//<YOUR SUBCLASS FUNCTIONS HERE>
