<?
/* fatlady is used to validate the configuration for the specific service.
 * FATLADY_prefix was defined to the path of Session Data.
 * 3 variables should be returned for the result:
 * FATLADY_result, FATLADY_node & FATLADY_message. */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inet.php";

function set_result($result, $node, $message)
{
	$_GLOBALS["FATLADY_result"] = $result;
	$_GLOBALS["FATLADY_node"]   = $node;
	$_GLOBALS["FATLADY_message"]= $message;
}
/************** EditPublicCommunity********************/

function EditPublicCommunity($path)
{
     $readonlycommunity = query($path."/readonlycommunity");
   //  TRACE_debug("readonlycommunity =".$readonlycommunity);
     if(strlen($readonlycommunity) >= 33)
     {
          set_result("FAILED","",i18n("Public community String can't exceeds 32 characters."));          
          return 0;
     }
     if(strlen($readonlycommunity) == 0)
     {
          set_result("FAILED","",i18n("Please input public community string.")); 
          return 0;     
     }
	  return 1;
}
/************** EditPrivateCommunity********************/

function EditPrivateCommunity($path)
{
     $readwritecommunity = query($path."/readwritecommunity");
    // TRACE_debug("readwritecommunity =".$readwritecommunity);
     if(strlen($readwritecommunity) >= 33)
     {
         set_result("FAILED","",i18n("Private community String can't exceeds 32 characters!!")); 
         return 0;       
     }
     if(strlen($readwritecommunity) == 0)
     {
         set_result("FAILED","",i18n("Please input private community string.")); 
        return 0;       
     }
	    return 1;
}
/**************for cli VACM AddView check********************/

