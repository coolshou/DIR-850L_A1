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
function http_server($sname, $uid, $ifname, $af, $ipaddr, $port, $hnap, $widget, $smart404, $miiicasa)
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
		"	Family ".$af.							"\n";
		if($ifname != "") {	echo "	Interface ".$ifname.					"\n";}
		/*for bridge 192.168.0.50 alias ip access*/
		if(strstr($uid,"BRIDGE") != "" && $port == "80" ) { echo "#	Address ".$ipaddr.	"\n";}
		else { echo "	Address ".$ipaddr.  "\n";}
	echo	
		"	Port ".$port.							"\n".
		"	Options { nodelay On }".				"\n";
//jef add +   for support use shareport.local to access shareportmobile	
	if($web_file_access == 1) 
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
		"   Virtual".                               "\n".
		"   {".                                     "\n".
		"       HOST shareport".                           "\n".
		"       Priority 1".                        "\n".
		"       Control".                           "\n".
		"       {".                                 "\n".
        "           Alias /".                       "\n".
		"           Location http://shareport.local".  "\n".
		"       }".                                 "\n".
		'   }'.                                     '\n';
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
	if ($uid=="LAN-1"||$uid=="WAN-1")	echo
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { txt }".	"\n".
		"			}".								"\n";
	if ($widget > 0)	echo
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { router_info.xml }"."\n".
		"				/usr/sbin/phpcgi { post_login.xml }"."\n".
		"			}".								"\n";	
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
	echo
		"	}".										"\n".
		"}".										"\n";
}

function mydlink_shareport($sname, $uid, $ifname, $af, $ipaddr, $port, $hnap, $widget, $smart404)
{
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
		"	Family ".$af.							"\n";
		if($ifname != "") {	echo "	Interface ".$ifname.					"\n";}
		/*for bridge 192.168.0.50 alias ip access*/
		if($uid == "BRIDGE-1" && $port == "80" ) { echo "#	Address ".$ipaddr.	"\n";}
		else { echo "	Address ".$ipaddr.  "\n";}
	echo	
		"	Port 8182".                             "\n".
		"	Virtual".								"\n".
		"	{".										"\n".
		"		AnyHost".							"\n".
		"		Priority 1".						"\n";
	echo
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /common/info.cgi".		"\n".
		"			Location /htdocs/mydlink/info.cgi".		"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { info.cgi }"."\n".
		"			}".								"\n".
		"		}".									"\n";
	echo
		"	}".										"\n".
		"}".										"\n";
}

function ssdp_server($sname, $uid, $ifname, $af, $ipaddr)
{
	if ($af=="inet6") return;
	echo
		"Server".									"\n".
		"{".										"\n".
		"	ServerName \"".$sname."\"".				"\n".
		"	ServerId \"".$uid."\"".					"\n".
		"	Family ".$af.							"\n".
		"	Interface ".$ifname.					"\n".
		"	Port 1900".								"\n".
		"	Address 239.255.255.250".				"\n".
		"	Datagrams On".							"\n".
		"	Virtual".								"\n".
		"	{".										"\n".
		"		AnyHost".							"\n".
		"		Priority 0".						"\n".
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /".						"\n".
		"			Location /htdocs/upnp/docs/".$uid."\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/htdocs/upnp/ssdpcgi { * }"."\n".
		"			}".								"\n".
		"		}".									"\n".
		"	}".										"\n".
		"}".										"\n".
		"\n";
}

function upnp_server($sname, $uid, $ifname, $af, $ipaddr, $port)
{
	if ($af=="inet6") return;
	echo
		"Server".									"\n".
		"{".										"\n".
		"	ServerName \"".$sname."\"".				"\n".
		"	ServerId \"".$uid."\"".					"\n".
		"	Family ".$af.							"\n".
		"	Interface ".$ifname.					"\n".
		"	Address ".$ipaddr.					"\n".
		"	Port ".$port.							"\n".
		"	Virtual".								"\n".
		"	{".										"\n".
		"		AnyHost".							"\n".
		"		Priority 0".						"\n".
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /".						"\n".
		"			Location /htdocs/upnp/docs/".$uid."\n".
		"		}".									"\n".
		"	}".										"\n".
		"}".										"\n".
		"\n";
}

