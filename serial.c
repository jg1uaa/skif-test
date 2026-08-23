// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include "serial.h"

static bool set_nonblock(int d, bool nonblock)
{
	int flags;

	return ((flags = fcntl(d, F_GETFL)) < 0 ||
		fcntl(d, F_SETFL, nonblock ?
		      (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) < 0);
}

int open_serial(char *serdev)
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

int wait_for_device(int fd, int rate, int debounce, int max)
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
