	<? include "/htdocs/webinc/body/draw_elements.php"; ?>
    <?
	
include "/htdocs/phplib/xnode.php";

function get_runtime_eth_path($uid)
{
	$p = XNODE_getpathbytarget("", "inf", "uid", $uid, 0);
	if($p == "") return $p;

	return XNODE_getpathbytarget("/runtime", "phyinf", "uid", query($p."/phyinf"));
}
function get_runtime_wifi_path($uid)
{
	$p = XNODE_getpathbytarget("", "phyinf", "wifi", $uid);
	if($p == "") return $p;
	return XNODE_getpathbytarget("/runtime", "phyinf", "uid", query($p."/uid"));
}

$tx = "/stats/tx/packets";
$rx = "/stats/rx/packets";

$tx_drop = "/stats/tx/drop";
$rx_drop = "/stats/rx/drop";
$tx_error = "/stats/tx/error";
$rx_error = "/stats/rx/error";
$collision = "/stats/tx/collision";
/* we reset the counters and get them immediately here, it may has the synchronization issue. */
/* Do RESET count */
if ($_POST["act"]!="")
{
	$p = get_runtime_eth_path("WAN-1");	
	if ($p != "")
	{
		$cnt=query($p.$tx);
		$cnt=$cnt+query($p."/stats/last_tx_pkt_cnt");
		set($p."/stats/last_tx_pkt_cnt",$cnt);
		
		$cnt=query($p.$rx);
		$cnt=$cnt+query($p."/stats/last_rx_pkt_cnt");
		set($p."/stats/last_rx_pkt_cnt",$cnt);

		$cnt=query($p.$tx_drop);
		$cnt=$cnt+query($p."/stats/last_tx_drop_cnt");
		set($p."/stats/last_tx_drop_cnt",$cnt);

		$cnt=query($p.$rx_drop);
		$cnt=$cnt+query($p."/stats/last_rx_drop_cnt");
		set($p."/stats/last_rx_drop_cnt",$cnt);

		$cnt=query($p.$tx_error);
		$cnt=$cnt+query($p."/stats/last_tx_error_cnt");
		set($p."/stats/last_tx_error_cnt",$cnt);
		
		$cnt=query($p.$rx_error);
		$cnt=$cnt+query($p."/stats/last_rx_error_cnt");
		set($p."/stats/last_rx_error_cnt",$cnt);

		$cnt=query($p.$collision);
		$cnt=$cnt+query($p."/stats/last_collision_cnt");
		set($p."/stats/last_collision_cnt",$cnt);

		setattr($p.$tx_drop,"get","scut -p ".query($p."/name").": -f 12 /proc/net/dev -b ".query($p."/stats/last_tx_drop_cnt")." "); 
		setattr($p.$rx_drop,"get","scut -p ".query($p."/name").": -f 4 /proc/net/dev -b ".query($p."/stats/last_rx_drop_cnt")." "); 
		
		setattr($p.$tx_error,"get","scut -p ".query($p."/name").": -f 11 /proc/net/dev -b ".query($p."/stats/last_tx_error_cnt")." "); 
		setattr($p.$rx_error,"get","scut -p ".query($p."/name").": -f 3 /proc/net/dev -b ".query($p."/stats/last_rx_error_cnt")." "); 
		
		setattr($p.$collision,"get","scut -p ".query($p."/name").": -f 14 /proc/net/dev -b ".query($p."/stats/last_collision_cnt")." ");
																
		setattr($p."/stats/rx/packets","get","scut -p ".query($p."/name").": -f 2 /proc/net/dev -b ".query($p."/stats/last_rx_pkt_cnt")." "); 
		setattr($p."/stats/tx/packets","get","scut -p ".query($p."/name").": -f 10 /proc/net/dev -b ".query($p."/stats/last_tx_pkt_cnt")." "); 
		set($p."/stats/reset", "dummy");
	}
	$p = get_runtime_eth_path("LAN-1");	
	if ($p != "") 
	{
		$cnt=query($p.$tx);
		$cnt=$cnt+query($p."/stats/last_tx_pkt_cnt");
		set($p."/stats/last_tx_pkt_cnt",$cnt);
		
		$cnt=query($p.$rx);
		$cnt=$cnt+query($p."/stats/last_rx_pkt_cnt");
		set($p."/stats/last_rx_pkt_cnt",$cnt);
		
		$cnt=query($p.$tx_drop);
		$cnt=$cnt+query($p."/stats/last_tx_drop_cnt");
		set($p."/stats/last_tx_drop_cnt",$cnt);

		$cnt=query($p.$rx_drop);
		$cnt=$cnt+query($p."/stats/last_rx_drop_cnt");
		set($p."/stats/last_rx_drop_cnt",$cnt);

		$cnt=query($p.$tx_error);
		$cnt=$cnt+query($p."/stats/last_tx_error_cnt");
		set($p."/stats/last_tx_error_cnt",$cnt);
		
		$cnt=query($p.$rx_error);
		$cnt=$cnt+query($p."/stats/last_rx_error_cnt");
		set($p."/stats/last_rx_error_cnt",$cnt);

		$cnt=query($p.$collision);
		$cnt=$cnt+query($p."/stats/last_collision_cnt");
		set($p."/stats/last_collision_cnt",$cnt);

		setattr($p.$tx_drop,"get","scut -p ".query($p."/name").": -f 12 /proc/net/dev -b ".query($p."/stats/last_tx_drop_cnt")." "); 
		setattr($p.$rx_drop,"get","scut -p ".query($p."/name").": -f 4 /proc/net/dev -b ".query($p."/stats/last_rx_drop_cnt")." "); 
		
		setattr($p.$tx_error,"get","scut -p ".query($p."/name").": -f 11 /proc/net/dev -b ".query($p."/stats/last_tx_error_cnt")." "); 
		setattr($p.$rx_error,"get","scut -p ".query($p."/name").": -f 3 /proc/net/dev -b ".query($p."/stats/last_rx_error_cnt")." "); 
		
		setattr($p.$collision,"get","scut -p ".query($p."/name").": -f 14 /proc/net/dev -b ".query($p."/stats/last_collision_cnt")." ");		
		setattr($p."/stats/rx/packets","get","scut -p ".query($p."/name").": -f 2 /proc/net/dev -b ".query($p."/stats/last_rx_pkt_cnt")." "); 
		setattr($p."/stats/tx/packets","get","scut -p ".query($p."/name").": -f 10 /proc/net/dev -b ".query($p."/stats/last_tx_pkt_cnt")." "); 
		set($p."/stats/reset", "dummy");
	}
	$p = get_runtime_wifi_path("WIFI-1");	
	if ($p != "") 
	{
		$cnt=query($p.$tx);
		$cnt=$cnt+query($p."/stats/last_tx_pkt_cnt");
		set($p."/stats/last_tx_pkt_cnt",$cnt);
		
		$cnt=query($p.$rx);
		$cnt=$cnt+query($p."/stats/last_rx_pkt_cnt");
		set($p."/stats/last_rx_pkt_cnt",$cnt);
		
				$cnt=query($p.$tx_drop);
		$cnt=$cnt+query($p."/stats/last_tx_drop_cnt");
		set($p."/stats/last_tx_drop_cnt",$cnt);

		$cnt=query($p.$rx_drop);
		$cnt=$cnt+query($p."/stats/last_rx_drop_cnt");
		set($p."/stats/last_rx_drop_cnt",$cnt);

		$cnt=query($p.$tx_error);
		$cnt=$cnt+query($p."/stats/last_tx_error_cnt");
		set($p."/stats/last_tx_error_cnt",$cnt);
		
		$cnt=query($p.$rx_error);
		$cnt=$cnt+query($p."/stats/last_rx_error_cnt");
		set($p."/stats/last_rx_error_cnt",$cnt);

		$cnt=query($p.$collision);
		$cnt=$cnt+query($p."/stats/last_collision_cnt");
		set($p."/stats/last_collision_cnt",$cnt);

		setattr($p.$tx_drop,"get","scut -p ".query($p."/name").": -f 12 /proc/net/dev -b ".query($p."/stats/last_tx_drop_cnt")." "); 
		setattr($p.$rx_drop,"get","scut -p ".query($p."/name").": -f 4 /proc/net/dev -b ".query($p."/stats/last_rx_drop_cnt")." "); 
		
		setattr($p.$tx_error,"get","scut -p ".query($p."/name").": -f 11 /proc/net/dev -b ".query($p."/stats/last_tx_error_cnt")." "); 
		setattr($p.$rx_error,"get","scut -p ".query($p."/name").": -f 3 /proc/net/dev -b ".query($p."/stats/last_rx_error_cnt")." "); 
		
		setattr($p.$collision,"get","scut -p ".query($p."/name").": -f 14 /proc/net/dev -b ".query($p."/stats/last_collision_cnt")." ");
		
		setattr($p."/stats/rx/packets","get","scut -p ".query($p."/name").": -f 2 /proc/net/dev -b ".query($p."/stats/last_rx_pkt_cnt")." "); 
		setattr($p."/stats/tx/packets","get","scut -p ".query($p."/name").": -f 10 /proc/net/dev -b ".query($p."/stats/last_tx_pkt_cnt")." "); 
		set($p."/stats/reset", "dummy");
	}
	$p = get_runtime_wifi_path("WIFI-2");	
	if ($p != "") 
	{
		$cnt=query($p.$tx);
		$cnt=$cnt+query($p."/stats/last_tx_pkt_cnt");
		set($p."/stats/last_tx_pkt_cnt",$cnt);
		
		$cnt=query($p.$rx);
		$cnt=$cnt+query($p."/stats/last_rx_pkt_cnt");
		set($p."/stats/last_rx_pkt_cnt",$cnt);
		
		$cnt=query($p.$tx_drop);
		$cnt=$cnt+query($p."/stats/last_tx_drop_cnt");
		set($p."/stats/last_tx_drop_cnt",$cnt);

		$cnt=query($p.$rx_drop);
		$cnt=$cnt+query($p."/stats/last_rx_drop_cnt");
		set($p."/stats/last_rx_drop_cnt",$cnt);

		$cnt=query($p.$tx_error);
		$cnt=$cnt+query($p."/stats/last_tx_error_cnt");
		set($p."/stats/last_tx_error_cnt",$cnt);
		
		$cnt=query($p.$rx_error);
		$cnt=$cnt+query($p."/stats/last_rx_error_cnt");
		set($p."/stats/last_rx_error_cnt",$cnt);

		$cnt=query($p.$collision);
		$cnt=$cnt+query($p."/stats/last_collision_cnt");
		set($p."/stats/last_collision_cnt",$cnt);

		setattr($p.$tx_drop,"get","scut -p ".query($p."/name").": -f 12 /proc/net/dev -b ".query($p."/stats/last_tx_drop_cnt")." "); 
		setattr($p.$rx_drop,"get","scut -p ".query($p."/name").": -f 4 /proc/net/dev -b ".query($p."/stats/last_rx_drop_cnt")." "); 
		
		setattr($p.$tx_error,"get","scut -p ".query($p."/name").": -f 11 /proc/net/dev -b ".query($p."/stats/last_tx_error_cnt")." "); 
		setattr($p.$rx_error,"get","scut -p ".query($p."/name").": -f 3 /proc/net/dev -b ".query($p."/stats/last_rx_error_cnt")." "); 
		
		setattr($p.$collision,"get","scut -p ".query($p."/name").": -f 14 /proc/net/dev -b ".query($p."/stats/last_collision_cnt")." ");
		
		setattr($p."/stats/rx/packets","get","scut -p ".query($p."/name").": -f 2 /proc/net/dev -b ".query($p."/stats/last_rx_pkt_cnt")." "); 
		setattr($p."/stats/tx/packets","get","scut -p ".query($p."/name").": -f 10 /proc/net/dev -b ".query($p."/stats/last_tx_pkt_cnt")." "); 
		set($p."/stats/reset", "dummy");
	}
}

