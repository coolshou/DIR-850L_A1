<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

$conf_base = "/var/run";
$zebra_conf = $conf_base."/zebra.conf";
$ripd_conf = $conf_base."/ripd.conf";
$log_base = "/var/log";
$zebra_log = $log_base."/zebra.log";
$ripd_log = $log_base."/ripd.log";

$IPV6=isfile("/proc/net/if_inet6");

if ($IPV6==1)
{
    $ripng_cnt = query("/route6/dynamic/ripng/count");
    $enable_ripngd = 0;
    if($ripng_cnt > 0)
    {
      	$i = 0;
      	foreach ("/route6/dynamic/ripng/entry")
    	{ 
    	  $i++;
    	  $interfaceup= query("/route6/dynamic/ripng/entry:".$i."/enable");
    	  if($interfaceup == 1)	 
    	  { 
    	    $enable_ripngd = 1;
    	    break;
    	  }	  
    	}
    }
	$ripngd_conf = $conf_base."/ripngd.conf";
	$ripngd_log = $log_base."/ripngd.log";
}

/* Create zebra config file */
$hostname = query("/device/hostname");
fwrite("w", $zebra_conf,
		"hostname ".$hostname."\n!\n");

    $rip_cnt = query("/route/dynamic/rip/count");
    $enable_ripd = 0;
    if($rip_cnt > 0)
    {
      	$i = 0;
      	foreach ("/route/dynamic/rip/entry")
    	{
    	  $i++;
    	  $interfaceup= query("/route/dynamic/rip/entry:".$i."/enable");
    	  if($interfaceup == 1)	 
    	  { 
    	    $enable_ripd = 1;
    	    break;
    	  }	  
    	}
    }
function get_comm_rip($cfgfile)
{  
	fwrite("a", $cfgfile,
			"redistribute connected\n".
			"redistribute static\n".
			"redistribute kernel\n!\n");
			
}
/* Create ripngd config file */

if ($IPV6 == 1 && $enable_ripngd == 1)
{
	fwrite("w", $ripngd_conf,
			"hostname ".$hostname."\n!\n".
			"router ripng\n");
}
if ($enable_ripd == 1)
{	
	fwrite("w", $ripd_conf,
			"hostname ".$hostname."\n!\n");
}

foreach ("/runtime/inf")
{
	$uid = query("uid");	
	$addrtype = query("inet/addrtype");
  	$runtime_uid = PHYINF_getruntimeifname($uid);
	if ($addrtype == "ipv4" && $enable_ripd == 1)
	{
	    
		foreach ("/route/dynamic/rip/entry")
		{		 		    
			if (query("enable") == 1)
			{			  	
				$rip_inf = PHYINF_getruntimeifname(query("inf"));				
				if ($rip_inf == $runtime_uid)
				{
				  	
					/* Add dev info. into zebra config file */					
					fwrite("a", $zebra_conf,
						"interface ".$rip_inf."\n".
						"link-detect\n!\n");

					/* Add dev info. into ripng config file */
					fwrite("a", $ripd_conf,
					    "router rip\n".
							"network ".$rip_inf."\n");
							
					get_comm_rip($ripd_conf);		
					$transmit = query("transmit");
					$receive = query("receive");
					/*2011_03_25 sandy*/
					/*$auth = query("auth");
					$keynumber = query("keynumber");*/
					/* use distribute-list to Filter RIP Routes  */
					if($transmit == "DISABLE")
						fwrite("a", $ripd_conf,
								"router rip\n".
								"distribute-list private out ".$rip_inf."\n!\n".
								"access-list private deny any\n!\n");          
						                      				
					if($receive == "DISABLE")
						fwrite("a", $ripd_conf,
								"router rip\n".
								"distribute-list private in ".$rip_inf."\n!\n".
								"access-list private deny any\n\n");
          
					/*Set RIP version to the default, send = v2, recv = v2 and v1*/			
          if($receive=="AUTO" || $transmit=="AUTO")
              { fwrite ("a",$ripd_conf,
                "router rip\n". 
                "no version\n\n"); }
         /*By interface set different key,but one interface only own it's key*/
         /*
           if($auth!="" && $keynumber !="")      
              { fwrite ("a",$ripd_conf,
              "key chain ripkey".$keynumber."\n".
              "key ".$keynumber."\n".
              "key-string ".$auth."\n!\n"); }   */
              
					/* under command must be used by interface   */   
					fwrite("a", $ripd_conf,
							"interface ".$rip_inf."\n");
					if($transmit=="RIP1")	{ fwrite ("a",$ripd_conf, "ip rip send version 1\n"); }
					if($transmit=="RIP2")	{ fwrite ("a",$ripd_conf, "ip rip send version 2\n"); }
					if($receive=="RIP1")	{ fwrite ("a",$ripd_conf, "ip rip receive version 1\n!\n"); }
					if($receive=="RIP2")	{ fwrite ("a",$ripd_conf, "ip rip receive version 2\n!\n"); }
  					/*	
  					if($auth!="")   { fwrite ("a",$ripd_conf, 
  					  "ip rip authentication mode md5\n".
  					  "ip rip authentication key-chain ripkey".$keynumber."\n"); }*/
  								
					
				}
			}
		}
	}
   
	$uid = query("uid"); 
  	if ($addrtype == "ipv6" && $enable_ripngd == 1)
  	{    					  
        foreach ("/route6/dynamic/ripng/entry")
        {
            if (query("enable") == 1)
            {
              $ripng_inf = PHYINF_getruntimeifname(query("inf"));		
              if ($ripng_inf == $runtime_uid)
              {
        				$path = XNODE_getpathbytarget("/runtime", "inf", "uid", $uid, 0);
        				$ipaddr = query($path."/inet/ipv6/ipaddr");
        				$prefix = query($path."/inet/ipv6/prefix");
         
        				if ($ripng_inf == "" || $ipaddr == "" || $prefix == "") continue;
        
        				$isLL = tolower(cut($ipaddr, 0, ':'));
        				if ($isLL == "fe80") continue;
    
        				fwrite("a", $zebra_conf,
        						"interface ".$ripng_inf."\n".
        						"link-detect\n".
        						"ipv6 address ".$ipaddr."/".$prefix."\n".
        						"ipv6 nd suppress-ra\n!\n");
        
        				
        				fwrite("a", $ripngd_conf,
        						"network ".$ripng_inf."\n");
    			}
    		}			
		}
	 }	
}
fwrite("a", $zebra_conf,		
		"interface lo\n"
	  );

