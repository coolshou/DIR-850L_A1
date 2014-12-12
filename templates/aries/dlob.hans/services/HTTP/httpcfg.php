Umask 026
PIDFile /var/run/httpd.pid
#LogGMT On
#ErrorLog /dev/console

Tuning
{
	#many services now use http, need enlarge this.
	NumConnections 128
	BufSize 12288
	InputBufSize 4096
	ScriptBufSize 4096
	NumHeaders 100
	Timeout 60
	ScriptTimeout 60
}

Control
{
	Types
	{
		text/html	{ html htm }
		text/xml	{ xml }
		text/plain	{ txt }
		image/gif	{ gif }
		image/jpeg	{ jpg }
		text/css	{ css }
		application/octet-stream { * }
	}
	Specials
	{
		Dump		{ /dump }
		CGI			{ cgi }
		Imagemap	{ map }
		Redirect	{ url }
	}
	External
	{
		/usr/sbin/phpcgi { php txt asp }
		/usr/sbin/scandir.sgi {sgi}
	}
}

<?
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/trace.php";

function echo_mydlink_http_control()
{
	//+++ for mydlink cloud cgi
	echo
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /goform".					"\n".
		"			Location /htdocs/mydlink".		"\n".
		"			PathInfo On".					"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { * }".	"\n".
		"			}".								"\n".
		"			Specials".						"\n".
		"			{".								"\n".
		"				CGI {form_login form_logout }".	"\n".
		"			}".								"\n".
		"		}".									"\n";
	echo
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /mydlink".				"\n".
		"			Location /htdocs/mydlink".		"\n".
		"			PathInfo On".					"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { * }".	"\n".
		"			}".								"\n".
		"		}".									"\n";
	echo
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /common".					"\n".
		"			Location /htdocs/mydlink".		"\n".
		"			PathInfo On".					"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { cgi }".	"\n".
		"			}".								"\n".
		"		}".									"\n";
}

