/******************************************************************************
 * WARNING: This file is not to be compiled to its own module. It is just     *
 * inculed by display_level.c.                                                *
 * Only use globals from display_level.c and do NOT declare new globals here! *
 ******************************************************************************/
#define GRADIENT_HEIGHT      SCREEN_HEIGHT
#define GRADIENT_COLOR_REG   0 // Between 0 and 31
#define NUM_GRADIENTS        2 // One extra for the NULL termination!

STATIC BOOL prepGradients(ULONG level_num)
{
  BOOL retval = TRUE;
  #include "level_display_gradients.h"

/*
  switch (level_num) {
    case 1:
    {
      static struct Gradient* grad_list[NUM_GRADIENTS] = {0};
      #ifdef CT_AGA
        grad_list[0] = createGradient(GRD_TYPE_AGA, GRADIENT_COLOR_REG, GRADIENT_HEIGHT, 0, GRADIENT_HEIGHT, 0, GRD_BLITABLE, example_huelist);
      #else
        grad_list[0] = createGradient(GRD_TYPE_OCS, GRADIENT_COLOR_REG, GRADIENT_HEIGHT, 0, GRADIENT_HEIGHT, 0, GRD_BLITABLE, example_huelist);
      #endif

      //Init the color_table of the gradient
      grad_list[0]->color_table = newColorTable_GRD(grad_list[0], CT_DEFAULT_STEPS, 0);
      grad_list[0]->color_table->state = CT_FADE_IN;

      #if BOTTOM_PANEL_HEIGHT > 0
      rainbow = createRainbow(grad_list, end_Instructions);
      #else
      rainbow = createRainbow(grad_list, NULL);
      #endif

      if (!rainbow) {
        retval = FALSE;
      }
    }
    break;
  }
*/

  return retval;
}

STATIC VOID freeGradients()
{
  if (rainbow) {
    if (rainbow->gradList) {
      struct Gradient** g = rainbow->gradList;

      while (*g) {
        freeGradient(*g);
        *g = NULL;
        g++;
      }
    }

    freeRainbow(rainbow); rainbow = NULL;
  }
}
