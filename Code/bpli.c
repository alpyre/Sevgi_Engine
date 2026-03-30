  MOVE_PH(BPL1PTH, 0),                        // CL_BPL1PTH   Set bitplane addresses
  MOVE(BPL1PTL, 0),                           //               "      "       "
#if BPLI_DEPTH > 1
  MOVE(BPL2PTH, 0),                           //               "      "       "
  MOVE(BPL2PTL, 0),                           //               "      "       "
#if BPLI_DEPTH > 2
  MOVE(BPL3PTH, 0),                           //               "      "       "
  MOVE(BPL3PTL, 0),                           //               "      "       "
#if BPLI_DEPTH > 3
  MOVE(BPL4PTH, 0),                           //               "      "       "
  MOVE(BPL4PTL, 0),                           //               "      "       "
#if BPLI_DEPTH > 4
  MOVE(BPL5PTH, 0),                           //               "      "       "
  MOVE(BPL5PTL, 0),                           //               "      "       "
#if BPLI_DEPTH > 5
  MOVE(BPL6PTH, 0),                           //               "      "       "
  MOVE(BPL6PTL, 0),                           //               "      "       "
#if BPLI_DEPTH > 6
  MOVE(BPL7PTH, 0),                           //               "      "       "
  MOVE(BPL7PTL, 0),                           //               "      "       "
#if BPLI_DEPTH > 7
  MOVE(BPL8PTH, 0),                           //               "      "       "
  MOVE(BPL8PTL, 0),                           //               "      "       "
#endif // BPLI_DEPTH > 7
#endif // BPLI_DEPTH > 6
#endif // BPLI_DEPTH > 5
#endif // BPLI_DEPTH > 4
#endif // BPLI_DEPTH > 3
#endif // BPLI_DEPTH > 2
#endif // BPLI_DEPTH > 1
