<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";

fwrite(w, $START, "#!/bin/sh\n");
fwrite(w, $STOP,  "#!/bin/sh\n");

/* Create /etc/TZ */
$index = query("/device/time/timezone");
if ($index=="" || $index==0) $index=61;
anchor("/runtime/services/timezone/zone:".$index);

/* Set and Save the TZ status */
set("/runtime/device/timezone/index", $index);
set("/runtime/device/timezone/name",  query("name"));

$TZ = get("s","gen");
if (query("/device/time/dst")=="1")
{
	$TZ = $TZ.get("s","dst");
	set("/runtime/device/timezone/dst", "1");
}
else set("/runtime/device/timezone/dst", "0");
fwrite("w", "/etc/TZ", $TZ."\n");

/* Originally add by Kloat. */
$tmp_date = query("/runtime/device/tmp_date"); del("/runtime/device/tmp_date");
$tmp_time = query("/runtime/device/tmp_time"); del("/runtime/device/tmp_time");
if ($tmp_date!="") set("/runtime/device/date", $tmp_date);
if ($tmp_time!="") set("/runtime/device/time", $tmp_time);
set("/runtime/device/timestate", "SUCCESS");

/* Manually set the date, clear NTP status. */
if ($tmp_date!="" || $tmp_time!="")
{
	set("/runtime/device/ntp/state", "MANUAL");
	set("/runtime/device/ntp/uptime", "");
	set("/runtime/device/ntp/server", "");
	set("/runtime/device/ntp/nexttime", "");
}

/* NTP ... */
$enable = query("/device/time/ntp/enable");
if($enable=="") $enable = 0;
$enablev6 = query("/device/time/ntp6/enable");
if($enablev6=="") $enablev6 = 0;
$server = query("/device/time/ntp/server");
$period = query("/device/time/ntp/period");	if ($period=="") $period="604800";
$period6 = query("/device/time/ntp6/period");	if ($period6=="") $period6="604800";
$ntp_run = "/var/run/ntp_run.sh";

