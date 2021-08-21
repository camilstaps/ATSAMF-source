/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

#ifndef _H_ATSAMF
#define _H_ATSAMF

#include <Arduino.h>

#include "settings.h"

#include "bands.h"
#include "buttons.h"
#include "display.h"
#include "key.h"
#include "memory.h"
#include "morse.h"

#ifdef __cplusplus
extern "C"{
#endif

enum state
#ifdef __cplusplus
  : unsigned char
#endif
  {
  S_STARTUP,
  S_DEFAULT,
  S_KEYING,
  S_ADJUST_CS,
  S_TUNE,
  S_CHANGE_BAND,
  S_DFE,
  S_MEM_SEND_WAIT,
  S_MEM_SEND_TX,
  S_MEM_ENTER_WAIT,
  S_MEM_ENTER,
  S_MEM_ENTER_REVIEW,
  S_CALIBRATION_CORRECTION,
  S_CALIBRATION_PEAK_IF,
  S_CALIBRATION_CHANGE_BAND,
  S_CALIBRATION_PEAK_RX,
  S_ERROR
};

struct atsamf {
  enum state state;

  struct key_state key;

  enum band band;
  unsigned long op_freq;
  unsigned long rit_tx_freq;

  unsigned char rit:1;
  unsigned char tuning_step:3;

  unsigned char tune_mode_on:1;

  unsigned char beacon:1;
  unsigned char mem_tx_index;

  struct display display;
  volatile struct inputs inputs;
};

#define TX_FREQ(state) (state.rit ? state.rit_tx_freq : state.op_freq)

extern struct atsamf state;
extern volatile unsigned long tcount;
extern const byte tuning_blinks[];
extern byte errno;

extern byte memory_index;
extern char buffer[MEMORY_LENGTH];

extern byte dfe_position;
extern unsigned long dfe_freq;


#define SIDETONE A0
#define MUTE A1
#define TXEN 13
#define DOTin A2
#define DASHin A3

#define SIDETONE_ENABLE() {tone(SIDETONE, SIDETONE_FREQ);}
#define SIDETONE_DISABLE() {noTone(SIDETONE);}

#define EEPROM_IF_FREQ   0 // 4 bytes
#define EEPROM_BAND      6 // 1 byte
#define EEPROM_CW_SPEED  7 // 1 byte
#define EEPROM_CAL_VALUE 8 // 4 bytes

#ifdef __cplusplus
}
#endif

#endif

// vim: tabstop=2 shiftwidth=2 expandtab:
