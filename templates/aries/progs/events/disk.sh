#!/bin/sh
echo [$0] [$1] ... > /dev/console


if [ "$1" = "stop" ]; then
	service STORAGE stop
elif [ "$1" = "start" ]; then
	service STORAGE start
else
	service STORAGE restart
fi

exit 0
