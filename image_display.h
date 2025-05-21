/******************************************************************************
 * ImageDisplay                                                               *
 ******************************************************************************/
#include "ILBM_image.h"
#include "chunky_image.h"
#include "image_spec.h"

//Public Attributes
//<SUBCLASS ATTRIBUTES HERE> ex: #define MUIA_ImageDisplay_{Attribute} 0x80430401
#define MUIA_ImageDisplay_Sheet     0x80430401 //(ISG)
#define MUIA_ImageDisplay_ImageSpec 0x80430402 //(ISG)
#define MUIA_ImageDisplay_Scale     0x80430403 //(ISG)

//Public Methods
//<SUBCLASS METHODS HERE>    ex: #define MUIM_ImageDisplay_{Method}    0x80430401
#define MUIM_ImageDisplay_Update    0x80430401

//Public Functions
struct MUI_CustomClass* MUI_Create_ImageDisplay(void);
