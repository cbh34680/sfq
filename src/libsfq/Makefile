
CC        = gcc
OBJS      = sfq-fio.o sfq-init.o sfq-info.o sfq-prt.o sfq-misc.o sfq-push.o sfq-pop.o sfq-shift.o sfq-map.o sfq-clear.o sfq-exec.o sfq-log.o
TARGET    = libsfq.so

INCDIRS   = -I../inc -I.
INCFILES  = ../inc/sfq.h sfq-lib.h
CFLAGSEXT = -O2

# -Wl,--stack,10485760
CFLAGS    = -Wall -fPIC -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 $(CFLAGSEXT)
LDFLAGS   = -Wall -shared

#
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $(INCDIRS) -c $<

$(OBJS): $(INCFILES)

clean:
	rm -f $(TARGET)
	rm -f *.o *.s *.i

debug:
	make clean
	make "CFLAGSEXT=-g -O0 -DSFQ_DEBUG_BUILD"

#

