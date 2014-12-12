<?
/* We use VID 2 for WAN port, VID 1 for LAN ports.
 * by David Hsieh <david_hsieh@alphanetworks.com> */
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/phyinf.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($errno)	{startcmd("exit ".$errno); stopcmd("exit ".$errno);}
function setup_switch($mode)
{
	stopcmd("rtlioc initvlan");

	/* Advanced VLAN setting is disabled, use default VLAN layout. */
	if ($mode=="bridge") startcmd("rtlioc bridgemode");
	else startcmd("rtlioc routermode");
	//startcmd("rtlioc enlan");
}
function setup_vlaninf($dev,$VID,$macaddr)
{
	$devname = $dev.".".$VID;
	startcmd(
			"vconfig add ".$dev." ".$VID."; ".
			"ip link set ".$devname." addr ".$macaddr."; ".
			"ip link set ".$devname." up"
			);
	stopcmd("ip link set ".$devname." down; vconfig rem ".$devname);
}
function layout_bridge()
{
	SHELL_info($START, "LAYOUT: Start bridge layout ...");

	/* Start .......................................................................... */
	/* Config RTL8367 as bridge mode layout. */
	setup_switch("bridge");

	/* Using WAN MAC address during bridge mode. */
	$mac = PHYINF_getmacsetting("BRIDGE-1");
	setup_vlaninf("eth2","1",$mac);

	/* Create bridge interface. */
	startcmd("brctl addbr br0; brctl stp br0 off; brctl setfd br0 0");
	startcmd("brctl addif br0 eth2.1");
	startcmd("brctl addif br0 wifig0");
	startcmd("brctl addif br0 wifia0");
	startcmd("ip link set br0 up");

	/* Setup the runtime nodes. */
	PHYINF_setup("ETH-1", "eth", "br0");

	/* Done */
	startcmd("xmldbc -s /runtime/device/layout bridge");
	startcmd("usockc /var/gpio_ctrl BRIDGE");
	startcmd("service ENLAN start");
	startcmd("service PHYINF.ETH-1 alias PHYINF.BRIDGE-1");
	startcmd("service PHYINF.ETH-1 start");

	/* Stop ........................................................................... */
	SHELL_info($STOP, "LAYOUT: Stop bridge layout ...");
	stopcmd("service PHYINF.ETH-1 stop");
	stopcmd("service PHYINF.BRIDGE-1 delete");
	stopcmd('xmldbc -s /runtime/device/layout ""');
	stopcmd("/etc/scripts/delpathbytarget.sh /runtime phyinf uid ETH-1");
	stopcmd("brctl delif br0 wifia0");
	stopcmd("brctl delif br0 wifig0");
	stopcmd("brctl delif br0 eth2.1");
	stopcmd("ip link set eth2.1 down");
	stopcmd("brctl delbr br0");
	//stopcmd("rtlioc initvlan");
	return 0;
}

