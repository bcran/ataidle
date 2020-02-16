PREFIX = /usr/local
CC ?= gcc
CFLAGS += -std=c99 -Wall -pedantic
LIBS =
SOURCES = ataidle.c
MAN = ataidle.8
PROG = ataidle
MAINTAINER = Bruce Cran <bruce@cran.org.uk>

all:	ataidle

ataidle:	ataidle.c

ataidle.c:
	$(CC) $(CFLAGS) $(LIBS) -o $(PROG) $(SOURCES)

install:
	install $(PROG) $(PREFIX)/sbin
	install $(MAN)  $(PREFIX)/man/man8

uninstall:
	rm $(PREFIX)/sbin/$(PROG)
	rm $(PREFIX)/man/man8/$(MAN)

clean: 
	rm -f *.o $(PROG)
