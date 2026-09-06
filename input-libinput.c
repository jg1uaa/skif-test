// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <libudev.h>
#include <libinput.h>
#include "queue.h"
#include "input-libinput.h"

static struct udev *udev;
static struct libinput *li;

static int open_restricted(const char *path, int flags, void *user_data)
{
	int fd = open(path, flags);
	return fd < 0 ? -errno : fd;
}
 
static void close_restricted(int fd, void *user_data)
{
	close(fd);
}
 
const static struct libinput_interface interface = {
	.open_restricted = open_restricted,
	.close_restricted = close_restricted,
};

int libinput_init(bool start, char *arg, int var)
{
	int ret = -1;

	if (!start) {
		ret = 0;
		goto fin2;
	}

	if ((udev = udev_new()) == NULL) {
		fprintf(stderr, "udev_new error\n");
		goto fin0;
	}

	if ((li = libinput_udev_create_context(&interface,
					       NULL, udev)) == NULL) {
		fprintf(stderr, "libinput_udev_create_context error\n");
		goto fin1;
	}

	libinput_udev_assign_seat(li, (arg == NULL) ? "seat0" : arg);
	ret = 0;
	fprintf(stderr, "press [Esc] to quit\n");

	goto fin0;

fin2:
	libinput_unref(li);
fin1:
	udev_unref(udev);
fin0:
	return ret;
}

void *libinput_thread(void *arg)
{
	struct libinput_event *ev;
	struct pollfd pfd;
	struct libinput_event_keyboard *kev;
	struct libinput_event_pointer *pev;
	enum libinput_event_type type;
	uint32_t key, last_key = 0;
	uint64_t us, last_us = 0;
	int sw, last_sw = 0;
	bool quit = false, first = true;
	struct queue_entry q;

	pfd.fd = libinput_get_fd(li);
	pfd.events = POLLIN;

	while (!quit) {
		poll(&pfd, 1, -1);
		libinput_dispatch(li);

		while ((ev = libinput_get_event(li)) != NULL) {
			type = libinput_event_get_type(ev);

			switch (type) {
			default:
				libinput_event_destroy(ev);
				continue;

			case LIBINPUT_EVENT_POINTER_BUTTON:
				pev = libinput_event_get_pointer_event(ev);
				key = libinput_event_pointer_get_button(pev);
				sw = libinput_event_pointer_get_button_state(pev);
				us = libinput_event_pointer_get_time_usec(pev);
				break;

			case LIBINPUT_EVENT_KEYBOARD_KEY:
				kev = libinput_event_get_keyboard_event(ev);
				key = libinput_event_keyboard_get_key(kev);
				sw = libinput_event_keyboard_get_key_state(kev);
				us = libinput_event_keyboard_get_time_usec(kev);
				quit = (key == 1); /* ESC */
				break;
			}

			if (first) {
				/* first time, capture initial status */
				first = false;
				last_key = key;
				last_sw = sw;
				last_us = us;
			} else if (sw == last_sw ||
				   (last_sw && (key != last_key))) {
				/*
				 * key/button status not changed, or
				 * difference key/button released
				 * -> ignore
				 */
			} else if (!quit) {
				q.elapsed_time_us = us - last_us;
				q.state = last_sw ? 1 : 0;
				quit = (enqueue(&q) < 0);
				last_key = key;
				last_sw = sw;
				last_us = us;
			}

			libinput_event_destroy(ev);

		}

	}

	notify_die();
	return NULL;
}
