<?
include "/htdocs/phplib/xnode.php";

$p = XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.1", 0);
$en_wifi = query($p."/active");
if ($en_wifi==1)
	$wifitag = 'b';
else
	$wifitag = 's';

$wifib = XNODE_getpathbytarget("/wifi", "entry", "uid", "WIFI-1", 0);
$auth = query($wifib."/authtype");
$encry = query($wifib."/encrtype");
if ($encry=="NONE")
	$myencry = I18N("h","None");
else if ($encry=="WEP")
	$myencry = I18N("h","WEP");
else if ($auth=="WPAPSK")
	$myencry = I18N("h","WPA-PSK");
else if ($auth=="WPA2PSK"||$auth=="WPA+2PSK")
	$myencry = I18N("h","WPA2-PSK");
else if ($auth=="WPA")
	$myencry = I18N("h","WPA");
else if ($auth=="WPA2"||$auth=="WPA+2")
	$myencry = I18N("h","WPA2");

?>	<div id="content" class="maincolumn_wizard">
		<div class="wizard_icon internet">
			<p class="wizard_capsule"><a class="wizard_tag"><span id="inet_led"><s><?echo I18N("h","Internet");?></s></span></a></p>
			<a class="crystal" href="wiz_wan.php"><?echo I18N("h","Internet");?></a>
		</div>
		<div class="wizard_icon device">
			<p class="wizard_router_name"><?echo I18N("h","Router")." : ".query("/runtime/device/vendor").' '.query("/runtime/device/modelname");?></p>
			<a class="crystal" href="javascript:;" id="btn_router"><?echo I18N("h","Router");?></a>
		</div>
		<div class="wizard_icon wifi<?if ($en_wifi!=1) echo "_disable";?>">
			<p class="wizard_capsule"><a class="wizard_tag">
				<<?=$wifitag?>><?echo I18N("h","Wireless Security");?> :</<?=$wifitag?>>
				<img src='../pic/<?if ($encry=="NONE") echo "light_unlock.png"; else echo "light_lock.png";?>' align='absmiddle'> <?=$myencry?></a>
			</p>
			<a class="crystal" href="wiz_wlan.php" ><?echo I18N("h","Wi-Fi");?></a>
		</div>
		<p class="hr_emboss">hr</p>
		<p id="inet_icon" />
		<p class="<?if ($en_wifi==1) echo "wifi_connected"; else echo "wifi_disconnected";?>" />
		<div class="wizard_bottom">
			<div class="wizard_icon setting_icon share" style="margin-left:160px">
				<a class="crystal" href="share.php"><?echo I18N("h","Share");?></a>
				<p class="wizard_capsule"><a class="wizard_tag_nolight"><b><?echo I18N("h","Share");?></b></a></p>
			</div>
			<div class="wizard_icon setting_icon guest">
				<a class="crystal" href="adv_gzone.php"><?echo I18N("h","Guest Zone");?></a>
				<p class="wizard_capsule"><a class="wizard_tag_nolight"><b><?echo I18N("h","Guest Zone");?></b></a></p>
			</div>
		</div>
	</div>
	<!-- Information popup -->
	<div id="popup_router">
		<table width="100%" border="0" align="center" cellpadding="0" cellspacing="0" class="setup_form">
		<tr>
			<td style="padding:0; text-align:center; vertical-align:bottom;"><img src="../pic/popup_arrow.gif" width="14" height="11" /></td>
		</tr>
		<tr>
			<th class="rc_orange_hd"><a id="close_x" class="close" href="#"><?echo I18N("h","close");?></a><?echo I18N("h","Router Information");?></th>
		</tr>
		<tr>
			<td class="rc_orange_ft_lightbg"><div class="padding_aisle">
				<table border="0" cellpadding="0" cellspacing="0" class="status_report">
				<tr>
					<td width="29%" nowrap="nowrap" class="td_right" style="border:none"><strong><?echo I18N("h","Router Model");?> :</strong></td>
					<td width="71%"><?echo query("/runtime/device/modelname");?></td>
				</tr>
				<tr>
					<td nowrap="nowrap" class="td_right" style="border:none"><strong><?echo I18N("h","Firmware version");?> :</strong></td>
					<td><?echo query("/runtime/device/firmwareversion");?><a href="./tools_firmware.php" class="floatRight"><?echo I18N("h","Check update");?></a></td>
				</tr>
				<tr>
					<td nowrap="nowrap" class="td_right" style="border:none"><strong><?echo I18N("h","Admin password");?> :</strong></td>
					<td><?echo query("/device/account/entry:1/password");?><a href="./tools_admin.php" class="floatRight"><?echo I18N("h","Change password");?></a></td>
				</tr>
				</table>
			</div></td>
		</tr>
		</table>
	</div>
	<!-- Information popup -->
