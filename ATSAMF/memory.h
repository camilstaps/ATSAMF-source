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