function layout_router($mode)
{
	SHELL_info($START, "LAYOUT: Start router layout ...");
	$Wan_index_number = query("/device/router/wanindex");
	/* Start .......................................................................... */
	/* Config RTL8367 as router mode layout. (1 WAN + 4 LAN) */
	setup_switch("router");

	//+++ hendry, for wifi topology
	$p = XNODE_getpathbytarget("", "phyinf", "uid", "ETH-1", 0);
	set($p."/bridge/ports/entry:1/uid",		"MBR-1");
	set($p."/bridge/ports/entry:1/phyinf",	"BAND24G-1.1");	
	set($p."/bridge/ports/entry:2/uid",		"MBR-2");
	set($p."/bridge/ports/entry:2/phyinf",	"BAND5G-1.1");	
	$p = XNODE_getpathbytarget("", "phyinf", "uid", "ETH-2", 0);
	set($p."/bridge/ports/entry:1/uid",		"MBR-1");
	set($p."/bridge/ports/entry:1/phyinf",	"BAND24G-1.2");	
	set($p."/bridge/ports/entry:2/uid",		"MBR-2");
	set($p."/bridge/ports/entry:2/phyinf",	"BAND5G-1.2");	
	//--- hendry
	
	/* Setup MAC address */
	$wanmac = PHYINF_getmacsetting("WAN-1");
	$lanmac = PHYINF_getmacsetting("LAN-1");
	setup_vlaninf("eth2","1",$lanmac);
        setup_vlaninf("eth2","2",$wanmac);

	/* set smaller tx queue len */
	startcmd("ifconfig eth2 txqueuelen 200");

	/* Create bridge interface. */
	startcmd("brctl addbr br0; brctl stp br0 off; brctl setfd br0 0");
	startcmd("brctl addif br0 eth2.1");
	//startcmd("brctl addif br0 wifig0");
	//startcmd("brctl addif br0 wifig0.1");
	startcmd("ip link set br0 up");
	if ($mode=="1W2L")
	{
		startcmd("brctl addbr br1; brctl stp br1 off; brctl setfd br1 0");
		//hendry, we let guestzone to bring br1 up 
		//startcmd("ip link set br1 up");;
	}

	/* Setup the runtime nodes. */
	if ($mode=="1W1L")
	{
		PHYINF_setup("ETH-1", "eth", "br0");
		PHYINF_setup("ETH-2", "eth", "eth2.2");
		/* set Service Alias */
		startcmd('service PHYINF.ETH-1 alias PHYINF.LAN-1');
		startcmd('service PHYINF.ETH-2 alias PHYINF.WAN-1');
		/* WAN: set extension nodes for linkstatus */
		$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-2", 0);
		startcmd('xmldbc -x '.$path.'/linkstatus "get:psts -i '.$Wan_index_number.'"');
	}
	else if ($mode=="1W2L")
	{
		PHYINF_setup("ETH-1", "eth", "br0");
		PHYINF_setup("ETH-2", "eth", "br1");
		PHYINF_setup("ETH-3", "eth", "eth2.2");
		/* set Service Alias */
		startcmd('service PHYINF.ETH-1 alias PHYINF.LAN-1');
		startcmd('service PHYINF.ETH-2 alias PHYINF.LAN-2');
		startcmd('service PHYINF.ETH-3 alias PHYINF.WAN-1');
		/* WAN: set extension nodes for linkstatus */
		$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-3", 0);
		startcmd('xmldbc -x '.$path.'/linkstatus "get:psts -i '.$Wan_index_number.'"');
	}

	//+++ hendry
	$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-1", 0);
	add($p."/bridge/port",	"BAND24G-1.1");	
	add($p."/bridge/port",	"BAND5G-1.1");	
	$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-2", 0);
	add($p."/bridge/port",	"BAND24G-1.2");	
	add($p."/bridge/port",	"BAND5G-1.2");	
	//--- hendry

	/* LAN: set extension nodes for linkstatus */
	$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-1", 0);
	startcmd('xmldbc -x '.$path.'/linkstatus:1 "get:psts -i 4"');
	startcmd('xmldbc -x '.$path.'/linkstatus:2 "get:psts -i 3"');
	startcmd('xmldbc -x '.$path.'/linkstatus:3 "get:psts -i 2"');
	startcmd('xmldbc -x '.$path.'/linkstatus:4 "get:psts -i 1"');

	/* Done */
	startcmd("xmldbc -s /runtime/device/layout router");
	startcmd("xmldbc -s /runtime/device/router/mode ".$mode);
	startcmd("usockc /var/gpio_ctrl ROUTER");
	startcmd("service PHYINF.ETH-1 start");
	startcmd("service PHYINF.ETH-2 start");
	if ($mode=="1W2L") startcmd("service PHYINF.ETH-3 start");

	/* Stop ........................................................................... */
	SHELL_info($STOP, "LAYOUT: Stop router layout ...");
	if ($mode=="1W2L")
	{
		stopcmd("service PHYINF.ETH-3 stop");
		stopcmd('service PHYINF.LAN-2 delete');
	}
	stopcmd("service PHYINF.ETH-2 stop");
	stopcmd("service PHYINF.ETH-1 stop");
	stopcmd('service PHYINF.WAN-1 delete');
	stopcmd('service PHYINF.LAN-1 delete');
	stopcmd('xmldbc -s /runtime/device/layout ""');
	stopcmd('/etc/scripts/delpathbytarget.sh /runtime phyinf uid ETH-1');
	stopcmd('/etc/scripts/delpathbytarget.sh /runtime phyinf uid ETH-2');
	stopcmd('/etc/scripts/delpathbytarget.sh /runtime phyinf uid ETH-3');
	//stopcmd('brctl delif br0 wifig0');
	stopcmd('brctl delif br0 eth2.1');
	//stopcmd('brctl delif br1 wifig0.1');
	stopcmd('ip link set eth2.1 down');
	stopcmd('ip link set eth2.2 down');
	stopcmd('brctl delbr br0; brctl delbr br1');
	stopcmd('vconfig rem eth2.1; vconfig rem eth2.2');
	return 0;
}

/* everything starts from here !! */
fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

$ret = 9;
$layout	= query("/device/layout");

startcmd("ifconfig lo up");
stopcmd("ifconfig lo down");

if ($layout=="router")
{
	/* only 1W1L & 1W2L supported for router mode. */
	$mode = query("/device/router/mode"); if ($mode!="1W1L") $mode = "1W2L";
	$ret = layout_router($mode);

	/* Start Hw_nat here */
	startcmd("service HW_NAT start");
}
else if ($layout=="bridge")
{
	$ret = layout_bridge();
}


startcmd("service PHYINF.WIFI start");
stopcmd("service PHYINF.WIFI stop");
startcmd("service DEVICE start");
stopcmd("service DEVICE stop");

error($ret);
?>
