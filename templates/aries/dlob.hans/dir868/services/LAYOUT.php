<?
/* vi: set sw=4 ts=4:

	PORT	Switch Port	VID
	====	===========	===
	CPU		PORT5		1,2
	WAN		PORT4		2
	LAN1	PORT0		1
	LAN2	PORT1		1
	LAN3	PORT2		1
	LAN4	PORT3		1

NOTE:	We use VLAN 2 for WAN port, VLAN 1 for LAN ports.
		by David Hsieh <david_hsieh@alphanetworks.com>
		
	QoS      WAN
	=======	 =======
	enable   eth0.2
	disable  eth0
	
NOTE:	By default, the WAN interface is eth0 in dir868(for ctf),
	but this will lead to the download traffic limited by eth0,
	so we change WAN interface to eth0.2 when QoS enable
	sammy_hsu 2012/09/28
*/
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/phyinf.php";

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function error($errno)	{startcmd("exit ".$errno); stopcmd("exit ".$errno);}

function acl_bwc_check()
{
	$acl_bwc_enable = 0;

	foreach ("/bwc/entry")
	{
		if (query("enable") == 1 && query("uid") != "" )
		{
			$acl_bwc_enable = 1;
		}
	}

	if(query("/acl/accessctrl/enable")=="1")
	{
		foreach ("/acl/accessctrl/entry")
		{
			if(query("webfilter/enable") == "1" || query("webfilter/logging") == "1")
			{
				$acl_bwc_enable = 1;
			}
		}
	}

	return $acl_bwc_enable;
}
function powerdown_lan()
{
	/*please check data sheet.Page 10h-14h: Address 00h-01h bit 11 is power down*/
	startcmd('et robowr 0x10 0x0 0x800');
	startcmd('et robowr 0x11 0x0 0x800');
	startcmd('et robowr 0x12 0x0 0x800');
	startcmd('et robowr 0x13 0x0 0x800');
}
function layout_bridge()
{
	SHELL_info($START, "LAYOUT: Start bridge layout ...");

	/* Clean up all of vlan's setting */
	$vlan_index=0;
	$vlan_num=16;
	while($vlan_index < $vlan_num)
	{
		startcmd("nvram set vlan".$vlan_index."ports=");
		$vlan_index++;
	}
	
	/* Start .......................................................................... */
	/* Config VLAN as bridge layout. */
	echo "echo Start bridge layout ... > /dev/console\n";
	$HZONE = "0 1 2 3 4 5*";
	$WZONE = "5*";
	

	startcmd('nvram set vlan1ports="'.$HZONE.'"');	/* Host zone (LAN ports) */
	//startcmd('nvram set vlan2ports="'.$WZONE.'"');	/* WAN port */
	/* Using WAN MAC address during bridge mode. */
	$mac = PHYINF_gettargetmacaddr("1BRIDGE", "ETH-1");
	if ($mac=="") $mac="00:de:fa:30:50:10";
	
	startcmd("nvram set et0macaddr=".$mac);	/* Host zone (LAN ports) */
	
	if (acl_bwc_check() == 0)
	{
		startcmd("insmod /lib/modules/ctf.ko");
	}
	startcmd("insmod /lib/modules/et.ko");
	startcmd("ifconfig eth0 allmulti");
	
	
	startcmd("ip link set eth0 up");

	startcmd("vconfig add eth0 1");

	$map_index=0;
	while ($map_index < 8)
	{
		startcmd("vconfig set_ingress_map eth0.1 ".$map_index." ".$map_index);
		$map_index++;
	}

	startcmd("ip link set eth0.1 addr ".$mac);
	startcmd("ip link set eth0.1 up");

	/* Create bridge interface. */
	startcmd("brctl addbr br0; brctl stp br0 off; brctl setfd br0 0");
	startcmd('brctl addif br0 eth0.1');
	startcmd('ip link set br0 up');
	/*for https need lo interface*/
	startcmd('ip link set lo up');
	/*for bridge 192.168.0.50 alias ip access*/
	startcmd('ifconfig br0:1 192.168.0.50 up');
	startcmd('service HTTP restart');
	/* Setup the runtime nodes. */
	PHYINF_setup("ETH-1", "eth", "br0");

	/* Done */
	startcmd('xmldbc -s /runtime/device/layout bridge');
	startcmd('usockc /var/gpio_ctrl BRIDGE');
	startcmd('service ENLAN start');
	startcmd('service PHYINF.ETH-1 alias PHYINF.BRIDGE-1');
	startcmd('service PHYINF.ETH-1 start');

	$p = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-1", 0);
	add($p."/bridge/port",	"WIFI-STA");

	/* Stop ........................................................................... */
	SHELL_info($STOP, "LAYOUT: Stop bridge layout ...");
	stopcmd("service PHYINF.ETH-1 stop");
	stopcmd('service PHYINF.BRIDGE-1 delete');
	stopcmd('xmldbc -s /runtime/device/layout ""');
	stopcmd('/etc/scripts/delpathbytarget.sh /runtime phyinf uid ETH-1');
	stopcmd('brctl delif br0 eth0.1');
	stopcmd('ip link set eth0.1 down');
	/*for bridge 192.168.0.50 alias ip access*/
	stopcmd('ifconfig br0:1 down');
	stopcmd('ip link set br0 down');
	stopcmd('brctl delbr br0');
	stopcmd('vconfig rem eth0.1');
	return 0;
}