function stunnel_server($sname, $uid, $ifname, $af, $ipaddr, $port, $wfa_port, $stunnel, $wfa_stunnel)
{
	$mydlink = query("/mydlink/register_st");
	
	if($mydlink == "1" || $stunnel=="1")
	{
	echo
		"Server".									"\n".
		"{".										"\n".
		"	ServerName \"".$sname."\"".				"\n".
		"	ServerId \"".$uid."\"".					"\n".
		"	Family ".$af.							"\n".
		"	Interface ".$ifname.					"\n".
		"	Address ".$ipaddr.						"\n".
		"	Port ".$port.							"\n".
		"	Virtual".								"\n".
		"	{".										"\n".
		"		AnyHost".							"\n".
		"		Priority 1".						"\n";
	if ($stunnel=="1")
	{
		echo	
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /".						"\n".
		"			Location /htdocs/web".			"\n".
		"			IndexNames { index.php }".		"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { txt }".	"\n".
		"			}".								"\n".
		"		}".									"\n";
	}
                echo
                "               Control".                                                       "\n".
                "               {".                                                                     "\n".
                "                       Alias /goform".                                 "\n".
                "                       Location /htdocs/mydlink".              "\n".
                "                       PathInfo On".                                   "\n".
                "                       External".                                              "\n".
                "                       {".                                                             "\n".
                "                               /usr/sbin/phpcgi { * }".        "\n".
                "                       }".                                                             "\n".
                "                       Specials".                                              "\n".
                "                       {".                                                             "\n".
                "                               CGI {form_login form_logout }". "\n".
                "                       }".                                                             "\n".
                "               }".                                                                     "\n";
                echo
                "               Control".                                                       "\n".
                "               {".                                                                     "\n".
                "                       Alias /mydlink".                                "\n".
                "                       Location /htdocs/mydlink".              "\n".
                "                       PathInfo On".                                   "\n".
                "                       External".                                              "\n".
                "                       {".                                                             "\n".
                "                               /usr/sbin/phpcgi { * }".        "\n".
                "                       }".                                                             "\n".
                "               }".                                                                     "\n";
                echo
                "               Control".                                                       "\n".
                "               {".                                                                     "\n".
                "                       Alias /common".                                 "\n".
                "                       Location /htdocs/mydlink".              "\n".
                "                       PathInfo On".                                   "\n".
                "                       External".                                              "\n".
                "                       {".                                                             "\n".
                "                               /usr/sbin/phpcgi { cgi }".      "\n".
                "                       }".                                                             "\n".
                "               }".                                                                     "\n";
	
		echo
		"	}".										"\n".
		"}".										"\n";		
	}
	
	if ($wfa_stunnel=="1")
	{	
	echo
		"Server".									"\n".
		"{".										"\n".
		"	ServerName \"".$sname."\"".				"\n".
		"	ServerId \"".$uid."\"".					"\n".
		"	Family ".$af.							"\n".
		"	Interface ".$ifname.					"\n".
		"	Address ".$ipaddr.						"\n".
		"	Port ".$wfa_port.							"\n".
		"	Virtual".								"\n".
		"	{".										"\n".
		"		AnyHost".							"\n".
		"		Priority 1".						"\n";		
	echo
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /".						"\n".
		"			Location /htdocs/web/webaccess".			"\n".
		"			IndexNames { index.php }".		"\n";
	echo
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { txt }".	"\n".
		"			}".								"\n";
	echo
		"		}".									"\n".
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
		"		}".									"\n";
	echo
		"	}".										"\n".
		"}".										"\n";
	}
	if ($mydlink=="1")
	{	
	echo
		"Server".									"\n".
		"{".										"\n".
		"	ServerName \"".$sname."\"".				"\n".
		"	ServerId \"".$uid."\"".					"\n".
		"	Family ".$af.							"\n".
		"	Interface ".$ifname.					"\n".
		"	Address ".$ipaddr.						"\n".
		"	Port 8182".							"\n".
		"	Virtual".								"\n".
		"	{".										"\n".
		"		AnyHost".							"\n".
		"		Priority 1".						"\n";		
	echo
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /common/info.cgi".		"\n".
		"			Location /htdocs/mydlink/info.cgi".		"\n".
		"			External".						"\n".
		"			{".								"\n".
		"				/usr/sbin/phpcgi { info.cgi }"."\n".
		"			}".								"\n".
		"		}".									"\n";
	echo
		"	}".										"\n".
		"}".										"\n";
	}
}

