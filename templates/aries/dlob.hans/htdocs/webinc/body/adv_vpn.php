 
<form id="mainform">
<div class="orangebox">
	<h1><?echo i18n("VPN");?></h1>
	<p><?echo i18n("A virtual private network (VPN) is a private network that interconnects remote networks through primarily public communication infrastructures such as the Internet.");?>	
	</p>
	<p>
		<input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" />
	</p>
</div>
<div class="blackbox">
	<h2><?echo i18n("PPTP SERVER");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Enable PPTP Server");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="pptp_enable" type="checkbox" onclick="PAGE.VPNenable();"/></span>
	</div>
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("User Account");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Account");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="account" type="text" size="20" maxlength="15" /></span>
	</div>	
	<div class="textinput">
		<span class="name"><?echo i18n("Password");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="password" type="text" size="20" maxlength="15" /></span>
	</div>
	<div class="centerline">
		<input type="button" id="user_add_edit" value="<?echo i18n("Add");?>" onclick="PAGE.UserAddEdit();">
	</div>	
	<table id="usertable" class="general">
	<tr>
		<th width="151px"><?echo i18n("Name");?></th>
		<th width="201px"><?echo i18n("Password");?></th>
		<th width="20px"></th>
		<th width="20px"></th>
	</tr>
	</table>
	<div class="gap"></div>
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("Authentication Protocol");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("No authentication");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="auth_none" type="checkbox" onclick="PAGE.AUTHcheck();"/></span>
	</div>
	<div class="textinput">
		<span class="name">CHAP</span>
		<span class="delimiter">:</span>
		<span class="value"><input id="auth_chap" type="checkbox"/></span>
	</div>
	<div class="textinput">
		<span class="name">MSCHAP</span>
		<span class="delimiter">:</span>
		<span class="value"><input id="auth_mschap" type="checkbox" onclick="PAGE.MPPEcheck();"/></span>
	</div>	
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("MPPE encryption");?></h2>
	<div class="textinput">
		<span class="name"><?echo i18n("Encrypted");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="encrypt_enable" type="checkbox"/></span>
	</div>
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("Isolation");?></h2>
	<p class="strong"><?echo i18n("Isolation with LAN.");?></p>	
	<div class="textinput">
		<span class="name"><?echo i18n("Isolation");?></span>
		<span class="delimiter">:</span>
		<span class="value"><input id="isolation_enable" type="checkbox"/></span>
	</div>
	<div class="gap"></div>
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</form>
