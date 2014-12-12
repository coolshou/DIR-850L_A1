<?

include "/htdocs/phplib/trace.php";

/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */
set("/acl/anti_spoof/enable",	query($SETCFG_prefix."/acl/anti_spoof/enable"));
set("/acl/dos/enable",	query($SETCFG_prefix."/acl/dos/enable"));
set("/acl/spi/enable",	query($SETCFG_prefix."/acl/spi/enable"));
set("/acl/applications/qq/action",       query($SETCFG_prefix."/acl/applications/qq/action"));
set("/acl/applications/msn/action",       query($SETCFG_prefix."/acl/applications/msn/action"));
set("/acl/applications/kaixin/action",       query($SETCFG_prefix."/acl/applications/kaixin/action"));

set("/device/passthrough/pptp",	query($SETCFG_prefix."/acl/alg/pptp"));
set("/device/passthrough/rtsp",	query($SETCFG_prefix."/acl/alg/rtsp"));
set("/device/passthrough/sip",		query($SETCFG_prefix."/acl/alg/sip"));
set("/device/passthrough/ipsec",		query($SETCFG_prefix."/acl/alg/ipsec"));

TRACE_error("==================================" );
$test=query($SETCFG_prefix."/acl/cone/udp_cone");
TRACE_error($test );
set("/acl/cone/udp_cone",	query($SETCFG_prefix."/acl/cone/udp_cone"));
set("/acl/cone/tcp_cone",	query($SETCFG_prefix."/acl/cone/tcp_cone"));
?>
