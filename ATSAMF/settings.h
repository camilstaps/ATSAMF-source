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

/* Customisable settings. For more details, see README.md. */
#define WPM_DEFAULT          20 /* Initial paddle speed in WPM */

#define KEY_MIN_SPEED         5 /* Minimal speed in WPM */
#define KEY_MAX_SPEED        30 /* Maximal speed in WPM */

#define SIDETONE_FREQ       600 /* Frequency of the sidetone, in Hz */

#define MEMORY_LENGTH        64 /* Length of memories, including spacing */
#define MEMORY_EEPROM_START  16 /* Start of memory block in EEPROM */

#define BEACON_INTERVAL      15 /* Interval of TXs in number of dot-times */

/* The tuning steps (rotated through with the encoder button) in mHz (max 8) */
#define TUNING_STEPS       {1000,       10000,      100000,      1000000}
/* Which digit to blink for each of these tuning steps */
#define TUNING_STEP_DIGITS {BLINK_NONE, BLINK_10HZ, BLINK_100HZ, BLINK_1KHZ}

/* The band plan. Should be one of the following:
 * - PLAN_IARU1
 * - PLAN_IARU2
 * - PLAN_IARU3
 * - PLAN_VK
 * It is possible to override the default operating frequency using:
 * #define DEFAULT_OP_FREQ_20 1405500000
 * ... setting the default frequency to 14.055 MHz on 20m.
 */
#define PLAN_IARU1

/* Custom default operating frequencies, per band, in mHz.
 * See bands.h for defaults */
//#define DEFAULT_OP_FREQ_40  703000000

/* Custom features. For more details, see README.md. */

/* Use user-defined LCD characters (disable for incompatible displays) */
#define OPT_USER_DEFINED_CHARACTERS

/* Erase EEPROM by pressing RIT for 8s */
#define OPT_ERASE_EEPROM

/* Obscure CW number abbrevations in DFE and more memories mode */
#define OPT_OBSCURE_MORSE_ABBREVIATIONS

// vim: tabstop=2 shiftwidth=2 expandtab:
