#!/bin/sh
echo [$0]: $1 ... > /dev/console
if [ "$1" = "start" ]; then
arpmonitor -i br0 &
fi
