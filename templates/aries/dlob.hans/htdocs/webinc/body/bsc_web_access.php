<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("Storage");?></h1>
	<p><?echo i18n("Web File Access allows you to use a web browser to remotely access files stored on an SD card or USB storage drive plugged into the router. To use this feature, check the Enable Web File Access checkbox, then use the Admin account or create user accounts to manage access to your storage devices. After plugging in an SD card or USB storage drive, the new device will appear in the list with a link to it. You can then use this link to connect to the drive and log in with a user account.");?></p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
	    <input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" /></p>
</div>

<div class="blackbox">
	<h2><?echo i18n("SHAREPORT WEB ACCESS");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable SharePort Web Access");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_webaccess" type="checkbox" onClick="PAGE.EnWebAccess();"/></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("HTTP Access Port");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="webaccess_httpport" type="text" size="20" maxlength="5" /></span>
	</div>	
	<div class="textinput">
		<span class="name"><?echo i18n("HTTPS Access Port");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="webaccess_httpsport" type="text" size="20" maxlength="5" /></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Allow Remote Access");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="en_webaccess_remote" type="checkbox" onClick="PAGE.EnWebAccess();"/></span>
	</div>	
	<div class="gap"></div>
</div>


<div class="blackbox">
	<h2><?echo i18n("User Creation");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("User Name");?></span>
		<span class="delimiter">:</span>
		<span class="value" id="user_choose">
			<input id="user_name" type="text" size="20" maxlength="15" />
			<span>&nbsp;&lt;&lt;&nbsp;</span>
			<span id="user_select"></span>
		</span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Password");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="pwd" type="password" size="20" maxlength="15" /></span>
	</div>
	<div class="textinput">
		<span class="name"><?echo i18n("Verify Password");?></span>
		<span class="delimiter">:</span>
		<span class="value">
			<input id="pwd_verify" type="password" size="20" maxlength="15" />
			<input type="button" value="<?echo i18n("Add/Edit");?>" onClick="PAGE.UserCreate();" />
		</span>
	</div>
	<div class="gap"></div>
</div>


<div class="blackbox">
	<h2><?echo i18n("USER LIST");?></h2>
	<table id="userlisttable" class="general">
		<tr>
			<th width="10px"><?echo i18n("No.");?></th>
			<th width="40px"><?echo i18n("User Name");?></th>
			<th width="100px"><?echo i18n("Access Path");?></th>
			<th width="40px"><?echo i18n("Permission");?></th>
			<th width="12px"><?echo i18n("Edit");?></th>
			<th width="18px"><?echo i18n("Delete");?></th>
		</tr>			
	</table>	
	<div class="gap"></div>
</div>


<div class="blackbox">
	<h2 id="devicen"></h2>
	<table id="devicetable" class="general">
		<tr>
			<th width="200px"><?echo i18n("Device");?></th>
			<th width="100px"><?echo i18n("Total Space");?></th>
			<th width="100px"><?echo i18n("Free Space");?></th>
		</tr>			
	</table>	
	<div class="gap"></div>
</div>


<div class="blackbox">
	<h2><?echo i18n("SHAREPORT ACCESS LINK");?></h2>
	<p><?echo i18n("You can then use this link to connect to the drive and log in with a user account.");?></p>
	<div class="textinput" id="showlink_http">
		<p id="http_link"></p>
	</div>
	<div class="textinput" id="showlink_ssl">
		<p id="https_link"></p>
	</div>
</div>

<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
<form>
