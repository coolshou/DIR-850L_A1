<?/* vi: set sw=4 ts=4: */
/*
 *	Supported mode of WRG-ND07.
 *		1BRIDGE - bridge mode with 1 interface.
 *		1W1L	- 1 WAN and 1 LAN router mode.
 *		1W2L	- 1 WAN and 2 LAN router mode.
 *
 *	Interface mapping
 *		BRIDGE-1	- br0
 *		LAN-1		- br0
 *		LAN-2		- br1
 *		WAN-1		- eth2.2
 *
 *	INF\MODE	1BRIDGE		1W1L		1W2L
 *	-----------	-----------	-----------	----------
 *	ETH-1		BRIDGE-1	LAN-1		LAN-1
 *	ETH-2		not used	WAN-1		LAN-2
 *	ETH-2		not used	not used	WAN-1
 */

include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($errno)	{startcmd("exit ".$errno); stopcmd( "exit ".$errno);}

/* This function is copied from LAYOUT.php */
function portnum($name)
{
	if      ($name=="PORT-1") return 4;
	else if ($name=="PORT-2") return 3;
	else if ($name=="PORT-3") return 2;
	else if ($name=="PORT-4") return 1;
	else if ($name=="PORT-5") return 0;
	return "";
}

function phyinf_setmedia($layout, $ifname, $media)
{
	/* Only support for WAN port now. CONFIG_WAN_AT_P4=y */
	$port = query("/device/router/wanindex");

	if($port == "")
	{
		if ($layout=="1W1L" && $ifname=="ETH-2") $port = 4;
			else if ($layout=="1W2L" && $ifname=="ETH-3") $port = 4;
			else return;
	}

	if ($media=="") $media="AUTO";
	startcmd("slinktype -i ".$port." -d ".$media);
	stopcmd( "slinktype -i ".$port." -d AUTO");

	startcmd("sleep 1");
}

function phyinf_setup($ifname)
{
	$phyinf	= XNODE_getpathbytarget("", "phyinf", "uid", $ifname, 0);
	if ($phyinf=="") { error("9"); return; }
	if (query($phyinf."/active")!="1") { error("8"); return; }

	/* Set media */
	$media = query($phyinf."/media/linktype"); if ($media=="") $media="AUTO";
	phyinf_setmedia($mode, $ifname, $media);
	startcmd("# PHYINF.".$ifname.": media=".$media.", VID=".$vid);

	/* Set IPv6 */
	if (isfile("/proc/net/if_inet6")==1)
	{
		/* IPv6 is disabled by default (we modified the kernel code).
		 * Enable IPv6 here. */
		$dev = PHYINF_getifname($ifname);
		if ($dev!="")
		{
			startcmd("echo 0 > /proc/sys/net/ipv6/conf/".$dev."/disable_ipv6");
			stopcmd( "echo 1 > /proc/sys/net/ipv6/conf/".$dev."/disable_ipv6");
		}
	}

	/* Set the MAC address */
	$stsp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $ifname, 0);
	if ($stsp=="")
	{
		/* The LAYOUT service should be start before PHYINF.XXX.
		 * We should never reach here !! */
		fwrite("w", "/dev/console", "PHYINF: The LAYOUT service should be start before PHYINF !!!\n");
	}
	else
	{
		$mac = query($phyinf."/macaddr");
		if ($mac=="") $mac = XNODE_get_var("MACADDR_".$ifname);
		$mac = tolower($mac);
		$curr= tolower(query($stsp."/macaddr"));
		startcmd("# MAC: currrent ".$curr.", target ".$mac);
		if ($mac != $curr)
		{
			SHELL_info($_GLOBALS["START"],
				"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n".
				"!!! Bad MAC address. Device may work abnormally. !!!\n".
				"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		}
	}
}
?>
