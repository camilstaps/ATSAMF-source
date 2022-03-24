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

#include "memory.h"

/**
 * Store the buffer in EEPROM.
 *
 * @param nr the index to store the message in.
 */
void store_memory(byte nr)
{
  int addr = MEMORY_EEPROM_START + nr * MEMORY_LENGTH;
  for (byte i = 0; i < MEMORY_LENGTH; i++)
    EEPROM.write(addr++, buffer[i]);
}

/**
 * Load a message memory from EEPROM into the RAM buffer.
 *
 * @param nr the index of the message to load.
 */
void load_memory(byte nr)
{
  int addr = MEMORY_EEPROM_START + nr * MEMORY_LENGTH;
  for (byte i = 0; i < MEMORY_LENGTH; i++)
    buffer[i] = EEPROM.read(addr++);
}

/**
 * Prepare the buffer to transmit. This replaces all 0x00 (space) and 0xff
 * (empty) from the end of the buffer with 0xff, s.t. the first 0xff that is
 * encountered from the start of the buffer marks the end of the message.
 */
void prepare_buffer_for_tx()
{
  for (byte i = MEMORY_LENGTH - 1; i; i--) {
    if (buffer[i] != 0x00 && buffer[i] != (char)0xff)
      return;
    buffer[i] = 0xff;
  }
}

/**
 * Plays the message buffer on the sidetone. This function only takes care of
 * timing. What actually happens is defined by key_handle_* functions.
 * See also morse() for a single character.
 */
void playback_buffer(void)
{
  prepare_buffer_for_tx();

  key_handle_start();
  for (byte i = 0; i < MEMORY_LENGTH; i++) {
    byte character = buffer[i];
    if (character == 0x00) {
      delay(state.key.dot_time);
    } else if (character == 0xff) {
      key_handle_end();
      return;
    }
    morse(character);
  }
  key_handle_end();
}

/**
 * Clear the message buffer.
 */
void empty_buffer(void)
{
  for (byte i = 0; i < MEMORY_LENGTH; i++)
    buffer[i] = 0xff;
}

// vim: tabstop=2 shiftwidth=2 expandtab:
