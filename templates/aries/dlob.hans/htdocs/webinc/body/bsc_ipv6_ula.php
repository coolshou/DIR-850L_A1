<?include "/htdocs/webinc/body/draw_elements.php";?>
<form id="mainform" onsubmit="return false;">
<!-- IPV6 orangebox START-->
<div class="orangebox"> 
	<h1><?echo i18n("IPv6 LOCAL CONNECTIVITY SETTINGS");?></h1>
		<p>
			<?echo i18n("Use this section to configure Unique Local IPv6 Unicast Address (ULA) settings for your router.");?>
			<?echo i18n("ULA is intended for local communications and not expected to be routable on the global Internet.");?>
		</p>
		<p>
			<input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
			<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" />
		</p>
</div>
<!-- IPV6 orangebox END-->
<!-- IPV6 ULA SETTINGS -->
<div class="blackbox" id="bbox_ula" style="display:none">
	<div id="box_ula_title" style="display:none">
		<h2><?echo i18n("IPv6 ULA SETTINGS");?></h2>
	</div>
	<div id="box_ula_body" style="display:none">
		<div class="textinput">
			<span class="name"><?echo i18n("Enable ULA");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="en_ula" value="" type="checkbox" onClick="PAGE.OnClickEnableUla();"/></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Use default ULA prefix");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input id="use_default_ula" value="" type="checkbox" onClick="PAGE.OnClickUseDefault();"/></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("ULA Prefix");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<span id="ula_span" style="display:none">
				<span><input id="ula_prefix" type="text" size="42" maxlength="45" /></span>
				<span id="ula_prefix_pl"></span>
				</span>
			</span>
		</div>
	</div>
	<div class="gap"></div>
</div>
<!-- IPV6 ULA SETTINGS END -->
<!-- Current IPv6 ULA SETTINGS -->
<div class="blackbox">
	<div id="box_cula_title" style="display:none"> 
		<h2><?echo i18n("Current IPv6 ULA SETTINGS");?></h2>
	</div>
	<div id="box_cula_prefix_body" style="display:none">
		<div class="textinput">
			<span class="name"><?echo i18n("Current ULA Prefix");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<span id="cula_prefix"></span>	
				<span id="cula_prefix_pl"></span>
			</span>
		</div>
	</div>
	<div id="box_cula_addr_body" style="display:none">
		<div class="textinput">
			<span class="name"><?echo i18n("LAN IPv6 ULA");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<span id="cula_addr"></span>	
				<span id="cula_addr_pl"></span>
			</span>
		</div>
	</div>
	<div class="gap"></div>
</div>
<!-- Current IPv6 ULA SETTINGS END -->
<div class="gap"></div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</form>
