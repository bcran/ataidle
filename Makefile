all:
	/bin/sh Make.sh	clean all
#make -f Makefile.`uname -s` clean all

clean:
	/bin/sh Make.sh clean
#	make -f Makefile.`uname -s` clean

install:
	/bin/sh Make.sh all install
#	make -f Makefile.`uname -s` all install

uninstall:
	/bin/sh Make.sh uninstall
#make -f Makefile.`uname -s` uninstall
