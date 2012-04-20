.PHONY: all clean

OBJECTS=ck_ticket.THROUGHPUT ck_ticket.LATENCY			\
	ck_mcs.THROUGHPUT ck_mcs.LATENCY			\
	ck_dec.THROUGHPUT ck_dec.LATENCY			\
	ck_cas.THROUGHPUT ck_cas.LATENCY			\
	ck_fas.THROUGHPUT ck_fas.LATENCY			\
	ck_clh.THROUGHPUT ck_clh.LATENCY			\
	linux_spinlock.THROUGHPUT linux_spinlock.LATENCY	\
	ck_ticket_pb.THROUGHPUT ck_ticket_pb.LATENCY		\
	ck_anderson.THROUGHPUT ck_anderson.LATENCY		\
	ck_spinlock.THROUGHPUT ck_spinlock.LATENCY

all: $(OBJECTS)

ck_spinlock.THROUGHPUT: ck_spinlock.c
	$(CC) -DTHROUGHPUT -o ck_spinlock.THROUGHPUT ck_spinlock.c $(CFLAGS) 

ck_spinlock.LATENCY: ck_spinlock.c
	$(CC) -DLATENCY -o ck_spinlock.LATENCY ck_spinlock.c $(CFLAGS) 

ck_ticket.THROUGHPUT: ck_ticket.c
	$(CC) -DTHROUGHPUT -o ck_ticket.THROUGHPUT ck_ticket.c $(CFLAGS) 

ck_ticket.LATENCY: ck_ticket.c
	$(CC) -DLATENCY -o ck_ticket.LATENCY ck_ticket.c $(CFLAGS) 

ck_mcs.THROUGHPUT: ck_mcs.c
	$(CC) -DTHROUGHPUT -o ck_mcs.THROUGHPUT ck_mcs.c $(CFLAGS) 

ck_mcs.LATENCY: ck_mcs.c
	$(CC) -DLATENCY -o ck_mcs.LATENCY ck_mcs.c $(CFLAGS) 

ck_dec.THROUGHPUT: ck_dec.c
	$(CC) -DTHROUGHPUT -o ck_dec.THROUGHPUT ck_dec.c $(CFLAGS) 

ck_dec.LATENCY: ck_dec.c
	$(CC) -DLATENCY -o ck_dec.LATENCY ck_dec.c $(CFLAGS) 

ck_cas.THROUGHPUT: ck_cas.c
	$(CC) -DTHROUGHPUT -o ck_cas.THROUGHPUT ck_cas.c $(CFLAGS) 

ck_cas.LATENCY: ck_cas.c
	$(CC) -DLATENCY -o ck_cas.LATENCY ck_cas.c $(CFLAGS) 

ck_fas.THROUGHPUT: ck_fas.c
	$(CC) -DTHROUGHPUT -o ck_fas.THROUGHPUT ck_fas.c $(CFLAGS) 

ck_fas.LATENCY: ck_fas.c
	$(CC) -DLATENCY -o ck_fas.LATENCY ck_fas.c $(CFLAGS) 

ck_clh.THROUGHPUT: ck_clh.c
	$(CC) -DTHROUGHPUT -o ck_clh.THROUGHPUT ck_clh.c $(CFLAGS) 

ck_clh.LATENCY: ck_clh.c
	$(CC) -DLATENCY -o ck_clh.LATENCY ck_clh.c $(CFLAGS) 

linux_spinlock.THROUGHPUT: linux_spinlock.c
	$(CC) -DTHROUGHPUT -o linux_spinlock.THROUGHPUT linux_spinlock.c $(CFLAGS) 

linux_spinlock.LATENCY: linux_spinlock.c
	$(CC) -DLATENCY -o linux_spinlock.LATENCY linux_spinlock.c $(CFLAGS) 

ck_ticket_pb.THROUGHPUT: ck_ticket_pb.c
	$(CC) -DTHROUGHPUT -o ck_ticket_pb.THROUGHPUT ck_ticket_pb.c $(CFLAGS) 

ck_ticket_pb.LATENCY: ck_ticket_pb.c
	$(CC) -DLATENCY -o ck_ticket_pb.LATENCY ck_ticket_pb.c $(CFLAGS) 

ck_anderson.THROUGHPUT: ck_anderson.c
	$(CC) -DTHROUGHPUT -o ck_anderson.THROUGHPUT ck_anderson.c $(CFLAGS) 

ck_anderson.LATENCY: ck_anderson.c
	$(CC) -DLATENCY -o ck_anderson.LATENCY ck_anderson.c $(CFLAGS) 

clean:
	rm -rf *.dSYM $(OBJECTS)

include ../../../build/regressions.build
CFLAGS+=$(PTHREAD_CFLAGS) -D_GNU_SOURCE -lm
