/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

#include "morse.h"

const byte MORSE_DIGITS[] = {M0,M1,M2,M3,M4,M5,M6,M7,M8,M9};

/**
 * Key out a character. This function only takes care of timing. What actually
 * happens is defined by the key_handle_* functions, according to the state.
 *
 * @param character the character to send.
 */
void morse(byte character)
{
  char i;

  for (i = 7; i >= 0; i--)
    if (character & (1 << i))
      break;

  for (i--; i >= 0; i--) {
    if (character & (1 << i)) {
      key_handle_dash();
      delay(state.key.dash_time);
    } else {
      key_handle_dot();
      delay(state.key.dot_time);
    }
    key_handle_dashdot_end();
    delay(state.key.dot_time);
  }

  delay(state.key.dash_time);
}

// vim: tabstop=2 shiftwidth=2 expandtab:
