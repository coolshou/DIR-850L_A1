<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/inf.php";

function startcmd($cmd)    {fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)     {fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}

function bwc_error($errno)
{
	startcmd("exit ".$errno."\n");
	stopcmd( "exit ".$errno."\n");
}

function copy_bwc_entry($from, $to)
{
	del($to."/bwc");
	set($to."/bwc/uid",				query($from."/uid"));
	set($to."/bwc/autobandwidth",	query($from."/autobandwidth"));
	set($to."/bwc/bandwidth",		query($from."/bandwidth"));

}

/* service_pre_trigger() and service_post_trigger() both are
   used to trigger other services which needed to start/restart/stop
   when bwc status change(start/restart/stop) */
function service_pre_trigger()
{
	/* remove/insert software nat/TurboNAT */
	startcmd("rmmod sw_tcpip");
	stopcmd("insmod /lib/modules/sw_tcpip.ko");
}
function service_post_trigger()
{
	/* restart IPTDEFCHAIN */
	startcmd("service IPTDEFCHAIN restart");
	stopcmd("service IPTDEFCHAIN restart");
}

function porttype_handle($entryp, $ipt_add_cmd, $ipt_del_cmd, $portrange, $mark_cmd) 
{
	$i = 0;
	while ( $i < 2 )
	{
		$tmp_ipt_add_cmd = "";
		$tmp_ipt_del_cmd = "";
		if ($portrange=="-1") {
			if ( $i == 0 ) { // for tcp
				$tmp_ipt_add_cmd = $ipt_add_cmd." -p tcp -m mport --ports 0:65535";
				$tmp_ipt_del_cmd = $ipt_del_cmd." -p tcp -m mport --ports 0:65535";
			}else { // for udp
				$tmp_ipt_add_cmd = $ipt_add_cmd." -p udp -m mport --ports 0:65535";
				$tmp_ipt_del_cmd = $ipt_del_cmd." -p udp -m mport --ports 0:65535";
			}
		} else if ($portrange=="0") {
			if ( $i == 0 ) { // for tcp
				$tmp_ipt_add_cmd = $ipt_add_cmd." -p tcp --dport ".query($entryp."/port/start");
				$tmp_ipt_del_cmd = $ipt_del_cmd." -p tcp --dport ".query($entryp."/port/start");
			}else { // for udp
				$tmp_ipt_add_cmd = $ipt_add_cmd." -p udp --dport ".query($entryp."/port/start");
				$tmp_ipt_del_cmd = $ipt_del_cmd." -p udp --dport ".query($entryp."/port/start");
			}
		} else {
			if ( $i == 0 ) { // for tcp
				$tmp_ipt_add_cmd = $ipt_add_cmd." -p tcp -m mport --ports ".query($entryp."/port/start").":".query($entryp."/port/end");
				$tmp_ipt_del_cmd = $ipt_del_cmd." -p tcp -m mport --ports ".query($entryp."/port/start").":".query($entryp."/port/end");
			}else { // for udp
				$tmp_ipt_add_cmd = $ipt_add_cmd." -p udp -m mport --ports ".query($entryp."/port/start").":".query($entryp."/port/end");
				$tmp_ipt_del_cmd = $ipt_del_cmd." -p udp -m mport --ports ".query($entryp."/port/start").":".query($entryp."/port/end");
			}
		}
		startcmd($tmp_ipt_add_cmd.$mark_cmd);
		$i++;
	} // while() --- END
}

function bwc_bc_start($rtbwcp, $name, $ifname)
{
	$tc_qd_add		= "tc qdisc add dev ".$ifname;
	$tc_qd_del		= "tc qdisc del dev ".$ifname;
	$tc_class_add	= "tc class add dev ".$ifname;
	$tc_class_del	= "tc class del dev ".$ifname;
	$tc_filter_add	= "tc filter add dev ".$ifname." parent 50: protocol ip prio 100";
	$tc_filter_del	= "tc filter del dev ".$ifname." parent 50: protocol ip prio 100";
	$ipt_flush_cmd	= "iptables -t mangle -F PRE.BWC.".$name;
	$ipt_add_prefix	= "iptables -t mangle -A PRE.BWC.".$name;
	$ipt_del_prefix	= "iptables -t mangle -D PRE.BWC.".$name;

	$unit = "kbit";

	/* trate: total rate (bandwidth) */
	$trate = query($rtbwcp."/bandwidth");
	$rate1 = $trate/4; $rate2 = $trate/4;
	$rate3 = $trate/4; $rate4 = $trate/4;
	$rate1_celi = $trate; $rate2_celi = $trate;
	$rate3_celi = $trate; $rate4_celi = $trate;

	$trate = $trate.$unit;
	$rate1 = $rate1.$unit; $rate2 = $rate2.$unit;
	$rate3 = $rate3.$unit; $rate4 = $rate4.$unit;
	$rate1_celi = $rate1_celi.$unit; $rate2_celi = $rate2_celi.$unit;
	$rate3_celi = $rate3_celi.$unit; $rate4_celi = $rate4_celi.$unit;

	/* clean all qdisc*/
	startcmd($tc_qd_del." root 2>/dev/null");

	/* add root qdisc */
	startcmd($tc_qd_add." root handle 50:0 htb default 20");

	/* add root class */
	startcmd($tc_class_add." parent 50:0 classid 50:1 htb rate ".$trate);

	/* add "50:10" class, and prio:1 */
	startcmd($tc_class_add." parent 50:1 classid 50:10 htb rate ".$rate1." ceil ".$rate1_celi." prio 1");

	/* add "50:20" class, and prio:2 */
	startcmd($tc_class_add." parent 50:1 classid 50:20 htb rate ".$rate2." ceil ".$rate2_celi." prio 2");

	/* add "50:30" class, and prio:3 */
	startcmd($tc_class_add." parent 50:1 classid 50:30 htb rate ".$rate3." ceil ".$rate3_celi." prio 3");

	/* add "50:40" class, and prio:4 */
	startcmd($tc_class_add." parent 50:1 classid 50:40 htb rate ".$rate4." ceil ".$rate4_celi." prio 4");

	/* add leaf qdisc */
	startcmd($tc_qd_add." parent 50:10 handle 5010: sfq perturb 10");
	startcmd($tc_qd_add." parent 50:20 handle 5020: sfq perturb 10");
	startcmd($tc_qd_add." parent 50:30 handle 5030: sfq perturb 10");
	startcmd($tc_qd_add." parent 50:40 handle 5040: sfq perturb 10");

	/* add filter, use fw when upstream */
	startcmd($tc_filter_add." handle 5010 fw classid 50:10");
	startcmd($tc_filter_add." handle 5020 fw classid 50:20");
	startcmd($tc_filter_add." handle 5030 fw classid 50:30");
	startcmd($tc_filter_add." handle 5040 fw classid 50:40");

	/* set mark, use iptables/mangle */
	startcmd($ipt_flush_cmd);
	foreach($rtbwcp."/rules/entry")
	{
		if (query("enable")=="1")
		{
			$bwcqd_name = query("bwcqd");
			$bwcqdp = XNODE_getpathbytarget("/bwcqd", "entry", "uid", $bwcqd_name, 0);
			if( $bwcqdp == "" ) { continue; }

			$entryp = $rtbwcp."/rules/entry:".$InDeX; 
			$ipt_add_cmd	= $ipt_add_prefix;
			$ipt_del_cmd	= $ipt_del_prefix;
			$startip = query("/ipv4/start");
			$endip = query("/ipv4/end");
			$int_start = ipv4hostid($startip, 0);
			$int_end = ipv4hostid($endip, 0);
			if($int_start > $int_end) { $iprange = $int_start - $int_end; }
			else { $iprange = $int_end - $int_start; }
			$portrange	 	= query("/port/range");
			$startport = query("/port/start");
			$endport = query("/port/end");
			if($startport > $endport) { $portrange = $startport - $endport; }
			else { $portrange = $endport - $startport; }
			$mark_cmd		= " -j MARK --set-mark 50".query($bwcqdp."/priority")."0";

			/* check iptype */
			if($iprange != "") {

				if ($iprange =="-1") {
					$mask = INF_getcurrmask(query("bwc_rule_inf"));
					$ipt_add_cmd = $ipt_add_cmd." -s ".query($entryp."/ipv4/start")."/".$mask;
					$ipt_del_cmd = $ipt_del_cmd." -s ".query($entryp."/ipv4/start")."/".$mask;

				} else if ($iprange =="0") {
				    $ipt_add_cmd = $ipt_add_cmd." -s ".query($entryp."/ipv4/start");
					$ipt_del_cmd = $ipt_del_cmd." -s ".query($entryp."/ipv4/start");

				} else {
					$ipt_add_cmd = $ipt_add_cmd." -m iprange --src-range ".query($entryp."/ipv4/start")."-".query($entryp."/ipv4/end");
					$ipt_del_cmd = $ipt_del_cmd." -m iprange --src-range ".query($entryp."/ipv4/start")."-".query($entryp."/ipv4/end");
				}

				/* check port type */
				if ($portrange != "") {
					porttype_handle($entryp, $ipt_add_cmd, $ipt_del_cmd, $porttype, $mark_cmd);
				} else {
					startcmd($ipt_add_cmd.$mark_cmd);
				}
			} else {

				/* check port type */
				if ($porttype != "") {
					porttype_handle($entryp, $ipt_add_cmd, $ipt_del_cmd, $porttype, $mark_cmd);
				} else {
					// ip and port both are empty, nothing need to do.
				}
			}
		}
	}
}

