// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "queue.h"
#include "input-libinput.h"
#include "input-skif.h"
#include "table.h"

extern char *optarg;
static uint32_t basetime_us = 100000;

#define CODEBUFFER_SIZE 16
static char codebuffer[CODEBUFFER_SIZE];
static int codebuffer_index = 0;

#define DIT_TOO_SHORT 'X'
#define DIT_GOOD '.'
#define DIT_OR_DAH '?'
#define DAH_GOOD '-'
#define DAH_TOO_LONG '='

#define SPACE_TOO_SHORT 'x'
#define SPACE_GOOD '_'
#define SPACE_TOO_LONG '!'
#define CHAR_SPACE '~'
#define WORD_SPACE '#'

#define isDecodeFinish(x) ((x) == CHAR_SPACE || (x) == WORD_SPACE)

struct ditdah_def {
	double dit_too_short;
	double dit_good;
	double dit_or_dah;
	double dah_good;
	double space_too_short;
	double space_good;
	double space_too_long;
	double char_space;
};

const struct ditdah_def normal_def = {
	.dit_too_short = 0.5,
	.dit_good = 1.5,
	.dit_or_dah = 2,
	.dah_good = 6,
	.space_too_short = 0.5,
	.space_good = 1.5,
	.space_too_long = 2,
	.char_space = 4,
};

static const struct ditdah_def *ddef = &normal_def;
static const struct morse_table *decode_table = table_en;
static bool verbose = false;

static void codebuffer_init(void)
{
	memset(codebuffer, 0, sizeof(codebuffer));
	codebuffer_index = 0;
}

static void simple_display(char c)
{
	if (c != SPACE_GOOD && !isDecodeFinish(c)) {
		putchar(c);
		fflush(stdout);
	}

	if (isDecodeFinish(c)) {
		printf(" [%s] ", decode_code(decode_table, codebuffer));
		fflush(stdout);
	}

	if (c == WORD_SPACE) {
		putchar('\n');
	}
}

static void verbose_display(char c, struct queue_entry *q)
{
	printf("%c %.3f\n", c, (double)q->elapsed_time_us / 1000);

	if (isDecodeFinish(c)) {
		printf("* %s [%s]\n\n",
		       codebuffer, decode_code(decode_table, codebuffer));
	}
}

static void push_status(struct queue_entry *q)
{
	char c;

	if (q->state) {
		if (q->elapsed_time_us < basetime_us * ddef->dit_too_short)
			c = DIT_TOO_SHORT;
		else if (q->elapsed_time_us < basetime_us * ddef->dit_good)
			c = DIT_GOOD;
		else if (q->elapsed_time_us < basetime_us * ddef->dit_or_dah)
			c = DIT_OR_DAH;
		else if (q->elapsed_time_us < basetime_us * ddef->dah_good)
			c = DAH_GOOD;
		else
			c = DAH_TOO_LONG;
	} else {
		if (q->elapsed_time_us < basetime_us * ddef->space_too_short)
			c = SPACE_TOO_SHORT;
		else if (q->elapsed_time_us < basetime_us * ddef->space_good)
			c = SPACE_GOOD;
		else if (q->elapsed_time_us < basetime_us * ddef->space_too_long)
			c = SPACE_TOO_LONG;
		else if (q->elapsed_time_us < basetime_us * ddef->char_space)
			c = CHAR_SPACE;
		else
			c = WORD_SPACE;
	}

	if (c != SPACE_GOOD && !isDecodeFinish(c)) {
		if (codebuffer_index < sizeof(codebuffer) - 1)
			codebuffer[codebuffer_index++] = c;
	}

	if (verbose)
		verbose_display(c, q);
	else
		simple_display(c);

	if (isDecodeFinish(c))
		codebuffer_init();
}

static void do_main(void)
{
	struct queue_entry q;
	int r, last_sw = -1;
	bool timeout = false, started = false;

	codebuffer_init();

	while (1) {
		r = dequeue(&q, (basetime_us * ddef->char_space) / 1000);

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
			q.elapsed_time_us = basetime_us * ddef->char_space;
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

	while ((ch = getopt(argc, argv, "l:d:ksjev")) != -1) {
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
		case 's':
			init = skif_init;
			thread = skif_thread;
			break;
		case 'j':
			decode_table = table_jp;
			break;
		case 'e':
			decode_table = table_en;
			break;
		case 'v':
			verbose = true;
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