$p = get_runtime_eth_path("WAN-1");
if ($p == "")
{	
	$wan1_tx = I18N("h","N/A");						$wan1_rx = I18N("h","N/A"); 
	$wan1_txdrop=I18N("h","N/A");					$wan1_rxdrop=I18N("h","N/A");
	$wan1_collision=I18N("h","N/A"); 				$wan1_error=I18N("h","N/A");
}
else		 
{
	$wan1_tx = query($p.$tx);	$wan1_rx = query($p.$rx);
	$wan1_txdrop=query($p.$tx_drop);	$wan1_rxdrop=query($p.$rx_drop);
	$wan1_collision=query($p.$collision);	$tmp_txerror=query($p.$tx_error);$tmp_rxerror=query($p.$rx_error);
	$wan1_error=$tmp_txerror+$tmp_rxerror;
}


$p = get_runtime_eth_path("LAN-1");
if ($p == "")
{
	$lan1_tx = I18N("h","N/A");						$lan1_rx = I18N("h","N/A");
	$lan1_txdrop=I18N("h","N/A");					$lan1_rxdrop=I18N("h","N/A");
	$lan1_collision=I18N("h","N/A"); 				$lan1_error=I18N("h","N/A");
}
else		 
{
	$lan1_tx = query($p.$tx);	$lan1_rx = query($p.$rx);
	$lan1_txdrop=query($p.$tx_drop);	$lan1_rxdrop=query($p.$rx_drop);
	$lan1_collision=query($p.$collision);	$tmp_txerror=query($p.$tx_error);$tmp_rxerror=query($p.$rx_error);
	$lan1_error=$tmp_txerror+$tmp_rxerror;
}

