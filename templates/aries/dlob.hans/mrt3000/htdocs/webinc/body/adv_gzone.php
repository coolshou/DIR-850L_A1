<?
include "/htdocs/phplib/xnode.php";

$p = XNODE_getpathbytarget("", "phyinf", "uid", "BAND24G-1.2", 0);
$wuid = query($p."/wifi");
$en_wifi = query($p."/active");

/* check 5G, if 2.4G GZone is disabled */
if ($en_wifi!=1&&$FEATURE_DUAL_BAND==1)
{
	$p = XNODE_getpathbytarget("", "phyinf", "uid", "BAND5G-1.2", 0);
	$wuid = query($p."/wifi");
	$en_wifi = query($p."/active");
}
if ($en_wifi==1)
{
	$gzone_disabled	= "display:none;";
	$gzone_enabled	= "display:block;";
}
else
{
	$gzone_disabled	= "display:block;";
	$gzone_enabled	= "display:none;";
}
?>	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Use another SSID and Key for your guests");?></h2>
		</div>
<!-- Begin of Guset Zone: STATUS -->
		<div id="gzone_status">
		<div id="gzone_disabled" class="rc_gradient_bd h_initial" style="<?=$gzone_disabled?>">
			<h6><?echo I18N("h",'The Guest Zone function is Disbled now.');?></h6>
		</div>
		<div id="gzone_enabled" class="rc_gradient_bd h_initial" style="<?=$gzone_enabled?>">
			<h6><?echo I18N("h",'Your guests can now use the wireless network.');?></h6>
			<p class="assistant"<?if ($FEATURE_DUAL_BAND!=1) echo ' style="display:none;"';?>><?echo I18N("h","2.4 GHz Frequency");?></p>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","SSID");?> :</b></td>
				<td width="674"><span id="st_ssid" class="maroon"></span></td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Key");?> :</b></td>
				<td width="674">
<!--					<input id="key" type="text" class="text_block" size="40" maxlength="63" /> 
					<?drawlabel("key");?><br />  -->
					<span id="st_key" class="maroon"></span>&nbsp;
					<span id="st_encry"></span>
				</td>
			</tr>
			</table>
			<div id="div_5G_status" style="display:none;">
			<p class="assistant"><?echo I18N("h","5 GHz Frequency");?></p>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","SSID");?> :</b></td>
				<td width="674"><span id="st_ssid_11a" class="maroon"></span></td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Key");?> :</b></td>
				<td width="674">
					<span id="st_key_11a" class="maroon"></span>&nbsp;
					<span id="st_encry_11a"></span>
				</td>
			</tr>
			</table>
			</div>
		</div>
		</div>
<!-- End of Guset Zone: STATUS -->
<!-- Begin of Guset Zone: CONFIG -->
		<div id="gzone_config" style="display:none;">
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","This SSID and key will be used when other devices in your home need to share the Internet connection.");?></h6>
			<p class="assistant"<?if ($FEATURE_DUAL_BAND!=1) echo ' style="display:none;"';?>><?echo I18N("h","2.4 GHz Frequency");?></p>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="180" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Enable Guest Zone");?> :</b></td>
				<td width="674">
					<input id="en_gzone" type="checkbox" onClick="PAGE.OnClickEnGzone('');" />
				</td>
			</tr>
			<tr>
				<td width="180" nowrap="nowrap" class="td_right"><b><?echo I18N("h","SSID");?> :</b></td>
				<td width="674">
					<input id="ssid" type="text" class="text_block" size="40" />
					<?drawlabel("ssid");?>
				</td>
			</tr>
			<tr>
				<td width="180" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Key");?> :</b></td>
				<td width="674">
					<input id="key" type="text" class="text_block" size="40" maxlength="63" />
					<?drawlabel("key");?><br />
					<span class="ashy"><?echo i18n('<a href="javascript:PAGE.GenKey(\'\');">Generate a random key</a> or enter a security key with 8-63 characters.');?></span>
				</td>
			</tr>
			</table>
			<div id="div_5G_config" style="display:none;">
			<p class="assistant"><?echo I18N("h","5 GHz Frequency");?></p>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Enable Guest Zone");?> :</b></td>
				<td width="674">
					<input id="en_gzone_11a" type="checkbox" onClick="PAGE.OnClickEnGzone('_11a');" />
				</td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","SSID");?> :</b></td>
				<td width="674">
					<input id="ssid_11a" type="text" class="text_block" size="40" />
					<?drawlabel("ssid_11a");?>
				</td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Key");?> :</b></td>
				<td width="674">
					<input id="key_11a" type="text" class="text_block" size="40" maxlength="63" />
					<?drawlabel("key_11a");?><br />
					<span class="ashy"><?echo i18n('<a href="javascript:PAGE.GenKey(\'_11a\');">Generate a random key</a> or enter a security key with 8-63 characters.');?></span>
				</td>
			</tr>
			</table>
			</div>
		</div>
		</div>
<!-- End of Guset Zone: CONFIG -->
		<div class="rc_gradient_submit">
			<button id="b_exit" type="button" class="submitBtn floatLeft" onclick="self.location.href='./home.php';">
				<b><?echo I18N("h","Cancel");?></b>
			</button>
			<button id="b_back" type="button" class="submitBtn floatLeft" onclick="PAGE.OnClickPre();">
				<b><?echo I18N("h","Back");?></b>
			</button>
			<button id="b_next" type="button" class="submitBtn floatRight" onclick="PAGE.OnClickNext();">
				<b><span id="btname"></span></b>
			</button>
			<button id="b_send" type="button" class="submitBtn floatRight" onclick="self.location.href='./home.php';">
				<b><?echo I18N("h","OK");?></b>
			</button><i></i>
		</div>
	</div>
	</form>
