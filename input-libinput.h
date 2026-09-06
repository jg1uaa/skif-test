// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#ifndef INPUT_LIBINPUT_H
#define INPUT_LIBINPUT_H

#include <stdbool.h>

int libinput_init(bool, char *, int);
void *libinput_thread(void *);

#endif

