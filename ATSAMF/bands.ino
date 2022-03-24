/**
 * Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl>
 *
 * This is software for the ATSAMF rig. Fore more information, see the
 * README.md file.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */

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
