HTTP/1.1 200 OK
Content-Type: text/xml; charset=utf-8

<?
echo "\<\?xml version='1.0' encoding='utf-8'\?\>";
include "/htdocs/phplib/xnode.php";
include "/htdocs/webinc/config.php";
$result = OK;
$nodebase="/runtime/hnap/GetScheduleSettings";
$nodebase=$nodebase."/entry";
del("/runtime/hnap/GetScheduleSettings");
$i=0;
foreach (/schedule/entry)
{
		$i++;
             	set($nodebase.":".$i."/ScheduleName", query("description"));
		//ScheduleDate
		$scheduledate = "0";
              if ("1" ==  query("mon"))
              {
			$scheduledate = "1";
		}
              if ("1" ==  query("tue"))
              {
			$scheduledate = $scheduledate."2";
		}
              if ("1" ==  query("wed"))
              {
			$scheduledate = $scheduledate."3";
		}
              if ("1" ==  query("thu"))
		{
			$scheduledate = $scheduledate."4";
		}		
              if ("1" ==  query("fri"))
		{
			$scheduledate = $scheduledate."5";
		}
              if ("1" ==  query("sat"))
		{
			$scheduledate = $scheduledate."6";
		}
		if ("1" ==  query("sun"))
		{
			$scheduledate = $scheduledate."7";
		}
		//All Week
		if ("1234567" ==  $scheduledate)
		{
			$scheduledate = "0";
		}
		set($nodebase.":".$i."/ScheduleDate", $scheduledate );

		//StartTimeInfo
		$start = query("start");
		$start_len = strlen($start);
		if ($start != "" && $start_len >= 4)
		{
			if ($start_len == 4)
			{
				$TimeHourValue = substr($start, 0, 1);
				$TimeMinuteValue = substr($start, 2, $start_len - 2);
			}
			else
			{
				$TimeHourValue = substr($start, 0, 2);
				$TimeMinuteValue = substr($start, 3, $start_len - 3);
			}
					
		}
		set($nodebase.":".$i."/ScheduleStartTimeInfo/TimeHourValue", $TimeHourValue );
		set($nodebase.":".$i."/ScheduleStartTimeInfo/TimeMinuteValue", $TimeMinuteValue );
		if ($TimeHourValue > 12)
		{
			set($nodebase.":".$i."/ScheduleStartTimeInfo/TimeMidDateValue", "PM" );
		}
		else
		{
			set($nodebase.":".$i."/ScheduleStartTimeInfo/TimeMidDateValue", "AM" );
		}
		
		//EndTimeInfo
		$end = query("end");
		$end_len = strlen($end);
		if ($end != "" && $end_len >= 4)
		{
			if ($end_len == 4)
			{
				$TimeHourValue = substr($end, 0, 1);
				$TimeMinuteValue = substr($end, 2, $end_len - 2);
			}
			else
			{
				$TimeHourValue = substr($end, 0, 2);
				$TimeMinuteValue = substr($end, 3, $end_len - 3);
			}
					
		}
		set($nodebase.":".$i."/ScheduleEndTimeInfo/TimeHourValue", $TimeHourValue );
		set($nodebase.":".$i."/ScheduleEndTimeInfo/TimeMinuteValue", $TimeMinuteValue );
		if ($TimeHourValue > 12)
		{
			set($nodebase.":".$i."/ScheduleEndTimeInfo/TimeMidDateValue", "PM" );
		}
		else
		{
			set($nodebase.":".$i."/ScheduleEndTimeInfo/TimeMidDateValue", "AM" );
		}

		//SchdeuleAllday
		if ($start == "0:00" && $end == "23:59")
		{
			set($nodebase.":".$i."/ScheduleAllDay", "True" );
		}
		else
		{
			set($nodebase.":".$i."/ScheduleAllDay", "Flase" );
		}
		
}
?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
xmlns:xsd="http://www.w3.org/2001/XMLSchema" 
xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
	<GetScheduleSettingsResponse xmlns="http://purenetworks.com/HNAP1/">
		<GetScheduleSettingsResult><?=$result?></GetScheduleSettingsResult>
 		<ScheduleInfoLists>
foreach($nodebase)
{
	echo "        <ScheduleInfo>\n";
	echo "          <ScheduleName>".query("ScheduleName")."</ScheduleName>\n";
	echo "          <ScheduleDate>".query("ScheduleDate")."</ScheduleDate>\n";
	echo "          <ScheduleAllDay>".query("ScheduleAllDay")."</ScheduleAllDay>\n";
	echo "          <ScheduleTimeFormat>"True"</ScheduleTimeFormat>\n";

	echo "          <ScheduleStartTimeInfo>\n";
	echo "          	<TimeHourValue>".query("ScheduleStartTimeInfo/TimeHourValue")."</TimeHourValue>\n";
	echo "          	<TimeMinuteValue>".query("ScheduleStartTimeInfo/TimeMinuteValue")."</TimeMinuteValue>\n";
	echo "          	<TimeMidDateValue>".query("ScheduleStartTimeInfo/TimeMidDateValue")."</TimeMidDateValue>\n";
	echo "          </ScheduleStartTimeInfo>\n";

	echo "          <ScheduleEndTimeInfo>\n";
	echo "          	<TimeHourValue>".query("ScheduleEndTimeInfo/TimeHourValue")."</TimeHourValue>\n";
	echo "          	<TimeMinuteValue>".query("ScheduleEndTimeInfo/TimeMinuteValue")."</TimeMinuteValue>\n";
	echo "          	<TimeMidDateValue>".query("ScheduleEndTimeInfo/TimeMidDateValue")."</TimeMidDateValue>\n";
	echo "          </ScheduleEndTimeInfo>\n";
	echo "        </ScheduleInfo>\n";
}
		</ScheduleInfoLists>
	</GetScheduleSettingsResponse>
  </soap:Body>
</soap:Envelope>

