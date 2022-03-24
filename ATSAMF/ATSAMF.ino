/**
 * Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl>
 *
 * This code is based on the original software for the SODA POP by Steven
 * Weber, which was copyright (C) 2017 Steven Weber KD1JV
 * <steve.kd1jv@gmail.com>.
 *
 ****************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 *****************************************************************************
 *
 * This is software for the ATSAMF rig. Fore more information, see the
 * README.md file.
 *
 * In order to compile, <Si5351Arduino-master> must be in the Arduino libary.
 * This library is available from http://www.etherkit.com.
 *
 * Pin functions:
 *  A0 - tone
 *  A1 - mute/qsk
 *  A2 - dash paddle
 *  A3 - dot paddle
 *  A4 - SDA TWI
 *  A5 - SCK TWI
 * D13 - TX enable
 *
 * Switch input bits locations:
 * 0 Encoder A
 * 1 Encoder B
 * 2 Encoder button
 * 3 RIT
 * 4 Keyer button
 */

#include <EEPROM.h>
#include <Wire.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include <si5351.h>

#include "ATSAMF.h"

#define SI5351_CLK_RX SI5351_CLK0
#define SI5351_CLK_TX SI5351_CLK1

#define RX_ON_TX_ON   0xfc
#define RX_OFF_TX_ON  0xfd
#define RX_ON_TX_OFF  0xfe
#define RX_OFF_TX_OFF 0xff

#define IF_DEFAULT 491480000ul

/* Global variables */
struct atsamf state;
volatile unsigned long tcount = 0;
const byte tuning_blinks[] = TUNING_STEP_DIGITS;
byte errno;

byte memory_index;
char buffer[MEMORY_LENGTH];

byte dfe_position;
unsigned long dfe_freq;

/* Local variables */
Si5351 si5351;
long cal_value = 15000;

unsigned long IFfreq;

const long tuning_steps[] = TUNING_STEPS;

byte memory_index_character;
byte memory_pointer;

byte dfe_character;

/**
 * The Timer1 ISR. Keeps track of a global timer, tcount, and calls ISRs for
 * all parts of the system.
 * See also key_isr(), buttons_isr() and display_isr().
 */
ISR (TIMER1_COMPA_vect)
{
  ++tcount;
  key_isr();
  buttons_isr();
}

static uint8_t confirm (const char *question)
{
  display_question(question);

  while (1) {
    if (state.inputs.keyer) {
      debounce_keyer();
      return 1;
    } else if (state.inputs.rit) {
      debounce_rit();
      return 0;
    }
    delay(1);
  }
}

/**
 * Arduino's initialisation routine.
 * Sets up the device's state, the Si5351, and loads persistsent settings from
 * the EEPROM.
 */
void setup(void)
{
  state.state = S_STARTUP;

  DDRD = 0xc0; /* D0-7 */
  PORTD = 0x3b; /* pull-ups */
  DDRB = 0xff; /* D8-13 */

  pinMode(SIDETONE, OUTPUT);
  pinMode(MUTE, OUTPUT);
  pinMode(DASHin, INPUT_PULLUP);
  pinMode(DOTin, INPUT_PULLUP);

  digitalWrite(MUTE, LOW);
  digitalWrite(TXEN, LOW);
  digitalWrite(6, LOW); /* for buttons */

  display_init();

  state.key.mode = KEY_IAMBIC;
  state.key.speed = EEPROM.read(EEPROM_CW_SPEED);
  if (state.key.speed > KEY_MAX_SPEED)
    state.key.speed = WPM_DEFAULT;
  state.key.timeout = 1;
  state.key.dash = 0;
  state.key.dot = 0;
  load_cw_speed();

  si5351.init(SI5351_CRYSTAL_LOAD_6PF, 0); //set PLL xtal load
  enable_rx_tx(RX_ON_TX_OFF);

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 238;
  TCCR1B = 0x0b;
  TIMSK1 |= 1 << OCIE1A;
  interrupts();

  state.band = (enum band) EEPROM.read(EEPROM_BAND);
  if (state.band == BAND_UNKNOWN) {
    state.band = (enum band) 0;
    state.state = S_CALIBRATION_CORRECTION;
  } else {
    state.state = S_DEFAULT;
  }

  fetch_calibration_data(); //load calibration data
  si5351.set_correction(cal_value); //correct the clock chip error
  state.tuning_step = 0;
  setup_band();

  display_delay(1500);

  if (state.state == S_DEFAULT) {
    state.state = S_STARTUP; // To show the band on startup
    invalidate_display();
    display_delay(1500);
    state.state = S_DEFAULT;
  } else if (state.state == S_CALIBRATION_CORRECTION) {
    calibration_set_correction();
    enable_rx_tx(RX_OFF_TX_ON);
  }

  invalidate_display();
  digitalWrite(MUTE, HIGH);

  if (digitalRead(DASHin) == LOW)
    state.key.mode = KEY_STRAIGHT;

  state.beacon = 0;

  power_adc_disable();
  power_spi_disable();
  set_sleep_mode(SLEEP_MODE_IDLE);
}

