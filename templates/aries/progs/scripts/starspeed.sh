#!/bin/sh
echo [$0]: user[$1] pass[$2] ifname[$3] region[$4]  > /dev/console
pppoe_user=$1
pppoe_pass=$2
ifname=$3
region=$4
peer_ips="192.168.0.1 192.168.10.1 192.168.1.1 192.167.0.1 10.0.0.138 10.0.0.2"
xmldbc -s /runtime/starspeed/username ""
xmldbc -s /runtime/starspeed/password ""
xmldbc -s /runtime/starspeed/userformat ""
curr_state=$region
echo "Current state : "$curr_state
case $curr_state in
	"normal" )
		xmldbc -s /runtime/starspeed/username $pppoe_user
		xmldbc -s /runtime/starspeed/password $pppoe_pass
		;;
	"peermac" | "nullmac" )
		for peer_ip in $peer_ips
		do
			if [ "$curr_state" = "nullmac" ];then
				break
			fi
			ip=`echo $peer_ip | grep "192.168.1.*"`
			if [ "$ip" != "" ];then
				client_ip="192.168.1.5"
			fi
			ip=`echo $peer_ip | grep "192.168.0.*"`
			if [ "$ip" != "" ];then
				client_ip="192.168.0.222"
			fi
			ip=`echo $peer_ip | grep "192.168.10.*"`
			if [ "$ip" != "" ];then
				client_ip="192.168.10.222"
			fi
			ip=`echo $peer_ip | grep "192.167.0.*"`
			if [ "$ip" != "" ];then
				client_ip="192.167.0.222"
			fi
			ip=`echo $peer_ip | grep "10.0.0.*"`
			if [ "$ip" != "" ];then
				client_ip="10.0.0.212"
			fi
			echo "client IP : "$client_ip
			if [ $client_ip != "" ];then
				ifconfig $ifname $client_ip
			else
				break
			fi
			peer_mac=`arpping -t $peer_ip -i $ifname`

			echo $peer_ip":"$peer_mac
			if [ "$peer_mac" != "no" -a "$peer_mac" != "" ];then
				pap_pass=`pap_crack $pppoe_user $pppoe_pass $peer_mac`
				break
			fi
		done
		echo $pap_pass
		if [ "$pap_pass" = "" -o "$curr_state" = "nullmac" ];then
			pap_pass=`pap_crack $pppoe_user $pppoe_pass`
		fi
		echo "pap pass":$pap_pass
		xmldbc -s /runtime/starspeed/username $pppoe_user
		xmldbc -s /runtime/starspeed/password $pap_pass
		;;
	"hubei" )
		username=`hubei $pppoe_user`
		xmldbc -s /runtime/starspeed/username $username
		xmldbc -s /runtime/starspeed/password $pppoe_pass
		;;
	"henan" )
		username=`henan $pppoe_user`
		xmldbc -s /runtime/starspeed/username $username
		xmldbc -s /runtime/starspeed/password $pppoe_pass
		;;
	"nanchang1" )
		username=`nanchang_campus $pppoe_user 18`
		xmldbc -s /runtime/starspeed/username $username
		xmldbc -s /runtime/starspeed/userformat "hex"
		xmldbc -s /runtime/starspeed/password $pppoe_pass
		;;
	"nanchang2" )
		username=`nanchang_campus $pppoe_user 0`
		xmldbc -s /runtime/starspeed/username $username
		xmldbc -s /runtime/starspeed/userformat "hex"
		xmldbc -s /runtime/starspeed/password $pppoe_pass
		;;
esac