if ($enable==1 && $enablev6==1)
{
	if ($server=="") fwrite(a, $START, 'echo "No NTP server, disable NTP client ..." > /dev/console\n');
	else
	{
		fwrite(w, $ntp_run, '#!/bin/sh\n');
		fwrite(a, $ntp_run,
			'echo "Run NTP client ..." > /dev/console\n'.
			'echo [$1] [$2] > /dev/console\n'.
			'STEP=$1\n'.
			'RESULT="Null"\n'.
			'xmldbc -s /runtime/device/ntp/state RUNNING\n'.
			'SERVER4='.$server.'\n'.
			'SERVER6=`xmldbc -g /runtime/device/ntp6/server | cut -f 1 -d " "`\n'.
			'if [ "$STEP" == "V4" ]; then\n'.
			'	xmldbc -t "ntp:'.$period.':'.$ntp_run.' $STEP"\n'.
			'	echo "ntpclient -h $SERVER4 -i 5 -s -4" > /dev/console\n'.
			'	ntpclient -h $SERVER4 -i 5 -s -4 > /dev/console\n'.
			'	if [ $? != 0 ]; then\n'.
			'		xmldbc -k ntp\n'.
			'		xmldbc -t "ntp:10:'.$ntp_run.' V6"\n'.
			'		echo NTP4 will run in 10 seconds! > /dev/console\n'.
			'	else\n'.
			'		RESULT="OK"\n'.
			'	fi\n'.
			'elif [ "$SERVER6" != "" ] && [ "$STEP" == "V6" ];then\n'.
			'   xmldbc -t "ntp:'.$period6.':'.$ntp_run.' $STEP"\n'.
			'   echo "ntpclient -h $SERVER6 -i 5 -s -6" > /dev/console\n'.
			'   ntpclient -h $SERVER6 -i 5 -s -6 > /dev/console\n'.
			'	if [ $? != 0 ]; then\n'.
			'		xmldbc -k ntp\n'.
			'		xmldbc -t "ntp:10:'.$ntp_run.' V4"\n'.
			'		echo NTP4 will run in 10 seconds! > /dev/console\n'.
			'	else\n'.
			'		RESULT="OK"\n'.
			'	fi\n'.
			'fi\n'.
			'if [ $RESULT == "OK" ]; then\n'.
			'	echo NTP will run in '.$period.' seconds! > /dev/console\n'.
			'	sleep 1\n'.
			'	UPTIME=`xmldbc -g /runtime/device/uptime`\n'.
			'	if [ "$STEP" == "V4" ]; then\n'.
			'		xmldbc -s /runtime/device/ntp/state SUCCESS\n'.
			'		xmldbc -s /runtime/device/ntp/uptime "$UPTIME"\n'.
			'		xmldbc -s /runtime/device/ntp/period '.$period.'\n'.
			'		xmldbc -s /runtime/device/ntp/server '.$server.'\n'.
			'	elif [ "$STEP" == "V6" ]; then\n'.
			'		xmldbc -s /runtime/device/ntp6/state SUCCESS\n'.
			'		xmldbc -s /runtime/device/ntp6/uptime "$UPTIME"\n'.
			'		xmldbc -s /runtime/device/ntp6/period '.$period6.'\n'.
			'		xmldbc -s /runtime/device/ntp6/server "$SERVER6"\n'.
			'	fi\n'.
			'	service schedule on\n'.
			'fi\n'
			);
		fwrite(a, $START, 'chmod +x '.$ntp_run.'\n');
		fwrite(a, $START, $ntp_run.' V4 > /dev/console &\n'); //default from 'V4'

		fwrite(a, $STOP,
			'xmldbc -k ntp\n'.
			'killall ntpclient\n'.
			'sleep 1\n'.
			'xmldbc -k ntp\n'.
			'xmldbc -s /runtime/device/ntp/state STOPPED\n'.
			'xmldbc -s /runtime/device/ntp/period ""\n'.
			'xmldbc -s /runtime/device/ntp/nexttime ""\n'
			);
	}
}
else if ($enable==1 && $enablev6==0)
{
	if ($server=="") fwrite(a, $START, 'echo "No NTP server, disable NTP client ..." > /dev/console\n');
	else
	{
		fwrite(w, $ntp_run, '#!/bin/sh\n');
		fwrite(a, $ntp_run,
			'echo "Run NTP client ..." > /dev/console\n'.
			'xmldbc -s /runtime/device/ntp/state RUNNING\n'.
			'xmldbc -t "ntp:'.$period.':'.$ntp_run.'"\n'.
			'ntpclient -h '.$server.' -i 5 -s -4 > /dev/console\n'.
			'if [ $? != 0 ]; then\n'.
			'	xmldbc -k ntp\n'.
			'	xmldbc -t "ntp:10:'.$ntp_run.'"\n'.
			'	echo NTP will run in 10 seconds! > /dev/console\n'.
			'else\n'.
			'	echo NTP will run in '.$period.' seconds! > /dev/console\n'.
			'	sleep 1\n'.
			'	xmldbc -s /runtime/device/ntp/state SUCCESS\n'.
			'	UPTIME=`xmldbc -g /runtime/device/uptime`\n'.
			'	xmldbc -s /runtime/device/ntp/uptime "$UPTIME"\n'.
			'	xmldbc -s /runtime/device/ntp/period '.$period.'\n'.
			'	xmldbc -s /runtime/device/ntp/server '.$server.'\n'.
			'	service schedule on\n'.
			'fi\n'
			);
		fwrite(a, $START, 'chmod +x '.$ntp_run.'\n');
		fwrite(a, $START, $ntp_run.' > /dev/console &\n');

		fwrite(a, $STOP,
			'xmldbc -k ntp\n'.
			'killall ntpclient\n'.
			'sleep 1\n'.
			'xmldbc -k ntp\n'.
			'xmldbc -s /runtime/device/ntp/state STOPPED\n'.
			'xmldbc -s /runtime/device/ntp/period ""\n'.
			'xmldbc -s /runtime/device/ntp/nexttime ""\n'
			);
	}
}
else if ($enable==0 && $enablev6==1)
{
	fwrite(w, $ntp_run, '#!/bin/sh\n');
	fwrite(a, $ntp_run,
		'echo "Run NTP6 client ..." > /dev/console\n'.
		'xmldbc -s /runtime/device/ntp6/state RUNNING\n'.
		'SERVER6=`xmldbc -g /runtime/device/ntp6/server | cut -f 1 -d " "`\n'.
		'xmldbc -t "ntp:'.$period6.':'.$ntp_run.'"\n'.
		'[ "$SERVER6" != "" ] && ntpclient -h $SERVER6 -i 5 -s -6 > /dev/console\n'.
		'if [ $? != 0 ]; then\n'.
		'	xmldbc -k ntp\n'.
		'	xmldbc -t "ntp:10:'.$ntp_run.'"\n'.
		'	echo NTP6 will run in 10 seconds! > /dev/console\n'.
		'else\n'.
		'	echo NTP6 will run in '.$period6.' seconds! > /dev/console\n'.
		'	sleep 1\n'.
		'	xmldbc -s /runtime/device/ntp6/state SUCCESS\n'.
		'	UPTIME=`xmldbc -g /runtime/device/uptime`\n'.
		'	xmldbc -s /runtime/device/ntp6/uptime "$UPTIME"\n'.
		'	xmldbc -s /runtime/device/ntp6/period '.$period6.'\n'.
		'	xmldbc -s /runtime/device/ntp6/server "$SERVER6"\n'.
		'	service schedule on\n'.
		'fi\n'
		);
	fwrite(a, $START, 'chmod +x '.$ntp_run.'\n');
	fwrite(a, $START, $ntp_run.' > /dev/console &\n');

	fwrite(a, $STOP,
		'xmldbc -k ntp\n'.
		'killall ntpclient\n'.
		'sleep 1\n'.
		'xmldbc -k ntp\n'.
		'xmldbc -s /runtime/device/ntp/state STOPPED\n'.
		'xmldbc -s /runtime/device/ntp/period ""\n'.
		'xmldbc -s /runtime/device/ntp/nexttime ""\n'
		);
}
else
{
	fwrite(a, $START, 'echo "NTP is disabled ..." > /dev/console\n');
}
?>