/**
 * Arduino's main loop. Checks what state we are in and calls the corresponding
 * loop_* function.
 */
void loop(void)
{
  if (!((uint8_t)tcount & 0x7f))
    display_isr();

  switch (state.state) {
    case S_DEFAULT:                 loop_default(); break;
    case S_KEYING:                  loop_keying(); break;
    case S_ADJUST_CS:               loop_adjust_cs(); break;
    case S_TUNE:                    loop_tune(); break;
    case S_CHANGE_BAND:             loop_change_band(); break;
    case S_DFE:                     loop_dfe(); break;
    case S_MEM_ENTER_WAIT:          loop_mem_enter_wait(); break;
    case S_MEM_ENTER:               loop_mem_enter(); break;
    case S_MEM_ENTER_REVIEW:        loop_mem_enter_review(); break;
    case S_MEM_SEND_WAIT:           loop_mem_send_wait(); break;
    case S_MEM_SEND_TX:             loop_mem_send_tx(); break;
    case S_CALIBRATION_CORRECTION:  loop_calibration_correction(); break;
    case S_CALIBRATION_PEAK_IF:     loop_calibration_peak_if(); break;
    case S_CALIBRATION_CHANGE_BAND: loop_change_band(); break;
    case S_CALIBRATION_PEAK_RX:     loop_calibration_peak_rx(); break;
    case S_ERROR:                   loop_error(); break;
    default:
      error(1);
      break;
  }

  sleep_mode();
}

/**
 * Loop for the S_DEFAULT state. From here, all functions can be enabled
 * through the buttons.
 *
 * Paddle or straight key enter the S_KEYING state.
 *
 * Rotary encoder:
 * - Turning adjusts the frequencies in the current step size.
 * - Pressing rotates through tuning step sizes (see tuning_steps).
 * - Holding for 1s enters S_DFE.
 *
 * Keyer:
 * - Pressing moves to S_MEM_SEND_WAIT, to transmit a message memory.
 * - Holding for 0.5s moves to S_ADJUST_CS, to adjust keying speed.
 * - Holding for 2s moves to S_MEM_ENTER_WAIT, to enter a message memory.
 *
 * RIT:
 * - Pressing en/disables RIT.
 * - Holding for 2s moves to S_CHANGE_BAND.
 * - Holding for 5s enters the calibration routine (S_CALIBRATION_CORRECTION).
 * - Holding for 8s erases the EEPROM settings (compile with OPT_ERASE_EEPROM).
 */
void loop_default(void)
{
  unsigned int duration;

  if (key_active()) {
    state.state = S_KEYING;
  // Tuning with the rotary encoder
  } else if (rotated_up()) {
    freq_adjust(tuning_steps[state.tuning_step]);
  } else if (rotated_down()) {
    freq_adjust(-tuning_steps[state.tuning_step]);
  } else if (state.inputs.encoder_button) {
    duration = time_encoder_button();
    if (duration > 1000) {
      if (state.key.mode == KEY_IAMBIC) {
        if (state.rit) {
          state.rit = 0;
          state.op_freq = state.rit_tx_freq;
          invalidate_frequencies();
        }
        state.state = S_DFE;
        setup_dfe();
      } else {
        morse(MX);
      }
      invalidate_display();
    } else if (duration > 50) {
      rotate_tuning_steps();
    }
  // Keyer switch for memory and code speed
  } else if (state.inputs.keyer) {
    duration = time_keyer();
    if (duration > 8000) {
      /* do nothing */
      invalidate_display();
    } else if (duration > 5000) {
      state.state = S_MEM_ENTER_WAIT;
      invalidate_display();
    } else if (duration > 2000) {
      state.state = S_TUNE;
      state.tune_mode_on = 0;
      invalidate_display();
    } else if (duration > 50) {
      state.state = S_MEM_SEND_WAIT;
      memory_index_character = 0xff;
      invalidate_display();
    }
  // RIT switch for RIT, changing band, calibration and erasing EEPROM
  } else if (state.inputs.rit) {
    duration = time_rit();
    if (duration >
#ifdef OPT_ERASE_EEPROM
        14000
#else
        11000
#endif
        ) {
      /* do nothing */
      invalidate_display();
    } else
#ifdef OPT_ERASE_EEPROM
    if (duration > 11000) {
      if (confirm("Erase EEPROM?"))
        ee_erase();
      invalidate_display();
    } else
#endif
    if (duration > 8000) {
      if (confirm("Calibrate?")) {
        state.state = S_CALIBRATION_CORRECTION;
        calibration_set_correction();
        enable_rx_tx(RX_OFF_TX_ON);
      }
      invalidate_display();
    } else if (duration > 5000) {
      state.state = S_CHANGE_BAND;
      if (state.rit) {
        state.rit = 0;
        state.op_freq = state.rit_tx_freq;
        state.tuning_step = 0;
        invalidate_frequencies();
      }
      invalidate_display();
    } else if (duration > 2000) {
      state.state = S_ADJUST_CS;
      invalidate_display();
    } else if (duration > 50) {
      if (state.rit) {
        state.rit = 0;
        state.op_freq = state.rit_tx_freq;
        state.tuning_step = 0;
        invalidate_frequencies();
      } else {
        state.rit = 1;
        state.rit_tx_freq = state.op_freq;
        state.tuning_step = 0;
      }
      invalidate_display();
    }
  }
}

