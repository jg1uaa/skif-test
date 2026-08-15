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

int send_and_verify(int fd, unsigned char c)
{
	unsigned char r;
	int i;

	for (i = 0; i < 10; i++) {
		write(fd, &c, sizeof(c));
		sleep(1);
		
		if (read(fd, &r, sizeof(r)) >= 1) {
			if (r == c) break;
			else return -1;
		}

		sleep(1);
	}
	if (i >= 10)
		return -1;

	return 0;
}

int wait_for_device(int fd)
{
	if (send_and_verify(fd, CMD_READY))
		return -1;

	if (send_and_verify(fd, CMD_RATE(1)))
		return -1;

	if (set_nonblock(fd, false)) {
		printf("non-block mode set failed\n");
		return -1;
	}

	return 0;
}
