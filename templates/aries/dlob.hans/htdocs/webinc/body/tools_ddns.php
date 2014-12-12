<? include "/htdocs/webinc/body/draw_elements.php"; ?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("Dynamic DNS");?></h1>
	<p>
		<?echo i18n("The Dynamic DNS feature allows you to host a server (Web, FTP, Game Server, etc...) using a domain name that you have purchased (www.whateveryournameis.com) with your dynamically assigned IP address. Most broadband Internet Service Providers assign dynamic (changing) IP addresses. Using a DDNS service provider, your friends can enter your host name to connect to your game server no matter what your IP address is.");?>
	</p>
	<p>
		<a href="http://www.dlinkddns.com/"><?echo i18n("Sign up for D-Link's Free DDNS service at www.DLinkDDNS.com.");?></a>
	</p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
		<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</div>
<div class="blackbox">
	<h2><?echo i18n("Dynamic DNS Settings");?></h2>
	<div class="centerline" align="center">
		<div class="textinput">
			<span class="name"><?echo i18n("Enable Dynamic DNS");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="checkbox" id="en_ddns" onclick="PAGE.EnableDDNS();"></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Server Address");?></span>
			<span class="delimiter">:</span>
			<span class="value">
				<select id="server">
					<option value="DLINK">dlinkddns.com(Free)</option>
					<option value="DYNDNS.C">DynDns.org(Custom)</option>
					<option value="DYNDNS">DynDns.org(Free)</option>
					<option value="DYNDNS.S">DynDns.org(Static)</option>
				</select>
			</span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Host Name");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="text" id="host" maxlength="60" size="40"></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Username or Key");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="text" id="user" maxlength="16" size="40"></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Password or Key");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="password" id="passwd" maxlength="16" size="40"></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Verify Password or Key");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="password" id="passwd_verify" maxlength="16" size="40"></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Timeout");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="text" id="timeout" maxlength="4" size="10"><?echo i18n("(hours)");?></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("Status");?></span>
			<span class="delimiter" >:</span>
			<span class="value" id="report" ></span>
		</div>
	</div>
	<div class="gap"></div>
</div>
<div class="blackbox">
	<h2><?echo i18n("Dynamic DNS For IPv6 HOSTS");?></h2>
			<div class="textinput">
			<span class="name"><?echo i18n("Enable");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="checkbox" id="en_ddns_v6" onclick="PAGE.EnableDDNS();"></span>
		</div>
		<div class="textinput">
			<span class="name"><?echo i18n("IPv6 Address");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="text" id="v6addr" maxlength="60" size="25">
			<input type="button" value="<<" class="arrow" onClick="OnClickPCArrow('<?=$INDEX?>');" />
			<? DRAW_select_v6dhcpclist("LAN-4","pc_".$INDEX, i18n("Computer Name"), "",  "", "1", "broad"); ?>
			</span>
		
		</div>

		<div class="textinput">
			<span class="name"><?echo i18n("Host Name");?></span>
			<span class="delimiter">:</span>
			<span class="value"><input type="text" id="v6host" maxlength="60" size="25"><?echo i18n("(e.g.: ipv6.mydomain.net)");?></span>
		</div>		
		<p><input type="button" id="add_ddns_v6" value="<?echo i18n("Save");?>" onclick="PAGE.AddDDNS();" />
		<input type="button" id="clear_ddns_v6" value="<?echo i18n("Clear");?>" onclick="ClearDDNS();" /></p>
		<div class="gap"></div>
</div>	
<div class="blackbox">
	<h2><?echo i18n("IPv6 DYNAMIC DNS LIST ");?></h2>
	<table id="v6ddns_list" class="general">
		<tr>
			<th width=10%><?echo i18n("Enable");?></th>
			<th width=44%><?echo i18n("Host Name");?></th>
			<th width=30%><?echo i18n("IPv6 Address");?></th>
			<th width=8%><?echo i18n("");?></th>
			<th width=8%><?echo i18n("");?></th>
		</tr>
	</table>
	<div class="gap"></div>
</div>	
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
</form>