/**
 * Loop for the S_KEYING state. In this state, buttons are disabled, and the
 * keying routing for paddle or straight key is called.
 */
void loop_keying(void)
{
  if (state.key.mode == KEY_IAMBIC) {
    iambic_key();
  } else if (digitalRead(DOTin) == LOW) {
    straight_key();
    state.state = S_DEFAULT;
  }
}

/**
 * Loop for the S_ADJUST_CS state. In this state, the key speed can be changed
 * using the rotary encoder and/or paddle.
 * The keyer switch selects the speed and returns to S_DEFAULT.
 */
void loop_adjust_cs(void)
{
  if (rotated_up()) {
    adjust_cs(1);
  } else if (rotated_down()) {
    adjust_cs(-1);
  } else if (state.key.mode == KEY_IAMBIC && !digitalRead(DASHin)) {
    adjust_cs(1);
    delay(200);
  } else if (state.key.mode == KEY_IAMBIC && !digitalRead(DOTin)) {
    adjust_cs(-1);
    delay(200);
  } else if (state.inputs.keyer) {
    state.state = S_DEFAULT;
    load_cw_speed();
    store_cw_speed();
    invalidate_display();
    debounce_keyer();
  }
}

/**
 * Loop for the S_TUNE state. The keyer switch turns transmission on/off. RIT
 * returns to S_DEFAULT.
 */
void loop_tune(void)
{
  if (state.inputs.rit) {
    debounce_rit();
    straight_key_handle_disable();
    state.state = S_DEFAULT;
    invalidate_display();
  } else if (state.inputs.keyer) {
    debounce_keyer();
    state.tune_mode_on = ~state.tune_mode_on;
    if (state.tune_mode_on)
      straight_key_handle_enable();
    else
      straight_key_handle_disable();
    invalidate_display();
  } else if (state.key.mode == KEY_IAMBIC) {
    if (state.tune_mode_on) {
      if (!digitalRead(DASHin)) {
        state.tune_mode_on = 0;
        straight_key_handle_disable();
      }
    } else {
      if (!digitalRead(DOTin)) {
        state.tune_mode_on = 1;
        straight_key_handle_enable();
      }
    }
    invalidate_display();
  }
}

/**
 * Loop for the S_CHANGE_BAND and S_CALIBRATION_CHANGE_BAND states. In these
 * states, the user can select the operating band using the rotary encoder.
 * The keyer switch moves on to S_DEFAULT (when in S_CHANGE_BAND) or
 * S_CALIBRATION_PEAK_RX (when in S_CALIBRATION_CHANGE_BAND).
 */
void loop_change_band(void)
{
  if (rotated_up()) {
    nextband(1);
    invalidate_display();
  } else if (rotated_down()) {
    nextband(-1);
    invalidate_display();
  } else if (state.inputs.keyer) {
    store_band();

    if (state.state == S_CALIBRATION_CHANGE_BAND) {
      state.state = S_CALIBRATION_PEAK_RX;
      invalidate_frequencies();
      enable_rx_tx(RX_ON_TX_ON);
    } else {
      state.state = S_DEFAULT;
    }

    invalidate_display();
    debounce_keyer();
  }
}

