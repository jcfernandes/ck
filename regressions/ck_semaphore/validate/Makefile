.PHONY: clean distribution

HEADER=../../../include/ck_semaphore.h
OBJECTS=ck_semaphore

all: $(OBJECTS)

ck_semaphore: $(HEADER) ck_semaphore.c
	$(CC) $(CFLAGS) -o ck_semaphore ck_semaphore.c

clean:
	rm -rf *~ *.o $(OBJECTS) *.dSYM

include ../../../build/regressions.build
CFLAGS+=$(PTHREAD_CFLAGS) -D_GNU_SOURCE
