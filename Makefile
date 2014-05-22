#CC      = xlc
CC      = gcc
#CFLAGS  = -O0 -q64 -g -D_GNU_SOURCE -D_THREAD_SAFE_ -D_REENTRANT_ -DWITH_MSISDN -DWITH_IMEI
#LDFLAGS = -O2 -q64 -g -lpthread
CFLAGS  = -O0  -ggdb3 -D_GNU_SOURCE -D_THREAD_SAFE_ -D_REENTRANT_ -DWITH_MSISDN -DWITH_IMEI
LDFLAGS = -O2  -g -lpthread

OBJECTS = config.o context.o reader-thread.o main.o comsumer-thread.o log.o

APP = saCellContext

.PHONY: clean all

all:: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(APP) $(OBJECTS)
	-mkdir -p ../bin
	mv -f $(APP) ../bin

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

config.c: config.h log.h
context.c: config.h context.h log.h reader-thread.h
reader-thread.c: reader-thread.h context.h comsumer-thread.h config.h log.h
comsumer-thread.c: comsumer-thread.h context.h config.h log.h
main.c: comsumer-thread.h context.h config.h log.h reader-thread.h
log.c: log.h

install:: all
	cp ../bin/$(APP) ../../../../bin/c

clean:
	-rm -f *.o ../bin/*