/**
 * Sets up the state for S_DFE.
 * This sets some global variables used in loop_dfe.
 * By default the user starts to enter the left-most digit (this corresponds to
 * dfe_position = 3). If some digits are the same in the low and high band,
 * they are fixed and the dfe_position is decreased.
 */
void setup_dfe(void)
{
  unsigned long power;
  dfe_character = 0xff;
  dfe_freq = 0;

  if (state.band == BAND_10) {
    dfe_position = 4;
    power = 100000000;
  } else {
    dfe_position = 3;
    power = 10000000;
  }

  while ((BAND_LIMITS_LOW[state.band] / power) * power == (BAND_LIMITS_HIGH[state.band] / power) * power) {
    dfe_position--;
    dfe_freq += (((BAND_LIMITS_LOW[state.band] / power) % 10) * power) / 10000;
    power /= 10;
  }
}

/**
 * Loop for the S_DFE state for direct frequency entry. In this state, the user
 * can enter a new frequency using the paddle.
 * When a new character has been detected (using keying routines), that
 * character is parsed as a number or abbreviation of a number. The digit is
 * set, the system moves on to the next digit. When an incorrect character was
 * detected, a question mark is sounded on the sidetone.
 * The RIT switch cancels the DFE and returns to S_DEFAULT.
 * The keyer switch sets the remaining digits to 0 and returns to S_DEFAULT.
 */
void loop_dfe(void)
{
  if (dfe_character != 0xff) {
    unsigned long add;
    switch (dfe_character) {
      case M0: case MT: add = 0; break;
#ifdef OPT_OBSCURE_MORSE_ABBREVIATIONS
      case M1: case MA: add = 1; break;
      case M2: case MU: add = 2; break;
      case M3: case MW: add = 3; break;
      case M4: case MV: add = 4; break;
      case M5: case MS: add = 5; break;
      case M6: case MB: add = 6; break;
      case M7: case MG: add = 7; break;
      case M8: case MD: add = 8; break;
#else
      case M1: add = 1; break;
      case M2: add = 2; break;
      case M3: add = 3; break;
      case M4: add = 4; break;
      case M5: add = 5; break;
      case M6: add = 6; break;
      case M7: add = 7; break;
      case M8: add = 8; break;
#endif
      case M9: case MN: add = 9; break;
      default:
        morse(Mquestion);
        dfe_character = 0xff;
        return;
    }
    dfe_character = 0xff;

    for (byte i = 0; i < dfe_position; i++)
      add *= 10;
    dfe_freq += add;

    if (dfe_position-- == 0) {
      set_dfe();
      morse(MR);
    }

    invalidate_display();
  } else if (state.inputs.keyer) {
    set_dfe();
    invalidate_display();
    morse(MR);
    debounce_keyer();
  } else if (state.inputs.rit) {
    state.state = S_DEFAULT;
    invalidate_display();
    morse(MX);
    debounce_rit();
  } else if (key_active()) {
    iambic_key();
  }
}

/**
 * Sets the frequency according to the recorded frequency in S_DFE state.
 * The operating frequency is fixed between the band limits.
 * The system returns to S_DEFAULT state.
 */
void set_dfe(void)
{
  if (state.band == BAND_10)
    state.op_freq = (BAND_LIMITS_LOW[state.band] / 1000000000u) * 1000000000u;
  else
    state.op_freq = (BAND_LIMITS_LOW[state.band] / 100000000) * 100000000;
  state.op_freq += dfe_freq * 10000;
  fix_op_freq(0);
  invalidate_frequencies();
  state.state = S_DEFAULT;
}

/**
 * Loop for the S_MEM_ENTER_WAIT state. In this state, the user can start
 * entering a new message memory.
 * When this state is entered with a straight key, the system returns to
 * S_DEFAULT state, since CW detection is not implemented for straight keys.
 * The keyer switch returns to the S_DEFAULT state.
 * The paddle moves on to the S_MEM_ENTER state.
 */
void loop_mem_enter_wait(void)
{
  memory_pointer = 0;

  if (state.inputs.rit || state.key.mode != KEY_IAMBIC) {
    state.state = S_DEFAULT;
    invalidate_display();
    morse(MX);
    debounce_keyer();
  } else if (key_active()) {
    state.state = S_MEM_ENTER;
  }
}

/**
 * Timer variable for loop_mem_enter(), keeping track of how long the key has
 * been inactive in order to insert word breaks.
 */
unsigned long quiet_since;

