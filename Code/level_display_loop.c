/******************************************************************************
 * WARNING: This file is not to be compiled to its own module. It is just     *
 * inculed by display_level.c.                                                *
 * Only use globals from display_level.c and do NOT declare new globals here  *
 ******************************************************************************/
STATIC VOID levelDisplayLoop()
{
  UWORD quitting = FALSE;
  struct ScrollInfo si = {0};
  struct MouseState ms = {0};

  while (TRUE) {
    UWORD pixels;

    #if !defined DOUBLE_BUFFER && defined DYNAMIC_COPPERLIST
    if (new_frame_flag % 2) {
      //Unsync detected. Re-scync with the copperlist set by the copper
      if (CopperList == CopperList1)
        CopperList = CopperList2;
      else
        CopperList = CopperList1;
    }
    #endif //DYNAMIC_COPPERLIST
    new_frame_flag = 1;

    doKeyboardIO();
    UL_VALUE(ms) = readMouse(0);

    if (keyState(RAW_ESC) && !quitting) {
      quitting = TRUE;
      color_table->state = CT_FADE_OUT;
      volume_table.state = PTVT_FADE_OUT;
    }
    if (quitting == TRUE && color_table->state == CT_IDLE && volume_table.state == PTVT_IDLE) {
      break;
    }

    if (UL_VALUE(ms)) {
      if (ms.deltaX > 0) si.right =  ms.deltaX;
      if (ms.deltaX < 0) si.left  = -ms.deltaX;
      if (ms.deltaY > 0) si.down  =  ms.deltaY;
      if (ms.deltaY < 0) si.up    = -ms.deltaY;
    }
    else {
      if (JOY_BUTTON1(1) || keyState(RAW_CTRL)) pixels = MAX_SCROLL_SPEED; else pixels = 1;

      if (JOY_UP(1)    || keyState(RAW_UP))    si.up    = pixels;
      if (JOY_DOWN(1)  || keyState(RAW_DOWN))  si.down  = pixels;
      if (JOY_LEFT(1)  || keyState(RAW_LEFT))  si.left  = pixels;
      if (JOY_RIGHT(1) || keyState(RAW_RIGHT)) si.right = pixels;
    }

    scroll(&si);

    updateVolume();
    updateColorTable_Partial(color_table, 1, color_table->colors);
    updateGameObjects();
    #ifdef DYNAMIC_COPPERLIST
    updateDynamicCopperList();
    #endif
//    *(WORD*)0xDFF180 = 0; //DEBUG (displays performance of the above algorithms)
    updateBOBs();

    //Wait until current frame completes
    #ifdef DOUBLE_BUFFER
    waitNextFrame();
    swapBuffers();
    #else
    waitTOF();
    #endif

    //blit the remaining secondPart tiles first thing on the next frame
    scrollRemaining(&si);

//    *(WORD*)0xDFF180 = 0x0F00; //DEBUG (displays performance of the above algorithms)
  }
}
