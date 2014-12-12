#!/bin/sh
echo "Check FW Now ..."
fwinfo="/tmp/fwinfo.xml"
model="`xmldbc -g /runtime/device/modelname`"
srv="wrpd.dlink.com.tw"
reqstr="/router/firmware/query.asp"
old_major=`cat /etc/config/buildver|cut -d'.' -f1`
old_minor=`cat /etc/config/buildver|cut -d'.' -f2|cut -c1-2`
buildver="_0"$old_major$old_minor

#+++sam_pan add for firmware onlone check.
fwcheckparameter="`xmldbc -g /device/fwcheckparameter`"
if [ "$fwcheckparameter" != "" ]; then
global=$fwcheckparameter$buildver
else
# Get hw revision sync. to hidden page (192.168.0.1/version.php) Joseph Chao
global="`xmldbc -g /runtime/devdata/hwver | sed 's/[^a-zA-Z]//g' | tr '[a-z]' '[A-Z]' | cut -c 1`"
global=$global"x_Default"
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
xmldbc -X /runtime/firmware
#echo "tcprequest \"$reqstr\" \"$srv\" 80 -f \"$fwinfo\" -t 5 -s"
`tcprequest "$reqstr" "$srv" 80 -f "$fwinfo" -t 5 -s`

if [ -f $fwinfo ]; then
	#get firmware information
	new_major=`grep Major /tmp/fwinfo.xml |sed 's/^[ \t]*//'|sed 's/<Major>//'|sed 's/<\/Major>//'`
	new_minor=`grep Minor /tmp/fwinfo.xml |sed 's/^[ \t]*//'|sed 's/<Minor>//'|sed 's/<\/Minor>//'`
	FWDownloadUrl=`grep Firmware /tmp/fwinfo.xml |sed 's/^[ \t]*//'|sed 's/<Firmware>//'|sed 's/<\/Firmware>//'`	
	if [ "$new_major" != "" ] || [ "$new_minor" != "" ]; then
		xmldbc -s /runtime/firmware/fwversion/Major $new_major
		xmldbc -s /runtime/firmware/fwversion/Minor $new_minor
		xmldbc -s /runtime/firmware/FWDownloadUrl $FWDownloadUrl
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