function bwc_bc_stop($rtbwcp, $name, $ifname)
{
	$tc_qd_del		= "tc qdisc del dev ".$ifname;
	$ipt_flush_cmd	= "iptables -t mangle -F PRE.BWC.".$name;

	/* clean all qdisc*/
	stopcmd($tc_qd_del." root 2>/dev/null");

	/* cleann all iptables/mangle/subchain rules */
	stopcmd($ipt_flush_cmd);
}

function bwc_tc_start($rtbwcp, $name, $ifname)
{
	$LANSTR="LAN-1";

	$tc_qd_add		= "tc qdisc add dev ".$ifname;
	$tc_qd_del		= "tc qdisc del dev ".$ifname;
	$tc_class_add	= "tc class add dev ".$ifname;
	$tc_class_del	= "tc class del dev ".$ifname;
	$tc_filter_del	= "tc filter del dev ".$ifname;
	$ipt_add_prefix	= "iptables -t mangle -A PRE.BWC.".$LANSTR;
	$def_class		= "40";

	$unit = "kbit";

	/* trate: total rate (bandwidth) */
	$trate = query($rtbwcp."/bandwidth");
	$trate = $trate.$unit;

	/* clean all qdisc*/
	startcmd($tc_qd_del." root>/dev/null");

	/* add root qdisc */
	startcmd($tc_qd_add." root handle 2: htb default ".$def_class);

	/* add root class */
	startcmd($tc_class_add." parent 2:0 classid 2:1 htb rate ".$trate);
	
	startcmd("");
	$tc_filter_add	= "tc filter add dev ".$ifname." prio ".$def_class;
	startcmd($tc_class_add." parent 2:1 classid 2:".$def_class." htb rate 1".$unit." ceil ".$trate);
	startcmd($tc_qd_add." parent 2:".$def_class." handle ".$def_class."0: sfq perturb 10");
	startcmd($tc_filter_add." parent 2: protocol all u32 match ip tos 0x00 0xe0 flowid 2:".$def_class);

	/* add class and filter for traffic from LAN PCs destinated to the router's LAN IP. */
	if( substr($name, 0, 3) == "LAN" )
	{
		$lanInfp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);
		$lanInet = query($lanInfp."/inet");
		$lanInetp= XNODE_getpathbytarget("/inet", "entry", "uid", $lanInet, 0);
		$lanAddr = query($lanInetp."/ipv4/ipaddr");

		startcmd("");
		$classid_base=2;
		$tc_filter_add	= "tc filter add dev ".$ifname." prio ".$classid_base;
		startcmd($tc_class_add." parent 2:0 classid 2:".$classid_base." htb rate 102400".$unit);
		startcmd($tc_qd_add." parent 2:".$classid_base." handle ".$classid_base."0: sfq perturb 10");
		startcmd($tc_filter_add." parent 2: protocol ip u32 match ip src ".$lanAddr."/1 flowid 2:".$classid_base);
	}

	$classid_base=10;
	$mark_base=10;
	foreach($rtbwcp."/rules/entry")
	{
		$tc_filter_add	= "tc filter add dev ".$ifname." prio ".$classid_base;
		if (query("enable")=="1")
		{
			$bwcf_name = query("bwcf");
			$bwcfp = XNODE_getpathbytarget("/bwc/bwcf", "entry", "uid", $bwcf_name, 0);
			if($bwcfp == "" ) { continue; }
			$bwcqd_name = query("bwcqd");
			$bwcqdp = XNODE_getpathbytarget("/bwc/bwcqd", "entry", "uid", $bwcqd_name, 0);
			if($bwcqdp == "" ) { continue; }

			$bandwidth = query($bwcqdp."/bandwidth");
			if($bandwidth == "" || $bandwidth == "0") { continue; }
			$rate = $bandwidth.$unit;
			
			$startip = query($bwcfp."/ipv4/start");
			$endip = query($bwcfp."/ipv4/end");
			$int_start = ipv4hostid($startip, 0);
			$int_end = ipv4hostid($endip, 0);
			
			if($int_start > $int_end)
			{
				$iprange = $int_start - $int_end + 1;
				$startip = query($bwcfp."/ipv4/end");
				$endip = query($bwcfp."/ipv4/start");
			}
			else
			{
				$iprange = $int_end - $int_start + 1;
			}

			startcmd("");
			if( $name == "WAN-1" )   /* Upload bandwidth control */
			{
				if(query($bwcqdp."/flag") == "MAXBD")
				{
					startcmd($tc_class_add." parent 2:1 classid 2:".$classid_base." htb rate 1".$unit." ceil ".$rate);
					startcmd($tc_qd_add." parent 2:".$classid_base." handle ".$classid_base."0: sfq perturb 10");
					startcmd($tc_filter_add." parent 2: protocol ip handle ".$mark_base." fw classid 2:".$classid_base);
					startcmd($ipt_add_prefix." -m iprange --src-range ".$startip."-".$endip." -j MARK --set-mark ".$mark_base);
					startcmd("echo ".$startip."-".$endip." ".$mark_base." 0 > /proc/fastnat/fortcmarksupport");
				}
				else if(query($bwcqdp."/flag") == "RSVBD")
				{
					startcmd($tc_class_add." parent 2:1 classid 2:".$classid_base." htb rate ".$rate." ceil ".$trate);
					startcmd($tc_qd_add." parent 2:".$classid_base." handle ".$classid_base."0: sfq perturb 10");
					startcmd($tc_filter_add." parent 2: protocol ip handle ".$mark_base." fw classid 2:".$classid_base);
					startcmd($ipt_add_prefix." -m iprange --src-range ".$startip."-".$endip." -j MARK --set-mark ".$mark_base);
					startcmd("echo ".$startip."-".$endip." ".$mark_base." 0 > /proc/fastnat/fortcmarksupport");
				}
				else
				{
				}
			}
			if( $name == "LAN-1" ) /* Download bandwidth control */
			{
				if(query($bwcqdp."/flag") == "MAXBD")
				{
					startcmd($tc_class_add." parent 2:1 classid 2:".$classid_base." htb rate 1".$unit." ceil ".$rate);
					startcmd($tc_qd_add." parent 2:".$classid_base." handle ".$classid_base."0: sfq perturb 10");
					startcmd($tc_filter_add." parent 2: protocol ip u32 match ip dst ".$startip."/".$iprange." flowid 2:".$classid_base);
				}
				else if(query($bwcqdp."/flag") == "RSVBD")
				{
					startcmd($tc_class_add." parent 2:1 classid 2:".$classid_base." htb rate ".$rate." ceil ".$trate);
					startcmd($tc_qd_add." parent 2:".$classid_base." handle ".$classid_base."0: sfq perturb 10");
					startcmd($tc_filter_add." parent 2: protocol ip u32 match ip dst ".$startip."/".$iprange." flowid 2:".$classid_base);
				}
				else
				{
					startcmd("echo Unknown Traffic Control Operation Mode...ERROR!!!");
				}
			}
		}
		$classid_base++;
		$mark_base++;
	}

}

