#!/bin/sh
echo [$0]: $1 ... > /dev/console
if [ "$1" = "start" ]; then
	event "STATUS.READY"		add "usockc /var/gpio_ctrl STATUS_GREEN"
#	event "STATUS.CRITICAL"		add "usockc /var/gpio_ctrl STATUS_AMBER_BLINK"
	event "STATUS.NOTREADY"		add "usockc /var/gpio_ctrl STATUS_AMBER"
	
	event "STATUS.GREEN"		add "usockc /var/gpio_ctrl STATUS_GREEN"
	event "STATUS.GREEBBLINK"	add "usockc /var/gpio_ctrl STATUS_GREEN_BLINK"
#	event "STATUS.GREEN.BLINK.SLOW"		add "usockc /var/gpio_ctrl STATUS_GREEN_BLINK_SLOW"
#	event "STATUS.GREEN.BLINK.NORMAL"	add "usockc /var/gpio_ctrl STATUS_GREEN_BLINK_NORMAL"
#	event "STATUS.GREEN.BLINK.FAST"		add "usockc /var/gpio_ctrl STATUS_GREEN_BLINK_FAST"
	
	event "STATUS.AMBER"		add "usockc /var/gpio_ctrl STATUS_AMBER"
	event "STATUS.AMBERBLINK"	add "usockc /var/gpio_ctrl STATUS_AMBER_BLINK"
#	event "STATUS.AMBER.BLINK.SLOW"		add "usockc /var/gpio_ctrl STATUS_AMBER_BLINK_SLOW"
#	event "STATUS.AMBER.BLINK.NORMAL"	add "usockc /var/gpio_ctrl STATUS_AMBER_BLINK_NORMAL"
#	event "STATUS.AMBER.BLINK.FAST"		add "usockc /var/gpio_ctrl STATUS_AMBER_BLINK_FAST"
	
	event "WAN-1.CONNECTED"		add "usockc /var/gpio_ctrl INET_ON"
	event "WAN-1.PPP.ONDEMAND"	add "usockc /var/gpio_ctrl INET_BLINK_SLOW"
	event "WAN-2.CONNECTED"		add "null"
	event "WAN-1.DISCONNECTED"	add "usockc /var/gpio_ctrl INET_OFF"
	event "WAN-2.DISCONNECTED"	add "null"
	event "WPS.INPROGRESS"		add "usockc /var/gpio_ctrl WPS_IN_PROGRESS"
	event "WPS.SUCCESS"			add "usockc /var/gpio_ctrl WPS_SUCCESS"
	event "WPS.OVERLAP"			add "usockc /var/gpio_ctrl WPS_OVERLAP"
	event "WPS.ERROR"			add "usockc /var/gpio_ctrl WPS_ERROR"
	event "WPS.NONE"			add "usockc /var/gpio_ctrl WPS_NONE"
	
	event "INET_UNLIGHT"	add "usockc /var/gpio_ctrl INET_UNLIGHT"
	event "INET_RECOVER"	add "usockc /var/gpio_ctrl INET_RECOVER"

#	event "INET.GREEN.BLINK.SLOW"		add "usockc /var/gpio_ctrl INET_GREEN_SLOW"
#	event "INET.GREEN.BLINK.NORMAL"		add "usockc /var/gpio_ctrl INET_GREEN_NORMAL"
#	event "INET.GREEN.BLINK.FAST"		add "usockc /var/gpio_ctrl INET_GREEN_FAST"
#	event "INET.AMBER.BLINK.SLOW"		add "usockc /var/gpio_ctrl INET_ORANGE_SLOW"
#	event "INET.AMBER.BLINK.NORMAL"		add "usockc /var/gpio_ctrl INET_ORANGE_NORMAL"
#	event "INET.AMBER.BLINK.FAST"		add "usockc /var/gpio_ctrl INET_ORANGE_FAST"

fi
