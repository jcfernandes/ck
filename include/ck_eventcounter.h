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

#ifndef _CK_EVENTCOUNTER_H
#define _CK_EVENTCOUNTER_H

#include <ck_cc.h>
#include <ck_md.h>
#include <ck_pr.h>
#include <ck_spinlock.h>
#include <ck_semaphore.h>
#include <ck_queue.h>
#include <stdbool.h>

struct ck_eventcounter_state {
	CK_LIST_ENTRY(ck_eventcounter_state) node;
	ck_semaphore_t sem;
	unsigned int epoch;
	char in_waitset;
	char spurious;
};
typedef struct ck_eventcounter_state ck_eventcounter_state_t;

typedef CK_LIST_HEAD(ck_eventcounter_waitset, ck_eventcounter_state) ck_eventcounter_waitset_t;

struct ck_eventcounter {
	ck_spinlock_t lock; // TODO: make type of spinlock configurable?
	ck_eventcounter_waitset_t waitset;
	unsigned int epoch;
};
typedef struct ck_eventcounter ck_eventcounter_t;

CK_CC_INLINE static void
ck_eventcounter_state_init(struct ck_eventcounter_state *state)
{
	ck_semaphore_init(&state->sem, 0);
	state->epoch = 0;
	state->in_waitset = false;
	state->spurious = false;
	return;
}

CK_CC_INLINE static void
ck_eventcounter_init(struct ck_eventcounter *ec)
{
	ck_spinlock_init(&ec->lock);
	CK_LIST_INIT(&ec->waitset);
	ec->epoch = 0;
	ck_pr_fence_store();
	return;
}

CK_CC_INLINE static void
ck_eventcounter_prepare_wait(struct ck_eventcounter *ec,
                             struct ck_eventcounter_state *state)
{
	if (state->spurious) {
		state->spurious = false;
		ck_semaphore_wait(&state->sem);
	}

	/*
	* Can the next line ever be reordered in a way that the store
	* precedes the if statement above?
	* */
	state->in_waitset = true;
	ck_spinlock_lock(&ec->lock);
	state->epoch = ec->epoch;
	CK_LIST_INSERT_HEAD(&ec->waitset, state, node);
	ck_spinlock_unlock(&ec->lock);
	ck_pr_fence_memory();
	return;
}

CK_CC_INLINE static void
ck_eventcounter_retire_wait(struct ck_eventcounter *ec,
                            struct ck_eventcounter_state *state)
{
	/*
	* Can the next line ever be reordered in a way that the store
	* succeeds the if statement that comes immediately after?
	* */
	state->spurious = true;

	/*
	* Without using ck's load interface in both if statements, the
	* compiler would be free to optimize this code, loading
	* state->in_waitset just once, correct?
	* */
	if (ck_pr_load_char(&state->in_waitset)) {
		ck_spinlock_lock(&ec->lock);
		if (ck_pr_load_char(&state->in_waitset)) {
			state->in_waitset = false;
			state->spurious = false;
			CK_LIST_REMOVE(state, node);
		}
		ck_spinlock_unlock(&ec->lock);
	}
	return;
}

CK_CC_INLINE static void
ck_eventcounter_wait(struct ck_eventcounter *ec,
                     struct ck_eventcounter_state *state)
{
	if (state->epoch == ec->epoch) {
		ck_semaphore_wait(&state->sem);
	} else {
		ck_eventcounter_retire_wait(ec, state);
	}
	return;
}

CK_CC_INLINE static void
ck_eventcounter_notify_one(struct ck_eventcounter *ec)
{
	ck_eventcounter_state_t *state;

	ck_pr_fence_memory();

	if (CK_LIST_EMPTY(&ec->waitset)) return;

	ck_spinlock_lock(&ec->lock);
	ec->epoch++;
	state = CK_LIST_FIRST(&ec->waitset);

	if (state != NULL) {
		CK_LIST_REMOVE(state, node);
		state->in_waitset = false;
	}

	ck_spinlock_unlock(&ec->lock);

	if (state != NULL) {
		ck_semaphore_signal(&state->sem);
	}
	return;
}

CK_CC_INLINE static void
ck_eventcounter_notify_all(struct ck_eventcounter *ec)
{
	ck_eventcounter_state_t *state, *tmp_state;
	ck_eventcounter_waitset_t tmp_waitset;

	ck_pr_fence_memory();

	if (CK_LIST_EMPTY(&ec->waitset)) return;

	ck_spinlock_lock(&ec->lock);
	ec->epoch++;

	CK_LIST_FOREACH(state, &ec->waitset, node) {
		state->in_waitset = false;
	}

	tmp_waitset = ec->waitset;
	CK_LIST_RESET(&ec->waitset);
	ck_spinlock_unlock(&ec->lock);

	CK_LIST_FOREACH_SAFE(state, &tmp_waitset, node, tmp_state) {
		ck_semaphore_signal(&state->sem);
	}
	return;
}

#endif /* _CK_EVENTCOUNTER_H */
