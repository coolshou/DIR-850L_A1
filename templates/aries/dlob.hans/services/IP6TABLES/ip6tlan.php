<?
include "/htdocs/phplib/xnode.php";

function IP6TLAN_build_command($name)
{
	fwrite("w",$_GLOBALS["START"], "#!/bin/sh\n");
	fwrite("a",$_GLOBALS["START"], "ip6tables -t filter -F FWD.".$name."\n");
	fwrite("a",$_GLOBALS["START"], "ip6tables -t filter -F INP.".$name."\n");

	$iptcmdFWD = "ip6tables -t filter -A FWD.".$name;
	$iptcmdIN  = "ip6tables -t filter -A INP.".$name;
	$path = XNODE_getpathbytarget("", "inf", "uid", $name, 0);

	if ($path!="")
	{
		$fw   = XNODE_get_var("FIREWALL6.USED");
		$security = query("/device/simple_security");
		if ($fw > 0)	fwrite("a", $_GLOBALS["START"],
							$iptcmdFWD." -j FIREWALL\n");
		if ($security > 0)	fwrite("a", $_GLOBALS["START"],
							$iptcmdFWD." -j FWD.SMPSECURITY.".$name."\n");
		if ($fw > 0)	fwrite("a", $_GLOBALS["START"],
							$iptcmdFWD." -j FIREWALL_POLICY\n");

		/* Outbound filter will be run faster to drop some packets. */
		fwrite("a",$_GLOBALS["START"],  $iptcmdFWD." -j FWD.OBFILTER\n");
		fwrite("a",$_GLOBALS["START"],  $iptcmdIN." -j INP.OBFILTER\n");
	}
	fwrite("a",$_GLOBALS["START"], "exit 0\n");

	fwrite("w",$_GLOBALS["STOP"],  "#!/bin/sh\n");
	fwrite("a",$_GLOBALS["STOP"],  "ip6tables -t filter -F FWD.".$name."\n");
	fwrite("a",$_GLOBALS["STOP"],  "ip6tables -t filter -F INP.".$name."\n");
	fwrite("a",$_GLOBALS["STOP"],  "exit 0\n");
}

?>
