<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("Advanced Network Settings");?></h1>
	<p><?echo i18n("These options are for users that wish to change the LAN settings. We do not recommend changing these settings from factory default. ");?>
	<?echo i18n("Changing these settings may affect the behavior of your network.");?>
	</p>
	<p>
		<input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" />
	</p>
</div>
<div class="blackbox">
	<h2><?echo i18n("UPNP");?></h2>
	<p><?echo i18n("Universal Plug and Play(UPnP) supports peer-to-peer Plug and Play functionality for network devices.");?></p>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable UPnP IGD");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="upnp" value="" type="checkbox"/></span>
	</div>
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("WAN Ping");?></h2>
	<p><?echo i18n("If you enable this feature, the WAN port of your router will respond to ping requests from the Internet that are sent to the WAN IP Address.");?></p>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable WAN Ping Response");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="icmprsp" value="" type="checkbox"/></span>
	</div>
	<br>
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("WAN Port Speed");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("WAN Port Speed");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<select id="wanspeed">
				<option value="1"><?echo i18n("10Mbps");?></option>
				<option value="2"><?echo i18n("100Mbps");?></option>
<? if($FEATURE_WAN1000FTYPE!="1") {echo "<!--";}?>
				<option value="3"><?echo i18n("1000Mbps");?></option>
				<option value="0"><?echo i18n("Auto 10/100/1000Mbps");?></option>
<? if($FEATURE_WAN1000FTYPE!="1") {echo "-->";}
   else {echo "<!--";}?>
   				<option value="0"><?echo i18n("Auto 10/100Mbps");?></option>
<? if($FEATURE_WAN1000FTYPE=="1") {echo "-->";}?>
			</select>
		</span>
	</div>
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><? if ($FEATURE_NOIPV6 =="0") { echo i18n("IPv4 Multicast Streams"); } else { echo i18n("Multicast Streams"); } ?></h2>
	<div class="textinput">
		<span class="name"><? if ($FEATURE_NOIPV6 =="0") { echo i18n("Enable IPv4 Multicast Streams"); } else { echo i18n("Enable Multicast Streams"); } ?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="mcast" type="checkbox" /></span>
	</div>
	<div class="textinput" style="display:none;">
		<span class="name"><?echo i18n("Wireless Enhance Mode");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="enhance" type="checkbox" /></span>
	</div>
	<div class="gap"></div>
</div>
<div class="blackbox" <?if ($FEATURE_NOIPV6 !="0") echo ' style="display:none;"';?>>
	<h2><?echo i18n("IPv6 Multicast Streams");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable IPv6 Multicast Streams");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="mcast6" type="checkbox" /></span>
	</div>
	<div class="textinput" style="display:none;">
		<span class="name"><?echo i18n("Wireless Enhance Mode");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="enhance6" type="checkbox" /></span>
	</div>
	<div class="gap"></div>
</div>
<div class="blackbox" <?if ($FEATURE_EEE !="1") echo ' style="display:none;"';?>>
	<h2><?echo i18n("EEE");?></h2>
	<p><?echo i18n("The goal of Energy Efficient Ethernet(EEE) is to reduce Ethernet power consumption by 50 percent or more.");?></p>	
	<div class="textinput">
		<span class="name"><?echo i18n("Enable EEE");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="eee" type="checkbox" /></span>
	</div>
	<div class="gap"></div>
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</form>
