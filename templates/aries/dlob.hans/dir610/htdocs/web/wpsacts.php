HTTP/1.1 200 OK
Content-Type: text/xml
<?
include "/htdocs/phplib/xnode.php";

function check_pin($pin)
{
	/* more checking added for WPS 2.0 
		We allow pin with : xxxx-xxxx
							xxxx xxxx
							xxxxxxxx
	*/
	$len = strlen($pin);
	$delim = "";
	
	//we support 4 digits
	if($len==4)
	{
		if(isdigit($pin)!=1) { return 0; }
		else 				 { return $pin; }
	}
	if($len==9)
	{
		if(cut_count($pin, "-")==2) 		{ $delim = "-"; }
		else if(cut_count($pin, " ")==2) 	{ $delim = " "; }
		else { return 0; }

		$val1=cut($pin,0,$delim);
		$val2=cut($pin,1,$delim);
		if(strlen($val1)!=4 || strlen($val2)!=4) { return 0; }		
		$pin = $val1.$val2;
	}
	
	if (isdigit($pin)!=1) return 0;	
	if (strlen($pin)!=8) return 0;
	$i = 0; $pow = 3; $sum = 0;
	while($i < 8)
	{
		$sum = $pow * substr($pin, $i, 1) + $sum;
		if ($pow == 3)  $pow = 1;
		else            $pow = 3;
		$i++;
	}
	$sum = $sum % 10;
	if ($sum == 0)  return $pin;
	else            return 0;
}

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
	$i = 0;
	while ($i < 2)
	{
		$i++;
		if($i==1)
			$uid = "BAND24G-1.1";
		else 
			$uid = "BAND5G-1.1";

        $p = XNODE_getpathbytarget("", "phyinf", "uid",$uid);
        if(query($p."/active")!="1") continue;			

		if ($_POST["action"] == "PIN")
		{
			$pin = check_pin($_POST["pin"]);
			if ($pin == 0)	{ fail(i18n("Invalid PIN code!"));	break; }

			$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $uid);
			if ($path == "") {fail(i18n("Invalid Path!")); break;}
			set($path."/media/wps/enrollee/method", "pin");
			set($path."/media/wps/enrollee/pin", $pin);
			event("WPSPIN");
		}
		else if ($_POST["action"] == "PBC")
		{
			$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $uid);
			if ($path == "") {fail(i18n("Invalid Path!")); break;}
			set($path."/media/wps/enrollee/method", "pbc");
			set($path."/media/wps/enrollee/pin", "00000000");
			event("WPSPBC.PUSH");
		}
		else
		{
			fail(i18n("Unknown ACTION!"));	break;
		}
		$RESULT = "OK";
		$REASON = "";
	}
}
?>
<?echo '<?xml version="1.0" encoding="utf-8"?>';?>
<wpsreport>
	<action><?echo $_POST["action"];?></action>
	<result><?=$RESULT?></result>
	<reason><?=$REASON?></reason>
</wpsreport>