function bwc_tc_stop($rtbwcp, $name, $ifname)
{
	$LANSTR="LAN-1";

	$tc_qd_del		= "tc qdisc del dev ".$ifname;
	$ipt_flush_cmd	= "iptables -t mangle -F PRE.BWC.".$LANSTR;

	/* clean all qdisc*/
	stopcmd($tc_qd_del." root>/dev/null");

	/* clean fastnat */
	stopcmd("echo 0 > /proc/fastnat/qos");
	stopcmd("echo > /proc/fastnat/fortcmarksupport");

	/* cleann all iptables/mangle/subchain rules */
	stopcmd($ipt_flush_cmd);
}

function aqc_tc_start($rtbwcp, $name, $ifname)
{
	$tc_qd_add		= "tc qdisc add dev ".$ifname;
	$tc_qd_del		= "tc qdisc del dev ".$ifname;
	$tc_class_add	= "tc class add dev ".$ifname;
	$tc_class_del	= "tc class del dev ".$ifname;
	$tc_filter_add	= "tc filter add dev ".$ifname;
	$tc_filter_del	= "tc filter del dev ".$ifname;
	$ipt_add_prefix	= "iptables -t mangle -A PST.BWC.".$name;
	$unit = "kbit";

	/* trate: total rate (bandwidth) */
	$trate = query($rtbwcp."/bandwidth");
	if($trate == 0 || $trate == "")
	{
		$trate = 102400;	/* 100Mbps */
	}

	/* Priority VO: Voice, VI: Video, BG: Background, BE: Best-Effort */
	/* ceil */
	$prio0_MAX=$trate * 90 / 100;	/* VO: Voice */
	$prio1_MAX=$trate * 90 / 100;	/* VI: Video */
	$prio2_MAX=$trate * 80 / 100;	/* BG: Background */
	$prio3_MAX=$trate * 80 / 100;	/* BE: Best-Effort */
	/* rate */
	$prio0_MIN=$trate * 40 / 100;	/* VO: Voice */
	$prio1_MIN=$trate * 45 / 100;	/* VI: Video */
	$prio2_MIN=$trate * 10 / 100;	/* BG: Background */
	$prio3_MIN=$trate * 5 / 100;	/* BE: Best-Effort */

	/* clean all qdisc*/
	startcmd($tc_qd_del." root 2>/dev/null");

	/* config tx queue length */
	startcmd("ip link set ".$ifname." txqueuelen 20 2>/dev/null");

	/* add root qdisc */
	startcmd($tc_qd_add." root handle 1: htb default 42");

	/* add root class */
	startcmd($tc_class_add." parent 1:0 classid 1:1 htb rate ".$trate.$unit);

	/* add leaf class */
	startcmd($tc_class_add." parent 1:1 classid 1:40 htb prio 0 rate ".$prio0_MIN.$unit." ceil ".$prio0_MAX.$unit." burst 0k cburst 0k");
	startcmd($tc_class_add." parent 1:1 classid 1:41 htb prio 1 rate ".$prio1_MIN.$unit." ceil ".$prio1_MAX.$unit." burst 0k cburst 0k");
	startcmd($tc_class_add." parent 1:1 classid 1:42 htb prio 2 rate ".$prio2_MIN.$unit." ceil ".$prio2_MAX.$unit." burst 0k cburst 0k");
	startcmd($tc_class_add." parent 1:1 classid 1:43 htb prio 3 rate ".$prio3_MIN.$unit." ceil ".$prio3_MAX.$unit." burst 0k cburst 0k");

	/* ADD CLASSIFICATION FILTER */
	/*startcmd($tc_filter_add." parent 1: protocol all prio 1 u32 match ip tos 0x00 0xE0 flowid 1:40");
	startcmd($tc_filter_add." parent 1: protocol all prio 1 u32 match ip tos 0x80 0xE0 flowid 1:41");
	startcmd($tc_filter_add." parent 1: protocol all prio 1 u32 match ip tos 0x40 0xE0 flowid 1:42");
	startcmd($tc_filter_add." parent 1: protocol all prio 1 u32 match ip tos 0x20 0xE0 flowid 1:43");*/

	/* auto qos classification */
	/* (Level 3,4) 340 : Voice, on-line games, 
	   (Level 5) 500 : Video, small packets,
	   (Level 6) 600 : Background, default,
	   (Level 7) 700 : Best-Effort, bad guys */

	startcmd($tc_filter_add." parent 1: protocol ip prio 10 handle 340 fw classid 1:40");
	startcmd($tc_filter_add." parent 1: protocol ip prio 20 handle 500 fw classid 1:41");
	startcmd($tc_filter_add." parent 1: protocol ip prio 40 handle 700 fw classid 1:43");

	startcmd($ipt_add_prefix." -m length --length 0:256 -j MARK --set-mark 500");
	startcmd($ipt_add_prefix." -m connautoqos --level 3 -j MARK --set-mark 340");
	startcmd($ipt_add_prefix." -m connautoqos --level 4 -j MARK --set-mark 340");
	startcmd($ipt_add_prefix." -m connautoqos --level 5 -j MARK --set-mark 500");
	startcmd($ipt_add_prefix." -m connautoqos --level 7 -j MARK --set-mark 700");
}

