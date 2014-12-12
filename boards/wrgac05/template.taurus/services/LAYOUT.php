<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($errno)	{startcmd("exit ".$errno); stopcmd("exit ".$errno);}

/****************************************************************************/

/* $prefix is something like: lanmac, wanmac. $i is a number (index).
 * The combination of $prefix & $i is the item of MAC address in devdata. */
function get_macaddr($prefix, $i)
{
	$mac = query("/runtime/devdata/".$prefix.$i);
	if ($mac=="") $mac = query("/runtime/devdata/".$prefix);
	if ($mac=="")
	{
		if ($i>0) $i--;
		/* $i should be less than 10 or we will have trouble. */
		if		($prefix=="wanmac")	$mac = "00:DE:FA:5E:A0:0".$i;
		else if	($prefix=="lanmac")	$mac = "00:DE:FA:5E:B0:0".$i;
		else						$mac = "00:DE:FA:5E:FF:0".$i;
	}
	return $mac;
}

/* Walk throuhg all the physical interfaces of the logical interface.
 * Prepare the MAC address and save in the GLOBAL variable space. */
function prepare_macaddr($prefix, $macnam)
{
	foreach ("/inf")
	{
		$uid = query("uid");
		if (cut($uid,0,"-")==$prefix)
		{
			$phyinf = query("phyinf");
			$phyinfp= XNODE_getpathbytarget("", "phyinf", "uid", $phyinf, 0);
			if ($phyinfp!="")
			{
				$macaddr = get_macaddr($macnam, cut($uid,1,"-"));
				XNODE_set_var("MACADDR_".$phyinf, $macaddr);
				startcmd("# choosing ".$macaddr." for ".$uid."/".$phyinf);
			}
		}
	}
}

/* In bridge mode, we use wanmac. */
function prepare_bridge_macaddr()
{
	prepare_macaddr("BRIDGE", "lanmac");
}

function prepare_router_macaddr($mdoe)
{
	prepare_macaddr("LAN", "lanmac");
	prepare_macaddr("WAN", "wanmac");
}

/****************************************************************************/

function layout_bridge()
{
	startcmd('echo "LAYOUT: BRIDGE" > /dev/console');

	/* Allocate MAC addresses for interfaces. */
	prepare_bridge_macaddr();

	/* Setup VLAN, bridge device and interfaces. */
	stopcmd('xmldbc -s /runtime/device/layout ""');

	/* Sart ... */
	$infp = XNODE_getpathbytarget("", "inf", "uid", "BRIDGE-1", 0);
	$phyinf = query($infp."/phyinf");
	$macaddr = XNODE_get_var("MACADDR_".$phyinf);

	/* Create bridge interface. */
	/* Realtek's suggestion to setup AP mode. */
	/*+++*/
	startcmd("echo 1 > /var/sys_op");
	startcmd("echo 1 > /proc/sw_nat");
	/* Don't apply VLAN. */
	startcmd("echo 0 > /proc/rtk_vlan_support");
	/*+++*/
	startcmd("brctl addbr br0");
	startcmd("brctl stp br0 off");
	startcmd("brctl setfd br0 0");
	startcmd("brctl addif br0 eth1");
	startcmd("ip link set br0 addr ".$macaddr);
	startcmd("ip link set br0 up");
	startcmd("ip link set eth1 addr ".$macaddr);
	startcmd("ip link set eth1 up");
	startcmd("ip link set lo up");
	/* Update the runtime nodes */
	PHYINF_setup($phyinf, "eth", "br0");

	startcmd("service PHYINF.".$phyinf." alias PHYINF.BRIDGE-1");
	startcmd("service PHYINF.".$phyinf." start");
	startcmd("xmldbc -s /runtime/device/layout bridge");
	startcmd("usockc /var/gpio_ctrl BRIDGE");
    $p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-1", 0);
	add($p."/bridge/port",  "BAND24G-1.1");

	startcmd("ifconfig br0:1 192.168.0.1 up");
	$p = XNODE_getpathbytarget("/runtime", "inf", "uid", "BRIDGE-1", 1);
	set($p."/ipalias/cnt",			1);
	set($p."/ipalias/ipv4/ipaddr:1",		"192.168.0.1");
	set($p."/devnam","br0"); 

	/* Stop ... */
	stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$phyinf);
	stopcmd("ip link set eth1 down");
	stopcmd("ip link set br0 down");
	stopcmd("brctl delbr br0");
	stopcmd("service PHYINF.".$phyinf." stop");
	stopcmd("service PHYINF.BRIDGE-1 delete");

	return 0;
}

