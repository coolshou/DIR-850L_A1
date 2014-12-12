	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
		<div class="rc_gradient_hd">
			<h2><span id="banner"></span></h2>
		</div>
<!-- Begin of WAN Wizard: STATUS -->
		<div id="wan_status" class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","You are now connected to the Internet.");?>
				<span id="st_banner"><?echo I18N("h",'If you want to change your Internet conneciton setting, please click on "Next".');?></span>
			</h6>
			<div id="st_ppp" style="display:none;">
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","User name");?> :</b></td>
					<td width="674"><span id="st_ppp_user" class="maroon"></span> <span id="st_ppp_type"></span></td>
				</tr>
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Password");?> :</b></td>
					<td width="674"><span id="st_ppp_pwd" class="maroon"></span></td>
				</tr>
				</table>
			</div>
			<div id="st_static" style="display:none;">
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","IP address");?> :</b></td>
					<td width="674"><span id="st_static_ipaddr" class="maroon"></span></td>
				</tr>
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Subnet mask");?> :</b></td>
					<td width="674"><span id="st_static_mask" class="maroon"></span></td>
				</tr>
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Gateway");?> :</b></td>
					<td width="674"><span id="st_static_gw" class="maroon"></span></td>
				</tr>
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","Primary DNS");?> :</b></td>
					<td width="674"><span id="st_static_dns" class="maroon"></span></td>
				</tr>
				</table>
			</div>
			<div id="st_dhcp">
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><b><?echo I18N("h","MAC address");?> :</b></td>
					<td width="674"><span id="st_dhcp_mac" class="maroon"></span></td>
				</tr>
				</table>
			</div>
		</div>
<!-- End of WAN Wizard: STATUS -->
<!-- Begin of WAN Wizard: CONFIG -->
		<div id="wan_config" class="rc_gradient_bd h_initial" style="display:none;">
			<h6><?echo I18N("h",'Your router is not connected to the Internet, please choose the options below.');?></h6>
			<div class="gradient_form_content">
				<p>
					<input type="radio" name="connect_mode" id="en_pppoe" onchange="PAGE.OnChangeConnMode();" />
					<label for="en_pppoe">
						<?echo I18N("h","I have Internet dial up user name and password provided by Internet Service Provider");?>
						(<?echo I18N("h","PPPoE");?>).
					</label>
				</p>
				<p>
					<input type="radio" name="connect_mode" id="en_static" onchange="PAGE.OnChangeConnMode();" />
					<label for="en_static">
						<?echo I18N("h","I have a static  IP address for Internet connection provided by Internet Service Provider.");?>
					</label>
				</p>
				<p>
					<input type="radio" name="connect_mode" id="en_dhcp" onchange="PAGE.OnChangeConnMode();" />
					<label for="en_dhcp">
						<?echo I18N("h","My Internet is connected automatically, I don't need to dial up or set up a IP address.");?>
						(<?echo I18N("h","DHCP");?>).
					</label>
				</p>
				<p>
					<input type="radio" name="connect_mode" id="others" onchange="PAGE.OnChangeConnMode();" />
					<label for="others"><?echo I18N("h","Others (go to Advanced Internet Settings).");?></label>
				</p>
			</div>
		</div>
