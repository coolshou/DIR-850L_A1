HTTP/1.1 200 OK
Content-Type: text/xml
<?
include "/htdocs/phplib/xnode.php";

function fail($reason)
{
	$_GLOBALS["RESULT"] = "FAIL";
	$_GLOBALS["REASON"] = $reason;
}

if ($AUTHORIZED_GROUP < 0)
{
	$result = "FAIL";
	$reason = i18n("Permission deny. The user is unauthorized.");
}
else
{
	if ($_POST["action"] == "SITESURVEY")
	{
		event("SITESURVEY");
		$RESULT = "OK";
		$REASON = "";			
	}	
	else	fail(i18n("Unknown ACTION!"));
}
?>
<?echo '<?xml version="1.0" encoding="utf-8"?>';?>
<sitesurveyreport>
	<action><?echo $_POST["action"];?></action>
	<result><?=$RESULT?></result>
	<reason><?=$REASON?></reason>
</sitesurveyreport>
