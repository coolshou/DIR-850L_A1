<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
include "/htdocs/phplib/xnode.php";
$wan1_infp_setcfg = XNODE_getpathbytarget($SETCFG_prefix, "inf", "uid", "WAN-1", 0);
$wan1_infp = XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);

set($wan1_infp."/open_dns/type", "parent");
set($wan1_infp."/open_dns/deviceid", query($wan1_infp_setcfg."/open_dns/deviceid"));
set($wan1_infp."/open_dns/parent_dns_srv/dns1", query($wan1_infp_setcfg."/open_dns/parent_dns_srv/dns1"));
set($wan1_infp."/open_dns/parent_dns_srv/dns2", query($wan1_infp_setcfg."/open_dns/parent_dns_srv/dns2"));
?>
