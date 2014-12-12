#!/bin/sh
ifconfig > /var/ifconfig
touch /var/antenna

if grep wifig0 /var/ifconfig
then
	iwpriv wifig0 set ccp_debug=2
	iwpriv wifig0 get_ccp_status >> /var/antenna
fi

if grep wifia0 /var/ifconfig
then
	iwpriv wifia0 set ccp_debug=2
	iwpriv wifia0 get_ccp_status >> /var/antenna
fi
sed 'N;s/\n/ /g' /var/antenna > /var/antenna_info
rm /var/antenna
exit 0