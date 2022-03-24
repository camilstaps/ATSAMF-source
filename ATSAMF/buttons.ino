/**
 * Copyright (C) 2020-2021 Camil Staps <pa5et@camilstaps.nl>
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

#include "buttons.h"

/**
 * The ISR for buttons. Should be called regularly (ideally from a timer ISR).
 * Set PORTD to inputs and stores the state of buttons to state.inputs.
 * Checks if the rotary encoder was turned and updates state.inputs.encoders.
 * Restores PORTD to outputs.
 */
void buttons_isr(void)
{
  state.inputs.port = ~PIND;
  if (state.inputs.encoder_clock != state.inputs.encoder_last_clock) {
    if (!state.inputs.encoder_last_clock) {
      if (state.inputs.encoder_data)
        state.inputs.up = 1;
      else
        state.inputs.down = 1;
    }
    state.inputs.encoder_last_clock = state.inputs.encoder_clock;
  }
}

/**
 * Time how long the RIT button was pressed.
 * Gives feedback about the selected option.
 */
unsigned int time_rit(void)
{
  unsigned int duration = 0;
  unsigned long start_time = tcount;

  do {
    duration = tcount - start_time;
    if (duration >
#ifdef OPT_ERASE_EEPROM
        14000
#else
        11000
#endif
        ) {
      display_feedback("Cancel...");
    } else
#ifdef OPT_ERASE_EEPROM
    if (duration > 11000) {
      display_feedback("Erase EEPROM...");
      display_progress(11000, 14000, duration);
    } else
#endif
    if (duration > 8000) {
      display_feedback("Recalibrate...");
      display_progress(8000, 11000, duration);
    } else if (duration > 5000) {
      display_feedback("Change band...");
      display_progress(5000, 8000, duration);
    } else if (duration > 2000) {
      display_feedback("Set CW speed...");
      display_progress(2000, 5000, duration);
    }
    else if (duration > 500) {
      display_feedback("RIT...");
      display_progress(500, 2000, duration);
    }
    delay(1);
  } while (state.inputs.rit);
  debounce_rit();

  if (duration > 500)
    display_feedback("");
  display_clear_progress();

  return duration;
}

/**
 * Time how long the keyer button was pressed.
 * Gives feedback about the selected option.
 */
unsigned int time_keyer(void)
{
  unsigned int duration = 0;
  unsigned long start_time = tcount;

  do {
    duration = tcount - start_time;
    if (duration > 8000) {
      display_feedback("Cancel...");
    } else if (duration > 5000) {
      display_feedback("Enter memory...");
      display_progress(5000, 8000, duration);
    } else if (duration > 2000) {
      display_feedback("Tune mode...");
      display_progress(2000, 5000, duration);
    } else if (duration > 500) {
      if (state.beacon)
        display_feedback("Beacon");
      else
        display_feedback("Send memory");
      display_progress(500, 2000, duration);
    }
    delay(1);
  } while (state.inputs.keyer); // wait until the bit goes high.
  debounce_keyer();

  if (duration > 500)
    display_feedback("");
  display_clear_progress();

  return duration;
}

/**
 * Time how long the encoder button was pressed.
 * Gives feedback about the selected option.
 */
unsigned int time_encoder_button(void)
{
  unsigned int duration = 0;
  unsigned long start_time = tcount;

  do {
    duration = tcount - start_time;
    if (duration > 1000)
      display_feedback("DFE...");
    else if (duration > 500) {
      display_feedback("Tuning step...");
      display_progress(500, 1000, duration);
    }
    delay(1);
  } while (state.inputs.encoder_button);
  debounce_encoder_button();

  if (duration > 500)
    display_feedback("");
  display_clear_progress();

  return duration;
}

/**
 * Debounce the keyer button.
 */
void debounce_keyer(void)
{
  do
    delay(50);
  while (state.inputs.keyer);
}

/**
 * Debounce the RIT button.
 */
void debounce_rit(void)
{
  do
    delay(50);
  while (state.inputs.rit);
}

/**
 * Debounce the rotary encoder button.
 */
void debounce_encoder_button(void)
{
  do
    delay(50);
  while (state.inputs.encoder_button);
}

/**
 * Check if the rotary encoder was turned downwards.
 * After checking, the status will be cleared.
 *
 * @return 1 if it was, 0 if not.
 */
byte rotated_down(void)
{
  if (!state.inputs.down)
    return 0;
  state.inputs.down = 0;
  return 1;
}

/**
 * Check if the rotary encoder was turned upwards.
 * After checking, the status will be cleared.
 *
 * @return 1 if it was, 0 if not.
 */
byte rotated_up(void)
{
  if (!state.inputs.up)
    return 0;
  state.inputs.up = 0;
  return 1;
}

// vim: tabstop=2 shiftwidth=2 expandtab:
