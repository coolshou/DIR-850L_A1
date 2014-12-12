
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo I18N("h", "MYDLINK SETTINGS");?></h1>
	<p><?echo I18N("h", "Setting and registering your product with mydlink will allow you to use its mydlink cloud services features, including online access and management of your device through mydlink portal website.");?></p>
	<p style="display:none">
		<input type="button" value="<?echo I18N("h", "Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo I18N("h", "Don't Save Settings");?>" onclick="BODY.OnReload();" />
	</p>
</div>
<div class="blackbox">
	<h2><?echo I18N("h", "MYDLINK");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("mydlink Service");?></span>
		<span class="delimiter">:</span>
		<span class="value"><? if(query("/mydlink/register_st")=="1"){ echo I18N("h", "Registered");} else { echo I18N("h", "Non-Registered");} ?></span>
	</div>
	<div class="textinput" <? if(query("/mydlink/register_st")!="1") echo 'style="display:none;"';?>>
		<span class="name"><?echo I18N("h", "mydlink Account");?></span>
		<span class="delimiter">:</span>
		<span class="value"><? if(query("/mydlink/regemail")!="") echo query("/mydlink/regemail"); ?></span>
	</div>	
	<div class="gap"></div>
</div>

<div class="blackbox">
	<h2><?echo I18N("h", "Register mydlink Service");?></h2>
	<div class="textinput">
		<span class="name"></span>
		<span class="delimiter"></span>
		<span class="value"><input type="button" value="<?echo I18N("h", "Register mydlink Service");?>" onclick="self.location.href='/wiz_mydlink.php'" <? if(query("/mydlink/register_st")=="1") echo "disabled";?>/></span>
	</div>
	<div class="gap"></div>
	<div class="gap"></div>
</div>

<div class="blackbox" style="display:none">
	<h2><?echo I18N("h", "REAL-TIME BROWSING HISTORY");?></h2>
	<div class="textinput">
		<span class="name"><?echo I18N("h", "Enable");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_dns_query" type="checkbox"/></span>
	</div>
	<div class="gap"></div>
</div>

<div class="blackbox" style="display:none">
	<h2><?echo I18N("h", "PUSH EVENT");?></h2>
	<div class="textinput">
		<span class="name"><?echo I18N("h", "Enable");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_push_event" type="checkbox" onclick="PAGE.PushEvent();"/></span>
	</div>
	<div class="gap"></div>
	<div style="margin-left:20px">
		<span><input id="en_notice_userlog" type="checkbox"/></span>
		<span><?echo I18N("h", "Notice of Online User Logging");?></span>
		<span><input id="en_notice_fwupgrade" type="checkbox"/></span>
		<span><?echo I18N("h", "Notice of Firmware Upgrade");?></span>				
	</div>
	<div style="margin-left:20px">
		<span><input id="en_notice_wireless" type="checkbox"/></span>
		<span><?echo I18N("h", "Notice of Wireless Intrusion");?></span>				
	</div>
	<div class="gap"></div>		
</div>

<p style="display:none">
	<input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" />
</p>
</form>