function aqc_tc_stop($rtbwcp, $name, $ifname)
{
	$tc_qd_del		= "tc qdisc del dev ".$ifname;
	$ipt_flush_cmd	= "iptables -t mangle -F PST.BWC.".$name;

	/* clean all qdisc*/
	stopcmd($tc_qd_del." root 2>/dev/null");

	/* clean fastnat */
	stopcmd("echo 0 > /proc/fastnat/qos");
	stopcmd("echo > /proc/fastnat/fortcmarksupport");

	/* cleann all iptables/mangle/subchain rules */
	stopcmd($ipt_flush_cmd);
}

function bwc_setup($name)
{
	$infp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);
	if ($infp=="" || $stsp=="")
	{
		SHELL_info($_GLOBALS["START"], "bwc_setup: (".$name.") no interface.");
		SHELL_info($_GLOBALS["STOP"],  "bwc_setup: (".$name.") no interface.");
		bwc_error("9");
		return;
	}
	$bwc_profile_name = query($infp."/bwc");
	if ($bwc_profile_name=="")
	{
		SHELL_info($_GLOBALS["START"], "bwc_setup: (".$name.") no bwc_profile_name, service stop.");
		SHELL_info($_GLOBALS["STOP"],  "bwc_setup: (".$name.") no bwc_profile_name, service stop.");
		bwc_error("8");
		return;
	}
	$bwcp = XNODE_getpathbytarget("/bwc", "entry", "uid", $bwc_profile_name, 0);
	if ($bwcp=="")
	{
		SHELL_info($_GLOBALS["START"], "bwc_setup: (".$name.") bwc node/profile not exist, service stop.");
		SHELL_info($_GLOBALS["STOP"],  "bwc_setup: (".$name.") bwc node/profile not exist, service stop.");
		bwc_error("8");
		return;
	}

	/* fill in runtime/inf:$InDeX/bwc */
	copy_bwc_entry($bwcp, $stsp);

	/* clean runtime node */
	stopcmd("sh /etc/scripts/delpathbytarget.sh "."/runtime "."inf "."uid ".$name." bwc");

	$ifname = PHYINF_getruntimeifname($name);

	$bwcp_flag = query($bwcp."/flag");
	if($bwcp_flag == "BC")
	{
		/* before everything start, trigger other services*/
		service_pre_trigger();	

		/* ex: $name=> WAN-1, $ifname=>eth2.2 */
		bwc_bc_start($stsp."/bwc", $name, $ifname);
		bwc_bc_stop($stsp."/bwc", $name, $ifname);

		/* everything is done, trigger other services*/
		service_post_trigger();	
    }
	else if($bwcp_flag == "TC")
	{
    	startcmd("");
    	startcmd("echo ".$name." Start Traffic Control system ...");
    	stopcmd("echo ".$name." Stop Traffic Control system ...");
		if( query($bwcp."/enable") == "1")
   	 	{
			/* ex: $name=> WAN-1, $ifname=>ppp0 */
			bwc_tc_start($bwcp, $name, $ifname);
			bwc_tc_stop($bwcp, $name, $ifname);
		}
		else
		{
			startcmd("echo ".$name." Traffic Control is disabled.\n"); 
			stopcmd("echo ".$name." Traffic Control is disabled.\n"); 
			return; 
    	}

	}
	else if($bwcp_flag == "AQC")
	{
    	startcmd("echo ".$name." Start Auto Qos Control system ...\n");
    	stopcmd("echo ".$name." Stop Auto Qos Control system ...\n");
		if( query($bwcp."/enable") == "1")
   	 	{
			/* ex: $name=> WAN-1, $ifname=>ppp0 */
			aqc_tc_start($bwcp, $name, $ifname);
			aqc_tc_stop($bwcp, $name, $ifname);
		}
		else
		{
			startcmd("echo ".$name." Auto Qos Control is disabled.\n"); 
			stopcmd("echo ".$name." Auto Qos Control is disabled.\n"); 
			return; 
    	}

	}
	else
	{
		SHELL_info($_GLOBALS["START"], "bwc_setup: (".$name.") bwc unknown flag, service stop.");
		SHELL_info($_GLOBALS["STOP"],  "bwc_setup: (".$name.") bwc unknown flag, service stop.");
		bwc_error("7");
		return;
	}
}

?>
