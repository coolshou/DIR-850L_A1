	<form id="mainform" onsubmit="return false;">
	<div id="content" class="maincolumn">
<!-- INITIAL: Start of Introduction -->
		<div id="init_intro">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Welcome to the setup page of your miiiCasa $1 router", query("/runtime/device/modelname"));?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","Thank you for purchasing this miiiCasa $1 router.", query("/runtime/device/modelname"));?>
				<?echo I18N("h","These setup pages will guide you through the setup procedure, so that you can start to experience miiiCasa");?>:</h6>
			<div class="gradient_form_content shrink">
				<div class="rc_gradient_ft4">
						<ul class="setup_welcome">
							<li><?echo I18N("h","Set up your router's Internet connection");?></li>
							<li class="punch">1. <?echo I18N("h","Create a password for your router");?></li>
							<li class="punch">2. <?echo I18N("h","Connect your router to the Internet");?></li>
							<li class="punch">3. <?echo I18N("h","Create your router's Wi-Fi SSID (the name of your Wireless network) and password");?></li>
							<li><?echo I18N("h","Experience miiiCasa!");?></li>
							<li class="punch"><?echo I18N("h","Plug in your USB storage and start enjoying miiiCasa.");?></li>
						</ul>
				</div>
			</div>
		</div>		
        <input type="hidden" id="pc_timezone_offset" value="" />
        <input type="hidden" id="pc_date" value="" />
        <input type="hidden" id="pc_time" value="" />      
		</div>
<!-- INITIAL: End of Introduction -->
<!-- INITIAL: Start of Password -->
		<div id="init_passwd" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Step 1").": ".I18N("h","Create a password for your router's setup system");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","Please create your router login password.");?>
				<?echo I18N("h","This is a very important security precaution, so please do it now.");?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Password");?> :</strong></td>
				<td width="674">
					<input type="password" class="text_block" id="usr_pwd" size="20" maxlength="15" />
					<?drawlabel("usr_pwd");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Verify Password");?> :</strong></td>
				<td><input type="password" class="text_block" id="usr_pwd2" size="20" maxlength="15" /></td>
			</tr>
			</table>
		</div>
		</div>
<!-- INITIAL: End of Password -->
<!-- INITIAL: Start of WAN Detecting -->
		<div id="init_wandetect" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Step 2").": ".I18N("h","Auto-connecting your router to the Internet");?></h2>
		</div>
		<div class="rc_gradient_ft" onclick="PAGE.OnClickNext();" style="cursor:pointer;">
			<h6><?echo I18N("h","Please wait while the router is auto-detecting your Internet connection.");?></h6>
			<div class="rc_gradient_ft2">
				<div class="rc_gradient_ft3"><br /><br /><br />
					&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
					<?echo I18N("h","Detecting your Internet connection type");?>
				</div>     
			</div>
		</div>
		</div>
<!-- INITIAL: End of WAN Detecting -->
<!-- INITIAL: Start of WANTYPE -->
		<div id="init_wantype" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Select the type of connection you need to establish.");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","Please choose one of the options below");?> :</h6>
			<div class="gradient_form_content">
				<p>
					<input type="radio" name="connect_mode" id="en_dhcp" value="dhcp" onclick="PAGE.OnClickWANType();" />
					<label for="en_dhcp"><b><?echo I18N("h","DHCP Connection")." (".I18N("h","Dynamic IP Address");?>)</b></label>
					<span class="ashy"><?echo I18N("h","If your Internet Service Provider automatically provides you with an IP Address.");?></span>
				</p>
				<p>
					<input type="radio" name="connect_mode" id="en_pppoe" value="pppoe" onclick="PAGE.OnClickWANType();" />
					<label for="en_pppoe"><b><?echo I18N("h","PPPoE Connection")." (".I18N("h","Username")." / ".I18N("h","Password");?>)</b></label>
					<span class="ashy"><?echo I18N("h","If your Internet connection requires a username and password.");?></span>
				</p>
				<p>
					<input type="radio" name="connect_mode" id="en_pptp" value="pptp" onclick="PAGE.OnClickWANType();" />
					<label for="en_pptp"><b><?echo I18N("h","PPTP Connection")." (".I18N("h","Username")." / ".I18N("h","Password");?>)</b></label>
					<span class="ashy"><?echo I18N("h","If your Internet connection requires a PPTP username and password.");?></span>
				</p>
				<p>
					<input type="radio" name="connect_mode" id="en_l2tp" value="l2tp" onclick="PAGE.OnClickWANType();" />
					<label for="en_l2tp"><b><?echo I18N("h","L2TP Connection")." (".I18N("h","Username")." / ".I18N("h","Password");?>)</b> </label>
					<span class="ashy"><?echo I18N("h","If your Internet connection requires an L2TP username and password.");?></span>
				</p>
				<p>
					<input type="radio" name="connect_mode" id="en_static" value="static" onclick="PAGE.OnClickWANType();" checked />
					<label for="en_static"><b><?echo I18N("h","Static IP Address Connection");?></b></label>
					<span class="ashy"><?echo I18N("h","If your Internet Service Provider provided you with IP-address information that has to be entered manually.");?></span>
				</p>
			</div>
		</div>
		</div>
<!-- INITIAL: End of WANTYPE -->
<!-- INITIAL: Start of DHCP -->
		<div id="init_dhcp" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Step 2").": ".I18N("h","Your connection type is DHCP. Please enter these details to set up your router for DHCP.");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","Your connection will be established automatically.");?>
				<?echo I18N("h","If your Internet Service Provider only allows one specific device to connect to the internet, please press the \"Clone MAC Address\" button below to clone your computer's MAC address to the router.");?>
				<?echo I18N("h","This will enable your Internet Service Provider to recognize your router.");?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","MAC address");?> :</strong></td>
				<td width="674">
					<?drawinputmac("macaddr");?> (<?echo I18N("h","Optional");?>)
					<?drawlabel("macaddr");?><br />
					<input type="button" value="<?echo I18N("h","Clone MAC Address");?>" onclick="PAGE.OnClickCloneMAC();"
						title="<?echo I18N("h","You can use this button to automatically copy the MAC address to your device.");?>" />
				</td>
			</tr>
			</table>
			<h6><?echo i18n('Or pick a <a href="javascript:PAGE.OnClickPre();">WAN Connection</a> of your choice.');?></h6>
		</div>
		</div>
<!-- INITIAL: End of DHCP -->
<!-- INITIAL: Start of static IP -->
		<div id="init_static" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Step 2").": ".I18N("h","Your connection type is Static IP. Please enter these details to set up your router for Static IP.");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","Your router can be connected directly to the Internet with your IP-address information.");?>
				<?echo I18N("h","Please enter the IP-address details as provided by your Internet Service Provider.");?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","IP address");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("static_ip");?>
					<?drawlabel("static_ip");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Subnet mask");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("static_mask");?>
					<?drawlabel("static_mask");?>
				</td>
			</tr>
			<tr>
				<td class="td_right"><strong><?echo I18N("h","Gateway");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("static_gw");?>
					<?drawlabel("static_gw");?>
				</td>
			</tr>
			<tr>
				<td class="td_right"><strong><?echo I18N("h","Primary DNS");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("static_dns");?>
					<?drawlabel("static_dns");?>
				</td>
			</tr>
			</table>
			<h6><?echo i18n('Or pick a <a href="javascript:PAGE.OnClickPre();">WAN Connection</a> of your choice.');?></h6>
		</div>
		</div>
<!-- INITIAL: End of Static IP -->
<!-- INITIAL: Start of PPPoE -->
		<div id="init_pppoe" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><? echo I18N("h","Step 2").": ".I18N("h","Your connection type is PPPoE. Please enter these details to set up your router for PPPoE.");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><? echo I18N("h","Please enter your Internet username and password, as provided by your Internet Service Provider.");?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Username");?> :</strong></td>
				<td width="674">
					<input type="text" class="text_block" id="pppoe_user" size="40" />
					<?drawlabel("pppoe_user");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Password");?> :</strong></td>
				<td><input type="password" class="text_block" id="pppoe_pwd" size="40" /></td>
			</tr>
			</table>
			<h6><? echo I18N("h","If your Internet Service Provider also provided you with a static IP, please enter the IP address and DNS IP.");?> &nbsp;&nbsp;
				<a onclick="PAGE.OnClickPPPStaticIP('pppoe');"><span class="mandatory2"><? echo I18N("h","Setup");?></span><img src="pic/down-arrow.gif" width="7" height="7" /></a>
			</h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content" id="pppoe_static" style="display:none;">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","IP address");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("pppoe_ip");?>
					<?drawlabel("pppoe_ip");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Primary DNS");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("pppoe_dns");?>
					<?drawlabel("pppoe_dns");?>
				</td>
			</tr>
			</table>
			<h6><?echo i18n('Or pick a <a href="javascript:PAGE.OnClickPre();">WAN Connection</a> of your choice.');?></h6>
		</div>
		</div>
<!-- INITIAL: End of PPPoE -->
<!-- INITIAL: Start of PPTP -->
		<div id="init_pptp" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><? echo I18N("h","Step 2").": ".I18N("h","Your connection type is PPTP. Please enter these details to set up your router for PPTP.");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><? echo I18N("h","Please enter your Internet username and password, as provided by your Internet Service Provider.");?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Username");?> :</strong></td>
				<td width="674">
					<input type="text" class="text_block" id="pptp_user" size="40" />
					<?drawlabel("pptp_user");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Password");?> :</strong></td>
				<td><input type="password" class="text_block" id="pptp_pwd" size="40" /></td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Server IP");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("pptp_svr");?>
					<?drawlabel("pptp_svr");?>
				</td>
			</tr>
			</table>
			<h6><? echo I18N("h","If your Internet Service Provider also provided you with a static IP, please enter the IP address and DNS IP.");?> &nbsp;&nbsp;
				<a onclick="PAGE.OnClickPPPStaticIP('pptp');"><span class="mandatory2"><? echo I18N("h","Setup");?></span><img src="pic/down-arrow.gif" width="7" height="7" /></a>
			</h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content" id="pptp_static" style="display:none;">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","IP address");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("pptp_ip");?>
					<?drawlabel("pptp_ip");?>
				</td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Subnet mask");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("pptp_mask");?>
					<?drawlabel("pptp_mask");?>
				</td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Gateway");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("pptp_gw");?>
					<?drawlabel("pptp_gw");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Primary DNS");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("pptp_dns");?>
					<?drawlabel("pptp_dns");?>
				</td>
			</tr>
			</table>
			<h6><?echo i18n('Or pick a <a href="javascript:PAGE.OnClickPre();">WAN Connection</a> of your choice.');?></h6>
		</div>
		</div>
<!-- INITIAL: End of PPTP -->
<!-- INITIAL: Start of L2TP -->
		<div id="init_l2tp" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><? echo I18N("h","Step 2").": ".I18N("h","Your connection type is L2TP. Please enter these details to set up your router for L2TP.");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><? echo I18N("h","Please enter your Internet username and password, as provided by your Internet Service Provider.");?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Username");?> :</strong></td>
				<td width="674">
					<input type="text" class="text_block" id="l2tp_user" size="40" />
					<?drawlabel("l2tp_user");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Password");?> :</strong></td>
				<td><input type="password" class="text_block" id="l2tp_pwd" size="40" /></td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Server IP");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("l2tp_svr");?>
					<?drawlabel("l2tp_svr");?>
				</td>
			</tr>
			</table>
			<h6><? echo I18N("h","If your Internet Service Provider also provided you with a static IP, please enter the IP address and DNS IP.");?> &nbsp;&nbsp;
				<a onclick="PAGE.OnClickPPPStaticIP('l2tp');"><span class="mandatory2"><? echo I18N("h","Setup");?></span><img src="pic/down-arrow.gif" width="7" height="7" /></a>
			</h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content" id="l2tp_static" style="display:none;">
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","IP address");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("l2tp_ip");?>
					<?drawlabel("l2tp_ip");?>
				</td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Subnet mask");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("l2tp_mask");?>
					<?drawlabel("l2tp_mask");?>
				</td>
			</tr>
			<tr>
				<td width="106" nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Gateway");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("l2tp_gw");?>
					<?drawlabel("l2tp_gw");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><? echo I18N("h","Primary DNS");?> :</strong></td>
				<td width="674">
					<?drawinputipaddr("l2tp_dns");?>
					<?drawlabel("l2tp_dns");?>
				</td>
			</tr>
			</table>
			<h6><?echo i18n('Or pick a <a href="javascript:PAGE.OnClickPre();">WAN Connection</a> of your choice.');?></h6>
		</div>
		</div>
