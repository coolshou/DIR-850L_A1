<?
include "/htdocs/phplib/xnode.php";
$inf_wan1 = XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);
$opendns_device_id = query($inf_wan1."/open_dns/deviceid");
?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1><?echo i18n("PARENTAL CONTROL");?></h1>
	<p><?echo i18n("Options to improve the speed and reliability of your Internet connection, to apply content filtering and to protect you from phishing sites. Choose from pre-configured bundles or register your router with OpenDNS&reg; to choose from 50 content categories for custom blocking.");?></p>
	<p><input type="button" value="<?echo i18n("Save Settings");?>" onClick="BODY.OnSubmit();" />
	    <input type="button" value="<?echo i18n("Don't Save Settings");?>" onClick="BODY.OnReload();" /></p>
</div>
<div class="blackbox">
	<h2><?echo i18n("SECURITY OPTIONS");?></h2>
	<br>
	<!--
	<div class="wiz-l1">
		<input id="adv_dns" type="radio" value="DHCP" onClick="PAGE.OnChangeOpenDNSType(this.id);" />
		<?echo i18n("Advanced DNS&#8482");?>
	</div>
	<div class="wiz-l2">
		<?echo i18n("Advanced DNS makes your Internet connection safer, faster, smarter and more reliable. It blocks phishing websites to protect you from identity theft.");?>
	</div>
	<div class="wiz-l1">
		<input id="familyshield" type="radio" value="familyshield" onClick="PAGE.OnChangeOpenDNSType(this.id);" />
		<?echo i18n("OpenDNS&reg; FamilyShield&#8482");?>
	</div>
	<div class="wiz-l2">
		<?echo i18n("Automatically block adult and phishing websites while improving the speed and reliability of your Internet connection.");?>
	</div>
	-->
	<div class="wiz-l1">
		<input id="parent_ctrl" type="radio" value="parent_ctrl" onClick="PAGE.OnChangeOpenDNSType(this.id);" />
		<?echo i18n("OpenDNS&reg; Parental Controls&#8482");?>
	</div>
	<div class="wiz-l2">
		<?echo i18n("OpenDNS Parental Controls provides award-winning Web content filtering while making your Internet connection safer, faster, smarter and more reliable. With more than 50 content categories to choose from it's effective against adult content, proxies, social networking, phishing sites, malware and more. Fully configurable from anywhere there is Internet access.");?>
	</div>
    <div class="wiz-l2">
		<a id="opendns_register" href="./open_dns.php"><? echo i18n("Manage your router");?></a>
		<? echo i18n("at <a id='opendns_config' href='#' onclick='window.open(\"https://www.opendns.com\")'>www.opendns.com</a>");?>
	</div>
	<div class="wiz-l1">
		<input id="none_opendns" type="radio" value="none_opendns" onClick="PAGE.OnChangeOpenDNSType(this.id);" />
		<?echo i18n("None: Static IP or Obtain Automatically From ISP");?>
	</div>
	<div class="wiz-l2">
		<?echo i18n("Use the DNS servers provided by your ISP, or enter your preferred DNS servers.");?>
	</div>		
	<div class="gap"></div>
</div>
<p><input type="button" value="<?echo i18n("Save Settings");?>" onclick="BODY.OnSubmit();" />
	<input type="button" value="<?echo i18n("Don't Save Settings");?>" onclick="BODY.OnReload();" /></p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<p>&nbsp;</p>
<form>