/**
 * Loop for the S_MEM_ENTER state. In this state, the user is keying in a new
 * message memory.
 * The keyer switch moves on to the S_MEM_ENTER_REVIEW state and plays the
 * recorded message to the user.
 * When the paddle is used, the new character is recorded.
 * When the paddle has not been used for 3 * the dash time, a word break is
 * recorded. No consecutive word breaks are recorded.
 */
void loop_mem_enter(void)
{
  if (state.inputs.keyer) {
    state.state = S_MEM_ENTER_REVIEW;
    invalidate_display();
    playback_buffer();
  } else if (key_active()) {
    iambic_key();
    quiet_since = tcount;
  } else if (quiet_since != 0
      && tcount - quiet_since > 3 * state.key.dash_time
      && buffer[memory_pointer-1] != 0x00) {
    buffer[memory_pointer++] = 0x00;
    buffer[memory_pointer] = 0xff;
    display_flash_circle(1);
  }
}

/**
 * Loop for the S_MEM_ENTER_REVIEW state. In this state, the user has just
 * heard the memory as he entered it.
 * The RIT switch returns to the S_MEM_ENTER_WAIT state, discarding the entry.
 * One of ten memories can be selected using the rotary encoder: turn to
 * select, then save with the keyer switch.
 */
void loop_mem_enter_review(void)
{
  if (rotated_up()) {
    memory_index++;
    memory_index %= 10;
    invalidate_display();
  } else if (rotated_down()) {
    memory_index--;
    if (memory_index == 0xff)
      memory_index = 9;
    invalidate_display();
  } else if (state.inputs.keyer) {
    store_memory(memory_index);
    morse(MM);
    if (memory_index + 1 >= 10)
      morse(MORSE_DIGITS[(memory_index + 1) / 10]);
    morse(MORSE_DIGITS[(memory_index + 1) % 10]);
    memory_index = 0;
    state.state = S_DEFAULT;
    invalidate_display();
    debounce_keyer();
  } else if (state.inputs.rit) {
    state.state = S_MEM_ENTER_WAIT;
    memory_pointer = 0;
    invalidate_display();
    debounce_rit();
  }
}

/**
 * Loop for the S_MEM_SEND_WAIT state. In this state, the user can choose the
 * memory to send. Picking a memory moves to the S_MEM_SEND_TX state, transmits
 * the memory and returns to S_DEFAULT.  The RIT switch returns to the
 * S_DEFAULT state.
 * One of ten memories can be selected using the rotary encoder: turn to
 * select, then transmit with the keyer switch.
 */
void loop_mem_send_wait(void)
{
  if (rotated_up()) {
    memory_index++;
    memory_index %= 10;
    invalidate_display();
  } else if (rotated_down()) {
    memory_index--;
    if (memory_index == 0xff)
      memory_index = 9;
    invalidate_display();
  } else if (state.inputs.keyer) {
    debounce_keyer();
    state.state = S_MEM_SEND_TX;
    load_memory_for_tx(memory_index);
    invalidate_display();
  } else if (key_active()) {
    iambic_key();
  } else if (memory_index_character != 0xff) {
    switch (memory_index_character) {
      case M0: case MT: memory_index = 0; break;
#ifdef OPT_OBSCURE_MORSE_ABBREVIATIONS
      case M1: case MA: memory_index = 1; break;
      case M2: case MU: memory_index = 2; break;
      case M3: case MW: memory_index = 3; break;
      case M4: case MV: memory_index = 4; break;
      case M5: case MS: memory_index = 5; break;
      case M6: case MB: memory_index = 6; break;
      case M7: case MG: memory_index = 7; break;
      case M8: case MD: memory_index = 8; break;
#else
      case M1: memory_index = 1; break;
      case M2: memory_index = 2; break;
      case M3: memory_index = 3; break;
      case M4: memory_index = 4; break;
      case M5: memory_index = 5; break;
      case M6: memory_index = 6; break;
      case M7: memory_index = 7; break;
      case M8: memory_index = 8; break;
#endif
      case M9: case MN: memory_index = 9; break;
      default:
        morse(Mquestion);
        memory_index_character = 0xff;
        break;
    }

    if (memory_index_character != 0xff) {
      invalidate_display();
      state.state = S_MEM_SEND_TX;
      load_memory_for_tx(memory_index);
      invalidate_display();
      memory_index_character = 0xff;
    }
  } else if (state.inputs.rit) {
    state.state = S_DEFAULT;
    invalidate_display();
    debounce_rit();
  }
}

