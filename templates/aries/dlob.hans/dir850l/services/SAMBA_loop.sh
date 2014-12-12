#!/bin/sh

while [ 1 -eq 1 ]
do
	samba_daemon=`ps | grep -v "grep" | grep "smbd -D"`

	sleep 1

	if [ "$samba_daemon" == "" ] ; then
		smbd -D
		echo "SAMBA server is dead !"
	fi
done
exit 0




