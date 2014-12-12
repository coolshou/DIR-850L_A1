<?
include "/htdocs/phplib/xnode.php";
?>
<form id="mainform" onsubmit="return false;">
<div class="orangebox">
	<h1>Version</h1>
	<div class="emptyline"></div>
	<div class="info">
		<span class="name">Bootcode Version :</span>
		<span class="value"><?echo query("/runtime/device/bootver");?></span>
	</div>
	<div class="info">
		<span class="name">Firmware External Version :</span>
		<span class="value">V<?echo cut(fread("", "/etc/config/buildver"), "0", "\n");?></span>
	</div>
	<div class="info">
		<span class="name">Firmware Internal Version :</span>
		<span class="value">V<?echo cut(fread("", "/etc/config/buildver"), "0", "\n");?><?echo cut(fread("", "/etc/config/buildrev"), "0", "\n");?></span>
	</div>
	<div class="info">
		<span class="name">Language Package :</span>
		<span class="value" id="langcode" style="text-transform:uppercase;"></span>
	</div>
	<div class="info">
		<span class="name">Date :</span>
		<span class="value"><?echo query("/runtime/device/date");?></span>
	</div>
	<div class="info">
		<span class="name">CheckSum :</span>
		<span class="value" id="checksum"></span>
	</div>
	<div class="info">
		<span class="name">WAN MAC :</span>
		<span class="value" style="text-transform:uppercase;"><?echo query("/runtime/devdata/wanmac");?></span>
	</div>
	<div class="info">
		<span class="name">LAN MAC :</span>
		<span class="value" style="text-transform:uppercase;"><?echo query("/runtime/devdata/lanmac");?></span>
	</div>
	<div class="info">
		<span class="name">WLAN MAC0 :</span>
		<span class="value" style="text-transform:uppercase;"><?echo query("/runtime/devdata/wlanmac");?></span>
	</div>
	<div class="info" <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
		<span class="name">WLAN MAC1 :</span>
		<span class="value" style="text-transform:uppercase;"><?echo query("/runtime/devdata/wlan5mac");?></span>
	</div>
	<div class="info">
		<span class="name">WLAN Domain(2.4GHz) :</span>
		<span class="value" style="text-transform:uppercase;"><?echo query("/runtime/devdata/countrycode");?></span>
	</div>
	<div class="info" <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
		<span class="name">WLAN Domain(5GHz) :</span>
		<span class="value" style="text-transform:uppercase;"><?echo query("/runtime/devdata/countrycode");?></span>
	</div>
<?
	if (isfile("/htdocs/webinc/body/version_3G.php")==1)
		dophp("load", "/htdocs/webinc/body/version_3G.php");
?>
	<div class="info">
		<span class="name">Firmware Query :</span>
		<span class="value" id="fwq"></span>
	</div>
	<div class="info">
		<span class="name">Kernel :</span>
		<span class="value"><?echo cut(fread("", "/proc/version"), "0", "(");?></span>
	</div>
	<div class="info">
		<span class="name">Apps :</span>
		<span class="value"><?echo cut(fread("", "/etc/config/builddate"), "0", "\n");?></span>
	</div>
	<div class="info">
		<span class="name">WLAN Driver :</span>
		<span class="value"><?echo query("/runtime/device/wlandriver");?></span>
	</div>
	<div class="info">
		<span class="name">SSID(2.4GHz) :</span>
		<pre style="font-family:Tahoma"><span class="value"><?$path = XNODE_getpathbytarget("/wifi", "entry", "uid", "WIFI-1", "0"); echo get(h,$path."/ssid");?></span></pre>
	</div>
	<div class="info" <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
		<span class="name">SSID(5GHz) :</span>
		<pre style="font-family:Tahoma"><span class="value"><?$path = XNODE_getpathbytarget("/wifi", "entry", "uid", "WIFI-3", "0"); echo get(h,$path."/ssid");?></span></pre>
	</div>
	<div class="info">
		<span class="name">2.4GHz Channel :</span>
		<span class="value" style="word-break:break-all">
		<?
			$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "BAND24G-1.1", "0");
			echo get(j,$path."/channel_list");
		?>
		</span>
	</div>
	<div class="info" <?if ($FEATURE_DUAL_BAND!="1") echo 'style="display:none;"';?>>
		<span class="name">5GHz Channel :</span>
		<span class="value" style="word-break:break-all">
		<?
			$path = XNODE_getpathbytarget("/runtime", "phyinf", "uid", "BAND5G-1.1", "0");
			echo get(j,$path."/channel_list");
		?>
		</span>
	</div>
	<div class="info">
		<span class="name">Factory Default :</span>
		<span class="value" id="configured"></span>
	</div>
	<div class="gap"></div>
	<div class="info">
		<span class="name"></span>
		<span class="value">
			<input type="button" value="Continue" onClick='self.location.href="index.php";' />
		</span>
	</div>
	<div class="emptyline"></div>
</div>
</form>
