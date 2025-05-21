///Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

//Amiga headers
#include <exec/exec.h>
#include <dos/dos.h>

//Amiga protos
#include <proto/dos.h>
#include <proto/exec.h>

/* MUI headers */
#include <libraries/mui.h>
#include <proto/muimaster.h>

#include "utility.h"
#include "toolbar.h"
///
///Globals
extern APTR g_MemoryPool;

#define NUM_IMAGE_SPECS (sizeof(g_imageFiles) / sizeof(STRPTR))
STRPTR g_imageFiles[] = {TOOLBAR_IMAGE_FILES};
STRPTR g_imageSpecs[NUM_IMAGE_SPECS] = {NULL};
///

BOOL createImageSpecs()
{
  ULONG num_specs = NUM_IMAGE_SPECS;
  ULONG i;
  UBYTE spec[256]; //WARNING! Buffer overrun potential!

  for (i = 0; i < num_specs; i++) {
    strcpy(spec, "5:");
    strcat(spec, TOOLBAR_IMAGE_DIR);
    strcat(spec, "/");
    strcat(spec, TOOLBAR_IMAGE_STYLE);
    strcat(spec, "/");
    strcat(spec, g_imageFiles[i]);

    g_imageSpecs[i] = AllocPooled(g_MemoryPool, strlen(spec) + 1);
    if (g_imageSpecs[i]) {
      strcpy(g_imageSpecs[i], spec);
    }
    else {
      freeImageSpecs();
      return FALSE;
    }
  }

  return TRUE;
}

VOID freeImageSpecs()
{
  ULONG num_specs = NUM_IMAGE_SPECS;
  LONG i;
  for (i = num_specs - 1; i >= 0; i--) {
    freeString(g_imageSpecs[i]);
    g_imageSpecs[i] = NULL;
  }
}

Object* MUI_NewImageButton(ULONG spec, STRPTR short_help, ULONG disabled)
{
  return MUI_NewObject(MUIC_Image,
    MUIA_Image_Spec, g_imageSpecs[spec],
    MUIA_ShortHelp, short_help,
    MUIA_InputMode, MUIV_InputMode_RelVerify,
    MUIA_Frame, MUIV_Frame_ImageButton,
    MUIA_Image_FreeVert, TRUE,
    MUIA_Image_FreeHoriz, TRUE,
    MUIA_FixHeight, TOOLBAR_BUTTON_HEIGHT,
    MUIA_FixWidth, TOOLBAR_BUTTON_WIDTH,
    MUIA_InnerLeft, 0,
    MUIA_InnerRight, 0,
    MUIA_InnerTop, 0,
    MUIA_InnerBottom, 0,
    MUIA_Disabled, disabled ? TRUE : FALSE,
  TAG_END);
}

Object* MUI_NewImageButtonSeparator()
{
  return MUI_NewObject(MUIC_Rectangle,
    MUIA_Rectangle_VBar, TRUE,
    MUIA_FixWidth, 6,
    MUIA_FixHeight, TOOLBAR_BUTTON_HEIGHT,
  TAG_END);
}

Object* MUI_HorizontalSeparator()
{
  return MUI_NewObject(MUIC_Rectangle,
    MUIA_Rectangle_HBar, TRUE,
    MUIA_FixHeight, 4,
  TAG_END);
}