function webaccess_server($webaccess_name, $uid, $ifname, $af, $ipaddr, $port)
{
	echo
		'Server'.									'\n'.
		'{'.										'\n'.
		"	ServerName \"".$webaccess_name."\"".	"\n".
		"	ServerId \"".$uid."\"".					"\n".
		"	Family ".$af.							"\n";
	//	if($ifname!=""){ echo "	Interface ".$ifname.					"\n";}
	echo	
		"	Address ".$ipaddr.						"\n".
		"	Port ".$port.							"\n".
		'	Virtual'.								'\n'.
		'	{'.										'\n'.
		"		AnyHost".							"\n".
		"		Priority 1".						"\n".
		"		Control".							"\n".
		"		{".									"\n".
		"			Alias /".						"\n".
		"			Location /htdocs/web/webaccess".			"\n".
		"			IndexNames { index.php }".		"\n";
	echo		
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
		'	}'.										'\n'.
		'}'.										'\n';		
}
$webaccess = query("/webaccess/enable");
$webaccess_http = query("/webaccess/httpenable");
$wfa_port = query("/webaccess/httpport");
$have_http = 0;
$model	= query("/runtime/device/modelname");
$ver	= query("/runtime/device/firmwareversion");
$smart404 = query("/runtime/smart404");
$sname	= "Linux, HTTP/1.1, ".$model." Ver ".$ver;	/* HTTP server name */
$suname = "Linux, UPnP/1.0, ".$model." Ver ".$ver;	/* UPnP server name */
$stunnel_name = "Linux, STUNNEL/1.0, ".$model." Ver ".$ver;	/* STUNNEL server name */
$webaccess_name = "Linux, WEBACCESS/1.0, ".$model." Ver ".$ver;	/* WEBACCESS server name */	
foreach("/runtime/services/http/server")
{
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
	if($ifname!="" || $ipaddr!="")
	{	
		if ($af!="")
		{
//			if		($mode=="HTTP") http_server($sname, $inf,$ifname,$af,$ipaddr,$port,$hnap,$widget,$smart404);
			/*if($mode=="HTTP" && $http_done == 0)  //do interface bind  
			{
				http_server($sname, $inf,$ifname,$af,$ipaddr,$port,$hnap,$widget,$smart404);
				$http_done = 1;
			}*/
			if($mode=="HTTP") 
			{
				$have_http = 1;
				http_server($sname, $inf,$ifname,$af,$ipaddr,$port,$hnap,$widget,$smart404);
				mydlink_shareport($sname, $inf,$ifname,$af,$ipaddr,$port,$hnap,$widget,$smart404);
			}
			else if	($mode=="SSDP") ssdp_server($sname, $inf,$ifname,$af,$ipaddr);
			else if	($mode=="UPNP") upnp_server($suname,$inf,$ifname,$af,$ipaddr,$port);
			else if	($mode=="STUNNEL") stunnel_server($stunnel_name,$inf,$ifname,$af,$ipaddr,$port,$wfa_port,$stunnel,$wfa_stunnel);
			else if	($mode=="WEBACCESS") webaccess_server($webaccess_name,$inf,$ifname,$af,$ipaddr,$port);
		}
	}
}

?>
