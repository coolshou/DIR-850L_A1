#!/bin/sh
echo [$0]: $1 ... > /dev/console
if [ -f "/proc/alpha_fast_route" ]; then
	echo 1 > /proc/alpha_fast_route
fi
