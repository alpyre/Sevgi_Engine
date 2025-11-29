STATIC VOID levelDisplayLoop()
{
  UWORD quitting = FALSE;
  struct GameObject* main_char = &current_level.gameobject_bank[0]->gameobjects[0];

  while (TRUE) {
    UBYTE main_char_direction;
    WORD scroll_x;
    WORD scroll_y;
    struct ScrollInfo si;
    //struct MouseState ms;

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
    //UL_VALUE(ms) = readMouse(0);

    if (keyState(RAW_ESC) && !quitting) {
      quitting = TRUE;
      color_table->state = CT_FADE_OUT;
      volume_table.state = PTVT_FADE_OUT;
    };
    if (quitting == TRUE && color_table->state == CT_IDLE && volume_table.state == PTVT_IDLE) {
      break;
    }

    //if (JOY_BUTTON1(1) || keyState(RAW_CTRL)) /* SHOOT? */;
    main_char->anim.state = AS_IDLE;
    main_char_direction = 0;

    if      (JOY_UP(1)    || keyState(RAW_UP))    { main_char->anim.state = AS_WALK; main_char_direction = UP; }
    else if (JOY_DOWN(1)  || keyState(RAW_DOWN))  { main_char->anim.state = AS_WALK; main_char_direction = DOWN; }
    if      (JOY_LEFT(1)  || keyState(RAW_LEFT))  { main_char->anim.state = AS_WALK; main_char_direction |= LEFT; }
    else if (JOY_RIGHT(1) || keyState(RAW_RIGHT)) { main_char->anim.state = AS_WALK; main_char_direction |= RIGHT; }

    if (main_char_direction) main_char->anim.direction = main_char_direction;

    #if TRUE
    // This routine below scrolls the tilemap following main_char
    scroll_x = main_char->x - *mapPosX - (SCREEN_WIDTH / 2);
    scroll_y = main_char->y - *mapPosY - (SCREEN_HEIGHT / 2);
    if (scroll_x > 0) si.right =  scroll_x;
    if (scroll_x < 0) si.left  = -scroll_x;
    if (scroll_y > 0) si.down  =  scroll_y;
    if (scroll_y < 0) si.up    = -scroll_y;
    #else
    // This routine below scrolls the tilemap with mouse inputs
    if (UL_VALUE(ms)) {
      if (ms.deltaX > 0) si.right =  ms.deltaX;
      if (ms.deltaX < 0) si.left  = -ms.deltaX;
      if (ms.deltaY > 0) si.down  =  ms.deltaY;
      if (ms.deltaY < 0) si.up    = -ms.deltaY;

      #define MAX_SCROLL_SPEED 16  // pixels per frame
      if (si.right > MAX_SCROLL_SPEED) si.right = MAX_SCROLL_SPEED;
      if (si.left  > MAX_SCROLL_SPEED) si.left  = MAX_SCROLL_SPEED;
      if (si.down  > MAX_SCROLL_SPEED) si.down  = MAX_SCROLL_SPEED;
      if (si.up    > MAX_SCROLL_SPEED) si.up    = MAX_SCROLL_SPEED;
    }
    #endif

    scroll(&si);

    updateVolume();
    updateColorTable_Partial(color_table, 1, color_table->colors);
    updateGameObjects();
    #ifdef DYNAMIC_COPPERLIST
    updateDynamicCopperList();
    #else
    #ifdef USE_CLP
    waitVBeam(8); //Make sure all color instructions on the copperlist are read
    setColorTable_CLP(color_table, CL_PALETTE, 1, color_table->colors); //No need to fade color 0
    #endif
    #endif
//    *(UWORD*)0xDFF180 = 0; //DEBUG (displays performance of the above algorithms)
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

//    *(UWORD*)0xDFF180 = 0x0F00; //DEBUG (displays performance of the above algorithms)
  }
}
