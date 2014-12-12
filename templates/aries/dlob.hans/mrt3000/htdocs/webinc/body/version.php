<?
include "/htdocs/phplib/xnode.php";
?>
<form id="mainform" onsubmit="return false;">
<div class="rc_gradient_bd">
	<h1><?echo i18n("Version");?></h1>
	<div class="emptyline"></div>
	<div class="info">
		<span class="name"><?echo i18n("Firmware External Version");?> :</span>
		<span class="value">V<?echo cut(fread("", "/etc/config/buildver"), "0", "\n");?></span>
	</div>
	<div class="info" style="display:none;">
		<span class="name"><?echo i18n("Firmware External Revision");?> :</span>
		<span class="value"><?echo cut(fread("", "/etc/config/buildrev"), "0", "\n");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("Firmware Internal Version");?> :</span>
		<span class="value" style="text-transform:uppercase;">V<?echo cut(fread("", "/etc/config/buildver"), "0", "\n");?><?echo cut(fread("", "/etc/config/buildrev"), "0", "\n");?></span>
	</div>
<?
	if (isfile("/htdocs/webinc/body/version_3G.php")==1)
		dophp("load", "/htdocs/webinc/body/version_3G.php");
?>
	<div class="info">
		<span class="name"><?echo i18n("Language Package");?> :</span>
		<span class="value" id="langcode"></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("Date");?> :</span>
		<span class="value"><?echo query("/runtime/device/date");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("CheckSum");?> :</span>
		<span class="value" id="checksum"></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("WLAN Domain");?> :</span>
		<span class="value"><?echo query("/runtime/devdata/countrycode");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("Bootcode Version");?> :</span>
		<span class="value"><?echo query("/runtime/device/bootver");?></span>
	</div>	
	<div class="info">
		<span class="name"><?echo i18n("Kernel");?> :</span>
		<span class="value"><?echo cut(fread("", "/proc/version"), "0", "(");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("Firmware Query");?> :</span>
		<span class="value" id="fwq"></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("Apps");?> :</span>
		<span class="value"><?echo cut(fread("", "/etc/config/builddate"), "0", "\n");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("WLAN Driver");?> :</span>
		<span class="value"><?echo query("/runtime/device/wlandriver");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("LAN MAC");?> :</span>
		<span class="value"><?echo query("/runtime/devdata/lanmac");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("WAN MAC");?> :</span>
		<span class="value"><?echo query("/runtime/devdata/wanmac");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("WLAN MAC");?> :</span>
		<span class="value"><?echo query("/runtime/devdata/wlanmac");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("SSID (2.4G)");?> :</span>
		<span class="value"><?$path = XNODE_getpathbytarget("/wifi", "entry", "uid", "WIFI-1", "0"); echo get(h,$path."/ssid");?></span>
	</div>
	<div class="info" <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
		<span class="name"><?echo i18n("SSID (5G)");?> :</span>
		<span class="value"><?$path = XNODE_getpathbytarget("/wifi", "entry", "uid", "WIFI-3", "0"); echo get(h,$path."/ssid");?></span>
	</div>
	<div class="info">
		<span class="name"><?echo i18n("Factory Default");?> :</span>
		<span class="value" id="configured"></span>
	</div>
	<div class="gap"></div>
	<div class="info">
		<span class="name"></span>
		<span class="value">
			<input type="button" value="<?echo i18n("Continue");?>" onClick='self.location.href="index.php";' />
		</span>
	</div>
	<div class="emptyline"></div>
</div>
</form>
