/**
 * Copyright (C) 2020-2022 Camil Staps <pa5et@camilstaps.nl>
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

#include <Adafruit_CharacterOLED.h>
#include "display.h"

#define BLINKED_ON ((((byte) tcount) >> 7) & 0x01)

#define LCD_RS  5
#define LCD_RW  6
#define LCD_EN  7
#define LCD_D4  8
#define LCD_D5  9
#define LCD_D6 10
#define LCD_D7 11

static Adafruit_CharacterOLED lcd(OLED_V2, LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

#ifdef OPT_USER_DEFINED_CHARACTERS
// https://www.quinapalus.com/hd44780udg.html
static uint8_t character_circ_closed[] = {0x0,0x0,0xe,0x1f,0x1f,0x1f,0xe,0x0};
static uint8_t character_m[] = {0x0,0x0,0x0,0x1a,0x15,0x15,0x11,0x0};
static uint8_t character_p[] = {0x0,0x0,0x0,0xc,0xa,0xc,0x8,0x0};
static uint8_t character_r[] = {0x0,0x0,0x1c,0x12,0x1c,0x14,0x12,0x0};
static uint8_t character_w[] = {0x0,0x0,0x0,0x15,0x15,0x15,0x1a,0x0};
static uint8_t character_arr_l[] = {0x0,0x2,0x6,0xe,0x1e,0xe,0x6,0x2};
static uint8_t character_arr_r[] = {0x0,0x8,0xc,0xe,0xf,0xe,0xc,0x8};
#endif

/**
 * Initialize the display: intialize library; show credits; create custom
 * characters.
 */
void display_init(void)
{
  lcd.begin(16, 2, LCD_EUROPEAN_II);
  lcd.setCursor(0, 0);
  lcd.print("   = ATSAMF =   ");
  lcd.setCursor(0, 1);
  lcd.print("   Rick PA5NN   ");

#ifdef OPT_USER_DEFINED_CHARACTERS
  lcd.createChar(1,character_circ_closed);
  lcd.createChar(2,character_m);
  lcd.createChar(3,character_p);
  lcd.createChar(4,character_r);
  lcd.createChar(5,character_w);
  lcd.createChar(6,character_arr_l);
  lcd.createChar(7,character_arr_r);
#endif

  state.display.line_1[0]='\0';
  state.display.line_2[0]='\0';
}

static uint8_t current_blinked_on = 0;
static unsigned short current_blinking_1 = 0;

/**
 * The ISR for the display. Should be called regularly, but *not* from an ISR
 * due to incompatibilities in the Adafruit library.
 */
void display_isr(void)
{
  if (BLINKED_ON == current_blinked_on)
    return;

  current_blinked_on = BLINKED_ON;

  int i = 0;
  lcd.setCursor(0,0);

  for (unsigned short mask = state.display.blinking_1; mask | current_blinking_1; mask >>= 1) {
    char c = state.display.line_1[i];
    if (!c)
      break;
    if ((mask & 0x1) && !current_blinked_on)
      c = ' ';
    lcd.print(c);
    i++;
  }

  current_blinking_1 = state.display.blinking_1;
}

static char current_line_1[16];
static char current_line_2[16];

/**
 * Send changes in the first line to the display.
 */
static void refresh_line_1(void)
{
  int i = 0;

  for (int j = 0; j < 16; j++) {
    if (state.display.line_1[j] == current_line_1[j])
      continue;

    lcd.setCursor(0,0);
    while (state.display.line_1[i]) {
      char c = state.display.line_1[i];
      if (!current_blinked_on && (state.display.blinking_1 & (1 << i)))
        c = ' ';
      lcd.print(c);
      i++;
    }
    while (i++ < 16)
      lcd.print(' ');

    strcpy(current_line_1, state.display.line_1);
    break;
  }
}

/**
 * Send changes in the second line to the display.
 */