function http_server($sname, $uid, $ifname, $af, $ipaddr, $port, $hnap, $widget, $smart404)
{
	$web_file_access = query("/webaccess/enable"); //jef add +   for support use shareport.local to access shareportmobile	
	
	$uid_prefix = cut($uid, 0, "-"); 
	/*if wan interface web server we do not bind interace for local loopback*/
	if($uid_prefix=="WAN")
	{
		$ifname = "";
	}
	echo
		"Server".									"\n".
		"{".										"\n".
		"	ServerName \"".$sname."\"".				"\n".
		"	ServerId \"".$uid."\"".					"\n".
		"	Family ".$af.						"\n";
		if($ifname != "") {	echo "	Interface ".$ifname."\n";}
	echo	
		"	Address ".$ipaddr.						"\n".
		"	Port ".$port.							"\n";
//jef add +   for support use shareport.local to access shareportmobile	
	if($web_file_access == "1") 
	{
		echo
		'	Virtual'.								'\n'.
		'	{'.										'\n'.
		"		HOST shareport.local".				"\n".
		"		Priority 1".						"\n".
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /".						"\n".
		"			Location /htdocs/web/webaccess".			"\n".
		"			IndexNames { index.php }".		"\n".
		"		}".                             	"\n".		
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /dws".					"\n".
		"			Location /htdocs/fileaccess.cgi".	"\n".
		"			PathInfo On".                   "\n".		
		"			External".						"\n".
		"			{".								"\n".
		"				/htdocs/fileaccess.cgi { * }"."\n".
		"			}".								"\n".
		"		}".                             	"\n".
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /dws/api/Login".			"\n".
		"			Location /htdocs/web/webfa_authentication.cgi".	"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/htdocs/web/webfa_authentication.cgi { * }"."\n".
		"			}".								"\n".
		"		}".                             	"\n".
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /dws/api/Logout".			"\n".
		"			Location /htdocs/web/webfa_authentication_logout.cgi".	"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/htdocs/web/webfa_authentication_logout.cgi { * }"."\n".
		"			}".								"\n".
		"		}".                             	"\n".
		'	}'.										'\n';
		echo
		"	Virtual".                               "\n".
		"	{".                                     "\n".
		"		HOST shareport".                    "\n".
		"		Priority 1".                        "\n".
		"		Control".                           "\n".
		"		{".                                 "\n".
        "			Alias /".                       "\n".
		"			Location http://shareport.local".  "\n".
		"		}".                                 "\n".
		'	}'.                                     '\n';
	}
//jef -
	echo	
		"	Virtual".								"\n".
		"	{".										"\n".
		"		AnyHost".							"\n".
		"		Priority 1".						"\n".
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /".						"\n".
		"			Location /htdocs/web".			"\n".
		"			IndexNames { index.php }".		"\n";
	if ($uid=="LAN-1"||$uid=="WAN-1")
	{
		echo
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { txt }".	"\n".
		"			}".								"\n";
	}
	if ($widget > 0)
	{
		echo
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { router_info.xml }"."\n".
		"				/usr/sbin/phpcgi { post_login.xml }"."\n".
		"			}".								"\n";
	}
	echo
		"		}".									"\n";
	echo
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /parentalcontrols".		"\n".
		"			Location /htdocs/parentalcontrols"."\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { php }".	"\n".
		"			}".								"\n".
		"		}".									"\n";
	if ($smart404 == "1")
	{
		echo
		'       Control'.                           '\n'.
		'       {'.                                 '\n'.
		'           Alias /smart404'.               '\n'.
		'           Location /htdocs/smart404'.     '\n'.
		'       }'.                                 '\n';
	}
	if ($hnap > 0)
	{
		echo
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /HNAP1".					"\n".
		"			Location /htdocs/HNAP1".		"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/hnap { hnap }".	"\n".
		"			}".								"\n".
		"			IndexNames { index.hnap }".		"\n".
		"		}".									"\n";
	}
	echo_mydlink_http_control();
	echo
		"	}".										"\n".
		"}".										"\n";
}

$wfa_port = query("/webaccess/httpport");
foreach("/runtime/services/http/server")
{
	$model	= query("/runtime/device/modelname");
	$ver	= query("/runtime/device/firmwareversion");
	$smart404 = query("/runtime/smart404");
	$sname	= "Linux, HTTP/1.1, ".$model." Ver ".$ver;	/* HTTP server name */
	$suname = "Linux, UPnP/1.0, ".$model." Ver ".$ver;	/* UPnP server name */
	$stunnel_name = "Linux, STUNNEL/1.0, ".$model." Ver ".$ver;	/* STUNNEL server name */
	$webaccess_name = "Linux, WEBACCESS/1.0, ".$model." Ver ".$ver;	/* WEBACCESS server name */	
	$mode 	= query("mode");
	$inf	= query("inf");
	$ifname	= query("ifname");
	$ipaddr	= query("ipaddr");
	$port	= query("port");
	$hnap	= query("hnap");
	$widget = query("widget");
	$af		= query("af");
	
	$stunnel = query("stunnel");
	$wfa_stunnel = query("wfa_stunnel");
/*
	if($ifname!="" || $ipaddr!="")
	{
		if ($af!="")
		{
			if		($mode=="HTTP") http_server($sname, $inf,$ifname,$af,$ipaddr,$port,$hnap,$widget,$smart404);
			else if	($mode=="SSDP") ssdp_server($sname, $inf,$ifname,$af,$ipaddr);
			else if	($mode=="UPNP") upnp_server($suname,$inf,$ifname,$af,$ipaddr,$port);
			else if	($mode=="STUNNEL") stunnel_server($stunnel_name,$inf,$ifname,$af,$ipaddr,$port$wfa_port,$stunnel,$wfa_stunnel);
			else if	($mode=="WEBACCESS") webaccess_server($webaccess_name,$inf,$ifname,$af,$ipaddr,$port);
		}
	}*/
}

?>