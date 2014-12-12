<? include "/htdocs/webinc/body/draw_elements.php"; ?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("FIREWALL & DMZ SETTINGS");?></h1>
	<p><?echo i18n('DMZ means "Demilitarized Zone".')." ".
		i18n('DMZ allows computers behind the router firewall to be accessible to Internet traffic.');?>
	<?echo i18n("Typically, your DMZ would contain Web servers, FTP servers and others.");?></p>
	<p>	<input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</div>
<div class="blackbox">
	<h2><?echo i18n("Firewall Settings");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable SPI");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="spi" type="checkbox"/></span>
	</div>
	<div class="gap"></div>
</div>

<div class="blackbox" style="display:none">
	<h2><?echo i18n("NAT Endpoint Filtering");?></h2>
	<div class="textinput">		
		<span class="name"></span>
		<span class="delimiter"></span>
		<span class="value"><input id="udp_end" name="udptype" type="radio" value="UDP_END"  /><?echo i18n("Endpoint Independent");?></span>
	</div>		
	<div class="textinput">
		<span class="name"><?echo i18n("UDP Endpoint Filtering");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="udp_add" name="udptype" type="radio" value="UDP_ADD"  /><?echo i18n("Address Restricted");?></span>
	</div>	
	<div class="textinput">
		<span class="name"></span>
		<span class="delimiter"></span>
		<span class="value"><input id="udp_pna" name="udptype" type="radio" value="UDP_PNA"  /><?echo i18n("Port And Address Restricted");?></span>
	</div>	

	<div class="gap"></div>
	<div class="textinput">
		<span class="name"></span>
		<span class="delimiter"></span>
		<span class="value"><input id="tcp_end" name="tcptype" type="radio" value="TCP_END"  /><?echo i18n("Endpoint Independent");?></span>
	</div>	
	<div class="textinput">
		<span class="name"><?echo i18n("TCP Endpoint Filtering");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="tcp_add" name="tcptype" type="radio" value="TCP_ADD"  /><?echo i18n("Address Restricted");?></span>
	</div>
	<div class="textinput">	
		<span class="name"></span>
		<span class="delimiter"></span>
		<span class="value"><input id="tcp_pna" name="tcptype" type="radio" value="TCP_PNA"  /><?echo i18n("Port And Address Restricted");?></span>
	</div>
	<div class="gap"></div>
</div>


<div class="blackbox">
	<h2><?echo i18n("Anti-Spoof checking");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable anti-spoof checking");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input type="checkbox" id="anti_spoof_enable" /></span>
	</div>
	<div class="gap"></div>
</div>

<div class="blackbox">
	<h2><?echo i18n("DMZ Host");?></h2>
	<p>
		<?echo i18n("The DMZ (Demilitarized Zone) option lets you set a single computer on your network outside of the router.");?>
		<?echo i18n("If you have a computer that cannot run Internet applications successfully from behind the router, then you can place the computer into the DMZ for unrestricted Internet access.");?>
	</p>
	<p>
		<strong><?echo i18n("Note");?>:</strong>
		<?echo i18n("Putting a computer in the DMZ may expose that computer to a variety of security risks.")." ".
			i18n("Use of this option is only recommended as a last resort.");?>
	</p>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable DMZ");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input type="checkbox" id="dmzenable" onclick="PAGE.OnClickDMZEnable();"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("DMZ IP Address");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="dmzhost" size="20" maxlength="15" value="0.0.0.0" type="text"/>
			<input modified="ignore" id="dmzadd" value="<<" onclick="PAGE.OnClickDMZAdd();" type="button"/>
			<? DRAW_select_dhcpclist("LAN-1","hostlist", i18n("Computer Name"), "", "", "1", "selectSty"); ?>
		</span>
	</div>
	<!--<div class="textinput">
		<span class="name"></span>
		<span class="delimiter"></span>
		<span class="value">
			<? DRAW_select_dhcpclist("LAN-1","hostlist", i18n("Computer Name"), "", "", "1", "selectSty"); ?>
		</span>
	</div>-->
	<div class="gap"></div>
</div>

<div class="blackbox">
	<h2><?echo i18n("Application Level Gateway (ALG) Configuration");?></h2>
	<div class="textinput">	
		<span class="name"><?echo i18n("PPTP");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="pptp" type="checkbox"/></span>
	</div>
	<div class="textinput">	
		<span class="name"><?echo i18n("IPSec (VPN)");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="ipsec" type="checkbox"/></span>
	</div>	
	<div class="textinput">	
		<span class="name"><?echo i18n("RTSP");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="rtsp" type="checkbox"/></span>
	</div>
	<div class="textinput">	
		<span class="name"><?echo i18n("SIP");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="sip" type="checkbox"/></span>
	</div>	
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</form>

