
TARGET  = lib/libsfq.so lib/libsfqc.so bin/sfqc-init bin/sfqc-info bin/sfqc-list bin/sfqc-pushc bin/sfqc-pushb bin/sfqc-pop bin/sfqc-shift bin/sfqc-clear

all: $(TARGET)

lib/libsfq.so: src/libsfq/libsfq.so

src/libsfq/libsfq.so:
	cd src/libsfq/; make $(MAKEOPT); cp libsfq.so ../../lib/libsfq.so

lib/libsfqc.so: src/sfqc/libsfqc.so

src/sfqc/libsfqc.so:
	cd src/sfqc/; make $(MAKEOPT); cp libsfqc.so ../../lib/libsfqc.so

bin/sfqc-init: src/sfqc/sfqc-init
	cd src/sfqc/; make $(MAKEOPT); cp sfqc-init ../../bin/sfqc-init

bin/sfqc-info: src/sfqc/sfqc-info
	cd src/sfqc/; make $(MAKEOPT); cp sfqc-info ../../bin/sfqc-info

bin/sfqc-list: src/sfqc/sfqc-list
	cd src/sfqc/; make $(MAKEOPT); cp sfqc-list ../../bin/sfqc-list

bin/sfqc-pushc: src/sfqc/sfqc-pushc
	cd src/sfqc/; make $(MAKEOPT); cp sfqc-pushc ../../bin/sfqc-pushc

bin/sfqc-pushb: src/sfqc/sfqc-pushb
	cd src/sfqc/; make $(MAKEOPT); cp sfqc-pushb ../../bin/sfqc-pushb

bin/sfqc-pop: src/sfqc/sfqc-pop
	cd src/sfqc/; make $(MAKEOPT); cp sfqc-pop ../../bin/sfqc-pop

bin/sfqc-shift: src/sfqc/sfqc-shift
	cd src/sfqc/; make $(MAKEOPT); cp sfqc-shift ../../bin/sfqc-shift

bin/sfqc-clear: src/sfqc/sfqc-clear
	cd src/sfqc/; make $(MAKEOPT); cp sfqc-clear ../../bin/sfqc-clear

debug:
	make clean
	make "MAKEOPT=debug"

clean:
	rm -f $(TARGET)
	cd src/sfqc/; make clean
	cd src/libsfq/; make clean

#

