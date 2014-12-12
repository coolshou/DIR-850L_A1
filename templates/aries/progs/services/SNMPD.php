<?
/* Mofified by Sandy, 2010/01/26 */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/inf.php";

fwrite(w,$START,'#!/bin/sh\n');
fwrite(w,$STOP, '#!/bin/sh\n');

$snmpd_var = "/var/snmpd.conf";
$snmpd_var_netsnmp = "/var/net-snmp/snmpd.conf";

$rwcomm       	= query("/snmp/readwritecommunity");
$rocomm       	= query("/snmp/readonlycommunity");



   	$snmpactive = query("/snmp/active"); 
	if($snmpactive=="1") // if snmp enable to produce config
	{ 
	      fwrite (w,$snmpd_var, "#######     SNMPv3 Configuration     #########\n");
	      fwrite (a,$snmpd_var, "engineIDType 3\n");      
	      fwrite (a,$snmpd_var, "engineIDNic ixp0\n");
	      fwrite (w,$snmpd_var, "#######  Traditional Access Control  #########\n");
	      /*sandy 2010_12_27 start*/	     
	      $snmp_inf = PHYINF_getruntimeifname(query("/snmp/inf"));
          foreach ("/runtime/inf")
          {
        	$uid = query("uid");       	
	        $runtime_inf = PHYINF_getruntimeifname($uid);	       	   
			if($runtime_inf == $snmp_inf)
			{
				$ipaddr = query("inet/ipv4/ipaddr");
  	      		fwrite (a,$snmpd_var, "agentaddress ".$ipaddr."\n");//agent bind ipaddr
	        }
          	if($uid == "LAN-1")
  	      	{ fwrite(a,$START,"service IPT.LAN-1 restart\n");}
  	      	if($uid == "LAN-2")
  	      	{ fwrite(a,$START,"service IPT.LAN-2 restart\n");}
  	      	if($uid == "LAN-3")
  	      	{ fwrite(a,$START,"service IPT.LAN-3 restart\n");}
  	      	if($uid == "LAN-4")
  	      	{ fwrite(a,$START,"service IPT.LAN-4 restart\n");}    
  	      	if($uid == "WAN-1")    
  	      	{ fwrite(a,$START,"service IPT.WAN-1 restart\n");}
  	      	if($uid == "WAN-2")    
  	      	{ fwrite(a,$START,"service IPT.WAN-2 restart\n");}
  	      	if($uid == "WAN-3")    
  	      	{ fwrite(a,$START,"service IPT.WAN-3 restart\n");}
  	      	if($uid == "WAN-4")    
  	      	{ fwrite(a,$START,"service IPT.WAN-4 restart\n");}	        
	      }
	      /*sandy 2010_12_27 end*/
	      fwrite (a,$snmpd_var, "rwcommunity ".$rwcomm." default"." .1"."\n");
	      fwrite (a,$snmpd_var, "rocommunity ".$rocomm." default"." .1"."\n");	    
        	/*send trap start*/ 
	      fwrite (a,$snmpd_var, "trapcommunity ".$rocomm." default"." .1"."\n");

	      $trapstatus = query("/snmp/snmptrap/active");
	      $snmptrap = XNODE_add_entry("/snmp/snmptrap", "TRAP");

	      if($trapstatus=="1")
	      {        
	          foreach ("/snmp/snmptrap/entry")
	          {
	              $traphostip = query("hostip");	          
	              fwrite(a,$snmpd_var,"trapsink ".$traphostip." 162"."\n");
	              fwrite(a,$snmpd_var,"trap2sink ".$traphostip." 162"."\n");
	          }
	      }  
	       /*send trap end*/
	       /*This will be a problem because of only R/W comm*/	      
      foreach("/snmp/vacmcommunity/entry")
      {
          $cmtype = query("communitytype");
          $cmviewname = query("communityviewname");
          $comm = query("community");
          foreach("/snmp/vacmview/entry")
          {
              $viewname = query("viewtreefamilyviewname");
              if($viewname==$cmviewname){
                  if($cmtype=="readonly"){                   
                      fwrite(a,$snmpd_var, "rocommunity"." ".$comm." "."default"." ".".".query("viewtreefamilysubtree")."\n");
                  }else if($cmtype=="readwrite"){
                      fwrite(a,$snmpd_var, "rwcommunity"." ".$comm." "."default"." ".".".query("viewtreefamilysubtree")."\n");
                  }
              }
          }
      }   
	     fwrite(a,$snmpd_var, "#######      VACM Configuration      #########\n");
	     foreach("/snmp/usm/entry")
    	{
            $username=query("securityname");
            $privtcl=query("privacyprotocol");
            $authtcl=query("authprotocol");
            $mapgroup=query("groupname");
            $authkey=query("authkey");
            $privkey=query("privacykey");
            foreach("/snmp/vacmaccess/entry"){
                $groupname=query("accessgroupname");
                if($mapgroup==$groupname){
                 //   $rview=query("accessreadviewname");
                //    $wview=query("accesswriteviewname");
                //    $nview=query("accessnotifyviewname");
                    if($wview==""){
                        fwrite(a,$snmpd_var, "com2sec"." ".$username." "."default"." "."public\n");
                    }else{
                        fwrite(a,$snmpd_var, "com2sec"." ".$username." "."default"." "."private\n");
                    }
                }
            }
           
            if ($authtcl=="none"){
                fwrite(a,$snmpd_var_netsnmp, "createUser ".$username."\n");
            }else{
                if ($privtcl=="none"){
                    fwrite(a,$snmpd_var_netsnmp, "createUser ".$username." ".$authtcl." ".$authkey." \n");
                }else{
                    fwrite(a,$snmpd_var_netsnmp, "createUser ".$username." ".$authtcl." ".$authkey." ".$privtcl." ".$privkey." \n");
                }
            }
        }
	     foreach("/snmp/usm/entry")
        {
            fwrite(a,$snmpd_var, "group"." ".query("groupname")." "."usm"."    ".query("securityname")."\n");  
        }
    
        foreach("/snmp/vacmview/entry")
        {
            fwrite(a,$snmpd_var, "view"." ".query("viewtreefamilyviewname")." ".query("viewtreefamilytype")." ".".".query("viewtreefamilysubtree")." "."80\n");
        }
        foreach("/snmp/vacmaccess/entry")
        {
            fwrite(a,$snmpd_var, "access"." ".query("accessgroupname")." "."\"\""." "."any"." ".query("accesssecuritylevel")." "."exact"." ".query("accessreadviewname")." ".query("accesswriteviewname")." ".query("accessnotifyviewname")."\n");
        }
        
          /*must produce snmp_start.sh and snmp_stop.sh at the same time */
        	 fwrite(a,$START,"snmpd -c ".$snmpd_var." -f -Le &  > /dev/console\nexit 0\n");//start PHP      
	         fwrite(a,$STOP,"killall snmpd\nexit 0\n"); //STOP PHP
	}
	else //snmp disable don't produce config and kill old daemon 
	{
        	 fwrite(a,$START,"exit 0\n");
	         fwrite(a,$STOP,"killall snmpd\nexit 0\n");
	}

?>
