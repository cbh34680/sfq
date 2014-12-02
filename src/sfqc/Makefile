
CC       = gcc
OBJS     = sfqc-lib.o sfqc-init.o sfqc-info.o sfqc-list.o sfqc-pushc.o sfqc-pushb.o sfqc-pop.o sfqc-shift.o sfqc-clear.o

TARGET   = $(TARGETL) $(TARGETE)
TARGETE  = sfqc-init sfqc-info sfqc-list sfqc-pushc sfqc-pushb sfqc-pop sfqc-shift sfqc-clear
TARGETL  = libsfqc.so

INCDIRS  = -I../inc -I.
INCFILES = ../inc/sfq.h sfqc-lib.h

CFLAGS   = -Wall -O2 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 $(CFLAGSEXT)
CFLAGSL  = $(CFLAGS) -fPIC
LDFLAGS  = -Wall -shared

LIBDIRS  = -L../libsfq -L.
LIBS     = -lsfqc -lsfq -lpthread -lm

#
all: $(TARGET)

sfqc-lib.o: sfqc-lib.c
	$(CC) $(CFLAGSL) $(INCDIRS) -c $<

libsfqc.so: sfqc-lib.o
	$(CC) $(LDFLAGS) -o $@ $<

sfqc-init: sfqc-init.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-info: sfqc-info.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-list: sfqc-list.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-pushc: sfqc-pushc.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-pushb: sfqc-pushb.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-pop: sfqc-pop.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-shift: sfqc-shift.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-clear: sfqc-clear.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

$(OBJS): $(INCFILES)

.c.o:
	$(CC) $(CFLAGS) $(INCDIRS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)

debug:
	make clean
	make "CFLAGSEXT=-g -O0 -DSFQ_DEBUG_BUILD"

#