/**
 * Loop for the S_MEM_SEND_TX state. In this state, on every iteration, the
 * next character of the buffer is transmitted.
 * At the end, we return to S_DEFAULT, unless we are in beacon mode. In beacon
 * mode, we wait for a time defined by BEACON_INTERVAL, and then repeat.
 * The keyer switch toggles beacon mode on and off. The RIT switch ends any
 * active transmission.
 */
void loop_mem_send_tx(void)
{
  if (state.inputs.keyer) {
    state.beacon = ~state.beacon;
    invalidate_display();
    debounce_keyer();
  } else if (state.inputs.rit) {
    state.state = S_DEFAULT;
    invalidate_display();
    debounce_rit();
  } else {
    byte character = buffer[state.mem_tx_index];
    switch (character) {
      case 0xff:
        state.mem_tx_index = 0;
        if (!state.beacon) {
          memory_index = 0;
          state.state = S_DEFAULT;
          invalidate_display();
        } else {
          for (byte i = 0; i < BEACON_INTERVAL; i++) {
            display_progress(0, BEACON_INTERVAL - 1, i);
            delay(state.key.dot_time);
            if (state.inputs.keyer || state.inputs.rit) {
              if (state.inputs.keyer)
                state.beacon = 0;
              memory_index = 0;
              state.state = S_DEFAULT;
              invalidate_display();
              debounce_keyer();
              debounce_rit();
              break;
            }
          }
        }
        break;
      case 0x00:
        delay(7 * state.key.dot_time);
        state.mem_tx_index++;
        break;
      default:
        key_handle_start();
        morse(character);
        key_handle_end();
        state.mem_tx_index++;
        break;
    }
  }
}

/**
 * Loop for the S_ERROR state. In this state, an alarm is sounded on the
 * sidetone. There is no recovery from this state except for turning off the
 * device.
 */
void loop_error(void)
{
  for (unsigned int f = 400; f < 1000; f += 10) {
    tone(SIDETONE, f);
    delay(10);
  }
  noTone(SIDETONE);
  delay(450);
}

/**
 * Loop for the S_CALIBRATION_CORRECTION state. In this state, the required
 * Si5351 correction value is found by the user by turning the rotary encoder
 * and fixing the frequency on TP3 to 10MHz.
 * The keyer switch stores the correction value to EEPROM and moves on to the
 * S_CALIBRATION_PEAK_IF state.
 */
void loop_calibration_correction(void)
{
  if (state.inputs.keyer) {
    EEPROM.write(EEPROM_CAL_VALUE + 0, cal_value);
    EEPROM.write(EEPROM_CAL_VALUE + 1, cal_value >> 8);
    EEPROM.write(EEPROM_CAL_VALUE + 2, cal_value >> 16);
    EEPROM.write(EEPROM_CAL_VALUE + 3, cal_value >> 24);

    state.state = S_CALIBRATION_PEAK_IF;
    state.op_freq = IFfreq == 0xfffffffful ? IF_DEFAULT : IFfreq;
    invalidate_frequencies();
    invalidate_display();
    enable_rx_tx(RX_ON_TX_OFF);

    debounce_keyer();
  } else if (rotated_up()) {
    cal_value -= 100;
    calibration_set_correction();
  } else if (rotated_down()) {
    cal_value += 100;
    calibration_set_correction();
  }
}

/**
 * Loop for the S_CALIBRATION_PEAK_IF state. In this state, the user can adjust
 * the IF frequency using the rotary encoder to peak the signal.
 * The keyer switch saves the frequency to the EEPROM and moves on to the
 * S_CALIBRATION_CHANGE_BAND state.
 */
void loop_calibration_peak_if(void)
{
  if (state.inputs.keyer) {
    IFfreq = state.op_freq;
    EEPROM.write(EEPROM_IF_FREQ + 0, state.op_freq);
    EEPROM.write(EEPROM_IF_FREQ + 1, state.op_freq >> 8);
    EEPROM.write(EEPROM_IF_FREQ + 2, state.op_freq >> 16);
    EEPROM.write(EEPROM_IF_FREQ + 3, state.op_freq >> 24);
    state.state = S_CALIBRATION_CHANGE_BAND;
    nextband(0);
    invalidate_frequencies();
    invalidate_display();

    debounce_keyer();
  } else if (rotated_up()) {
    state.op_freq += 1000;
    invalidate_frequencies();
    invalidate_display();
  } else if (rotated_down()) {
    state.op_freq -= 1000;
    invalidate_frequencies();
    invalidate_display();
  }
}