if ($enable_ripd == 1)
{	
	fwrite("a", $zebra_conf,		
			"link-detect\n" );
}

if ($IPV6 == 1 && $enable_ripngd == 1)
{
  fwrite("a", $zebra_conf,		
		"ipv6 forwarding\n" );
		get_comm_rip($ripngd_conf);		  
}	

/* Start script */
fwrite("w", $START, "#!/bin/sh\n");
if ($enable_ripd == 1 || $enable_ripngd == 1)
{
  fwrite("a", $START,
  		"if [ -f ".$zebra_conf." ]; then\n".
  		"	zebra -f ".$zebra_conf." -d;\n".
  		"	zebraup=$?\n".
  		"fi\n"
  	  );
}
if ($enable_ripd == 1)
{
	fwrite("a", $START,
			"if [ -f ".$ripd_conf." -a \$zebraup = 0 ]; then\n".
			"	ripd -f ".$ripd_conf." -d;\n".
			"fi\n"
		  );
}
if ($enable_ripngd == 1)
{
	fwrite("a", $START,
			"if [ -f ".$ripngd_conf." -a \$zebraup = 0 ]; then\n".
			"	ripngd -f ".$ripngd_conf." -d;\n".
			"fi\n"
		  );
}
fwrite("a", $START, "exit 0\n");

/* Stop script */
fwrite("w", $STOP, "#!/bin/sh\n");

if ($enable_ripd == 1)
{
	fwrite("a", $STOP,
		"/etc/scripts/killpid.sh /var/run/ripd.pid\n".
		"if [ -f ".$ripd_conf." ]; then\n".
		"	rm -f ".$ripd_conf.";\n".
		"fi\n"
		);
}

if ($enable_ripngd == 1)
{
	fwrite("a", $STOP,
		"/etc/scripts/killpid.sh /var/run/ripngd.pid\n".
		"if [ -f ".$ripngd_conf." ]; then\n".
		"	rm -f ".$ripngd_conf.";\n".
		"fi\n"
		);
}
if ($enable_ripd == 1 || $enable_ripngd == 1)
{
  fwrite("a", $STOP,
  		"/etc/scripts/killpid.sh /var/run/zebra.pid\n".
  		"if [ -f ".$zebra_conf." ]; then\n".
  		"	rm -f ".$zebra_conf.";\n".
  		"fi\n".
  		"exit 0\n"
  	  );
}
?>
