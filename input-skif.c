// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include "input-skif.h"
#include "skif-arduino.h"
#include "queue.h"

#define Rate 1 /* 8kHz (125usec) */
#define Debounce 32 /* 4ms */
#define MaxCounter DEFAULT_MAX_COUNTER

/* XXX Rate=0 not supported */
#define TicksToMicroSeconds(x) ((x) * 125 * (1 << (Rate - 1)))
#define TICKS_LIMIT (10000000 / TicksToMicroSeconds(1))

static int PinStatus = 0;
static int Ticks = 0;
static bool Timeout = false;
static int fd_ser;

static bool set_nonblock(int d, bool nonblock)
{
	int flags;

	return ((flags = fcntl(d, F_GETFL)) < 0 ||
		fcntl(d, F_SETFL, nonblock ?
		      (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0);
}

static int open_serial(char *serdev)
{
	int fd;
	struct termios t;

	if ((fd = open(serdev,
		       O_RDWR | O_NOCTTY | O_EXCL | O_NONBLOCK)) < 0)
		goto fin0;

	memset(&t, 0, sizeof(t));
#if B38400 == 38400
	cfsetospeed(&t, 500000);
	cfsetispeed(&t, 500000);
#else
	cfsetospeed(&t, B500000);
	cfsetispeed(&t, B500000);
#endif
	t.c_cflag |= CREAD | CLOCAL | CS8;
	t.c_iflag = INPCK;
	t.c_oflag = 0;
	t.c_lflag = 0;
	t.c_cc[VTIME] = 0;
	t.c_cc[VMIN] = 1;

	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &t);
	
fin0:
	return fd;
}

static int send_command(int fd, unsigned char *c, int len)
{
	unsigned char r;
	int i;

	/* send command */
	write(fd, c, len);

	/* wait for response */
	for (i = 0; i < 10; i++) {
		if (read(fd, &r, sizeof(r)) < 1) {
			usleep(10000);
			continue;
		}

		if (r == 0)
			break;
		i = 0;
	}

	return (i < 10) ? 0 : -1;
}

static int wait_for_device(int fd, int rate, int debounce, int max)
{
	int i;
	char c[2];

	c[0] = CMD_RESET;
	for (i = 0; i < 10; i++) {
		if (!send_command(fd, c, 1))
			break;
		usleep(100000);
	}
	if (i >= 10)
		return -1;

	c[0] = CMD_RATE(rate);
	send_command(fd, c, 1);

	c[0] = CMD_DEBOUNCE_COUNTER;
	c[1] = debounce;
	send_command(fd, c, 2);

	c[0] = CMD_MAX_COUNTER;
	c[1] = max;
	send_command(fd, c, 2);

	if (set_nonblock(fd, false)) {
		printf("non-block mode set failed\n");
		return -1;
	}

	return 0;
}

int skif_init(bool start, char *arg, int var)
{
	int ret = -1;

	if ((fd_ser = open_serial((arg == NULL) ? "/dev/ttyACM0" : arg)) < 0) {
		fprintf(stderr, "device open error\n");
		goto fin0;
	}

	fprintf(stderr, "wait for device...\n");
	if (wait_for_device(fd_ser, Rate, Debounce, MaxCounter)) {
		fprintf(stderr, "device not ready\n");
		goto fin1;
	}

	ret = 0;
	fprintf(stderr, "device ready\n");

	goto fin0;

fin1:
	close(fd_ser);
fin0:
	return ret;
}

static bool get_status_and_time(unsigned char status)
{
	int pin, counter;
	struct queue_entry q;
	bool quit = false;

	pin = status & PIN0_ON;
	counter = status & COUNTER_MASK;

	if (!counter) {
		PinStatus = pin;
		Ticks = 0;
		Timeout = false;
	} else if (PinStatus != pin) {
		q.elapsed_time_us = TicksToMicroSeconds(Ticks);
		q.state = PinStatus ? 1 : 0;
		quit = (enqueue(&q) < 0);
		PinStatus = pin;
		Ticks = counter;
		Timeout = false;
	} else if (!Timeout) {
		if (Ticks < TICKS_LIMIT) {
			Ticks += counter;
		} else {
			q.elapsed_time_us = TicksToMicroSeconds(TICKS_LIMIT);
			q.state = PinStatus ? 1 : 0;
			quit = (enqueue(&q) < 0);
			Timeout = true;
		}
	}

	return quit;
}

void *skif_thread(void *arg)
{
	int r;
	unsigned char c;

	c = CMD_START;
	write(fd_ser, &c, sizeof(c));

	while (1) {
		r = read(fd_ser, &c, sizeof(c));
		if (r < 0) {
			break;
		} if (r == 0) {
			sleep(1);
			continue;
		}

		get_status_and_time(c);
	}

	notify_die();
	return NULL;
}
