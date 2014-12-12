#!/bin/sh
echo [$0]: $1 ... > /dev/console
if [ "$1" = "start" ]; then
	service SNMPD start
else
	service SNMPD stop
fi