static void refresh_line_2(void)
{
  int i = 0;

  for (int j = 0; j < 16; j++) {
    if (state.display.line_2[j] == current_line_2[j])
      continue;

    lcd.setCursor(0,1);
    while (state.display.line_2[i])
      lcd.print(state.display.line_2[i++]);
    while (i++ < 16)
      lcd.print(' ');

    strcpy(current_line_2, state.display.line_2);
    break;
  }
}

/**
 * Send changes to the display.
 */
void refresh_display(void)
{
  refresh_line_1();
  refresh_line_2();
}

/**
 * Recompute the display according to the current state.
 */
void invalidate_display(void)
{
  state.display.blinking_1 = 0;

  display_freq();
  display_rit();
  state.display.line_2[0] = '\0';

  switch (state.state) {
    case S_DEFAULT:
    case S_KEYING:
      {
        int i=0;
        if (state.key.speed >= 10)
          state.display.line_2[i++] = '0' + state.key.speed / 10;
        state.display.line_2[i++] = '0' + state.key.speed % 10;
#ifdef OPT_USER_DEFINED_CHARACTERS
        state.display.line_2[i++] = '\5';
        state.display.line_2[i++] = '\3';
        state.display.line_2[i++] = '\2';
#else
        state.display.line_2[i++] = 'w';
        state.display.line_2[i++] = 'p';
        state.display.line_2[i++] = 'm';
#endif
        state.display.line_2[i++] = '\0';
      }
      break;
    case S_ADJUST_CS:
      display_cs();
      break;
    case S_STARTUP:
      strcpy(state.display.line_2, "Band: ");
      display_band(6);
      break;
    case S_TUNE:
      strcpy(state.display.line_2, "Tune mode      o");
      if (state.tune_mode_on)
#ifdef OPT_USER_DEFINED_CHARACTERS
        state.display.line_2[15] = '\1';
#else
        state.display.line_2[15] = '-';
#endif
      break;
    case S_CHANGE_BAND:
    case S_CALIBRATION_CHANGE_BAND:
#ifdef OPT_USER_DEFINED_CHARACTERS
      strcpy(state.display.line_2, "Band: \7");
#else
      strcpy(state.display.line_2, "Band: <");
#endif
      display_band(7);
      if (!state.display.line_2[10])
        state.display.line_2[10] = ' ';
#ifdef OPT_USER_DEFINED_CHARACTERS
      state.display.line_2[11] = '\6';
#else
      state.display.line_2[11] = '>';
#endif
      state.display.line_2[12] = '\0';
      break;
    case S_DFE:
      display_dfe();
      break;
    case S_MEM_ENTER_WAIT:
    case S_MEM_ENTER:
      strcpy(state.display.line_2, "Enter memory");
      break;
    case S_MEM_ENTER_REVIEW:
#ifdef OPT_USER_DEFINED_CHARACTERS
      strcpy(state.display.line_2, "Store mem? \7..\6");
#else
      strcpy(state.display.line_2, "Store mem? <..>");
#endif
      state.display.line_2[12] = '0' + (memory_index + 1) / 10;
      state.display.line_2[13] = '0' + (memory_index + 1) % 10;
      break;
    case S_MEM_SEND_WAIT:
      if (state.beacon)
#ifdef OPT_USER_DEFINED_CHARACTERS
        strcpy(state.display.line_2, "Beacon \7..\6");
#else
        strcpy(state.display.line_2, "Beacon <..>");
#endif
      else
#ifdef OPT_USER_DEFINED_CHARACTERS
        strcpy(state.display.line_2, "Memory \7..\6");
#else
        strcpy(state.display.line_2, "Memory <..>");
#endif
      state.display.line_2[8] = '0' + (memory_index + 1) / 10;
      state.display.line_2[9] = '0' + (memory_index + 1) % 10;
      break;
    case S_MEM_SEND_TX:
      if (state.beacon)
        strcpy(state.display.line_2, "Beacon ..");
      else
        strcpy(state.display.line_2, "Memory ..");
      state.display.line_2[7] = '0' + (memory_index + 1) / 10;
      state.display.line_2[8] = '0' + (memory_index + 1) % 10;
      break;
    case S_CALIBRATION_CORRECTION:
      strcpy(state.display.line_1, "Fix 10MHz at TP3");
      strcpy(state.display.line_2, "and press KEYER");
      break;
    case S_CALIBRATION_PEAK_IF:
      display_freq();
      strcpy(state.display.line_2, "Peak TP2 w/ CP3");
      break;
    case S_CALIBRATION_PEAK_RX:
      strcpy(state.display.line_1, "Peak RX with CP2");
      strcpy(state.display.line_2, "and CP3");
      break;
    case S_ERROR:
      sprintf(state.display.line_2, "Error %d", errno);
      break;
  }

  refresh_display();
}