/**
 * Loop for the S_CALIBRATION_PEAK_RX state. In this state, the user can trim
 * CT1 and CT2 to peak the signal.
 * The keyer switch returns to S_DEFAULT.
 */
void loop_calibration_peak_rx(void)
{
  if (state.inputs.keyer) {
    state.state = S_DEFAULT;
    setup_band();
    enable_rx_tx(RX_ON_TX_OFF);
    invalidate_display();
    debounce_keyer();
  }
}

/**
 * Rotate through the tuning steps. The display is updated.
 * See tuning_steps.
 */
void rotate_tuning_steps(void)
{
  state.tuning_step++;
  if (state.tuning_step >= (state.rit ? 2 : sizeof(tuning_blinks)))
    state.tuning_step = 0;
  invalidate_display();
}

/**
 * Used for detecting morse characters when using the paddle.
 */
byte morse_char;

/**
 * Handle the start of a morse character. This enables mute. When in TX mode,
 * the RX clock is disabled and the TX is enabled. The detected character is
 * reset.
 * Also see key_handle_end();
 */
void key_handle_start(void)
{
  morse_char = 0x01;
}

/**
 * Handle the end of a morse character. This disables mute, enables RX and
 * disables TX. In MEM_ENTER mode, the detected character is stored.
 * Also see key_handle_start().
 */
void key_handle_end(void)
{
  if (state.state == S_KEYING) {
    state.state = S_DEFAULT;
  } else if (state.state == S_MEM_ENTER) {
    if (memory_pointer == MEMORY_LENGTH) {
      error(2);
      return;
    }
    buffer[memory_pointer++] = morse_char;
    buffer[memory_pointer] = 0xff;
    display_flash_circle(0);
  } else if (state.state == S_DFE) {
    dfe_character = morse_char;
  } else if (state.state == S_MEM_SEND_WAIT) {
    memory_index_character = morse_char;
  }
}

/**
 * Handle a dash. In TX modes, this will enable TXEN. In other cases, the
 * detected character is updated. The sidetone will be enabled.
 * A 1ms delay is included between enabling the clock and switching on +12V TX to 
 * ensure clock is running before DC is applied to the PA.
 * Also see key_handle_dot() and key_handle_dashdot_end().
 */
void key_handle_dash(void)
{
  digitalWrite(MUTE, LOW);
  SIDETONE_ENABLE();
  if (state.state == S_KEYING || state.state == S_MEM_SEND_TX) {
    enable_rx_tx(RX_OFF_TX_ON);
    delay(1);
    digitalWrite(TXEN, HIGH);
  } else {
    morse_char = (morse_char << 1) | 0x01;
  }
}

/**
 * See key_handle_dash().
 */
void key_handle_dot(void)
{
  digitalWrite(MUTE, LOW);
  SIDETONE_ENABLE();
  if (state.state == S_KEYING || state.state == S_MEM_SEND_TX) {
    enable_rx_tx(RX_OFF_TX_ON);
    delay(1);
    digitalWrite(TXEN, HIGH);
  } else {
    morse_char <<= 1;
  }
}

/**
 * Handle the end of a dash or dot.
 * A 5ms delay is included between switching off the +12V TX and switching off the clock to 
 * ensure anti key-click tail is completed.
 * See key_handle_dash() and key_handle_dot().
 */
void key_handle_dashdot_end(void)
{
  SIDETONE_DISABLE();
  digitalWrite(TXEN, LOW);
  delay(5);
  enable_rx_tx(RX_ON_TX_OFF);
  digitalWrite(MUTE, HIGH);
}

/**
 * Handle enabling of the straight key. The straight key is always in TX mode,
 * so no state checking here (compare to the key_* routines).
 * Also see straight_key_handle_disable().
 */
void straight_key_handle_enable(void)
{
  SIDETONE_ENABLE();
  digitalWrite(MUTE, LOW);
  enable_rx_tx(RX_OFF_TX_ON);
  delay(1);
  digitalWrite(TXEN, HIGH);
}

/**
 * See straight_key_handle_enable().
 */
void straight_key_handle_disable(void)
{
  digitalWrite(TXEN, LOW);
  delay(5);
  enable_rx_tx(RX_ON_TX_OFF);
  digitalWrite(MUTE, HIGH);
  SIDETONE_DISABLE();
}

/**
 * Set the calibratioin of the Si5351 chip and reset the frequency to 10MHz for
 * calibration purposes.
 */
void calibration_set_correction(void)
{
  si5351.set_correction(cal_value);
  si5351.set_freq(1000000000, 0ull, SI5351_CLK1);
}

