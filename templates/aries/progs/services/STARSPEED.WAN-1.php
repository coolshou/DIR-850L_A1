<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");
function starspeed($name)
{
	/* Get the interface */
	$infp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);
	if ($infp == "") return;

	$inet	= query($infp."/inet");
	$inetp	= XNODE_getpathbytarget("/inet", "entry", "uid", $inet, 0);
	$phyinf = query($infp."/phyinf");

	$user	= get("s", $inetp."/ppp4/username");
	$pass	= get("s", $inetp."/ppp4/password");
	$ifname = PHYINF_getifname($phyinf);
	$enable = query($inetp."/ppp4/pppoe/starspeed/enable");
	$region = get("s", $inetp."/ppp4/pppoe/starspeed/region");

	if ($enable == 1)
		fwrite("a",$_GLOBALS["START"], "/etc/scripts/starspeed.sh \"".$user."\" \"".$pass."\" \"".$ifname."\" \"".$region."\"\n");
}
starspeed("WAN-1");
fwrite("a",$START, "exit 0\n");
fwrite("a", $STOP, "exit 0\n");
?>
