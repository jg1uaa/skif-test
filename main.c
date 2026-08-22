// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "serial.h"

extern char *optarg;

static int PinStatus = 0;
static int Ticks = 0;
static int Rate = DEFAULT_RATE;
static int Debounce = DEFAULT_DEBOUNCE_COUNTER;
static int MaxCounter = DEFAULT_MAX_COUNTER;

#define TicksToMilliSeconds(x) ((x) * 0.0625 * (1 << Rate))
#define TICKS_LIMIT ((int)(10000.0 / TicksToMilliSeconds(1)))

static char *pinstatus_char(unsigned char pin)
{
	static char c[3];

	c[0] = (pin & PIN1_ON) ? 'o' : '-';
	c[1] = (pin & PIN0_ON) ? 'o' : '-';
	c[2] = '\0';

	return c;
}

static void get_status_and_time(unsigned char status)
{
	int pin, counter;

	pin = status & PIN_MASK;
	counter = status & COUNTER_MASK;

	if (!counter) {
		printf("%s (initial state)\n", pinstatus_char(pin));
		PinStatus = pin;
		Ticks = 0;
	} else if (PinStatus != pin) {
		printf("%s: %9.3f ms\n",
		       pinstatus_char(PinStatus), TicksToMilliSeconds(Ticks));
		PinStatus = pin;
		Ticks = counter;
	} else {
		if (Ticks < TICKS_LIMIT)
			Ticks += counter;
	}
}

static int do_main(int fd)
{
	unsigned char c;

	c = CMD_START;
	write(fd, &c, sizeof(c));

	while (1) {
		if (read(fd, &c, sizeof(c)) < 1) {
			sleep(1);
			continue;
		}

		get_status_and_time(c);
	}
}

int main(int argc, char *argv[])
{
	int ch, fd;
	char *port = NULL;

	while ((ch = getopt(argc, argv, "l:r:d:m:")) != -1) {
		switch (ch) {
		case 'l':
			port = optarg;
			break;
		case 'r':
			Rate = atoi(optarg);
			break;
		case 'd':
			Debounce = atoi(optarg);
			break;
		case 'm':
			MaxCounter = atoi(optarg);
			break;
		}
	}

	if (port == NULL) {
		fprintf(stderr, "%s -l [device] -r [(rate)] "
			"-d [(debounce)] -m [(max count)]\n",
			argv[0]);
		goto fin0;
	}

	fd = open_serial(port);
	if (fd < 0) {
		fprintf(stderr, "device open error\n");
		goto fin0;
	}

	fprintf(stderr, "wait for device...\n");
	if (wait_for_device(fd, Rate, Debounce, MaxCounter)) {
		fprintf(stderr, "device not ready\n");
		goto fin1;
	}

	fprintf(stderr, "device ready\n");
	do_main(fd);

fin1:
	close(fd);
fin0:
	return 0;
}
