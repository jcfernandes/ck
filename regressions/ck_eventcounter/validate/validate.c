/*
 * Copyright 2013 João Fernandes.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>

#include <ck_pr.h>
#include <ck_eventcounter.h>
#include <ck_spinlock.h>

#include "../../common.h"

#ifndef ITERATE
#define ITERATE 500
#endif

static struct affinity a;
static int nthr;
static ck_eventcounter_t g_ec;
static int g_counter;
static int g_tid;

static void *
test_signal_all(void *null CK_CC_UNUSED)
{
	ck_eventcounter_state_t state;
	ck_eventcounter_state_init(&state);
	int const tid = ck_pr_faa_int(&g_tid, 1);

	aff_iterate(&a);

	while (ck_pr_load_int(&g_counter) < ITERATE) {
		ck_eventcounter_prepare_wait(&g_ec, &state);
		int const tmp = ck_pr_load_int(&g_counter);
		if (tmp % nthr == tid) {
			ck_eventcounter_retire_wait(&g_ec, &state);
			ck_pr_inc_int(&g_counter);
			ck_eventcounter_notify_all(&g_ec);
		} else {
			ck_eventcounter_wait(&g_ec, &state);
		}
	}
	return NULL;
}

static void *
test_signal_one(void *null CK_CC_UNUSED)
{
	ck_eventcounter_state_t state;
	ck_eventcounter_state_init(&state);
	int const tid = ck_pr_faa_int(&g_tid, 1);

	aff_iterate(&a);

	while (ck_pr_load_int(&g_counter) < ITERATE) {
		ck_eventcounter_prepare_wait(&g_ec, &state);
		int const tmp = ck_pr_load_int(&g_counter);
		if (tmp % nthr == tid) {
			ck_eventcounter_retire_wait(&g_ec, &state);
			ck_pr_inc_int(&g_counter);
			ck_eventcounter_notify_one(&g_ec);
		} else {
			ck_eventcounter_notify_one(&g_ec);
			ck_eventcounter_wait(&g_ec, &state);
		}
	}
	return NULL;
}

int
main(int argc, char *argv[])
{
	pthread_t *threads;
	int i;

	if (argc != 3) {
		fprintf(stderr, "Usage: correct <number of threads> <affinity delta>\n");
		exit(EXIT_FAILURE);
	}

	nthr = atoi(argv[1]);
	if (nthr <= 0) {
		fprintf(stderr, "ERROR: Number of threads must be greater than 0\n");
		exit(EXIT_FAILURE);
	}

	threads = malloc(sizeof(pthread_t) * nthr);
	if (threads == NULL) {
		fprintf(stderr, "ERROR: Could not allocate thread structures\n");
		exit(EXIT_FAILURE);
	}

	a.delta = atoi(argv[2]);
	ck_eventcounter_init(&g_ec);

	fprintf(stderr, "Creating threads (event counter - signal all)...");
	for (i = 0; i < nthr; i++) {
		if (pthread_create(&threads[i], NULL, test_signal_all, NULL)) {
		fprintf(stderr, "ERROR: Could not create thread %d\n", i);
		exit(EXIT_FAILURE);
		}
	}
	fprintf(stderr, "done\n");

	fprintf(stderr, "Waiting for threads to finish correctness regression...\n");
	for (i = 0; i < nthr; i++)
		pthread_join(threads[i], NULL);
	fprintf(stderr, "done (passed)\n");

	fprintf(stderr, "Creating threads (event counter - signal one)...");
	g_counter = g_tid = 0;
	for (i = 0; i < nthr; i++) {
		if (pthread_create(&threads[i], NULL, test_signal_one, NULL)) {
			fprintf(stderr, "ERROR: Could not create thread %d\n", i);
			exit(EXIT_FAILURE);
		}
	}
	fprintf(stderr, "done\n");

	fprintf(stderr, "Waiting for threads to finish correctness regression...\n");
	for (i = 0; i < nthr; i++)
		pthread_join(threads[i], NULL);
	fprintf(stderr, "done (passed)\n");

	return (0);
}
