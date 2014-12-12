#!/bin/sh
echo [$0]: $1 ... > /dev/console
if [ "$1" = "start" ]; then
	event INFSVCS.BRIDGE-1.UP	add "event STATUS.READY"
	event INFSVCS.BRIDGE-1.DOWN	add "event STATUS.NOTREADY"
	service BRIDGE start
else
	service BRIDGE stop
fi
