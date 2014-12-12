<?
include "/htdocs/phplib/trace.php"; 
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/inf.php";

fwrite("w",$START, "#!/bin/sh\n");
fwrite("w",$STOP,  "#!/bin/sh\n");

function startcmd($cmd)	{fwrite(a,$_GLOBALS["START"], $cmd."\n");}
function stopcmd($cmd)	{fwrite(a,$_GLOBALS["STOP"], $cmd."\n");}
function http_error($errno)
{
}

// Create /var/run/storage_account_root
function get_alldisklist($rw)
{
	$storagelist = "";
	foreach("/webaccess/device/entry")
	{ 
		//iphone app enable only one device at same time
		$valid = query("valid");
		if($valid == "1")
		{
			foreach("entry")
			{
				$storagelist = $storagelist.",".query("uniquename").":".query("uniquename")."::".$rw;
			}
		}
	}
	return $storagelist;
}

function comma_handle($password) //password 
{
	// because of ',' is special charcter in storage_account_root
	// so we add '\' in front of ',' to do special handle
	// handle '\' first then ','
	// admin only, i block special character use in creating WFA normal user account
	//start 
	$bslashcount = cut_count($password, "\\");
	$count_tmp = 1;
	if($bslashcount > 0)
		$tmp_pass = cut($password, 0, "\\");
	else
		$tmp_pass = "";
	while($count_tmp < $bslashcount)
	{
		$tmp_str = cut($password, $count_tmp, "\\");
		$tmp_pass = $tmp_pass ."\\\\".$tmp_str;
		$count_tmp = $count_tmp + 1;
	}
	if($bslashcount > 0)
		$password = $tmp_pass;
		
		
	$commacount = cut_count($password, ",");
	$count_tmp = 1;
	if($bslashcount > 0)
		$tmp_pass = cut($password, 0, ",");
	else
		$tmp_pass = "";
	while($count_tmp < $commacount)
	{
		$tmp_str = cut($password, $count_tmp, ",");
		$tmp_pass = $tmp_pass ."\\,".$tmp_str;
		$count_tmp = $count_tmp + 1;
	}
	if($commacount > 0)
		return $tmp_pass;
	else
		return $password;
	//end
}
function setup_wfa_account()
{
	$ACCOUNT = "/var/run/storage_account_root";
	/*for the admin is special*/
	$admin_path = XNODE_getpathbytarget("/device/account", "entry", "name", "Admin", 0);
	$admin_passwd = query($admin_path."/password");
	$admin_disklist=get_alldisklist("rw");
	$check_dist=get_alldisklist();

	$admin_passwd = comma_handle($admin_passwd);
	
	/* note1: we hope even if there has no partition on the device,
	   the user still can login sharePort (but see nothing),
	   instead of showing the "authentication fail message" */
	if($admin_disklist==""){$admin_disklist=",:::";}
	
	fwrite("w", $ACCOUNT, "admin:".$admin_passwd.$admin_disklist."\n");
	
	foreach("/webaccess/account/entry")
	{
		if(tolower(query("username"))==tolower("Admin"))
		{
			continue;	
		}
		$storage_msg = "";
		$idx = 0;
		foreach("entry")
		{
			if(tolower(query("path"))=="root")
			{
				$rw = query("permission");
				$storage_msg = get_alldisklist($rw);
			}
			else if(tolower(query("path"))=="none")   //jef add to allow user login without set any path
			{
				$storage_msg = ",:::";
			}
			else  // if(tolower(query("path")) !="none")
			{
				$device = cut(query("path"), 0, ":");	
				$path = cut(query("path"), 1, ":");
				if(isempty($path)==1)
				{
					//$path=$device;
					$alias=$device;
				}
				else
				{
					$count = cut_count($path, "/") - 1;
					$alias = cut($path, $count, "/");
				}
				$permission = query("permission");
				
				if(strstr($storage_msg, $alias)>=0 && tolower($path) !="")
				{
					$idx=$idx+1;
					$storage_msg = $storage_msg.",".$device.":".$alias."_".$idx.":".$path.":".$permission;
				}
				else
				{$storage_msg = $storage_msg.",".$device.":".$alias.":".$path.":".$permission;}
			}		
		}
			
	//	if($storage_msg!="")
	//	{
			if($check_dist==""){$storage_msg=",:::";} //same with note1
			$user_passwd = query("passwd");
			$user_passwd = comma_handle($user_passwd);
			fwrite("a", $ACCOUNT, query("username").":".$user_passwd.$storage_msg."\n");
	//	}	
	}
}

/*  prepare data for http to create httpd.conf (service WEBACCESS) */
function webaccesssetup($name)
{
	/* Get the interface */
	$infp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);

	if ($infp=="")
	{
		SHELL_info($_GLOBALS["START"], "webaccesssetup: (".$name.") not exist.");
		SHELL_info($_GLOBALS["STOP"],  "webaccesssetup: (".$name.") not exist.");
		http_error("9");
		return;
	}

	/* Get the "runtime" physical interface */
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);

	if ($stsp!="")
	{
		$phy = query($stsp."/phyinf");
		if ($phy!="")
		{
			$phyp = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $phy, 0);
			if ($phyp!="" && query($phyp."/valid")=="1")
				$ifname = query($phyp."/name");
		}
	}

	/* Get address family & IP address */
	$atype = query($stsp."/inet/addrtype");

	if      ($atype=="ipv4") {$af="inet"; $ipaddr=query($stsp."/inet/ipv4/ipaddr");}
	else if ($atype=="ppp4") {$af="inet"; $ipaddr=query($stsp."/inet/ppp4/local");}
	else if ($atype=="ipv6") {$af="inet6";$ipaddr=query($stsp."/inet/ipv6/ipaddr");}
	else if ($atype=="ppp6") {$af="inet6";$ipaddr=query($stsp."/inet/ppp6/local");}

	if($af != "inet")
	{
		SHELL_info($_GLOBALS["START"], "webaccesssetup: (".$name.") not ipv4.");
		SHELL_info($_GLOBALS["STOP"],  "webaccesssetup: (".$name.") not ipv4.");
		http_error("9");
		return;
	}

	if ($ifname==""||$af==""||$ipaddr=="")
	{
		SHELL_info($_GLOBALS["START"], "webaccesssetup: (".$name.") no phyinf.");
		SHELL_info($_GLOBALS["STOP"],  "webaccesssetup: (".$name.") no phyinf.");
		http_error("9");
		return;
	}
	$webaccess = query("/webaccess/enable");
	$port = query("/webaccess/httpport");
	if($port == "")	$port=8181;
    //if($name=="LAN-1")  //jef mark 2013/05/31
	if($name=="LAN-1" || $name=="WAN-1")  
	//enalbe wan side port listen for DirectServer API. 
	//iptables will block connect except DirectServer
	//if remote access enabled, iptables will allow all connection
	{
		$webaccess_http=1;
		$dirty = 0;
	}
	else
	{
		
   		$webaccess_http = query("webaccess/httpenable");
   		$dirty = 0;
   		$ifname="";
	}
	
	
	$stsp = XNODE_getpathbytarget("/runtime/services/http", "server", "uid", "WEBACCESS.".$name, 0);
	if ($stsp=="")
	{
		if ($webaccess != 1)
		{
			SHELL_info($_GLOBALS["START"], "webaccesssetup: (".$name." with code ".$webaccess.") not active.");
			SHELL_info($_GLOBALS["STOP"],  "webaccesssetup: (".$name." with code ".$webaccess.") not active.");
			http_error("8");
			return;
		}
		else
		{
			$dirty++;
			if($webaccess_http=="1")
			{
				$stsp = XNODE_getpathbytarget("/runtime/services/http","server","uid","WEBACCESS.".$name,1);
				set($stsp."/mode",  "WEBACCESS");
				set($stsp."/inf",   $name);
				set($stsp."/ifname", $ifname);
				set($stsp."/ipaddr",$ipaddr);
				set($stsp."/port",  $port);
				set($stsp."/af",    $af);
			}
		}
	}
	else
	{
		if ($webaccess != 1) { $dirty++; del($stsp);}
		else
		{
			if($webaccess_http=="1")
			{
				if (query($stsp."/mode")!="WEBACCESS")   { $dirty++; set($stsp."/mode", "WEBACCESS"); }
				if (query($stsp."/inf")!=$name)         { $dirty++; set($stsp."/inf", $name); }
				if (query($stsp."/ifname")!=$ifname)    { $dirty++; set($stsp."/ifname", $ifname); }
				if (query($stsp."/ipaddr")!=$ipaddr)    { $dirty++; set($stsp."/ipaddr", $ipaddr); }
				if (query($stsp."/port")!=$port)        { $dirty++; set($stsp."/port", $port); }
				if (query($stsp."/af")!=$af)            { $dirty++; set($stsp."/af", $af); }
			}
		}
	}

	stopcmd('sh /etc/scripts/delpathbytarget.sh runtime/services/http server uid WEBACCESS.'.$name);
}


function check_portfw_range($port, $port_list)
{
    $ret=0;
    $cnt = cut_count($port_list, ",");
    $idx = 0;
    while ($idx <= $cnt)
    {
        if($idx > 0)
            {$t_port = cut($port_list,$idx,",");}
        else
            {$t_port=$port_list;}
        if($port==$t_port)
            {return 1;}
        else if (cut_count($t_port, "-") > 1)
        {
            $t_port_s = cut($t_port,0,"-");
            $t_port_e = cut($t_port,1,"-");

            if(strtoul($port, 10) >= strtoul($t_port_s, 10))
            {
                if(strtoul($port, 10) <= strtoul($t_port_e, 10))
                    {return 1;}
            }
        }
        $idx++;
    }
    return $ret;
}


function check_port_conflict($port)  //check public_ds_port conflict with Portforward or VirtualServer
{                                
    $ret=0;                      
    $pwd_count=query("/nat/entry/portforward/entry#");
    foreach("/nat/entry/portforward/entry")           
    {                                                 
          if(query("enable")=="1")         
          {                       
              if(check_portfw_range($port, query("tport_str"))=="1")
              {
                  return 1;
              }
          }                
    }          
    foreach("/nat/entry/virtualserver/entry")
    {                                        
        if(query("enable")=="1")             
        {                       
            if(query("external/start")==$port)
            {                                 
                return 1;                     
            }            
        }    
    }                                         
    return $ret;                              
}                                             


function check_usable_public_port()  //for GetDirectServer API
{                                     
    set("/runtime/webaccess/public_ds_port", query("/webaccess/httpport"));

    if(check_port_conflict("80")=="0")
    {
        set("/runtime/webaccess/public_ds_port", 80);
        return;
    }
    if(check_port_conflict("443")=="0")
    {
        set("/runtime/webaccess/public_ds_port", 443);
        return;
    }
    if(check_port_conflict("21")=="0")
    {
        set("/runtime/webaccess/public_ds_port", 21);
        return;
    }
    if(check_port_conflict("20")=="0")
    {
        set("/runtime/webaccess/public_ds_port", 20);
        return;
    }
}

if(query("webaccess/enable")=="1") 
{
	setup_wfa_account();
	webaccesssetup("WAN-1");
	webaccesssetup("LAN-1");
	check_usable_public_port();
}

/* start HTTP service */
if ($dirty>0) $action="restart"; else $action="start";


$partition_count = query("/runtime/device/storage/disk/count");

startcmd("killall -9 fileaccessd"); //jef
startcmd("fileaccessd &"); //jef

startcmd("iptables -t nat -F PRE.WFA"); //jef
startcmd("service STUNNEL restart");
startcmd("service HTTP restart");
startcmd("service IPT.WAN-1 restart");
startcmd("xmldbc -P /etc/scripts/wfa_igd_handle.php -V MODE=WAN_IP &"); //jef

/*
if($partition_count =="" || $partition_count=="0")
{
	fwrite("a", $START, "echo \"No HD found\"  > /dev/console\n");
	startcmd("service WEBACCESS stop");
}
*/
startcmd("exit 0");

stopcmd("killall -9 upnpc"); //jef
stopcmd("killall -9 fileaccessd"); //jef
stopcmd("service HTTP restart");
stopcmd("rm /var/run/storage_account_root");
stopcmd("exit 0");

?>
