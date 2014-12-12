#!/bin/sh
echo [$0]: $1 ... > /dev/console
if [ "$1" = "start" ]; then
event BRIDGE-1.UP	add "service INFSVCS.BRIDGE-1 start"
event BRIDGE-1.DOWN	add "service INFSVCS.BRIDGE-1 stop"

event REBOOT		add "/etc/events/reboot.sh"
event FRESET		add "/etc/events/freset.sh"
event UPDATERESOLV	add "/etc/events/UPDATERESOLV.sh"
event SEALPAC.SAVE	add "/etc/events/SEALPAC-SAVE.sh"
event SEALPAC.LOAD	add "/etc/events/SEALPAC-LOAD.sh"
event SEALPAC.CLEAR	add "/etc/events/SEALPAC-CLEAR.sh"

event SEALPAC.LOAD
fi
