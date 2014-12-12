<!-- css for calling explorer.php -->
<link rel="stylesheet" href="/portal/comm/smbb.css" type="text/css"> 
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo I18N("h","AFP SERVER SETTINGS");?></h1>
	<p><?echo I18N("h","Configure AFP Server for sharing files using Mac OS X and original Mac OS.");?>	
	</p>
	<p>
		<input type="button" value="<?echo I18N("h","Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo I18N("h","Don't Save Settings");?>" onclick="BODY.OnReload();" />		
	</p>
</div>
<div class="blackbox">
	<h2><?echo I18N("h","AFP SERVER");?></h2>
	<div class="textinput">
		<span class="name"><?echo I18N("h","AFP Server");?></span>
		<span class="delimiter">:</span>
		<span class="value">          	
			<input value="1"  id="afp_active" name="afp_active" type="radio" onclick="PAGE.check_afp_enable(1, 'afp_active');"><?echo I18N("h","Enable");?>
			<input value="0"  id="afp_active" name="afp_active" type="radio" onclick="PAGE.check_afp_enable(1, 'afp_active');"><?echo I18N("h","Disable");?>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo I18N("h","Folder");?></span>
		<span class="delimiter">:</span>
		<span class="value">
            <input type=text id="the_sharepath" size=30 readonly>
            <input type="button" id="But_Browse" value="<?echo i18n("Browse");?>" onClick="PAGE.open_browser();">
            <input type="hidden" id="f_flow_value" size="20">
            <input type="hidden" id="f_device_read_write" size="2">
            <input type="hidden" id="f_read_write" size="2">			
		</span>
	</div>
	
	<div class="gap"></div>
	<div class="gap"></div>	
</div>
<p><input type="button" value="<?echo I18N("h","Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo I18N("h","Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
</form>