function layout_router($mode)
{
	startcmd('echo "LAYOUT: ROUTER" > /dev/console');

	/* Allocate MAC addresses for interfaces. */
	prepare_router_macaddr();

	/* Setup VLAN, bridge device and interfaces. */
	stopcmd('xmldbc -s /runtime/device/layout ""');

	/* Sart ... */
	$lanp		= XNODE_getpathbytarget("", "inf", "uid", "LAN-1", 0);
	$lanphy		= query($lanp."/phyinf");
	$lanmac		= XNODE_get_var("MACADDR_".$lanphy);
	$wanp		= XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);
	$wanphy		= query($wanp."/phyinf");
	$wanphyinfp	= XNODE_getpathbytarget("", "phyinf", "uid", $wanphy, 0);
	$wanmac		= query($wanphyinfp."/macaddr");
	if ($wanmac=="") $wanmac = XNODE_get_var("MACADDR_".$wanphy);

	/* Create LAN interface. */
	/**/
	/* Realtek's suggestion to setup RG mode. */
	/*+++*/
	startcmd("echo 0 > /var/sys_op");
	startcmd("echo 1 > /proc/hw_nat");
	startcmd("echo 0 > /proc/sw_nat");
	startcmd("echo 0 > /proc/rtk_vlan_support");
	/*++*/
	startcmd("brctl addbr br0");
	startcmd("brctl stp br0 off");
	startcmd("brctl setfd br0 0");
	startcmd("brctl addif br0 eth0");
	startcmd("ip link set br0 addr ".$lanmac);
	startcmd("ip link set br0 up");
	startcmd("ip link set eth0 addr ".$lanmac);
	startcmd("ip link set eth0 up");
	startcmd("ip link set lo up");
	startcmd("service PHYINF.".$lanphy." alias PHYINF.LAN-1");
	startcmd("service PHYINF.".$lanphy." start");

	/* Create WAN interface. */
	startcmd("ip link set eth1 addr ".$wanmac);
	startcmd("ip link set eth1 up");

	/* Update the runtime nodes */
	PHYINF_setup($lanphy, "eth", "br0");
	PHYINF_setup($wanphy, "eth", "eth1");

	startcmd("service PHYINF.".$wanphy." alias PHYINF.WAN-1");
	startcmd("service PHYINF.".$wanphy." start");
	startcmd('xmldbc -s /runtime/device/layout router');
	startcmd("xmldbc -s /runtime/device/router/mode ".$mode);
	startcmd("usockc /var/gpio_ctrl ROUTER");

    $p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-1", 0);
	add($p."/bridge/port",  "BAND24G-1.1");
	add($p."/bridge/port",  "BAND24G-1.2");
	add($p."/bridge/port",  "BAND5G-1.1");
	add($p."/bridge/port",  "BAND5G-1.2");
	/* Stop ... */
	stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$lanphy);
	stopcmd("phpsh /etc/scripts/delpathbytarget.php BASE=/runtime NODE=phyinf TARGET=uid VALUE=".$wanphy);
	stopcmd("ip link set eth1 down");
	stopcmd("ip link set eth0 down");
	stopcmd("ip link set br0 down");
	stopcmd("brctl delbr br0");
	stopcmd("service PHYINF.".$lanphy." stop");
	stopcmd("service PHYINF.".$wanphy." stop");
	stopcmd("service PHYINF.LAN-1 delete");
	stopcmd("service PHYINF.WAN-1 delete");

	return 0;
}

/****************************************************************************/
fwrite("w",$START,"#!/bin/sh\n");
fwrite("w", $STOP,"#!/bin/sh\n");

/* Setup layout */
$LAYOUT = query("/device/layout");
if ($LAYOUT=="router")
{
	/* only 1W1L & 1W2L supported for router mode. */
	$mode = query("/device/router/mode"); if ($mode!="1W1L") $mode = "1W2L";
	$ret = layout_router($mode);
}

else if	($LAYOUT=="bridge") $ret = layout_bridge();
startcmd("service PHYINF.WIFI start");
stopcmd("service PHYINF.WIFI stop");

error($ret);
?>
