<? 
		include "/htdocs/phplib/xnode.php";
		include "/htdocs/webinc/body/draw_elements.php";
 ?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("SYSLOG");?></h1>
	<p>
		<?echo i18n("The SysLog options allow you to send log information to a Syslog Server.");?>
	</p>
	
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</div>
<div class="blackbox">
	<h2><?echo i18n("SYSLOG SETTINGS");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable Logging To SysLog Server");?></span>
		<span class="delimiter">:</span>
		<span class="value">
		<input id="syslogenable" type="checkbox" onclick="PAGE.OnClickSYSLOGEnable();"/>
		</span>
	</div>
	<br>
	<div class="textinput" id="div_logip" style="display:none;">
		<span class="name"><?echo i18n("Syslog Server IP Address");?></span>
		<span class="delimiter">:</span>
		<span class="value">
		<input id="sysloghost" size="15" maxlength="15" value="" type="text"/>
			<input modified="ignore" id="syslogadd" value="<<" onclick="PAGE.OnClickSYSLOGAdd();" type="button"/>
			<? DRAW_select_dhcpclist("LAN-1","hostlist", i18n("Computer Name"), "", "", "1", "selectSty", "180px"); ?>
       </span>
	</div>  
		<div class="gap"></div>
</div>

<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
</form>

