
TARGETI = include/sfq.h
TARGETL = lib/libsfq.so lib/libsfqc.so
TARGETE = bin/sfqc-init bin/sfqc-info bin/sfqc-list bin/sfqc-pusht bin/sfqc-pushb bin/sfqc-pop bin/sfqc-shift bin/sfqc-clear bin/sfqc-sets

TARGET  = $(TARGETI) $(TARGETL) $(TARGETE)

all: $(TARGET)

include/sfq.h: src/inc/sfq.h
	cp -p $^ include

lib/libsfq.so: src/libsfq/libsfq.so
	cp -p $^ $@

lib/libsfqc.so: src/sfqc/libsfqc.so
	cp -p $^ $@

src/libsfq/libsfq.so:
	cd src/libsfq/; make $(MAKEOPT); cp -p libsfq.so ../../lib/libsfq.so

src/sfqc/libsfqc.so:
	cd src/sfqc/; make $(MAKEOPT); cp -p libsfqc.so ../../lib/libsfqc.so

bin/sfqc-init: src/sfqc/sfqc-init
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-init ../../bin/sfqc-init

bin/sfqc-info: src/sfqc/sfqc-info
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-info ../../bin/sfqc-info

bin/sfqc-list: src/sfqc/sfqc-list
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-list ../../bin/sfqc-list

bin/sfqc-pusht: src/sfqc/sfqc-pusht
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-pusht ../../bin/sfqc-pusht

bin/sfqc-pushb: src/sfqc/sfqc-pushb
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-pushb ../../bin/sfqc-pushb

bin/sfqc-pop: src/sfqc/sfqc-pop
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-pop ../../bin/sfqc-pop

bin/sfqc-shift: src/sfqc/sfqc-shift
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-shift ../../bin/sfqc-shift

bin/sfqc-clear: src/sfqc/sfqc-clear
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-clear ../../bin/sfqc-clear

bin/sfqc-sets: src/sfqc/sfqc-sets
	cd src/sfqc/; make $(MAKEOPT); cp -p sfqc-sets ../../bin/sfqc-sets

debug:
	make clean
	make "MAKEOPT=debug"

clean:
	rm -f $(TARGET)
	cd src/sfqc/; make clean
	cd src/libsfq/; make clean

#

