  MOVE_PH(SPR0PTH, 0),                        // CL_SPR0PTH   Set sprite pointers
  MOVE(SPR0PTL, 0),                           //               "     "      "
#if SPRI_DDFSTRT > 0x18
  MOVE(SPR1PTH, 0),                           //               "     "      "
  MOVE(SPR1PTL, 0),                           //               "     "      "
  MOVE(SPR2PTH, 0),                           //               "     "      "
  MOVE(SPR2PTL, 0),                           //               "     "      "
#if SPRI_DDFSTRT > 0x20
  MOVE(SPR3PTH, 0),                           //               "     "      "
  MOVE(SPR3PTL, 0),                           //               "     "      "
  MOVE(SPR4PTH, 0),                           //               "     "      "
  MOVE(SPR4PTL, 0),                           //               "     "      "
#if SPRI_DDFSTRT > 0x28
  MOVE(SPR5PTH, 0),                           //               "     "      "
  MOVE(SPR5PTL, 0),                           //               "     "      "
  MOVE(SPR6PTH, 0),                           //               "     "      "
  MOVE(SPR6PTL, 0),                           //               "     "      "
#if SPRI_DDFSTRT > 0x30
  MOVE(SPR7PTH, 0),                           //               "     "      "
  MOVE(SPR7PTL, 0),                           //               "     "      "
#endif // SPRI_DDFSTRT > 0x30
#endif // SPRI_DDFSTRT > 0x28
#endif // SPRI_DDFSTRT > 0x20
#endif // SPRI_DDFSTRT > 0x18
  MOVE_PH(SPR0POS, 0),                        // CL_SPR0POS   Set sprite controls
  MOVE(SPR0CTL, 0),                           //               "     "      "
#if SPRI_DDFSTRT > 0x18
  MOVE(SPR1POS, 0),                           //               "     "      "
  MOVE(SPR1CTL, 0),                           //               "     "      "
  MOVE(SPR2POS, 0),                           //               "     "      "
  MOVE(SPR2CTL, 0),                           //               "     "      "
#if SPRI_DDFSTRT > 0x20
  MOVE(SPR3POS, 0),                           //               "     "      "
  MOVE(SPR3CTL, 0),                           //               "     "      "
  MOVE(SPR4POS, 0),                           //               "     "      "
  MOVE(SPR4CTL, 0),                           //               "     "      "
#if SPRI_DDFSTRT > 0x28
  MOVE(SPR5POS, 0),                           //               "     "      "
  MOVE(SPR5CTL, 0),                           //               "     "      "
  MOVE(SPR6POS, 0),                           //               "     "      "
  MOVE(SPR6CTL, 0),                           //               "     "      "
#if SPRI_DDFSTRT > 0x30
  MOVE(SPR7POS, 0),                           //               "     "      "
  MOVE(SPR7CTL, 0),                           //               "     "      "
#endif // SPRI_DDFSTRT > 0x30
#endif // SPRI_DDFSTRT > 0x28
#endif // SPRI_DDFSTRT > 0x20
#endif // SPRI_DDFSTRT > 0x18
