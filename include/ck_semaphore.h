/*
 * Copyright 2012 João Fernandes
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

#ifndef _CK_SEMAPHORE_H
#define _CK_SEMAPHORE_H

#include <stdbool.h>
#include <ck_cc.h>
#include <ck_pr.h>

struct ck_semaphore {
	unsigned int counter;
};
typedef struct ck_semaphore ck_semaphore_t;

CK_CC_INLINE static void
ck_semaphore_init(struct ck_semaphore * sem, unsigned int value)
{
	ck_pr_store_uint(&sem->counter, value);
	return;
}

CK_CC_INLINE static unsigned int
ck_semaphore_getvalue(struct ck_semaphore * sem)
{
	ck_pr_fence_load();
	return ck_pr_load_uint(&sem->counter);
}

CK_CC_INLINE static void
ck_semaphore_wait(struct ck_semaphore * sem)
{
	unsigned int current = ck_pr_load_uint(&sem->counter);
	do {
		if (current != 0) continue;
		while ((current = ck_pr_load_uint(&sem->counter)) == 0) ck_pr_stall();
	} while (ck_pr_cas_uint_value(&sem->counter, current, current - 1, &current) == false);
	ck_pr_fence_memory();
	return;
}

CK_CC_INLINE static bool
ck_semaphore_trywait(struct ck_semaphore * sem)
{
	unsigned int current = ck_pr_load_uint(&sem->counter);
	if (current == 0) return false;
	if (ck_pr_cas_uint(&sem->counter, current, current - 1) == true) {
		ck_pr_fence_memory();
		return true;
	}
	return false;
}

CK_CC_INLINE static void
ck_semaphore_signal(struct ck_semaphore * sem)
{
	ck_pr_fence_memory();
	ck_pr_inc_uint(&sem->counter);
	return;
}

#endif /* _CK_SEMAPHORE_H */

