#!/bin/sh
echo "Check FW Now ..."
fwinfo="/tmp/fwinfo.xml"
model="`xmldbc -g /runtime/device/modelname`"
brand="`xmldbc -g /runtime/device/vendor`"
srv="`xmldbc -g /runtime/device/fwinfosrv`"
reqstr="`xmldbc -g /runtime/device/fwinfopath`"
old_major=`cat /etc/config/buildver|cut -d'.' -f1`
old_minor=`cat /etc/config/buildver|cut -d'.' -f2|cut -c1-2`
buildver="_0"$old_major$old_minor
#+++sam_pan add for firmware onlone check.
fwcheckparameter="`xmldbc -g /device/fwcheckparameter`"
if [ "$fwcheckparameter" != "" ]; then
global=$fwcheckparameter$buildver
else
global="Ax_Default"
fi
#---sam_pan
reqstr=$reqstr"?brand=$brand&model=$model&major=$old_major&minor=$old_minor"
reqstr="GET $reqstr HTTP/1.1
Accept:*/*
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1)
Host: $srv
Connection: Close

"
rm -f $fwinfo
xmldbc -X /runtime/firmware
#echo "tcprequest \"$reqstr\" \"$srv\" 80 -f \"$fwinfo\" -t 5 -s"
`tcprequest "$reqstr" "$srv" 80 -f "$fwinfo" -t 5 -s`

if [ -f $fwinfo ]; then
	#get firmware information
	new_major=`grep major /tmp/fwinfo.xml |sed 's/^[ \t]*//'|sed 's/<major>//'|sed 's/<\/major>//'`
	new_minor=`grep minor /tmp/fwinfo.xml |sed 's/^[ \t]*//'|sed 's/<minor>//'|sed 's/<\/minor>//'`
	if [ "$new_major" != "" ] || [ "$new_minor" != "" ]; then
		#write by tools_firmware.php
		#xmldbc -s /runtime/firmware/fwversion/Major $new_major
		#xmldbc -s /runtime/firmware/fwversion/Minor $new_minor
		xmldbc -s /runtime/firmware/fwversion/Major "0"
		xmldbc -s /runtime/firmware/fwversion/Minor "0"	
	fi
	if [ "$new_major" != "" ]; then
		if [ $new_major -gt $old_major -o $new_major -eq $old_major -a $new_minor -gt $old_minor ]; then
			echo "Have new Firmware"
			xmldbc -s /runtime/firmware/havenewfirmware 1
		fi
	else
		xmldbc -s /runtime/firmware/state "NORESPONSE"
	fi
else
	xmldbc -s /runtime/firmware/state "NORESPONSE"
fi


