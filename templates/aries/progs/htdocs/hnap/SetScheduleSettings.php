HTTP/1.1 200 OK 
Content-Type: text/xml; charset=utf-8 
Content-Length: <Number of Bytes/Octets in the Body> 
<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
$nodebase="/runtime/hnap/SetScheduleSettings/";
$NumberOfEntry = query($nodebase."NumberOfEntry");
$max_rules=query("/schedule/max");
if($max_rules=="") 
{ 
	$max_rules=10; 
}
$result = "OK";
$i=0;

fwrite("w",$ShellPath, "#!/bin/sh\n");
fwrite("a",$ShellPath, "echo [$0] > /dev/console\n");

If ($NumberOfEntry <=$max_rules) 
{
	$result = "ERROR";
	fwrite("a",$ShellPath, "echo \"We got a error in setting, so we do nothing...\" > /dev/console ");
}
else
{

	//-----Clear  old schedule
	$old_count=query("/schedule/count");
	while($old_count>0)
	{ 
		del("/schedule/entry:".$old_count);
		$old_count--;
	}
	set("/schedule/seqno", $NumberOfEntry+1);
	set("/schedule/count", $NumberOfEntry);
	foreach($nodebase)
	{
		$i++;
		set("/schedule/entry:".$i."/uid", "SCH-".$NumberOfEntry));
		set("/schedule/entry:".$i."/description", query("ScheduleName"));
		set("/schedule/entry:".$i."/exclude", "0");
		$flag = "flase";

		if ("0" == query("ScheduleDate"))
		{
			set("/schedule/entry:".$i."/sun", "1");
			set("/schedule/entry:".$i."/mon", "1");
			set("/schedule/entry:".$i."/tue", "1");
			set("/schedule/entry:".$i."/wed", "1");
			set("/schedule/entry:".$i."/thu", "1");
			set("/schedule/entry:".$i."/fri", "1");
			set("/schedule/entry:".$i."/sat", "1");
		}
		else
		{
			if  ("" != strchr(query("ScheduleDate"),"1"))
			{
				set("/schedule/entry:".$i."/mon", "1");
				$flag = "true";
			}
			else
			{
				set("/schedule/entry:".$i."/mon", "0");
			}
			
			if  ("" != strchr(query("ScheduleDate"),"1"))
			{
				set("/schedule/entry:".$i."/tue", "1");
				$flag = "true";
			}
			else
			{
				set("/schedule/entry:".$i."/tue", "0");
			}
			
			if  ("" != strchr(query("ScheduleDate"),"1"))
			{
				set("/schedule/entry:".$i."/wed", "1");
				$flag = "true";
			}
			else
			{
				set("/schedule/entry:".$i."/wed", "0");
			}
			
			if  ("" != strchr(query("ScheduleDate"),"1"))
			{
				set("/schedule/entry:".$i."/thu", "1");
				$flag = "true";
			}
			else
			{
				set("/schedule/entry:".$i."/thu", "0");
			}
			
			if  ("" != strchr(query("ScheduleDate"),"1"))
			{
				set("/schedule/entry:".$i."/fri", "1");
				$flag = "true";
			}
			else
			{
				set("/schedule/entry:".$i."/fri", "0");
			}
			
			if  ("" != strchr(query("ScheduleDate"),"1"))
			{
				set("/schedule/entry:".$i."/sat", "1");
				$flag = "true";
			}
			else
			{
				set("/schedule/entry:".$i."/sat", "0");
			}

			if ((isdigit(query("ScheduleDate"))!="1"  || $flag ==" flase")
			{
				$result = "ERROR_BAD_ScheduleInfo";
			}
		}

		//time
		if ("True" == query("ScheduleAllDay"))
		{
			set("/schedule/entry:".$i."/start", "0:00");
			set("/schedule/entry:".$i."/end", "23:59");
		}
		else
		{
			$starttimehour = query("ScheduleStartTimeInfo/TimeHourValue");
			$starttimeminute = query("ScheduleStartTimeInfo/TimeMinuteValue");
			$endtimehour = query("ScheduleEndTimeInfo/TimeHourValue");
			$endtimeminute = query("ScheduleEndTimeInfo/TimeMinuteValue");
		
			if ("Flase" == query("ScheduleTimeFormat"))    //12-Hour format
			{

				if (12 >= $starttimehour  && 12 >= $endtimehour )
				{
					if ("AM" == query("ScheduleStartTimeInfo/TimeMidDateValue"))
					{
						$start_time = $starttimehour.":".$starttimeminute;
					}  
					else  if ("PM" == query("ScheduleStartTimeInfo/TimeMidDateValue"))
					{
						$starttimehour = $starttimehour+12;
						$start_time = "$starttimehour".":".$starttimeminute;
					}
					else
					{
						$result = "ERROR_BAD_ScheduleInfo";
					}

					if ("AM" == query("ScheduleEndTimeInfo/TimeMidDateValue"))
					{
						$end_time = $endtimehour.":".$endtimeminute;
					}
					else if ("PM" == query("ScheduleEndTimeInfo/TimeMidDateValue"))
					{
						$endtimehour = $endtimehour+12;
						$end_time = "$endtimehour".":".$endtimeminute;
					}
					else
					{
						$result = "ERROR_BAD_ScheduleInfo";
					}


					if ($starttimehour >$endtimehour   || ($starttimehour == $endtimehour  && $starttimeminute >$endtimeminute))
					{
						$result = "ERROR_BAD_ScheduleInfo";
					}
				} 
				else
				{
					$result = "ERROR_BAD_ScheduleInfo";
				}	
			}
			else  if ("True" == query("ScheduleTimeFormat"))  //24-Hour format
			{
				if (24 >= $starttimehour  && 24 >= $endtimehour )
				{
					if ("AM" == query("ScheduleStartTimeInfo/TimeMidDateValue") && 12 >= $starttimehour)
					{
						$start_time = $starttimehour.":".$starttimeminute;
					}  
					else  if ("PM" == query("ScheduleStartTimeInfo/TimeMidDateValue")  && 12 <= $starttimehour)
					{
						$start_time = $starttimehour.":".$starttimeminute;
					}
					else
					{
						$result = "ERROR_BAD_ScheduleInfo";
					}

					if ("AM" == query("ScheduleEndTimeInfo/TimeMidDateValue") && 12 >= $endtimehour)
					{
						$end_time = $endtimehour.":".$endtimeminute;
					}
					else if ("PM" == query("ScheduleEndTimeInfo/TimeMidDateValue")&& 12 <= $endtimehour)
					{
						$end_time = $endtimehour.":".$endtimeminute;
					}
					else
					{
						$result = "ERROR_BAD_ScheduleInfo";
					}

					if ($starttimehour >$endtimehour   || ($starttimehour == $endtimehour  && $starttimeminute >$endtimeminute))
					{
						$result = "ERROR_BAD_ScheduleInfo";
					}
				} 
				else
				{
					$result = "ERROR_BAD_ScheduleInfo";
				}	
			}
			else
			{
				$result = "ERROR_BAD_ScheduleInfo";
			}


			if($result=="OK")
			{
				set("/schedule/entry:".$i."/start", $start_time);
				set("/schedule/entry:".$i."/end", $end_time);
			}
		}         	
	}
	if($result=="OK")
	{
		fwrite("a",$ShellPath, "/etc/scripts/dbsave.sh > /dev/console\n");
		fwrite("a",$ShellPath, "xmldbc -s /runtime/hnap/dev_status '' > /dev/console\n");
		set("/runtime/hnap/dev_status", "ERROR");
	}
	else
	{
		fwrite("a",$ShellPath, "echo \"We got a error in setting, so we do nothing...\" > /dev/console");
	}
}

?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
xmlns:xsd="http://www.w3.org/2001/XMLSchema" 
xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"> 
<soap:Body> 
<SetScheduleSettingsResponse xmlns="http://purenetworks.com/HNAP1/"> 
<SetScheduleSettingsResult><?=$result?></SetScheduleSettingsResult> 
</SetScheduleSettingsResponse> 
</soap:Body> 
</soap:Envelope> 
