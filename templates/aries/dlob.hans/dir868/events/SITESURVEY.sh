#!/bin/sh

interfaces="wifig0 wifia0"
sleeping_interfaces=""

# send scan command to interfaces
for interface in $interfaces 
do
	status=`wl -i $interface bss`
	if [ "$status" == "up" ]; then
		echo "send scan command for $interface ..."
		wl -i $interface scan
	else
		echo "wakeup interface and send scan command for $interface ..."
		sleeping_interfaces="$sleeping_interfaces ""$interface"
		wl -i $interface up
		wl -i $interface scan
	fi
done

# wait for a while
sleep 3;

# shutdown sleeping interfaces
for interface in $sleeping_interfaces
do
	echo "shutdown interface $interface ..."
	wl -i $interface down
done

# handle scan results
rm -f /tmp/sitesurvey.txt
touch /tmp/sitesurvey.txt

# collect all results
for interface in $interfaces 
do
	echo "retrieve scan results for $interface ..."
	wl -i $interface scanresults >> /tmp/sitesurvey.txt
done

# clean old data first
xmldbc -P /etc/scripts/wifi/sitesurvey.php -V ACTION=CLEAN

parse2db -f /tmp/sitesurvey.txt -s /etc/scripts/sitesurveyhlper.sh
rm -f /tmp/sitesurvey.txt
