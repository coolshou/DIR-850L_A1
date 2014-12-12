<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

function startcmd($cmd)			{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)			{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function pifsetup_error($errno)	{startcmd("exit ".$errno); stopcmd("exit ".$errno);}

function phyinf_setmedia($layout, $ifname, $media)
{
	/* Only support for WAN port now. */
	$wanindex = query("/device/router/wanindex");
	if($wanindex == "") 
	{
		TRACE_error("you do not assigned /device/router/wanindex using default 0\n");
		$wanindex = 0;
	}
	if		($layout=="1W1L" && $ifname=="ETH-2") $port = $wanindex;
	else if	($layout=="1W2L" && $ifname=="ETH-3") $port = $wanindex;
	else return;

	if ($media=="") $media="AUTO";
	startcmd("slinktype -i ".$port." -d ".$media);
	stopcmd( "slinktype -i ".$port." -d AUTO");

	startcmd("sleep 1");
}

function phyinf_setipv6($layout, $ifname)
{
	$stsp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $ifname, 0);
	if ($stsp!="")
	{
			$if_name = query($stsp."/name");
	}
	if ($layout=="1W2L")
	{
		if		($ifname=="ETH-1") $phy=$if_name;
		else if ($ifname=="ETH-2") $phy=$if_name;
		else if ($ifname=="ETH-3") $phy=$if_name;
	}
	else
	{
		if		($ifname=="ETH-1") $phy=$if_name;
		else if	($ifname=="ETH-2") $phy=$if_name;
	}

	if($ifname=="ETH-1")
	{
		startcmd('event IPV6ENABLE add "echo 0 > /proc/sys/net/ipv6/conf/'.$phy.'/disable_ipv6"');
	}
	else
	{
		startcmd('event IPV6ENABLE insert "echo 0 > /proc/sys/net/ipv6/conf/'.$phy.'/disable_ipv6"');
	}

	/*if ($phy!="")
	{
		startcmd("echo 0 > /proc/sys/net/ipv6/conf/".$phy."/disable_ipv6");
		stopcmd( "echo 1 > /proc/sys/net/ipv6/conf/".$phy."/disable_ipv6");
	}*/
}

function phyinf_setup($ifname)
{
	$phyinf	= XNODE_getpathbytarget("", "phyinf", "uid", $ifname, 0);
	if ($phyinf=="") { pifsetup_error("9"); return; }
	if (query($phyinf."/active")!="1") { pifsetup_error("8"); return; }

	/* Get layout mode */
	$layout = query("/runtime/device/layout");
	if		($layout=="bridge") $mode = "1BRIDGE";
	else if	($layout=="router") $mode = query("/runtime/device/router/mode");
	else { pifsetup_error("9"); return; }
	if ($mode=="") $mode = "1W2L";

	/* Set media */
	$media = query($phyinf."/media/linktype");
	phyinf_setmedia($mode, $ifname, $media);

	/* Set IPv6 */
	if (isfile("/proc/net/if_inet6")==1)
	{
		/**********************************************************************************
		 * only enable ipv6 function at br0(LAN) and eth2.2(WAN), other disable by default
		 *********************************************************************************/
		phyinf_setipv6($mode, $ifname);
	}

	/* Set the MAC address */
	$stsp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $ifname, 0);
	if ($stsp=="")
	{
		/* The LAYOUT service should be start before PHYINF.XXX.
		 * We should never reach here !! */
		fwrite("w", "/dev/console", "PHYINF: The LAYOUT service should be start before PHYINF !!!\n");
	}
    else if (query($stsp."/bridge/port#")>0)
	{  
		/* DO NOT allow to change the bridge device's MAC address. */
		startcmd("# ".$ifname." is a bridge device, skip MAC address setting.");
	}
	else
	{
		$mac = PHYINF_gettargetmacaddr($mode, $ifname);
		$curr= tolower(query($stsp."/macaddr"));
		if ($mac != $curr)
		{
			fwrite("w", "/dev/console", "PHYINF.".$ifname.": cfg[".$mac."] curr[".$curr."], restart the device !!!\n");
			//startcmd('xmldbc -t "restart:3:/etc/init0.d/rcS"');
			$if_name = query($stsp."/name");
			startcmd('ifconfig '.$if_name.' down');
			startcmd('ifconfig '.$if_name.' hw ether '.$mac);
			startcmd('ifconfig '.$if_name.' up');
		}
	}
}
?>
