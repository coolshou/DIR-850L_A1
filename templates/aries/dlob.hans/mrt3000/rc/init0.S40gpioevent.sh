#!/bin/sh
echo [$0]: $1 ... > /dev/console
if [ "$1" = "start" ]; then
	event "WAN-1.CONNECTED"		add "usockc /var/gpio_ctrl INET_ON"
	event "WAN-1.PPP.ONDEMAND"	add "usockc /var/gpio_ctrl INET_BLINK_SLOW"
	event "WAN-2.CONNECTED"		add "null"
	event "WAN-1.DISCONNECTED"	add "usockc /var/gpio_ctrl INET_OFF"
	event "WAN-2.DISCONNECTED"	add "null"
fi
