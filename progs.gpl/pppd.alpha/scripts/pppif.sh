#!/bin/sh
usage="Usage: pppif.sh [linkname]"
[ "$1" = "" ] && echo $usage && exit 9
if [ -f /var/run/ppp-$1.pid ]; then
	pfile -l2 -f /var/run/ppp-$1.pid
fi
exit 0
