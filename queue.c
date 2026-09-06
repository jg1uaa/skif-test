// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "queue.h"

static unsigned int index_in = 0, index_out = 0;
static struct queue_entry qentry[QUEUE_DEPTH];
static pthread_mutex_t mutex;
static pthread_cond_t cond;
static pthread_condattr_t attr;
static volatile bool die = false;

void notify_die(void)
{
	pthread_mutex_lock(&mutex);
	die = true;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}	

int enqueue(struct queue_entry *q)
{
	int ret;

	pthread_mutex_lock(&mutex);

	if (die) {
		ret = -1;
	} else if (index_in - index_out < QUEUE_DEPTH) {
		memcpy(&qentry[index_in++ % QUEUE_DEPTH], q, sizeof(*q));
		pthread_cond_signal(&cond);
		ret = 0;
	} else {
		ret = 1;
	}

	pthread_mutex_unlock(&mutex);
	return ret;
}	

int dequeue(struct queue_entry *q, int timeout_ms)
{
	int ret;
	struct timespec origin, timeout;

	pthread_mutex_lock(&mutex);

	clock_gettime(CLOCK_MONOTONIC, &origin);
	origin.tv_nsec += (long)timeout_ms * 1000000L;
	timeout.tv_sec = origin.tv_sec + (origin.tv_nsec / 1000000000L);
	timeout.tv_nsec = origin.tv_nsec % 1000000000L;

	while (!die && index_out == index_in) {
		if ((ret = pthread_cond_timedwait(&cond, &mutex,
						  &timeout)) == ETIMEDOUT) {
			break;
		}
	}

	if (die) {
		ret = -1;
	} else if (index_out != index_in) {
		memcpy(q, &qentry[index_out++ % QUEUE_DEPTH], sizeof(*q));
		ret = 0;
	} else {
		ret = 1;
	}

	pthread_mutex_unlock(&mutex);
	return ret;
}

int queue_init(bool start)
{
	int ret = -1;

	if (!start) {
		ret = 0;
		goto fin3;
	}

	index_in = index_out = 0;
	die = false;

	if (pthread_mutex_init(&mutex, NULL)) {
		fprintf(stderr, "pthread_mutex_init error\n");
		goto fin0;
	}

	if (pthread_condattr_init(&attr)) {
		fprintf(stderr, "pthread_condattr_init error\n");
		goto fin1;
	}

	if (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
		fprintf(stderr, "pthread_condattr_setclock error\n");
		goto fin2;
	}

	if (pthread_cond_init(&cond, &attr)) {
		fprintf(stderr, "pthread_cond_init error\n");
		goto fin2;
	}

	ret = 0;
	goto fin0;

fin3:
	pthread_cond_destroy(&cond);
fin2:
	pthread_condattr_destroy(&attr);
fin1:
	pthread_mutex_destroy(&mutex);
fin0:
	return ret;
}
