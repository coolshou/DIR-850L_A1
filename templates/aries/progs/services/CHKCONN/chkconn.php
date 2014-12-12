<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/xnode.php";

function chkconn($inf)
{
	$infp	= XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
	$inet	= query($infp."/inet");
	$inetp	= XNODE_getpathbytarget("inet", "entry", "uid", $inet, 0);
	$addrtype = query($inetp."/addrtype");
	if ($addrtype == "ppp4")
	{
		$dialupmode = query($inetp."/".$addrtype."/dialup/mode");
	}
	$active	= query($infp."/active");
	$disable= query($infp."/disable");
	$backup	= query($infp."/backup");
	$interval= query($infp."/chkinterval");
	if ($interval == "" || $interval < 15) $interval=15;

	if ($addrtype == "ipv4")
		$check = "yes";
	else if ($addrtype == "ppp4" && $dialupmode == "auto")
		$check = "yes";
	else
		$check = "no";

	if ($active == 1 && $disable == 0 && $backup != "" && $check == "yes")
	{
		fwrite(a,$_GLOBALS["START"], 'xmldbc -t chkconn.'.$inf.':'.$interval.':"sh /etc/scripts/backupconn.sh NULL '.$inf.' connected"\n');
		fwrite(a,$_GLOBALS["STOP"], 'killall chkconn\n');
		fwrite(a,$_GLOBALS["STOP"], 'xmldbc -k chkconn.'.$inf.'\n');
	}
}

?>