/**
 * Fetch calibration data from EEPROM.
 */
void fetch_calibration_data(void)
{
  IFfreq =                 EEPROM.read(EEPROM_IF_FREQ + 3);
  IFfreq = (IFfreq << 8) + EEPROM.read(EEPROM_IF_FREQ + 2);
  IFfreq = (IFfreq << 8) + EEPROM.read(EEPROM_IF_FREQ + 1);
  IFfreq = (IFfreq << 8) + EEPROM.read(EEPROM_IF_FREQ + 0);

  cal_value =                    EEPROM.read(EEPROM_CAL_VALUE + 3);
  cal_value = (cal_value << 8) + EEPROM.read(EEPROM_CAL_VALUE + 2);
  cal_value = (cal_value << 8) + EEPROM.read(EEPROM_CAL_VALUE + 1);
  cal_value = (cal_value << 8) + EEPROM.read(EEPROM_CAL_VALUE + 0);
}

/**
 * Enable/disable the RX and TX clocks. This is faster than
 * Si5351::output_enable().
 *
 * @param option one of RX_ON_TX_ON, RX_OFF_TX_ON, RX_ON_TX_OFF, RX_OFF_TX_OFF.
 */
void enable_rx_tx(byte option)
{
  Wire.beginTransmission(0x60);
  Wire.write(3);
  Wire.write(option);
  Wire.endTransmission();
}

/**
 * Adjust the operating frequency by an offset.
 * The display and Si5351 frequencies are updated.
 *
 * @param step the offset.
 */
void freq_adjust(long step)
{
  state.op_freq += step;
  fix_op_freq(step);
  invalidate_display();
  invalidate_frequencies();
}

/**
 * Fix the operating frequency between the band limits.
 */
void fix_op_freq(long step)
{
#ifdef PLAN_VK
  if (state.band == BAND_80) {
    if (state.op_freq > BAND_80_LOW_TOP && state.op_freq < BAND_80_GAP_MIDDLE){	
	  state.op_freq = BAND_80_HIGH_BOTTOM;
	  return;
	}
    else if (state.op_freq < BAND_80_HIGH_BOTTOM && state.op_freq > BAND_80_GAP_MIDDLE) {	  
	  state.op_freq = BAND_80_LOW_TOP;
	  return;
	}
  }
#endif
  if (state.op_freq > BAND_LIMITS_HIGH[state.band])
    state.op_freq = BAND_LIMITS_HIGH[state.band];
  if (state.op_freq < BAND_LIMITS_LOW[state.band])
    state.op_freq = BAND_LIMITS_LOW[state.band];

  if (state.rit) {
    /* to prevent the display from overflowing */
    long diff = state.op_freq - state.rit_tx_freq;
    if (diff >= 1000000 || diff <= -1000000)
      state.op_freq -= step;
  }
}

/**
 * Reset the frequencies (both RX and TX) used by the Si5351 chip. This
 * function should be called any time something frequency-related happens, s.t.
 * it is not needed in keying routines (to make their response time lower).
 */
void invalidate_frequencies(void)
{
  unsigned long freq;

  if (state.state == S_CALIBRATION_PEAK_IF)
    freq = state.op_freq;
  else if (state.op_freq >= IFfreq)
    freq = state.op_freq - IFfreq;
  else
    freq = state.op_freq + IFfreq;

  si5351.set_freq(freq, 0ull, SI5351_CLK_RX);
  si5351.set_freq(TX_FREQ(state), 0ull, SI5351_CLK_TX);
}

/**
 * Prepare a memory for transmission. This loads the memory, prepares the
 * buffer and resets the character index.
 */
void load_memory_for_tx(byte index)
{
  load_memory(index);
  prepare_buffer_for_tx();
  state.mem_tx_index = 0;
}

/**
 * Store the key speed in EEPROM.
 */
void store_cw_speed(void)
{
  EEPROM.write(EEPROM_CW_SPEED, state.key.speed);
}

#ifdef OPT_ERASE_EEPROM
/**
 * Erase settings from EEPROM. This does not clear the message memories.
 */
void ee_erase(void)
{
  for (byte i = 0; i <= MEMORY_EEPROM_START; i++)
    EEPROM.write(i, 0xff);

  display_feedback("EEPROM erased.");
  display_delay(1500);
}
#endif

/**
 * Enter the error state. This error is non-recoverable and should only be used
 * in very rare cases.
 */
void error(byte er)
{
  errno = er;
  state.state = S_ERROR;
  invalidate_display();
}

// vim: tabstop=2 shiftwidth=2 expandtab:
