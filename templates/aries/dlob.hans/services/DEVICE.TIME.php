<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";


function operator_append($op, $num)
{		
	if($op == "-")
	{
		if($num > 0)
		{
			$num = "-".$num;	
		}		
	}	
	return $num;
}

function operator_append2($op, $num)
{
	if($op == "+")
	{
		$num = "-".$num;
	}
	return $num;
}

function gen_two_digit($num)
{		
	if($num < 0)
	{
		$num = substr($num, 1, 2); //skip "-";
	}	
	if($num < 10) $num = "0".$num;
	
	return $num;
}	

function get_timezone_zoneindex($uid)
{
	foreach("/runtime/services/timezone/zone")
	{
		$zone_uid = query("uid");       	
		if ($zone_uid == $uid)
			return $InDeX;
	}
}

fwrite(w, $START, "#!/bin/sh\n");
fwrite(w, $STOP,  "#!/bin/sh\n");

$DEBUG = 0;

/* Create /etc/TZ */
$index = query("/device/time/timezone");
if ($index=="" || $index==0) $index=61;
$zone_idx = get_timezone_zoneindex($index);
anchor("/runtime/services/timezone/zone:".$zone_idx);

/* Set and Save the TZ status */
set("/runtime/device/timezone/index", $index);
set("/runtime/device/timezone/name",  query("name"));

$TZ = get("s","gen");
if (query("/device/time/dst")=="1")
{	
	set("/runtime/device/timezone/dst", "1");
	
	//sam_pan add
	
	$dstmanual = query("/device/time/dstmanual");
	$dstoffset = query("/device/time/dstoffset");
		
	$gmttime = substr($TZ, 3, strlen($TZ)-3);
	$op1 = substr($gmttime,0,1);
	$h1  = substr($gmttime,1,2);
	$h1  = operator_append($op1, $h1);		
	$m1  = substr($gmttime,4,2);
	$m1  = operator_append($op1, $m1);
			
	$op2 = substr($dstoffset,0,1);
	$h2  = substr($dstoffset,1,2);
	$h2  = operator_append2($op2, $h2);	
	$m2  = substr($dstoffset,4,2);
	$m2  = operator_append2($op2, $m2);
			
	if($DEBUG==1)
	{									
		echo "GMT".$op1." ".$h1." ".$m1."\n";
		echo "GDT".$op2." ".$h2." ".$m2."\n";
	}
					
	$h = $h1+$h2;
	$m = $m1+$m2;
	
	if($DEBUG==1)
	{
		echo "compute: ->".$h." ".$m."\n";
	}	
	
	if($m > 0 && $h < 0) $h++; 			
	if($m < 0 && $h > 0) $h--;
	if($m <= 0) {$m = gen_two_digit($m);}
			
	if($m >= 60)
	{				
		$a = 0;
		if($h < 0) 
		{
			$a = "-1";
		}
		else 
		{
			$a =" 1";
		}
		$h= $h + $a;
		$m = $m - 60;
		if($m <= 0) {$m = gen_two_digit($m);}
	}
			
	if($h >= 0)
		{ $op3 = "+";}
	else
		{ $op3 = "-"; }		
	
	$h = gen_two_digit($h);						
	$dstoffset = $op3.$h.":".$m;		
	if($DEBUG==1)
	{
		echo "result:".$dstoffset."\n";		
	}		
	
	$TZ = $TZ."GDT".$dstoffset.query("/device/time/dstmanual");
	echo $TZ."\n";
}
else 
{
	set("/runtime/device/timezone/dst", "0");
}

$ntp_enable = query("/device/time/ntp/enable");

/* Originally add by Kloat. */
$tmp_date = query("/runtime/device/tmp_date"); del("/runtime/device/tmp_date");
$tmp_time = query("/runtime/device/tmp_time"); del("/runtime/device/tmp_time");
if($ntp_enable==0)
{
   $tmp_date = query("/device/time/date");
   $tmp_time = query("/device/time/time");
}
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
			'	echo "'.$TZ.'" > /etc/TZ\n'. /* sam_add */
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
			'	echo "'.$TZ.'" > /etc/TZ\n'. /* sam_add */
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
		'	echo "'.$TZ.'" > /etc/TZ\n'. /* sam_add */
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
	fwrite(a, $START, 'echo "'.$TZ.'" > /etc/TZ\n');
}


?>
