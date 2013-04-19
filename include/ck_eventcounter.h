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
        /*
         * No need for memory fence as all accesses to shared data
         * are synchronized.
         */
	return;
}

CK_CC_INLINE static void
ck_eventcounter_init(struct ck_eventcounter *ec)
{
	ck_spinlock_init(&ec->lock);
	CK_LIST_INIT(&ec->waitset);
	ec->epoch = 0;
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

	ck_spinlock_lock(&ec->lock);
	state->epoch = ec->epoch;
	CK_LIST_INSERT_HEAD(&ec->waitset, state, node);
	state->in_waitset = true;
	ck_spinlock_unlock(&ec->lock);
	return;
}

CK_CC_INLINE static void
ck_eventcounter_retire_wait(struct ck_eventcounter *ec,
                            struct ck_eventcounter_state *state)
{
        state->spurious = true;
        /*
         * TODO(?): is the following DCL correctly implemented?
         */
	if (ck_pr_load_char(&state->in_waitset)) {
		ck_spinlock_lock(&ec->lock);
		if (ck_pr_load_char(&state->in_waitset)) {
			CK_LIST_REMOVE(state, node);
			state->in_waitset = false;
			state->spurious = false;
		}

		ck_spinlock_unlock(&ec->lock);
	}

	return;
}

CK_CC_INLINE static void
ck_eventcounter_wait(struct ck_eventcounter *ec,
                     struct ck_eventcounter_state *state)
{
        /*
         * The calling thread has to serialize with the signaling thread.
         */
	if (state->epoch == ec->epoch) {
                /*
                 * Full memory fence inside.
                 */
		ck_semaphore_wait(&state->sem);
	} else {
                /*
                 * Load memory fence to serialize with the signaling thread.
                 */
                ck_pr_fence_load();
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
	state = CK_LIST_FIRST(&ec->waitset);
	if (state != NULL) {
		state->in_waitset = false;
		CK_LIST_REMOVE(state, node);
	}

        /*
         * Incrementing the epoch comes in the end of this critical section
         * in order to minimize the chance of having waiting threads
         * going through the slow path.
         */
	ec->epoch++;
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

	ck_pr_fence_memory();
	if (CK_LIST_EMPTY(&ec->waitset)) return;

	ck_spinlock_lock(&ec->lock);
        /*
         * Either we deep copy the waitset and are able to signal threads
         * outside the critical section, or we have to do it inside
         * the critical section as it is now.
         */
	CK_LIST_FOREACH_SAFE(state, &ec->waitset, node, tmp_state) {
		state->in_waitset = false;
                ck_semaphore_signal(&state->sem);
	}

	CK_LIST_RESET(&ec->waitset);
	ec->epoch++;
	ck_spinlock_unlock(&ec->lock);
	return;
}

#endif /* _CK_EVENTCOUNTER_H */
