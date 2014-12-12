#!/bin/sh
echo "Check FW Now ..."
#check the dns server is ready
for ii in 0 1 2 3 4 5
do
	status=`grep  nameserver /etc/resolv.conf`
	if [ "$status" == "" ]; then
		echo "dns server not ready ..."
		sleep 3
		if [ $ii == 5 ] ;then
		echo "no dns server abort fw check"
		exit 0
		fi
	else
		break;
	fi
done 

#fot the wan up flow,the dns is not setup in this event WAN-1.up.We do sleep to wait it updated

fwinfo="/tmp/fwinfo.xml"
model="`xmldbc -g /runtime/device/modelname`"
srv="wrpd.dlink.com.tw"
reqstr="/router/firmware/query.asp"
old_major=`cat /etc/config/buildver|cut -d'.' -f1`
#echo $old_major=1
old_minor=`cat /etc/config/buildver|cut -d'.' -f2|cut -c1-2`
#echo $old_minor=03
buildver="_0"$old_major$old_minor

#+++sam_pan add for firmware onlone check.
fwcheckparameter="`xmldbc -g /device/fwcheckparameter`"
if [ "$fwcheckparameter" != "" ]; then
global=$fwcheckparameter$buildver
else
global="Ax_Default"
fi
#---sam_pan

reqstr=$reqstr"?model=$model\_$global"
reqstr="GET $reqstr HTTP/1.1
Accept:*/*
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1)
Host: $srv
Connection: Close

"
rm -f $fwinfo
cat /etc/resolv.conf
echo "tcprequest \"$reqstr\" \"$srv\" 80 -f \"$fwinfo\" -t 5 -s"
`tcprequest "$reqstr" "$srv" 80 -f "$fwinfo" -t 5 -s`

if [ -f $fwinfo ]; then
	#get firmware information
	new_major=`grep Major /tmp/fwinfo.xml |sed 's/^[ \t]*//'|sed 's/<Major>//'|sed 's/<\/Major>//'`
	new_minor=`grep Minor /tmp/fwinfo.xml |sed 's/^[ \t]*//'|sed 's/<Minor>//'|sed 's/<\/Minor>//'`
	if [ "$new_major" != "" ]; then
		if [ $new_major -gt $old_major -o $new_major -eq $old_major -a $new_minor -gt $old_minor ]; then
			echo "Send F/W notice to Mydlink"
			usockc /var/mydlinkeventd_usock NEW_FW
		else
			echo "This firmware is the latest version."
		fi
	else
		echo "This online firmware check fail."
	fi
else
	echo "This online firmware check fail.http fail"
fi


