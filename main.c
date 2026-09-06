// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "queue.h"
#include "input-libinput.h"
#include "input-skif.h"

extern char *optarg;
static uint32_t basetime_us = 100000;

static void push_status(struct queue_entry *q)
{
	char c;

	if (q->state) {
		if (q->elapsed_time_us < basetime_us / 2)
			c = 'X';
		else if (q->elapsed_time_us < (basetime_us * 3) / 2)
			c = '.';
		else if (q->elapsed_time_us < basetime_us * 2)
			c = '?';
		else if (q->elapsed_time_us < basetime_us * 6)
			c = '-';
		else
			c = 'X';
	} else {
		if (q->elapsed_time_us < basetime_us / 2)
			c = 'x';
		else if (q->elapsed_time_us < (basetime_us * 3) / 2)
			c = 0;
		else if (q->elapsed_time_us < basetime_us * 2)
			c = '!';
		else if (q->elapsed_time_us < basetime_us * 4)
			c = ' ';
		else
			c = '\n';
	}

	if (c) {
		putchar(c);
		fflush(stdout);
	}
}

static void do_main(void)
{
	struct queue_entry q;
	int r, last_sw = -1;
	bool timeout = false, started = false;

	while (1) {
		r = dequeue(&q, basetime_us / 100);

		if (r < 0) {
			/* error */
			break;
		} else if (r == 0) {
			if (q.state != last_sw) {
				if (!timeout)
					push_status(&q);

				started = true;
				timeout = false;
				last_sw = q.state;
			}
		} else if (r > 0 && started && !timeout) {
			q.state = !last_sw;
			q.elapsed_time_us = basetime_us * 10;
			push_status(&q);

			timeout = true;
		}
	}
}

int main(int argc, char *argv[])
{
	int ch;
	char *port = NULL;
	pthread_t tid;
	int (*init)(bool, char *, int) = skif_init;
	void *(*thread)(void *) = skif_thread;

	while ((ch = getopt(argc, argv, "l:d:k")) != -1) {
		switch (ch) {
		case 'l':
			port = optarg;
			break;
		case 'd':
			basetime_us = atoi(optarg) * 1000;
			break;
		case 'k':
			init = libinput_init;
			thread = libinput_thread;
			break;
		}
	}

	if (queue_init(true) < 0) {
		fprintf(stderr, "que_init error\n");
		goto fin0;
	}

	if ((*init)(true, port, 0) < 0) {
		fprintf(stderr, "device initialize error\n");
		goto fin1;
	}

	if (pthread_create(&tid, NULL, thread, NULL)) {
		fprintf(stderr, "pthread_create error\n");
		goto fin2;
	}

	do_main();

	pthread_join(tid, NULL);
fin2:
	(*init)(false, port, 0);
fin1:
	queue_init(false);
fin0:
	return 0;
}