<!-- End of WAN Wizard: CONFIG -->
<!-- Begin of WAN Wizard: CONFIG PPPoE -->
		<div id="wan_config_pppoe" class="rc_gradient_bd h_initial" style="display:none;">
			<h6><?echo I18N("h","Please enter user name and password to login to your Internet Service Provider (ISP) account.");?></h6>
			<div class="gradient_form_content">
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","User name");?> :</strong></td>
					<td width="674">
						<input type="text" class="text_block" id="ppp_user" size="30" />
						<?drawlabel("ppp_user");?>
					</td>
				</tr>
				<tr>
					<td nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Password");?> :</strong></td>
					<td>
						<input type="password" class="text_block" id="ppp_pwd" size="30" />
						<?drawlabel("ppp_pwd");?>
					</td>
				</tr>
				</table>
				<div style="padding-left:15px; font-size:15px">
					<input type="checkbox" id="pppoe_fixedip" onclick="PAGE.OnClickFixedIP();" />
					<label for="pppoe_fixedip"><?echo I18N("h","My ISP also gave me a static IP (ex: 200.137.10.194)");?></label>
				</div>
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","IP address");?> :</strong></td>
					<td width="674">
						<?drawinputipaddr("pppoe_ipaddr");?>
						<?drawlabel("pppoe_ipaddr");?>
					</td>
				</tr>
				<tr>
					<td nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Primary DNS");?> :</strong></td>
					<td>
						<?drawinputipaddr("pppoe_dns");?>
						<?drawlabel("pppoe_dns");?>
					</td>
				</tr>
				</table>
			</div>
		</div>
<!-- End of WAN Wizard: CONFIG PPPoE -->
<!-- Begin of WAN Wizard: CONFIG STATIC IP -->
		<div id="wan_config_static" class="rc_gradient_bd h_initial" style="display:none;">
			<h6><?echo I18N("h","Please enter IP address provided by your Internet Server Provider (ISP).");?></h6>
			<div class="gradient_form_content">
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","IP address");?> :</strong></td>
					<td width="674">
						<?drawinputipaddr("static_ipaddr");?>
						<?drawlabel("static_ipaddr");?>
					</td>
				</tr>
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Subnet mask");?> :</strong></td>
					<td width="674">
						<?drawinputipaddr("static_mask");?>
						<?drawlabel("static_mask");?>
					</td>
				</tr>
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Gateway");?> :</strong></td>
					<td width="674">
						<?drawinputipaddr("static_gw");?>
						<?drawlabel("static_gw");?>
					</td>
				</tr>
				<tr>
					<td nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Primary DNS");?> :</strong></td>
					<td>
						<?drawinputipaddr("static_dns");?>
						<?drawlabel("static_dns");?>
					</td>
				</tr>
				</table>
			</div>
		</div>
<!-- End of WAN Wizard: CONFIG STATIC IP -->
<!-- Begin of WAN Wizard: CONFIG DHCP -->
		<div id="wan_config_dhcp" class="rc_gradient_bd h_initial" style="display:none;">
			<h6><?echo I18N("h","Some ISP may only allow one specific device in your home connecting to the internet.").' '.
				I18N("h","Clone MAC address allows your ISP to recognize your router.");?></h6>
			<div class="gradient_form_content">
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
				<tr>
					<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","MAC address");?> :</strong></td>
					<td width="674">
						<?drawinputmac("macclone");?> (<?echo I18N("h","optional");?>)
						<?drawlabel("macclone");?><br />
						<button id="button" title="<?echo I18N("h","You can use this button to automatically copy the MAC address to your device.");?>"
							onclick="PAGE.OnClickMacClone();"><?echo I18N("h","Clone MAC Address");?></button>
					</td>
				</tr>
				</table>
			</div>
		</div>
<!-- End of WAN Wizard: CONFIG DHCP -->
<!-- End of WAN Wizard: CONFIG -->
		<div class="rc_gradient_submit">
			<button id="b_exit" type="button" class="submitBtn floatLeft" onclick="self.location.href='./home.php';">
				<b><?echo I18N("h","Cancel");?></b>
			</button>
			<button id="b_back" type="button" class="submitBtn floatLeft" onclick="PAGE.OnClickPre();" style="display:none;">
				<b><?echo I18N("h","Back");?></b>
			</button>
			<button id="b_next" type="button" class="submitBtn floatRight" onclick="PAGE.OnClickNext();">
				<b><span id="btname"></span></b>
			</button>
			<button id="b_send" type="button" class="submitBtn floatRight" onclick="self.location.href='./home.php';" style="display:none;">
				<b><?echo I18N("h","OK");?></b>
			</button><i></i>
		</div>
	</div>
	</form>
