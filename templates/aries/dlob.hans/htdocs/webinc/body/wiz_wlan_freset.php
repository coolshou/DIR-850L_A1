<?
function wiz_buttons()
{
	echo '<div class="emptyline"></div>\n'.
		 '	<div class="centerline">\n'.
		 '		<input type="button" name="b_pre" value="'.i18n("Prev").'" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_next" value="'.i18n("Next").'" onClick="PAGE.OnClickNext();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_exit" value="'.i18n("Cancel").'" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_send" value="'.i18n("Save").'" onClick="BODY.OnSubmit();" />&nbsp;&nbsp;\n'.
		 '	</div>\n'.
		 '	<div class="emptyline"></div>';
}
function wiz_buttons2()
{
	echo '<div class="emptyline"></div>\n'.
		 '	<div class="centerline">\n'.
		 '		<input type="button" name="b_pre" value="'.i18n("Prev").'" onClick="PAGE.OnClickPre();" />&nbsp;&nbsp;\n'.
		 '		<input type="button" name="b_send" value="'.i18n("Save").' & '.i18n("Connect").'" onClick="BODY.OnSubmit();" />&nbsp;&nbsp;\n'.		 
		 '		<input type="button" name="b_exit" value="'.i18n("Cancel").'" onClick="PAGE.OnClickCancel();" />&nbsp;&nbsp;\n'.
		 '	</div>\n'.
		 '	<div class="emptyline"></div>';
}
?>
<form id="mainform" onsubmit="return false;">
<!-- Start of Stage 1 -->
<div id="wiz_stage_1" class="blackbox" style="display:none;">
	<h2><? echo i18n("Step 5").": ".i18n("Welcome TO THE D-LINK WIRELESS SECURITY SETUP WIZARD");?></h2>
	<div><p class="strong">
		<? echo i18n("Give your network a name, using up to 32 characters.");?>
	</p></div>
	<div class="gap"></div>
	<div class="textinput">
		<span class="name"><?echo i18n("Wireless Network Name")."(SSID)";?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="wiz_ssid" type="text" size="20" maxlength="32" />
		</span>
	</div>
	<br>
	
	<div class="gap"></div>	
	<div class="wiz-l1" text-align="left">
		<input type="radio" id="autokey" name="key" checked="checked" onClick="PAGE.OnChangeNetworkKeyType();"/>
		<? echo i18n("Automatically assign a network key")." (".i18n("Recommended").")";?><br />
	</div>
	<div class="wiz-l2">
		<? echo i18n("To prevent outsiders from accessing your network, the router will automatically assign a security to your network.");?>
	</div>
	<div class="gap"></div>
	<div class="wiz-l1">
		<input type="radio" id="manukey" name="key" onClick="PAGE.OnChangeNetworkKeyType();"/>
		<? echo i18n("Manually assign a network key");?><br />
	</div>
	<div class="wiz-l2">
		<? echo i18n("Use this options if you prefer to create our own key.");?>
	</div>
	<div class="wiz-l2">
		<? echo i18n("The WPA (Wi-Fi Protected Access) key must meet one of following guidelines:");?>
	</div>
	<div class="wiz-l2">
		<? echo "- ".i18n("Between 8 and 63 characters")." (".i18n("A longer WPA key is more secure than a short one")." )".;?>
	</div>
	<div class="textinput">
		<span class="name" id="wl_sec"><?echo i18n("Wireless Network Key");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="wiz_key" type="text" maxlength="64" /></span>
	</div>	
	<div><p class="strong">
		<? echo i18n("Note").": ".i18n("All D-Link wireless adapters currently support WPA.");?>
	</p></div>
	<?wiz_buttons();?>
</div>
<!-- End of Stage 1 -->

<!-- Start of Stage 2 -->
<div id="wiz_stage_2" class="blackbox" style="display:none;">
	<h2><? echo i18n("Setup Complete!");?></h2>
	<div><p class="strong">
		<? echo i18n("The Setup Wizard has completed.")." ".
				i18n("Click the Connect button to save your settings and restart the router.");?>
	</p></div>
	<div class="textinput">
        <span class="name"><?echo i18n("Connection Type");?></span>
        <span class="delimiter">:</span>
        <span class="value" id="st_wantype"></span>
    </div>
    <div class="textinput">
        <span class="name" id= "name_wanipaddr"></span>
        <span class="delimiter">:</span>
        <span class="value" id="st_wanipaddr"></span>
    </div>
    <div class="textinput">
        <span class="name"><?echo i18n("Subnet Mask");?></span>
        <span class="delimiter">:</span>
        <span class="value" id="st_wannetmask"></span>
    </div>
    <div class="textinput">
        <span class="name" id= "name_wangateway"></span>
        <span class="delimiter">:</span>
        <span class="value" id="st_wangateway"></span>
    </div>
    <div class="textinput">
        <span class="name"><?echo i18n("Primary DNS Server");?></span>
        <span class="delimiter">:</span>
        <span class="value" id="st_wanDNSserver"></span>
    </div>
    <div class="textinput" >
        <span class="name"><?echo i18n("Secondary DNS Server");?></span>
        <span class="delimiter">:</span>
        <span class="value" id="st_wanDNSserver2"></span>
    </div>    
    <div class="gap"></div>    
    <div class="gap"></div>
	
	<div><p class="strong">
		<? echo i18n("Below is a detailed summary of your wireless security settings.")." ".
				i18n("Please print this page out, or write the information on a piece of paper, so you can configure the correct settings on your wireless client adapters.");?>
	</p></div>
	<div class="gap"></div>
	<div class="textinput">
		<span class="name"><? echo i18n("Wireless Network Name")." (SSID)";?></span>
		<span class="delimiter">:</span>
		<span id="ssid" class="value"></span>
	</div>
	<div class="textinput">
		<span class="name"><? echo i18n("Security Mode");?></span>
		<span class="delimiter">:</span>
		<span class="value"><? echo i18n("Auto (WPA or WPA2) - Personal");?></span>
	</div>
	<div class="textinput">
		<span class="name"><? echo i18n("Cipher Type");?></span>
		<span class="delimiter">:</span>
		<span class="value">TKIP and AES</span>
	</div>
	<div class="textinput">
		<span class="name"><? echo i18n("Pre-Shared Key");?></span>
		<span class="delimiter">:</span>
		<span id="s_key" class="value"></span>
	</div>
	<div id="l_key" class="centerline" style="display:none;"></div>	
	<?wiz_buttons2();?>
</div>
<!-- End of Stage 2 -->
</form>
