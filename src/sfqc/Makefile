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

CC       = gcc
OBJSL    = sfqc-lib.o sfqc-to.o base64.o
OBJSE    = sfqc-init.o sfqc-info.o sfqc-list.o sfqc-pusht.o sfqc-pushb.o sfqc-pop.o sfqc-shift.o sfqc-clear.o sfqc-sets.o
OBJS     = $(OBJSL) $(OBJSE)

TARGETE  = sfqc-init sfqc-info sfqc-list sfqc-pusht sfqc-pushb sfqc-pop sfqc-shift sfqc-clear sfqc-sets
TARGETL  = libsfqc.so
TARGET   = $(TARGETL) $(TARGETE)

INCDIRS  = -I. -I../inc -I../../include
INCFILES = ../inc/sfq.h sfqc-lib.h base64.h

CFLAGS   = -Wall -O2 -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 $(CFLAGSEXT)
CFLAGSL  = $(CFLAGS) -fPIC
LDFLAGS  = -Wall -shared

LIBDIRS  = -L. -L../libsfq -L../../lib
LIBS     = -lsfqc -lsfq -lpthread -lm -luuid -ljansson -lcap

#
all: $(TARGET)

libsfqc.so: $(OBJSL)
	$(CC) $(LDFLAGS) -o $@ $^

sfqc-lib.o: sfqc-lib.c
	$(CC) $(CFLAGSL) $(INCDIRS) -c $<

sfqc-to.o: sfqc-to.c
	$(CC) $(CFLAGSL) $(INCDIRS) -c $<

base64.o: base64.c
	$(CC) $(CFLAGSL) -Wno-unused-but-set-variable $(INCDIRS) -c $<

#
sfqc-init: sfqc-init.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-info: sfqc-info.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-list: sfqc-list.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-pusht: sfqc-pusht.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-pushb: sfqc-pushb.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-pop: sfqc-pop.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-shift: sfqc-shift.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-clear: sfqc-clear.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

sfqc-sets: sfqc-sets.o
	$(CC) $(CFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

$(OBJS): $(INCFILES) Makefile

.c.o:
	$(CC) $(CFLAGS) $(INCDIRS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)

debug:
	make clean
	make "CFLAGSEXT=-g -O0 -DSFQ_DEBUG_BUILD"

#

