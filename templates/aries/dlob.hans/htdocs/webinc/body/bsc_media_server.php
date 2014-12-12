<!-- css for calling explorer.php -->
<link rel="stylesheet" href="/portal/comm/smbb.css" type="text/css"> 
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("MEDIA SERVER");?></h1>
	<p><?echo i18n("DLNA (Digital Living Network Alliance) is the standard for the interoperability of Network Media Devices (NMDs). The user can enjoy multi-media applications (music, pictures and videos) on your network connected PC or media devices.");?>
		<?echo i18n("The iTunes server will allow iTunes software to automatically detect and play music from the router.");?>	
	</p>
	<br>
	<p><b><?echo i18n("NOTE: The shared media may not be secure. Allowing any devices to stream is recommended only on secure networks.");?></b>
	</p>
	<p>
		<input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" />
	</p>
</div>

<div class="blackbox">
	<h2><?echo I18N("h","DLNA SERVER");?></h2>
	<div class="textinput">
		<span class="name"><?echo I18N("h","DLNA Server");?></span>
		<span class="delimiter">:</span>
		<span class="value">          	
			<input value="1"  id="dlna_active" name="dlna_active" type="radio" onclick="PAGE.check_enable('dlna');"><?echo I18N("h","Enable");?>
			<input value="0"  id="dlna_active" name="dlna_active" type="radio" onclick="PAGE.check_enable('dlna');"><?echo I18N("h","Disable");?>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo I18N("h","DLNA Server Name");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input type="text" id="dlna_name" size=20 value="" maxlength=15></span>
	</div>	
	<div class="textinput">
		<span class="name"><?echo I18N("h","Folder");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input type="checkbox" id="dlna_root" value="ON" onClick="PAGE.check_root_path('dlna');"><?echo I18N("h","root");?></span>
	</div>
	<div class="textinput" id="dlna_chamber" style="display">
		<span class="name"></span>
		<span class="delimiter"></span>
		<span class="value">
            <input type=text id="dlna_sharepath" size=30 readonly>
            <input type="button" id="dlna_browse" value="<?echo i18n("Browse");?>" onClick="PAGE.open_browser();PAGE.path_selected='dlna';">			
		</span>
	</div>
	<div class="gap"></div>
	<div class="gap"></div>	
</div>

<div class="blackbox">
	<h2><?echo I18N("h","ITUNES SERVER");?></h2>
	<div class="textinput">
		<span class="name"><?echo I18N("h","iTunes Server");?></span>
		<span class="delimiter">:</span>
		<span class="value">          	
			<input value="1"  id="itunes_active" name="itunes_active" type="radio" onclick="PAGE.check_enable('itunes');"><?echo I18N("h","Enable");?>
			<input value="0"  id="itunes_active" name="itunes_active" type="radio" onclick="PAGE.check_enable('itunes');"><?echo I18N("h","Disable");?>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo I18N("h","Folder");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input type="checkbox" id="itunes_root" value="ON" onClick="PAGE.check_root_path('itunes');"><?echo I18N("h","root");?></span>
	</div>
	<div class="textinput" id="itunes_chamber" style="display">
		<span class="name"></span>
		<span class="delimiter"></span>
		<span class="value">
            <input type=text id="itunes_sharepath" size=30 readonly>
            <input type="button" id="itunes_browse" value="<?echo i18n("Browse");?>" onClick="PAGE.open_browser();PAGE.path_selected='itunes';">			
		</span>
	</div>
	<div class="gap"></div>
	<div class="gap"></div>	
</div>

<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
<!--The selected path from the portal window would be tranfered to the input text with the_sharepath id.-->
<div><input type=text id="the_sharepath" size=30 readonly style="display:none"></div>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
</form>