/**
 * Briefly show a circle in the right bottom. When the argument is non-zero,
 * the circle is closed.
 */
void display_flash_circle(uint8_t type)
{
  lcd.setCursor(15,1);
#ifdef OPT_USER_DEFINED_CHARACTERS
  lcd.print(type ? '\1' : 'o');
#else
  lcd.print(type ? '#' : 'o');
#endif
  delay(100);
  lcd.setCursor(15,1);
  lcd.print(' ');
}

/**
 * Display a question on the first line with Yes / No options on the second
 * line.
 */
void display_question(const char *question)
{
  strcpy(state.display.line_1, question);
  strcpy(state.display.line_2, "RIT=No Keyer=Yes");
  refresh_display();
}

/**
 * Display feedback on the second line (used while a button is pressed to
 * select some action).
 */
void display_feedback(const char *feedback)
{
  strcpy(state.display.line_2, feedback);
  refresh_display();
}

/**
 * Display the progress of a value between a minimum and a maximum as an
 * emptying bar in the right bottom.
 */
void display_progress(short min, short max, short val)
{
  uint8_t loading_bar_char[] = {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1};
  uint8_t progress = 8 * (val - min) / (max - min);

  for (uint8_t i = 0; i < progress; i++)
    loading_bar_char[i] = 0;

  lcd.createChar(0, loading_bar_char);
  lcd.setCursor(15,1);
  lcd.print('\0');
}

/**
 * Clear the progress indicator after display_progress().
 */
void display_clear_progress(void)
{
  lcd.setCursor(15,1);
  lcd.print(' ');
}

/**
 * Delay for some time and display the progress in the right bottom.
 */
void display_delay(short millis)
{
  uint8_t loading_bar_char[] = {0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1};
  millis /= 8;

  lcd.createChar(0, loading_bar_char);
  lcd.setCursor(15,1);
  lcd.print('\0');
  for (int i = 0; i < 8; i++) {
    delay(millis);
    loading_bar_char[i] = 0;
    lcd.createChar(0, loading_bar_char);
  }
  lcd.setCursor(15,1);
  lcd.print(' ');
}

/**
 * Displays the code speed in WPM on the second line, with arrows to change it.
 */
void display_cs(void)
{
#ifdef OPT_USER_DEFINED_CHARACTERS
  state.display.line_2[0] = '\6';
#else
  state.display.line_2[0] = '<';
#endif
  state.display.line_2[1] = state.key.speed >= 10 ? ('0' + state.key.speed / 10) : ' ';
  state.display.line_2[2] = '0' + state.key.speed % 10;
#ifdef OPT_USER_DEFINED_CHARACTERS
  strcpy(&state.display.line_2[3], "\7 \5\3\2");
#else
  strcpy(&state.display.line_2[3], "> wpm");
#endif
}

/**
 * Displays `r` and the RIT offset in kHz.
 * Above 9.99 and below -9.99, the display overflows.
 */
void display_rit(void)
{
  unsigned long offset;

  if (!state.rit)
    return;

  state.display.line_1[9] = ' ';
#ifdef OPT_USER_DEFINED_CHARACTERS
  state.display.line_1[10] = '\4';
#else
  state.display.line_1[10] = 'R';
#endif

  if (state.rit_tx_freq > state.op_freq) {
    offset = state.rit_tx_freq - state.op_freq;
    state.display.line_1[11]='-';
  } else {
    offset = state.op_freq - state.rit_tx_freq;
    state.display.line_1[11] = '+';
  }

  offset /= 100;
  state.display.line_1[12] = '0' + (offset % 10000) / 1000;
  state.display.line_1[13] = '.';
  state.display.line_1[14] = '0' + (offset % 1000) / 100;
  state.display.line_1[15] = '0' + (offset % 100) / 10;

  state.display.blinking_1 <<= 10;
}