<!-- INITIAL: End of L2TP -->
<!-- INITIAL: Start of WLAN -->
		<div id="init_wlan" style="display:none;">
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Step 3").": ".I18N("h","Create your router's Wi-Fi SSID and password");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h",'This Wi-Fi SSID is the name of your Wireless LAN (Local Area Network, or "wireless network", or "wireless intranet") as it will appear on a list of available networks.');?>
				<?echo I18N("h","This password will be required whenever any device wants to wirelessly connect to your router.");?>
				<?echo I18N("h","Please create a name and password for your wireless connection (wireless network).");?></h6>
			<h6>* <?echo I18N("h","Please note that your wireless password is protected by WPA2 encryption by default.");?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content">
			<tr>
				<td width="152" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","SSID")." (".I18N("h","network name");?>) :</strong></td>
				<td>
					<input type="text" class="text_block" id="wlan_ssid" size="40" />
					<?drawlabel("wlan_ssid");?>
				</td>
			</tr>
			<tr>
				<td nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Password");?> :</strong></td>
				<td>
					<input type="text" class="text_block" id="wlan_key" size="40" />
					<?drawlabel("wlan_key");?><br />
					<span class="ashy"><?echo i18n('<a href="javascript:PAGE.GenKey(\'wlan_\');">Generate a random password</a> or enter a security key with 8-63 characters.');?></span>
				</td>
			</tr>
			</table>
			<h6>
				<?echo i18n('If you need another SSID and Key for your guests. Please <a onclick="PAGE.GZToggle();"><span class="mandatory2">click here</span>');?>
				<img src="pic/down-arrow.gif" width="7" height="7" /></a>
			</h6>
			<div id="gzone" style="display:none;">
				<h6><?echo I18N("h","This SSID and key will be used when other devices in your home need to share the Internet connection.");?></h6>
				<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content" >
				<tr>
					<td width="158" nowrap="nowrap" class="td_right"><strong><?echo I18N("h","SSID");?> :</strong></td>
					<td width="562">
						<input type="text" class="text_block" id="gz_ssid" size="40" />
						<?drawlabel("gz_ssid");?>
					</td>
				</tr>
				<tr>
					<td nowrap="nowrap" class="td_right"><strong><?echo I18N("h","Key");?> :</strong></td>
					<td>
						<input type="text" class="text_block" id="gz_key" size="40" />
						<?drawlabel("gz_key");?><br />
						<span class="ashy"><?echo i18n('<a href="javascript:PAGE.GenKey(\'gz_\');">Generate a random key</a> or enter a security key with 8-63 characters.');?></span>
					</td>
				</tr>
				</table>
			</div>
		</div>
		</div>
<!-- INITIAL: End of WLAN -->
<!-- INITIAL: Start of Final Status -->
		<div id="init_status" style="display:none;">
		<ul class="capsule_btn">
			<li><a href="javascript:window.print();" class="cap_print"><span><?echo I18N("h","Print");?></span></a></li>
			<li><a href="#" class="cap_save" onclick="PAGE.saveRouterInfo();"><span><?echo I18N("h","Save");?></span></a></li>
		</ul><i></i>
		<div class="rc_gradient_hd">
			<h2><?echo I18N("h","Your router's Internet-connection setup has been completed");?></h2>
		</div>
		<div class="rc_gradient_bd h_initial">
			<h6><?echo I18N("h","Congratulations! Your router's Internet-connection setup has been successfully completed.");?>
				<?echo I18N("h","Below are your configured settings.");?>
				<?echo I18N("h","You may print them out or save them by clicking on the tabs in the top right-hand corner of this page.");?></h6>
			<h6><?echo I18N("h",'When you are done, please click the "');?><span id="b_casa_text1"></span><?echo I18N("h",'" below to continue.');?></h6>
			<table border="0" cellspacing="0" cellpadding="0" class="gradient_form_content shrink">
			<tr>
				<td width="390" nowrap="nowrap">
					<ul class="setup_done">
						<li><?echo I18N("h","Router Login Password");?></li>
						<li class="punch"><?echo I18N("h","Password");?>: <span id="st_pwd"></span></li>
					</ul>
					<ul class="setup_done">
						<li><?echo I18N("h","Internet Connection");?> (<span id="st_wanmode"></span>)</li>
						<li id="f_pppname" class="punch" style="display:none;"><?echo I18N("h","Username");?>: <span id="st_pppname"></span></li>
						<li id="f_ppppwd" class="punch" style="display:none;"><?echo I18N("h","Password");?>: <span id="st_ppppwd"></span></li>
						<li id="f_svr" class="punch" style="display:none;"><?echo I18N("h","Server IP");?>: <span id="st_svr"></span></li>
						<li id="f_ip" class="punch" style="display:none;"><?echo I18N("h","IP address");?>: <span id="st_ipaddr"></span></li>
						<li id="f_mask" class="punch" style="display:none;"><?echo I18N("h","Subnet mask");?>: <span id="st_mask"></span></li>
						<li id="f_gw" class="punch" style="display:none;"><?echo I18N("h","Gateway");?>: <span id="st_gw"></span></li>
						<li id="f_dns" class="punch" style="display:none;"><?echo I18N("h","Primary DNS");?>: <span id="st_dns"></span></li>
					</ul>
				</td>
				<td width="390" nowrap="nowrap">
					<ul class="setup_done">
						<li><?echo I18N("h","Router Wi-Fi Info")." (".I18N("h","WPA2");?>)</li>
						<li class="punch"><?echo I18N("h","SSID")." (".I18N("h","network name");?>): <span id="st_ssid"></span></li>
						<li class="punch"><?echo I18N("h","Password");?>: <span id="st_key"></span></li>
					</ul>
				</td>
			</tr>
			</table>
		</div>
		</div>
<!-- INITIAL: End of Final Status -->
		<div class="rc_gradient_submit">
			<button id="b_back" type="button" class="submitBtn floatLeft" style="display:none;" onclick="PAGE.OnClickPre();">
				<b><?echo I18N("h","Back");?></b>
			</button>
			<button id="b_next" type="button" class="submitBtn floatRight" onclick="PAGE.OnClickNext();">
				<b><?echo I18N("h","Next");?></b>
			</button>
			<button id="b_casa" type="button" class="submitBtn floatRight" style="display:none;"
				onclick="self.location.href='http://www.miiicasa.com/doc/welcome';">
				<b><span id="b_casa_text2"></span></b>
			</button><i></i>
		</div>
	</div>
	</form>
