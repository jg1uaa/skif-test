// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#ifndef INPUT_SKIF_H
#define INPUT_SKIF_H

#include <stdbool.h>

int skif_init(bool, char *, int);
void *skif_thread(void *);

#endif