function layout_router($mode)
{
	SHELL_info($START, "LAYOUT: Start router layout ...");

	$qos_en = get("","/bwc/entry:1/enable");

	/* Clean up all of vlan's setting */
	$vlan_index=0;
	$vlan_num=16;
	while($vlan_index < $vlan_num)
	{
		startcmd("nvram set vlan".$vlan_index."ports=");
		$vlan_index++;
	}
	
	/* Start .......................................................................... */
	/* Config VLAN as router mode layout. (1 WAN + 4 LAN) */
	
	$i=0;
	while ($i < 4)
	{
		if ($HZONE!="") {$HZONE=$HZONE." ";}
		$HZONE=$HZONE.$i;
		$i++;
	}
	if($qos_en=="1")	{$WZONE = "4 5*";}
	else				{$WZONE = "4 5u";}
	if ($HZONE!="") {$HZONE=$HZONE." 5*";} else {$HZONE=$HZONE."5*";}
	
	startcmd('nvram set vlan1ports="'.$HZONE.'"');	/* Host zone (LAN ports) */
	startcmd('nvram set vlan2ports="'.$WZONE.'"');	/* WAN port */
		
	$lanmac = PHYINF_gettargetmacaddr($mode, "ETH-1");
	if		($mode=="1W1L") $wanmac = PHYINF_gettargetmacaddr("1W1L", "ETH-2");
	else if	($mode=="1W2L") $wanmac = PHYINF_gettargetmacaddr("1W2L", "ETH-3");
	if ($wanmac=="") $wanmac = "00:de:fa:30:50:10";
	if ($lanmac=="") $lanmac = "00:de:fa:30:50:00";
	
	startcmd("nvram set et0macaddr=".$lanmac);	/* Host zone (LAN ports) */
	
	if (acl_bwc_check() == 0)
	{
		startcmd("insmod /lib/modules/ctf.ko");
	}
	startcmd("insmod /lib/modules/et.ko");
	powerdown_lan();
	startcmd("ifconfig eth0 allmulti");
	
	/* Setup MAC address */
	/* Check User configuration for WAN port. */
	startcmd("ip link set eth0 up");
	
	startcmd("vconfig add eth0 1");
	if($qos_en=="1")	{startcmd("vconfig add eth0 2");}

	$map_index=0;
	while ($map_index < 8)
	{
		startcmd("vconfig set_ingress_map eth0.1 ".$map_index." ".$map_index);
		$map_index++;
	}
	if($qos_en=="1")
	{
		$map_index=0;
		while ($map_index < 8)
		{
			startcmd("vconfig set_ingress_map eth0.2 ".$map_index." ".$map_index);
			$map_index++;
		}
	}
	

	startcmd("ip link set eth0.1 addr ".$lanmac);
	startcmd("ip link set eth0.1 up");
	if($qos_en=="1")
	{
		startcmd("ip link set eth0.2 addr ".$wanmac);
		startcmd("ip link set eth0.2 up");
	}
	else
	{
		startcmd("ip link set eth0 addr ".$wanmac);
		startcmd("ip link set eth0 up");
	}
	/*for https need lo interface*/
	startcmd('ip link set lo up');
	

	/* Create bridge interface. */
	startcmd("brctl addbr br0; brctl stp br0 off; brctl setfd br0 0;");
	startcmd("brctl addif br0 eth0.1");
	startcmd("ip link set br0 up");
	if ($mode=="1W2L")
	{
		startcmd("brctl addbr br1; brctl stp br1 off; brctl setfd br1 0;");
		startcmd("ip link set br1 up");
	}

	/* Setup the runtime nodes. */
	$Wan_index_number = query("/device/router/wanindex");
	if ($mode=="1W1L")
	{
		PHYINF_setup("ETH-1", "eth", "br0");
		if($qos_en=="1")		{PHYINF_setup("ETH-2", "eth", "eth0.2");}
		else					{PHYINF_setup("ETH-2", "eth", "eth0");}
		/* set Service Alias */
		startcmd('service PHYINF.ETH-1 alias PHYINF.LAN-1');
		startcmd('service PHYINF.ETH-2 alias PHYINF.WAN-1');
		/* WAN: set extension nodes for linkstatus */
		$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-2", 0);
		//startcmd('xmldbc -x '.$path.'/linkstatus "get:psts -i 4"');
		startcmd('xmldbc -x '.$path.'/linkstatus "get:psts -i '.$Wan_index_number.'"');
	}
	else if ($mode=="1W2L")
	{
		PHYINF_setup("ETH-1", "eth", "br0");
		PHYINF_setup("ETH-2", "eth", "br1");
		if($qos_en=="1")		{PHYINF_setup("ETH-3", "eth", "eth0.2");}
		else					{PHYINF_setup("ETH-3", "eth", "eth0");}
		/* set Service Alias */
		startcmd('service PHYINF.ETH-1 alias PHYINF.LAN-1');
		startcmd('service PHYINF.ETH-2 alias PHYINF.LAN-2');
		startcmd('service PHYINF.ETH-3 alias PHYINF.WAN-1');
		/* WAN: set extension nodes for linkstatus */
		$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "ETH-3", 0);
		//startcmd('xmldbc -x '.$path.'/linkstatus "get:psts -i 4"');
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

	startcmd('xmldbc -x '.$path.'/linkstatus:1 "get:psts -i 0"');
	startcmd('xmldbc -x '.$path.'/linkstatus:2 "get:psts -i 1"');
	startcmd('xmldbc -x '.$path.'/linkstatus:3 "get:psts -i 2"');
	startcmd('xmldbc -x '.$path.'/linkstatus:4 "get:psts -i 3"');

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
		stopcmd("service PHYINF.LAN-2 delete");
	}
	stopcmd('service PHYINF.ETH-2 stop');
	stopcmd('service PHYINF.ETH-1 stop');
	stopcmd('service PHYINF.WAN-1 delete');
	stopcmd('service PHYINF.LAN-1 delete');
	stopcmd('xmldbc -s /runtime/device/layout ""');
	stopcmd('/etc/scripts/delpathbytarget.sh /runtime phyinf uid ETH-1');
	stopcmd('/etc/scripts/delpathbytarget.sh /runtime phyinf uid ETH-2');
	stopcmd('/etc/scripts/delpathbytarget.sh /runtime phyinf uid ETH-3');
	stopcmd('brctl delif br0 eth0.1');
	if($qos_en=="1")	{stopcmd('ip link set eth0.2 down');}
	stopcmd('ip link set eth0.1 down');
	stopcmd('ip link set br0 down');
	stopcmd('brctl delbr br0; brctl delbr br1');
	if($qos_en=="1")	{stopcmd('vconfig rem eth0.1; vconfig rem eth0.2');}
	else				{stopcmd('vconfig rem eth0.1');}
	stopcmd("rmmod et");
	return 0;
}

/* everything starts from here !! */
fwrite("w",$START, "#!/bin/sh\n");
fwrite("w", $STOP, "#!/bin/sh\n");

$ret = 9;
$layout = query("/device/layout");
if ($layout=="router")
{
	/* only 1W1L & 1W2L supported for router mode. */
	$mode = query("/device/router/mode"); if ($mode!="1W1L") $mode = "1W2L";
	$ret = layout_router($mode);
}
else if ($layout=="bridge")
{
	$ret = layout_bridge();
}

/* driver is not installed yet, we move this to s52wlan (tom, 20120405) */
/* startcmd("service PHYINF.WIFI start");*/ 
stopcmd("service PHYINF.WIFI stop");


error($ret);

?>
