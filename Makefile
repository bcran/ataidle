installdir = /usr/local/sbin

ataidle: ataidle.c
	
ataidle.c:
	gcc -o ataidle ataidle.c -std=c99

install:
	cp ataidle $(installdir)

uninstall:
	rm $(installdir)/ataidle

clean: 
	rm ataidle
