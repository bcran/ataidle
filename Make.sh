#!/bin/sh

OSTYPE=`uname -s`

case $OSTYPE in
FreeBSD)		
	make -f Makefile.FreeBSD $1 $2
	;;
Linux)
	make -f Makefile.Linux $1 $2 || make -f Makefile.altLinux $1 $2
	;;
*)
	echo "Unsupported OS"
	;;
esac
