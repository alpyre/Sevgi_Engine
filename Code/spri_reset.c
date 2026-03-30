MOVE(SPR0CTL, 0),                           //              Minimal sprite reset
#if AVAIL_HSPRITES > 1
MOVE(SPR1CTL, 0),                           //               "     "        "
MOVE(SPR2CTL, 0),                           //               "     "        "
MOVE(SPR3CTL, 0),                           //               "     "        "
MOVE(SPR4CTL, 0),                           //               "     "        "
#if AVAIL_HSPRITES > 5
MOVE(SPR5CTL, 0),                           //               "     "        "
MOVE(SPR6CTL, 0),                           //               "     "        "
#if AVAIL_HSPRITES > 7
MOVE(SPR7CTL, 0),                           //               "     "        "
#endif // AVAIL_HSPRITES > 7
#endif // AVAIL_HSPRITES > 5
#endif // AVAIL_HSPRITES > 1
