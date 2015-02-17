#
# [target : dependent]
#
# $@ | target
# $% | target member
#    | --> target=libxx.a(xx.o) ... $@=libxx.a $%=xx.o
#    |
# $< | dependent first
# $? | dependent newest target
# $^ | dependent all
#
# ---------------------------------------------------------------------
#
# [json]
# https://jansson.readthedocs.org/en/2.7/index.html
#
# ---------------------------------------------------------------------
#
# [base64]
# http://src.gnu-darwin.org/src/contrib/wpa_supplicant/base64.h
# http://src.gnu-darwin.org/src/contrib/wpa_supplicant/base64.c
#

CC        = gcc

#
OBJSL     = sfqc-xinetd.o sfqc-push.o sfqc-lib.o sfqc-take.o base64.o

OBJSE     = sfqc-init.o sfqc-info.o sfqc-list.o sfqc-pusht.o sfqc-pushb.o
OBJSE    += sfqc-pop.o sfqc-shift.o sfqc-clear.o sfqc-reset-procs.o sfqc-sets.o sfqc-disable.o
OBJSE    += sfqc-pushx.o sfqc-popx.o sfqc-shiftx.o

OBJS      = $(OBJSL) $(OBJSE)

#
TARGETL   = libsfqc.so

TARGETE   = sfqc-init sfqc-info sfqc-list sfqc-pusht sfqc-pushb sfqc-pop
TARGETE  += sfqc-shift sfqc-clear sfqc-reset-procs sfqc-sets sfqc-disable
TARGETE  += sfqc-pushx sfqc-popx sfqc-shiftx

TARGET    = $(TARGETL) $(TARGETE)

#
INCDIRS   = -I. -I../inc -I../../include
INCFILES  = ../inc/sfq.h sfqc-lib.h base64.h

CFLAGS    = -Wall -O2 -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 $(CFLAGSEXT)
SCFLAGS   = $(CFLAGS) -fPIC

SLDFLAGS  = -Wall -shared
ELDFLAGS  = -Wall

LIBDIRS   = -L. -L../libsfq -L../../lib
LIBS      = -lsfqc -lsfq -lpthread -lm -luuid -ljansson -lcap -lrt

#
all: $(TARGET)

libsfqc.so: $(OBJSL)
	$(CC) $(SLDFLAGS) -o $@ $^

sfqc-xinetd.o: sfqc-xinetd.c
	$(CC) $(SCFLAGS) $(INCDIRS) -c $<

sfqc-push.o: sfqc-push.c
	$(CC) $(SCFLAGS) $(INCDIRS) -c $<

sfqc-lib.o: sfqc-lib.c
	$(CC) $(SCFLAGS) $(INCDIRS) -c $<

sfqc-take.o: sfqc-take.c
	$(CC) $(SCFLAGS) $(INCDIRS) -c $<

base64.o: base64.c
	$(CC) $(SCFLAGS) -Wno-unused-but-set-variable $(INCDIRS) -c $<

#
sfqc-popx.o: sfqc-pop.c
	$(CC) $(CFLAGS) $(INCDIRS) -DFROM_XINETD -o $@ -c $<

sfqc-shiftx.o: sfqc-shift.c
	$(CC) $(CFLAGS) $(INCDIRS) -DFROM_XINETD -o $@ -c $<

sfqc-pushx: sfqc-pushx.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-shiftx: sfqc-shiftx.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

#
sfqc-init: sfqc-init.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-info: sfqc-info.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-list: sfqc-list.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-clear: sfqc-clear.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-reset-procs: sfqc-reset-procs.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-sets: sfqc-sets.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-disable: sfqc-disable.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

#
sfqc-pusht: sfqc-pusht.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-pushb: sfqc-pushb.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-pop: sfqc-pop.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-popx: sfqc-popx.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-shift: sfqc-shift.o
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

#
$(OBJS): $(INCFILES) Makefile

.c.o:
	$(CC) $(CFLAGS) $(INCDIRS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)

debug:
	make clean
	make "CFLAGSEXT=-g -O0 -DSFQ_DEBUG_BUILD"

#

