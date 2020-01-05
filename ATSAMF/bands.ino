/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

#include "bands.h"

/**
 * Switch to another band. The display and frequencies are updated.
 *
 * @param up: 0 to go down (lower frequency), anything else to go up
 */
void nextband(int8_t up_or_down)
{
  state.band = (enum band) (((int8_t) state.band) + up_or_down);

  if (state.band == BAND_UNKNOWN)
    state.band = (enum band) ((byte) LAST_BAND - 1);
  else if (state.band >= LAST_BAND)
    state.band = (enum band) 0;

  setup_band();
}

/**
 * Update frequencies after a band change.
 */
void setup_band(void)
{
  state.op_freq = BAND_OP_FREQS[state.band];
  invalidate_frequencies();
}

/**
 * Store the current band in the EEPROM.
 */
void store_band(void)
{
  EEPROM.write(EEPROM_BAND, (byte) state.band);
}

// vim: tabstop=2 shiftwidth=2 expandtab:
