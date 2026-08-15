// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#ifndef SERIAL_H
#define SERIAL_H

#include "skif-arduino.h"

int open_serial(char *);
int wait_for_device(int, int);

#endif
