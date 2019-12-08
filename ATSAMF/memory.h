/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

#ifndef _H_MEMORY
#define _H_MEMORY

#include "ATSAMF.h"

void store_memory(byte);
void transmit_memory(byte);
void playback_buffer(void);
void prepare_buffer_for_tx(void);
void empty_buffer(void);

#endif

// vim: tabstop=2 shiftwidth=2 expandtab:
