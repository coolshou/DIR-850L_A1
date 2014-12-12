HTTP/1.1 200 OK
Content-Type: text/xml
<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

function fail($reason)
{
	$_GLOBALS["RESULT"] = "FAIL";
	$_GLOBALS["REASON"] = $reason;
}

//TRACE_error("hendry : action=".$_POST["action"].",value=".$_POST["value"]);

if ($AUTHORIZED_GROUP < 0)
{
	$result = "FAIL";
	$reason = i18n("Permission deny. The user is unauthorized.");
}
else
{
	if ($_POST["action"] == "sethostname")
	{
		$value = $_POST["value"];
		if ($value != "")
		{
			set("/device/gw_name", $value);
			event("SHAREPORT.SETGWNAME");
			$RESULT = "OK";
			$REASON = "";					
		}
	}	
	else	fail(i18n("Unknown ACTION!"));
}
?>
<?echo '<?xml version="1.0" encoding="utf-8"?>';?>
<shareportreport>
	<action><?echo $_POST["action"];?></action>
	<result><?=$RESULT?></result>
	<reason><?=$REASON?></reason>
</shareportreport> 