function VACM_AddView($path)
{
   $j = 0;     
   foreach ($path."/entry")    
   {
      $j++;
      $num = query($path."/entry#");
      if (query("uid")=="")
      {
        break;
      }
   }  
		$viewname = query($path."/entry:".$j."/viewtreefamilyviewname");             
    $viewoid = query($path."/entry:".$j."/viewtreefamilysubtree"); 
    $viewtype = query($path."/entry:".$j."/viewtreefamilytype");
    /*
    TRACE_debug("2.viewname=".$viewname);
    TRACE_debug("2.viewoid=".$viewoid);
    TRACE_debug("2.viewtype=".$viewtype);
    */

     if(strlen($viewname) >= $_GLOBALS["VACM_MAX_LEN_NAME"]) 
     { 
         set_result("FAILED","","Community String can't exceeds 32 characters!!.");  
         return 0;      
     }   
     else if(strlen($viewoid) > $_GLOBALS["VACM_OID_LEN"]) 
     {
         set_result("FAILED","","OID can't exceeds 128 characters!");
        return 0;        
     }  
     else  if($viewtype != "included" && $viewtype != "excluded") 
     {  
         set_result("FAILED","","Type is error.Type must be included or excluded");  
         return 0;
     }  
         
     else
     {          
      /* check if view table is full &   check if view exist */             
         	$vn_entry = 1;    
         while ($vn_entry <= $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
        {  
         
            $db_viewname	= query("/snmp/vacmview/entry:".$vn_entry."/viewtreefamilyviewname");
            /*
            TRACE_debug("db_viewname=".$db_viewname);
            TRACE_debug("viewname=".$viewname); 
            */
  	       if($db_viewname == ""){return 1;}
           else if($db_viewname == $viewname)
           { 
             set_result("FAILED","","The Host name existed.");  
             return 0;           
           }          
           else if ($vn_entry == $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
           { 
             set_result("FAILED","","The Host table is full.");
            	return 0;            	 
           }           
           $vn_entry++;            
        }
      }             
   return 1;
}
/**************for cli AddCommunity check********************/

function CLI_AddCommunity($path)
{   
      $j = 0;     
     foreach ($path."/entry")    
     {
        $j++;
        $num = query($path."/entry#");
        if (query("uid")=="")
        {
          break;
        }
     }  
		$communityname = query($path."/entry:".$j."/communityname");             
    $communityviewname = query($path."/entry:".$j."/communityviewname"); 
    $communitytype = query($path."/entry:".$j."/communitytype");
    /*    
     TRACE_debug("communityname=".$communityname);
     TRACE_debug("communityviewname=".$communityviewname); 
     TRACE_debug("communitytype=".$communitytype); 
     */     
      if(strlen($communityname) >= 33) //support 32 character
      {
         set_result("FAILED","","Community String exceeds 32 characters!!.");
         return 0;
      }
      else if(strlen($communityviewname) >= $_GLOBALS["VACM_MAX_LEN_NAME"])  //support 32 character
      {
         set_result("FAILED","","View Name can't exceeds 32 characters!.");
         return 0;
      }
      else if($communitytype != "readonly" && $communitytype != "readwrite")
      {
         set_result("FAILED","","Type must be readonly/readwrite .");
         return 0;
      }
   /*check if view name exist */ 
        $vn_entry=1;
         while ($vn_entry <= $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
        { 	    
  	      $db_viewname	= query("/snmp/vacmview/entry:".$vn_entry."/viewtreefamilyviewname");    	             // TRACE_debug("db_viewname=".$db_viewname);
           if($db_viewname == "")
           { 
              set_result("FAILED","","The view name doesn't exist !");
              return 0;
           }
           else if($communityviewname == $db_viewname)
           {          
              return 1;
           }    
           else if($vn_entry == $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
           {return 0;}      
          $vn_entry++; 
        }    
          /*check if community table is full &   check if community exist */   
     
         $cm_entry=1;
         while ($cm_entry <= $_GLOBALS["MAX_EPMCOMM_ENTRIES"])
        {      	
  	      $db_communityname	= query("/snmp/vacmcommunity/entry:".$cm_entry."/communityname");
           if($db_communityname == ""){return 1;}
           else if($db_communityname == $communityname)
           {
               set_result("FAILED","","The community name existed.");
               return 0;
           }
           else if ($cm_entry == $_GLOBALS["MAX_EPMCOMM_ENTRIES"])
           {
            	  set_result("FAILED","","The community table is full.");
            	  return 0;
           }
           $cm_entry++; 
        }     
    return 1;   
}
/**************for cli Add host check********************/
function CLI_AddHost($path)
{
      $j = 0;     
     foreach ($path."/snmptrap/entry")    
     {
        $j++;
        $num = query($path."/snmptrap/entry#");
        if (query("uid")=="")
        {
          break;
        }
     }  

		$hostip = query($path."/snmptrap/entry:".$j."/hostip");
		$runcommunity = query($path."/snmptrap/entry:".$j."/community");
		$security_model = query($path."/vacmaccess/entry:".$j."/accesssecuritymodel");
		$security_level = query($path."/vacmaccess/entry:".$j."/accesssecuritylevel");
		/*
		TRACE_debug("hostip=".$hostip);
		TRACE_debug("runcommunity=".$runcommunity);
		TRACE_debug("security_model=".$security_model);
		TRACE_debug("security_level=".$security_level);
		*/
		if (INET_validv4addr($hostip)==0)		  
    {
       set_result("FAILED","","Please input correct IP address!.");
       return 0;       
    }
    
    else if($security_model != "v1" && $security_model != "v2c" && $security_model != "v3")
    {
       set_result("FAILED","","SnmpType shoule be v1/v2c/v3.");
       return 0;
    }
    else if($security_level != "noauthnonriv"&& $security_level != "authnopriv"&& $security_level != "authpriv")
    {
       set_result("FAILED","","AuthType shoule be none/noauthnonriv/authnopriv/authpriv.");
       return 0;
    }
    else if(strlen($runcommunity) >= $_GLOBALS["VACM_MAX_LEN_NAME"]) //support 32 character
    {
       set_result("FAILED","","Community String can't exceeds 32 characters!!.");
       return 0;
    }  
    else
    {
        $hs_entry=1;
         while ($hs_entry <= $_GLOBALS["MAX_EPMADDR_ENTRIES"])
        {      	
  	      $db_hostip	= query("/snmp/snmptrap/entry:".$hs_entry."/hostip");  	
  	     //  TRACE_debug("db_hostip=".$db_hostip);  
  	       if($db_hostip == ""){return 1;}
           else if($db_hostip == $hostip)
           {
               set_result("FAILED","","The hostip existed.");
               return 0;
           }
           else if ($hs_entry == $_GLOBALS["MAX_EPMADDR_ENTRIES"])
           {
            	 set_result("FAILED","","The trap table is full.");
            	 return 0;
           }
           $hs_entry++; 
  	    }
    } 
    if($security_model == "v1" || $security_model == "v2c")  
    {
        $cm_found = "-1"; //FAILED   
        $cm_entry=0;
         while ($cm_entry <= $_GLOBALS["MAX_EPMCOMM_ENTRIES"])
        {
          $cm_entry++; 
          $db_communityname	= query("/snmp/vacmcommunity/entry:".$cm_entry."/communityname");
        //  TRACE_debug("db_communityname=".$db_communityname);
           if($db_communityname == "")
           { 
              $cm_found =0;        
           }
           else if($db_communityname == $runcommunity)
           {
            $cm_found = 1;          
           }
           else if($cm_entry == $_GLOBALS["MAX_EPMCOMM_ENTRIES"])
           {
              $cm_found = 0;
           }
           
        }
        if($cm_found == 0) //check private or not
        {
          $readwritecommunity = query($path."/readwritecommunity");
       //   TRACE_debug("readwritecommunity=".$readwritecommunity);
           if($readwritecommunity == "")
           {
              $cm_found = 0;     
           }
           else if($readwritecommunity == $runcommunity)
           {
					    $cm_found = 1;
           }
        }
        if($cm_found == 0) //check pubilc or not
        {
          $readonlycommunity = query($path."/readonlycommunity");
        //  TRACE_debug("readwritecommunity=".$readwritecommunity);
          if($readonlycommunity == "")
           {
              $cm_found = 0;      
           }
           else if($readwritecommunity == $runcommunity)
           {
					    $cm_found = 1;
           }
        }
        if($cm_found == 0)
        {
           set_result("FAILED","","The community doesn't exist !");
           return 0;
        }
    } 
    else if($security_model == "v3")
    {
       $sn_entry =1;
      while($sn_entry<= $_GLOBALS["MAX_USM_USER_ENTRIES"])
      {
         $securityname = query("/snmp/usm/entry:".$sn_entry."/securityname");  
         $authprotocol = query("/snmp/usm/entry:".$sn_entry."/authprotocol");		
      	 $privacyprotocol = query("/snmp/usm/entry:".$sn_entry."/privacyprotocol");
      	  if($securityname == ""){break;}
      	  else if($securityname == $runcommunity )
      	  {
      	    if($security_level =="authnopriv")/* auth_nopriv */
      	    { 
        	    if($authprotocol =="none")
        	    {
        	       set_result("FAILED","","The specified user does seting AuthProtocol!");
        	       return 0;
        	    }
        	  }
        	  else if($security_level =="authpriv") /* auth_priv */
        	  {
        	    if($authprotocol =="none")
        	    {
        	       set_result("FAILED","","The specified user does seting AuthProtocol!");
        	       return 0;
        	    }
        	    if($privacyprotocol =="none")
        	    {
        	       set_result("FAILED","","The specified user does not set privacy protocol!");
        	       return 0;
        	    }
        	  }
        	   break;
      	  }
      	  else if($sn_entry == $_GLOBALS["MAX_USM_USER_ENTRIES"])
          {
             set_result("FAILED","","The user doesn't exist !");
             return 0;
          }		
      }
      $sn_entry++;
    }
     return 1;
}
/**************for cli VACM AddGroup check********************/
function VACM_AddGroup($path)
{     
      $j = 0;     
     foreach ($path."/entry")    
     {
        $j++;
        $num = query($path."/entry#");
        if (query("uid")=="")
        {
          break;
        }
     } 
    $groupname = query($path."/entry:".$j."/accessgroupname"); 
    $seculevel = query($path."/entry:".$j."/accesssecuritylevel");
		$readview = query($path."/entry:".$j."/accessreadviewname");
		$writeview = query($path."/entry:".$j."/accesswriteviewname");
		$notifyview = query($path."/entry:".$j."/accessnotifyviewname");
	//	TRACE_debug("groupname=".$groupname);
		if(strlen($groupname) >= $_GLOBALS["VACM_MAX_LEN_NAME"]) 
    { 
       set_result("FAILED","","Group name can't exceeds 32 characters!!.");
       return 0;        
    } 
    else if($seculevel != "noauthnonriv" && $seculevel != "authnopriv" && $seculevel != "authpriv")  
    {
       set_result("FAILED","","Security Level shoule be noauthnonriv/authnopriv/authpriv.");
       return 0;
    }   
    else if(strlen($readview) >= $_GLOBALS["VACM_MAX_LEN_NAME"]) 
    { 
       set_result("FAILED","","ReadView name can't exceeds 32 characters!!.");       
       return 0; 
    }
    else if(strlen($writeview) >= $_GLOBALS["VACM_MAX_LEN_NAME"]) 
    { 
       set_result("FAILED","","WriteView name can't exceeds 32 characters!!.");  
       return 0;      
    }
    else if(strlen($notifyview) >= $_GLOBALS["VACM_MAX_LEN_NAME"]) 
    { 
       set_result("FAILED","","NotifyView name can't exceeds 32 characters!!."); 
       return 0;       
    }
     $vi_entry=1;
     while ($vi_entry <= $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
    {      	
       $db_viewname	= query("/snmp/vacmview/entry:".$vi_entry."/viewtreefamilyviewname"); 
     //  TRACE_debug("db_viewname=".$db_viewname);
       if($db_viewname == "")  
       {  
           set_result("FAILED","","ReadView doesn't exist !");
           return 0;
       } 
       else if($db_viewname == $readview)
       {
          return 1;
       }
       else if($vi_entry == $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
       {
          set_result("FAILED","","ReadView doesn't exist !");
          return 0;
       }
       $vi_entry++; 
    }  
     $vi_entry=1;
     while ($vi_entry <= $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
    {      	
       $db_viewname	= query("/snmp/vacmview/entry:".$vi_entry."/viewtreefamilyviewname"); 
     //  TRACE_debug("db_viewname=".$db_viewname);
       if($db_viewname == "")  
       {  
           set_result("FAILED","","WriteView doesn't exist !");
           return 0;
       } 
       else if($db_viewname == $writeview)
       {
          return 1;
       }
       else if($vi_entry == $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
       {
          set_result("FAILED","","WriteView doesn't exist !");
          return 0;
       }
       $vi_entry++; 
    } 
     $vi_entry=1;
     while ($vi_entry <= $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
    {      	
       $db_viewname	= query("/snmp/vacmview/entry:".$vi_entry."/viewtreefamilyviewname"); 
     //  TRACE_debug("db_viewname=".$db_viewname);
       if($db_viewname == "")  
       {  
           set_result("FAILED","","notifyview doesn't exist !");
           return 0;
       } 
       else if($db_viewname == $notifyview)
       {
          return 1;
       }
       else if($vi_entry == $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
       {
          set_result("FAILED","","notifyview doesn't exist !");
          return 0;
       }
       $vi_entry++; 
    }  
     $gn_entry=1;
     while ($gn_entry <= $_GLOBALS["VACM_MAX_VIEW_ENTRIES"])
    {      	
       $db_groupname	= query("/snmp/vacmaccess/entry:".$gn_entry."/accessgroupname"); 
     //  TRACE_debug("db_groupname=".$db_groupname);
       if($db_groupname == "") {return 1;}
       else if($db_groupname == $groupname)
       {
           set_result("FAILED","","The group name existed");
           return 0;
       }
       else if($gn_entry == $_GLOBALS["VACM_MAX_GROUP_ENTRIES"])
       {
          set_result("FAILED","","The group table is full");
          return 0;
       }
       $gn_entry++; 
    }   
     return 1;
}    
/**************for cli VACM AddGroup check********************/
function USM_AddUsmUser($path)
{
      $j = 0; $ParamOk=0;     
     foreach ($path."/entry")    
     {
        $j++;
        $num = query($path."/entry#");
        if (query("uid")=="")
        {
          break;
        }
     } 
    $securityname = query($path."/entry:".$j."/securityname");
    $usergroupname = query($path."/entry:".$j."/groupname"); 
    $authprotocol = query($path."/entry:".$j."/authprotocol");
		$authkey = query($path."/entry:".$j."/authkey");
		$privacyprotocol = query($path."/entry:".$j."/privacyprotocol");
		$privacykey = query($path."/entry:".$j."/privacykey");
		
		// TRACE_debug("authkey=".$authkey);
		 
		if(strlen($securityname) >= $_GLOBALS["USER_NAME_MAX_LEN"]) 
    { 
       set_result("FAILED","","Username name can't exceeds 32 characters!!."); 
       return 0;       
    } 
    else if(strlen($usergroupname) >= $_GLOBALS["USER_NAME_MAX_LEN"]) 
    { 
       set_result("FAILED","","UserGroup name can't exceeds 32 characters!!.");  
       return 0;      
    }
    else if($authprotocol == "none" && $authprotocol == "MD5" && $authprotocol == "SHA")  
    {
       set_result("FAILED","","Auth Type shoule be none/MD5/SHA.");
       return 0;
    } 
    else if(strlen($authkey) < 8 || strlen($authkey) > 20) 
    { 
       set_result("FAILED","","Auth key must be in the length of 8-20 characters!"); 
       return 0;      
    }
    else if($privacyprotocol == "none" && $privacyprotocol == "DES" )  
    {
       set_result("FAILED","","Privacy Type shoule be none/DES.");
       return 0;
    }
    else if(strlen($privacykey) < 8 || strlen($privacykey) > 16) 
    { 
       set_result("FAILED","","Privacy key must be in the length of 8-16 characters!"); 
       return 0;   
    }
    else
    {
      	/* Auth Check */
     
      $authkeylength= strlen($authkey);
      $idx = charcodeat($authkey, "1");
      $i=0;
      $err = 0;
      while ($i<=$authkeylength)
      {        
        $eachchar= charcodeat($authkey, $i);       
        if($eachchar == $idx) $err++; 
        $i++;          
      }      
      if ($err == $authkeylength)
      {
         set_result("FAILED","","Auth key can't use the same character!"); 
         return 0;
      }
      /* Priv Check */
      $privacylength= strlen($privacykey);
      $idx = charcodeat($privacykey, "1");
      $i=0;
      $err = 0;    
      while ($i<=$privacylength)
      {       
        $eachchar= charcodeat($privacykey, $i);
        if($eachchar == $idx) $err++;   
        $i++;     
      }
      if ($err == $privacylength)
      { 
        set_result("FAILED","","privacy key can't use the same character!");
        return 0;
      }
    $ParamOk =1;     
    }
    if($ParamOk ==1)
	  {
	      $sn_entry=1;
        while ($sn_entry <= $_GLOBALS["MAX_USM_USER_ENTRIES"])
        { 
            $db_securityname	= query("/snmp/usm/entry:".$sn_entry."/securityname"); 
            if($db_securityname == "") {return 1;}
           else if($db_securityname == $securityname)
           {
               set_result("FAILED","","The username existed");
               return 0;
           }
           else if($sn_entry == $_GLOBALS["MAX_USM_USER_ENTRIES"])
           {
              set_result("FAILED","","The user table is full");
              return 0;
           }
           $sn_entry++;  
       } 
       $found =0;
       $gn_entry=0;
        while ($gn_entry <= $_GLOBALS["MAX_USM_USER_ENTRIES"])
        { 
            $gn_entry++;  
            $db_groupname	= query("/snmp/vacmaccess/entry:".$gn_entry."/accessgroupname"); 
        //    TRACE_debug("db_groupname=".$db_groupname);    
            if($db_groupname == "") 
            {  
              $found = 0;
              break;
            }
           else if($db_groupname == $usergroupname)
           {
             $found = 1;
             break;
           }
           else if($gn_entry == $_GLOBALS["MAX_USM_USER_ENTRIES"])
           { 
             $found = 0;
             break;            
           }           
       }      
    	if ($found ==0)
    	{
    	    set_result("FAILED","","The group doesn't exist");
    	    return 0;
    	}
    	else
    	{
    	 // TRACE_debug("gn_entry=".$gn_entry);
    	  $db_securitylevel	= query("/snmp/vacmaccess/entry:".$gn_entry."/accesssecuritylevel"); 
    	  if($db_securitylevel =="authnopriv" && $authprotocol == "none")  
    	   { 
    	     set_result("FAILED","","Add rejected, group is auth required and no priv needed!");
    	     return 0;
    	   }
    	  else if($db_securitylevel =="authpriv" && $authprotocol == "none")  
    	  {
    	   set_result("FAILED","","Add rejected, group is both auth and priv required!");
    	   return 0;
    	   }
    	  else if($authprotocol != "none" && $privacyprotocol =="none")  
    	  {
    	   set_result("FAILED","","Add rejected, group is both auth and priv required!");
    	   return 0;
    	  } 
    	}
       
	  }
    return 1;    
}

$_GLOBALS["VACM_MAX_VIEW_ENTRIES"]= 30;
$_GLOBALS["VACM_MAX_LEN_NAME"] =33;
$_GLOBALS["VACM_OID_LEN"] =128;
$_GLOBALS["MAX_EPMCOMM_ENTRIES"] 	=10;
$_GLOBALS["MAX_EPMADDR_ENTRIES"] 	=10;
$_GLOBALS["VACM_MAX_GROUP_ENTRIES"]	=30;
$_GLOBALS["USER_NAME_MAX_LEN"]	=32;
$_GLOBALS["MAX_USM_USER_ENTRIES"] 	=10;


set_result("FAILED", "", ""); //defaule value if faile return 


function fatlady_snmp($prefix)
{
  /*in order to find access which one table-------------------*/
  /*-------case 1------*/
   $case1 =0; $case2 =0; $case3 =0; $case4 =0; $case5 =0;
   $j = 0;
   $view_path = $prefix."/snmp/vacmview";
  foreach ($view_path."/entry")  
  {	  
        $j++;
        $num = query($view_path."/entry#");
        if (query("uid")=="")
        {
          break;
        }
  }
  	$viewname = query($view_path."/entry:".$j."/viewtreefamilyviewname");             
    $viewoid = query($view_path."/entry:".$j."/viewtreefamilysubtree"); 
    $viewtype = query($view_path."/entry:".$j."/viewtreefamilytype");
    $viewuid = query($view_path."/entry:".$j."/uid");
    /*
     TRACE_debug("1. viewname=".$viewname);
     TRACE_debug("1. viewoid=".$viewoid);
     TRACE_debug("1. viewtype=".$viewtype);
     TRACE_debug("1. viewuid=".$viewuid);
     */
  if ($viewname!="" && $viewoid!="" && $viewtype!="" && $viewuid=="")
  {
    $case1 = 1;
  }
   /*-------case 2------*/
  $j = 0;
  $community_path = $prefix."/snmp/vacmcommunity";
  foreach ($community_path."/entry")  
  {	  
        $j++;
        $num = query($community_path."/entry#");
        if (query("uid")=="")
        {
          break;
        }
  }      
  $communityname = query($community_path."/entry:".$j."/communityname");             
  $communityviewname = query($community_path."/entry:".$j."/communityviewname"); 
  $communitytype = query($community_path."/entry:".$j."/communitytype");
  $communityuid = query($community_path."/entry:".$j."/uid");
   
  if ($communityname!="" && $communityviewname!="" && $communitytype!="" && $communityuid=="")
  {
  	$case2 = 1;
  }
   /*-------case 3------*/
  
   $j = 0; 
  $group_path = $prefix."/snmp/vacmaccess";       
  foreach ($group_path."/entry")    
  {
    $j++;
    $num = query($group_path."/entry#");
    if (query("uid")=="")
    {
      break;
    }
  } 
   $groupname = query($group_path."/entry:".$j."/accessgroupname"); 
  $seculevel = query($group_path."/entry:".$j."/accesssecuritylevel");
	$readview = query($group_path."/entry:".$j."/accessreadviewname");
	$writeview = query($group_path."/entry:".$j."/accesswriteviewname");
	$notifyview = query($group_path."/entry:".$j."/accessnotifyviewname");
	/*
   TRACE_debug("groupname=".$groupname);
   TRACE_debug("seculevel=".$seculevel);
   TRACE_debug("readview=".$readview);
   TRACE_debug("writeview=".$writeview);
   TRACE_debug("notifyview=".$notifyview);
   */
  if ($groupname!="" && $seculevel!="" && $readview!="" && $writeview!="" && $notifyview!="")
  {
  	$case3 = 1;
  } 
   /*-------case 4------*/
  $j = 0;
  $host_path = $prefix."/snmp";   
   foreach ($host_path."/snmptrap/entry")    
   {
      $j++;
      $num = query($host_path."/snmptrap/entry#");
      if (query("uid")=="")
      {
        break;
      }
   }       
  $hostip = query($host_path."/snmptrap/entry:".$j."/hostip");
  $runcommunity = query($host_path."/snmptrap/entry:".$j."/community");
  $security_model = query($host_path."/vacmaccess/entry:".$j."/accesssecuritymodel");
  $security_level = query($host_path."/vacmaccess/entry:".$j."/accesssecuritylevel");
  $trap_uid = query($host_path."/snmptrap/entry:".$j."/uid");
  /*
   TRACE_debug("hostip=".$hostip);
   TRACE_debug("runcommunity=".$runcommunity);
   TRACE_debug("security_model=".$security_model);
   TRACE_debug("security_level=".$security_level);
   TRACE_debug("trap_uid=".$trap_uid);
   */
  if ($hostip!="" && $trap_uid=="")
  {
  	$case4 = 1;
  }
  /*-------case 5------*/  
  $j = 0; 
  $usm_path = $prefix."/snmp/usm";       
  foreach ($group_path."/entry")    
  {
    $j++;
    $num = query($group_path."/entry#");
    if (query("uid")=="")
    {
      break;
    }
  } 
      $securityname = query($usm_path."/entry:".$j."/securityname");
      $usergroupname = query($usm_path."/entry:".$j."/groupname"); 
      $authprotocol = query($usm_path."/entry:".$j."/authprotocol");
  		$authkey = query($usm_path."/entry:".$j."/authkey");
  		$privacyprotocol = query($usm_path."/entry:".$j."/privacyprotocol");
  		$privacykey = query($usm_path."/entry:".$j."/privacykey");
  		$num_uid = query($usm_path."/entry:".$j."/uid");
  
  if ($securityname!="" && $usergroupname!="" && $authprotocol!="" && $authkey!="" && $privacyprotocol!="" && $privacykey!="" && $num_uid=="")
  {
  	$case5 = 1;
  }
  

  if (EditPublicCommunity($prefix."/snmp")!=1)return; 
  if (EditPrivateCommunity($prefix."/snmp")!=1)return; 
 /*
  TRACE_debug("case1=".$case1); 
  TRACE_debug("case2=".$case2); 
  TRACE_debug("case3=".$case3); 
  TRACE_debug("case4=".$case4); 
  TRACE_debug("case5=".$case5); 
 */
   
  if($case1==1)
  {
    if (VACM_AddView($prefix."/snmp/vacmview")!=1)return; 
  }  
  if($case2==1)
  {
    if (CLI_AddCommunity($prefix."/snmp/vacmcommunity")!=1)return; 
  } 
  if($case3==1)
  {
    if (VACM_AddGroup($prefix."/snmp/vacmaccess")!=1)return;
  }
  if($case4==1)
  {
    if (CLI_AddHost($prefix."/snmp")!=1)return;
  }
  if($case5==1)
  {
    if (USM_AddUsmUser($prefix."/snmp/usm")!=1)return;
  } 
  set($prefix."/valid", 1);
	set_result("OK","",""); 
}

fatlady_snmp($FATLADY_prefix);

?>