/**
 * Displays the current frequency in kHz on the first line.
 * Blinks a digit when the tuning step is set large.
 */
void display_freq(void)
{
  unsigned long frequency = TX_FREQ(state)/100;

  if (state.band == BAND_10) {
    state.display.line_1[0] = '0' + (frequency%10000000) / 1000000;
    state.display.line_1[1] = '0' + (frequency%1000000) / 100000;
    state.display.line_1[2] = '0' + (frequency%100000) / 10000;
    state.display.line_1[3] = '0' + (frequency%10000) / 1000;
    state.display.line_1[4] = '.';
    state.display.line_1[5] = '0' + (frequency%1000) / 100;
  } else {
    state.display.line_1[0] = '0' + (frequency%1000000) / 100000;
    state.display.line_1[1] = '0' + (frequency%100000) / 10000;
    state.display.line_1[2] = '0' + (frequency%10000) / 1000;
    state.display.line_1[3] = '.';
    state.display.line_1[4] = '0' + (frequency%1000) / 100;
    state.display.line_1[5] = '0' + (frequency%100) / 10;
  }
  state.display.line_1[6] = 'k';
  state.display.line_1[7] = 'H';
  state.display.line_1[8] = 'z';
  state.display.line_1[9] = '\0';

  state.display.blinking_1 |= tuning_blinks[state.tuning_step];
  if (state.band == BAND_10)
    state.display.blinking_1 <<= 1;
}

/**
 * Displays the new frequency for DFE on the first line, and "DFE" on the
 * second line.
 */
void display_dfe(void)
{
  unsigned long frequency = dfe_freq*100;
  if (state.band == BAND_10) {
    state.display.line_1[0] = '0' + (frequency%10000000) / 1000000;
    state.display.line_1[1] = '0' + (frequency%1000000) / 100000;
    state.display.line_1[2] = '0' + (frequency%100000) / 10000;
    state.display.line_1[3] = '0' + (frequency%10000) / 1000;
    state.display.line_1[4] = '.';
    state.display.line_1[5] = '0' + (frequency%1000) / 100;
  } else {
    state.display.line_1[0] = '0' + (frequency%1000000) / 100000;
    state.display.line_1[1] = '0' + (frequency%100000) / 10000;
    state.display.line_1[2] = '0' + (frequency%10000) / 1000;
    state.display.line_1[3] = '.';
    state.display.line_1[4] = '0' + (frequency%1000) / 100;
    state.display.line_1[5] = '0' + (frequency%100) / 10;
  }
  state.display.line_1[6] = 'k';
  state.display.line_1[7] = 'H';
  state.display.line_1[8] = 'z';
  state.display.line_1[9] = '\0';

  if (state.band == BAND_10) {
    state.display.blinking_1 = 1 << (4 - dfe_position);
    if (state.display.blinking_1 == 0x10) // skip the decimal point
      state.display.blinking_1 = 0x20;
  } else {
    state.display.blinking_1 = 1 << (3 - dfe_position);
    if (state.display.blinking_1 == 0x8) // skip the decimal point
      state.display.blinking_1 = 0x10;
  }

  strcpy(state.display.line_2, "DFE");
}

/**
 * Displays the current band somewhere on the second line.
 */
void display_band(uint8_t start)
{
  state.display.line_2[start  ] = BAND_DIGITS_1[state.band];
  state.display.line_2[start+1] = BAND_DIGITS_2[state.band];
  state.display.line_2[start+2] = BAND_DIGITS_3[state.band];
  state.display.line_2[start+3] = BAND_DIGITS_4[state.band];
  state.display.line_2[start+4] = '\0';
}

// vim: tabstop=2 shiftwidth=2 expandtab:
