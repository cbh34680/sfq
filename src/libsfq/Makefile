
CC         = gcc

OBJSL      = sfq-fio.o sfq-sem.o sfq-init.o sfq-info.o sfq-prt.o sfq-misc.o sfq-stat.o
OBJSL     += sfq-push.o sfq-pop.o sfq-shift.o sfq-map.o sfq-clear.o sfq-exec.o sfq-eloop.o sfq-log.o
OBJSL     += sfq-trc.o
OBJSE      = libtest-main.o
OBJS       = $(OBJSL) $(OBJSE)

TARGET     = libsfq.so libtest-main

INCDIRS    = -I. -I../inc
INCFILES   = sfq-lib.h ../inc/sfq.h
CFLAGSEXT  = -O2

# -Wl,--stack,10485760
CFLAGS    = -Wall -fPIC -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 $(CFLAGSEXT)
SLDFLAGS  = -Wall -shared
ELDFLAGS  = -Wall

LIBDIRS   = -L.
LIBS      = -lsfq -lpthread -lm -luuid -ljansson -lcap

#
all: $(TARGET)

libsfq.so: $(OBJSL)
	$(CC) $(SLDFLAGS) -o $@ $(OBJSL)

libtest-main: $(OBJSE)
	$(CC) $(ELDFLAGS) $(LIBDIRS) -o $@ $< $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCDIRS) -c $<

$(OBJS): $(INCFILES) Makefile

clean:
	rm -f $(TARGET)
	rm -f *.o *.s *.i

debug:
	make clean
	make "CFLAGSEXT=-g -O0 -DSFQ_DEBUG_BUILD"

#
# [注意]
#
# トレースファイルはロックされないので複数プロセス & 複数スレッド
# からライブラリを呼び出さないこと
#
trace:
	make clean
	make "CFLAGSEXT=-DSFQ_TRACE_FILE=\\\"/tmp/sfq-trace.log\\\""

#

