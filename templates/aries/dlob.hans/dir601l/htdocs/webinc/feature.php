<?
include "/htdocs/phplib/trace.php";

$FEATURE_NOSCH = 0;			/* if this model doesn't support scheudle, set it as 1. */
$FEATURE_NOPPTP = 0;		/* if this model doesn't support PPTP, set it as 1. */
$FEATURE_NOL2TP = 0;		/* if this model doesn't support L2TP, set it as 1.*/

if(query("/runtime/devdata/countrycode")=="RU")
{
	$FEATURE_NORUSSIAPPTP = 0;	/* if this model doesn't support Russia PPTP, set it as 1.*/
	$FEATURE_NORUSSIAPPPOE = 0;	/* if this model doesn't support Russia PPPoE, set it as 1. */
	$FEATURE_NORUSSIAL2TP = 0; 	/* if this model doesn't support Russia L2TP, set it as 1. */		
}
else
{
	$FEATURE_NORUSSIAPPTP = 1;	/* if this model doesn't support Russia PPTP, set it as 1.*/
	$FEATURE_NORUSSIAPPPOE = 1;	/* if this model doesn't support Russia PPPoE, set it as 1. */
	$FEATURE_NORUSSIAL2TP = 1; 	/* if this model doesn't support Russia L2TP, set it as 1. */
}

$FEATURE_DHCPPLUS = 0;		/* if this model supports DHCP+, set it as 1. */

$FEATURE_NOEASYSETUP = 1;	/* if this model has no easy setup page, set it as 1. */
$FEATURE_NOIPV6 = 0;	/* if this model has no IPv6, set it as 1. */
$FEATURE_NOAPMODE = 0;
$FEATURE_INBOUNDFILTER = 1;	/* if this model has inbound filter, set it as 1.*/
$FEATURE_DUAL_BAND = 0;		/* if this model has 5 Ghz, set it as 1.*/
$FEATURE_WAN1000FTYPE = 0;  /* if this model wan port has giga speed(1000Mbps), set it as 1.*/
$FEATURE_PARENTALCTRL = 0;  /* if this model has parental control(OpenDNS service) function, set it as 1.*/
$FEATURE_EEE = 0;  /* if this model has Energy Efficient Ethernet, set it as 1.*/
$FEATURE_NOGUESTZONE = 1; /* if this model doesn't support Guest zone, set it as 1.*/
$FEARURE_UNCONFIGURABLEQOS = 1;
?>
