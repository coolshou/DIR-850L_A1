<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/phyinf.php";

function getphyinf($inf)
{
	$infp = XNODE_getpathbytarget("", "inf", "uid", $inf, 0);
	if ($infp == "") return "";
	$phyinf = query($infp."/phyinf");
	return PHYINF_getifname($phyinf);
}

fwrite(w, $START, "#!/bin/sh\n");
fwrite(w, $STOP, "#!/bin/sh\n");
$wan = getphyinf("WAN-1");
$lan = getphyinf("LAN-1");
$layout = query("/runtime/device/layout");
if ($layout == "router" && $wan!="" && $lan!="")
{
	anchor("/device/passthrough");
	if (query("ipv6")=="1")  $ipv6 = 1; else $ipv6 = 0;
	if (query("pppoe")=="1") $ppoe = 2; else $ppoe = 0;
	$pthrough = $ipv6 + $ppoe;
	
	fwrite(a,$START,'echo '.$pthrough.' > /proc/custom_Passthru\n');
	if ($pthrough != 0)
	{
		fwrite(a,$START,'brctl addif br0 peth0\n');
		fwrite(a,$START,'ifconfig peth0 up\n');
		
		fwrite(a,$STOP,'echo 0 > proc/custom_Passthru\n');
		fwrite(a,$STOP,'brctl delif br0 peth0\n');
		fwrite(a,$STOP,'ifconfig peth0 down\n');
	}
}

/* restart IPT.LAN-{#} to add or remove the rules for VPN passthrough in the chain FWD.LAN-{#}. */
$i = 1;
while ($i>0)
{
	$ifname = "LAN-".$i;
	$ifpath = XNODE_getpathbytarget("", "inf", "uid", $ifname, 0);
	if ($ifpath == "") { $i=0; break; }
	fwrite("a",$_GLOBALS["START"], "service IPT.".$ifname." restart\n");
	fwrite("a",$_GLOBALS["STOP"],  "service IPT.".$ifname." restart\n");
	$i++;
}
fwrite(a, $START,'exit 0\n');
fwrite(a, $STOP, 'exit 0\n');
?>