$p = get_runtime_wifi_path("WIFI-1");
if ($p == "")
{
	$wifi_tx = I18N("h","N/A");						$wifi_rx = I18N("h","N/A");
	$wifi_txdrop=I18N("h","N/A");					$wifi_rxdrop=I18N("h","N/A");
	$wifi_collision=I18N("h","N/A"); 				$wifi_error=I18N("h","N/A");
}
else		 
{
	$wifi_tx = query($p.$tx);	$wifi_rx = query($p.$rx);
	$wifi_txdrop=query($p.$tx_drop);	$wifi_rxdrop=query($p.$rx_drop);
	$wifi_collision=query($p.$collision);	$tmp_txerror=query($p.$tx_error);$tmp_rxerror=query($p.$rx_error);
	$wifi_error=$tmp_txerror+$tmp_rxerror;
}

$p = get_runtime_wifi_path("WIFI-2");
if ($p == "")
{
	$wifi_tx_aband = I18N("h","N/A");						$wifi_rx_aband = I18N("h","N/A");
	$wifi_txdrop_aband=I18N("h","N/A");					$wifi_rxdrop_aband=I18N("h","N/A");
	$wifi_collision_aband=I18N("h","N/A"); 				$wifi_error_aband=I18N("h","N/A");
}
else		 
{
	$wifi_tx_aband = query($p.$tx);	$wifi_rx_aband = query($p.$rx);
	$wifi_txdrop_aband=query($p.$tx_drop);	$wifi_rxdrop_aband=query($p.$rx_drop);
	$wifi_collision_aband=query($p.$collision);	$tmp_txerror=query($p.$tx_error);$tmp_rxerror=query($p.$rx_error);
	$wifi_error_aband=$tmp_txerror+$tmp_rxerror;
}
?>
	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<ul class="navi">
			<li class="start"><a href="./advanced.php"><span><?echo I18N("h","Advanced Settings");?></span></a></li>
			<li><?echo I18N("h","Statistics");?></li>
			<i></i>
		</ul>
		<a href="#" class="icon get_info" id="btn_info" title="<?echo I18N("h","Help");?>"><?echo I18N("h","More information");?></a>
		<table width="100%" border="0" cellspacing="0" cellpadding="0" class="setup_form">
		<tr>
			<th colspan="4" class="rc_gray5_hd"><h2><?echo I18N("h","Traffic Statistics");?></h2></th>
		</tr>
		<tr>
			<td colspan="4" class="gray_bg border_2side"><cite><?echo I18N("h","Traffic Statistics displays Receive and Transmit packets passing through the device.");?></cite></td>
		</tr>
        
        <tr>
        	<td colspan="4" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","LAN Statistics");?></p></td>
        </tr>
        <tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Sent");?> : </td>
            <td width="25%" class="gray_bg border_right"><?=$lan1_tx?></td>
            <td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Received");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$lan1_rx?></td>
		</tr>
        <tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","TX Packets Dropped");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$lan1_txdrop?></td>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RX Packets Dropped");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$lan1_rxdrop?></td>
		</tr>

		<tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Collisions");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$lan1_collision?></td>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Errors");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$lan1_error?></td>
		</tr>
        <tr>
        	<td colspan="4" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","WAN Statistics");?></p></td>
        </tr>
        <tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Sent");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wan1_tx?></td>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Received");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wan1_rx?></td>
		</tr>
		<tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","TX Packets Dropped");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wan1_txdrop?></td>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RX Packets Dropped");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wan1_rxdrop?></td>
		</tr>

		<tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Collisions");?> : </td>
            <td width="25%" class="gray_bg border_right"><?=$wan1_collision?></td>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Errors");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wan1_error?></td>
		</tr>
        <tr>
        	<td colspan="4" class="gray_bg border_2side"><p class="subitem"><?echo I18N("h","WIRELESS Statistics");?></p></td>
        </tr>
        <tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Sent");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wifi_tx?></td>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Received");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wifi_rx?></td>
		</tr>
		<tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","TX Packets Dropped");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wifi_txdrop?></td>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","RX Packets Dropped");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wifi_rxdrop?></td>
		</tr>

		<tr>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Collisions");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wifi_collision?></td>
			<td width="25%" nowrap="nowrap" class="td_right gray_bg border_left"><?echo I18N("h","Errors");?> : </td>
			<td width="25%" class="gray_bg border_right"><?=$wifi_error?></td>
		</tr>
        <tr>
			<td colspan="4" class="rc_gray5_ft">
				<button value="submit" class="submitBtn floatRight" onclick="(function(){self.location='<?=$TEMP_MYNAME?>.php?r='+COMM_RandomStr(5);})();"><b><?echo I18N("h","Refresh");?></b></button>
				<button value="submit" class="submitBtn floatLeft" onclick="location.href='./advanced.php';"><b><?echo I18N("h","Cancel");?></b></button>
			</td>
		</tr>
		</table>
	</div>
	</form>
