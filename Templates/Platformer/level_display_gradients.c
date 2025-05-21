/******************************************************************************
 * WARNING: This file is not to be compiled to its own module. It is just     *
 * inculed by display_level.c.                                                *
 * Only use globals from display_level.c and do NOT declare new globals here! *
 ******************************************************************************/
#define SKY_GRD_HEIGHT    172
#define SKY_GRD_COLOR_REG   9 // Between 0 and 31
#define NUM_GRADIENTS       2 // One extra for the NULL termination!

#if SKY_GRD_HEIGHT > SCREEN_HEIGHT
  #undef SKY_GRD_HEIGHT
  #define SKY_GRD_HEIGHT SCREEN_HEIGHT
#endif

STATIC BOOL prepGradients(ULONG level_num)
{
  BOOL retval = TRUE;

  #include "level_display_gradients.h"

  switch (level_num) {
    case 1:
    {
      static struct Gradient* grad_list[NUM_GRADIENTS] = {0};
      #ifdef CT_AGA
        grad_list[0] = createGradient(GRD_TYPE_AGA, SKY_GRD_COLOR_REG, SKY_GRD_HEIGHT, 0, SKY_GRD_HEIGHT, 0, GRD_BLITABLE, sky_hues_1);
      #else
        grad_list[0] = createGradient(GRD_TYPE_OCS, SKY_GRD_COLOR_REG, SKY_GRD_HEIGHT, 0, SKY_GRD_HEIGHT, 0, GRD_BLITABLE, sky_hues_1);
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
