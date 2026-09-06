// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include <stdint.h>
#include <stdbool.h>

struct queue_entry {
	uint32_t elapsed_time_us;
	int state;
};

#define QUEUE_DEPTH 16

void notify_die(void);
int enqueue(struct queue_entry *);
int dequeue(struct queue_entry *, int);
int queue_init(bool);
