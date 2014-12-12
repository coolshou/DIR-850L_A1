TEST PHP file for sealpac.
--------------------------

<?

if ($test == "test")
{
	echo i18n("The value of the variable 'test' is '$1'.!", $test);
	echo "\n";
}
else
{
	echo i18n("The value of the variable 'test' is not '$1'!\n", $test);
}

$var1 = i18n("IP address");
$var2 = i18n("Subnet Mask");
$var3 = i18n("Default Gateway");
$var4 = i18n("DHCP server");
$var5 = i18n("OK");
$var6 = i18n("Fail");
$var7 = i18n("Start IP");
$var8 = i18n("Apply");

$m_title_ap_mode    = i18n("Access Point Mode");
$m_desc_ap_mode     = i18n("Use this to disable NAT on the router and turn it into an Access Point.");
$m_enable_ap_mode   = i18n("Enable Access Point Mode");
$m_title_wan_type   = i18n("Internet Connection Type");
$m_desc_wan_type    = i18n("Choose the mode to be used by the router to connect to the Internet.");

$m_wan_type     = i18n("My Internet Connection is");
$m_static_ip    = i18n("Static IP");
$m_dhcp         = i18n("Dynamic IP (DHCP)");
$m_username     = i18n("Username");
$m_password     = i18n("Password");
$m_pppoe        = i18n("PPPoE");
$m_pptp         = i18n("PPTP");
$m_l2tp         = i18n("L2TP");
$m_russia       = i18n("Russia");
$m_dualaccess   = i18n("Dual Access");
$m_mppe         = i18n("MPPE");

$m_title_static = i18n("Static IP Address Internet Connection Type");
$m_desc_static  = i18n("Enter the static address information provided by your Internet Service Provider (ISP).");

$m_comment_isp  = i18n("(assigned by your ISP)");
$m_subnet       = i18n("Subnet Mask");
$m_isp_gateway  = i18n("ISP Gateway Address");
$m_macaddr      = i18n("MAC Address");
$m_optional     = i18n("optional");
$m_clone_mac    = i18n("Clone MAC Address");
$m_primary_dns  = i18n("Primary DNS Address");
$m_secondary_dns= i18n("Secondary DNS Address");
$m_mtu          = i18n("MTU");

$m_title_dhcp   = i18n("Dynamic IP (DHCP) Internet Connection Type");
$m_desc_dhcp    = i18n("Use this Internet connection type if your Internet Service Provider (ISP) didn't provide you with IP Address information and/or a username and password.");

$m_host_name        = i18n("Host Name");
$m_ppp_idle_time    = i18n("Maximum Idle Time");
$m_ppp_connect_mode = i18n("Connect mode select");
$m_always_on        = i18n("Always-on");
$m_manual           = i18n("Manual");
$m_on_demand        = i18n("Connect-on demand");

$__info_from_isp    = i18n("Enter the information provided by your Internet Service Provider (ISP).");

$m_title_pppoe          = i18n("PPPoE");
$m_title_russia_pppoe   = i18n("Russia PPPOE (DUAL Access)");
$m_desc_pppoe           = $__info_from_isp;
$m_title_physical       = i18n("WAN Physical Setting");

$m_dynamic_pppoe    = i18n("Dynamic PPPoE");
$m_static_pppoe     = i18n("Static PPPoE");
$m_retype_pwd       = i18n("Retype Password");
$m_pppoe_svc_name   = i18n("Service Name");
$m_minutes          = i18n("Minutes");
$m_auto_dns         = i18n("Receive DNS from ISP");
$m_manual_dns       = i18n("Enter DNS Manually");

$m_title_pptp           = i18n("PPTP");
$m_title_russia_pptp    = i18n("Russia PPTP (DUAL Access)");
$m_desc_pptp            = $__info_from_isp;

$m_title_l2tp   = i18n("L2TP");
$m_desc_l2tp    = $__info_from_isp;

$m_dynamic_ip       = i18n("Dynamic IP");
$m_static_ip        = i18n("Static IP");
$m_gateway          = i18n("Gateway");
$m_dns              = i18n("DNS");
$m_server_ip        = i18n("Server IP/Name");
$m_pptp_account     = i18n("PPTP Account");
$m_pptp_password    = i18n("PPTP Password");
$m_pptp_retype_pwd  = i18n("PPTP Retype Password");
$m_l2tp_account     = i18n("L2TP Account");
$m_l2tp_password    = i18n("L2TP Password");
$m_l2tp_retype_pwd  = i18n("L2TP Retype Password");

$m_auth_server  = i18n("Auth Server");
$m_login_server = i18n("Login Server IP/Name");

$a_invalid_ip       = i18n("Invalid IP address !");
$a_invalid_netmask  = i18n("Invalid subnet mask !");
$a_invalid_mac      = i18n("Invalid MAC address !");
$a_invalid_mtu      = i18n("Invalid MTU value !");
$a_invalid_hostname = i18n("Invalid host name !");
$a_invalid_username = i18n("Invalid user name !");
$a_password_mismatch= i18n("The confirm password does not match the new password !");
$a_invalid_idletime = i18n("Invalid idle time !");

$a_srv_in_different = i18n("Invalid server IP address ! The server and router addresses should be in the same network.");
$a_gw_in_different  = i18n("Invalid gateway IP address ! The gateway and router addresses should be in the same network.");
$a_server_empty     = i18n("Server IP/Name can not be empty !");
$a_account_empty    = i18n("Account can not be empty !");

$test = I18N("h", "This is a test case for I18N !!!");

?>

some test here <? $i18n = i18n("somthing here need i18n !!"); echo $i18n; ?> no more test.
