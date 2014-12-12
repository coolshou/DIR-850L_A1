<?
/* setcfg is used to move the validated session data to the configuration database.
 * The variable, 'SETCFG_prefix',  will indicate the path of the session data. */

include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

set("/snmp/readwritecommunity",		query($SETCFG_prefix."/snmp/readwritecommunity"));
set("/snmp/readonlycommunity",		query($SETCFG_prefix."/snmp/readonlycommunity"));
set("/snmp/active",		query($SETCFG_prefix."/snmp/active"));
set("/snmp/snmptrap/active",		query($SETCFG_prefix."/snmp/snmptrap/active"));
/*****************VACM view table*****************/
$vacmview_cnt = query($SETCFG_prefix."/snmp/vacmview/count");
$vacmview_seqno = query($SETCFG_prefix."/snmp/vacmview/seqno");

//TRACE_debug("vacmview_cnt=".$vacmview_cnt);
//TRACE_debug("vacmview_seqno=".$vacmview_seqno);
foreach ($SETCFG_prefix."/snmp/vacmview/entry")
{
  
	    if ($InDeX > $vacmview_seqno) break;
      if (query("uid")=="")
      {
          set("uid", "VACMVIEW-".$vacmview_seqno);
          $vacmview_seqno++;
          $vacmview_cnt++;
      }  

    set("/snmp/vacmview/entry:".$InDeX."/uid",           query("uid"));
    set("/snmp/vacmview/entry:".$InDeX."/viewtreefamilyviewname",  query("viewtreefamilyviewname"));
    set("/snmp/vacmview/entry:".$InDeX."/viewtreefamilytype",   query("viewtreefamilytype"));
    set("/snmp/vacmview/entry:".$InDeX."/viewtreefamilysubtree",   query("viewtreefamilysubtree"));

}
set("/snmp/vacmview/seqno", $vacmview_seqno);
set("/snmp/vacmview/count", $vacmview_cnt);
/*****************VACM community table*****************/
$vacmcommunity_cnt = query($SETCFG_prefix."/snmp/vacmcommunity/count");
$vacmcommunity_seqno = query($SETCFG_prefix."/snmp/vacmcommunity/seqno");
//TRACE_debug("vacmcommunity_cnt=".$vacmcommunity_cnt);
//TRACE_debug("vacmcommunity_seqno=".$vacmcommunity_seqno);
foreach ($SETCFG_prefix."/snmp/vacmcommunity/entry")
{
	    if ($InDeX > $vacmcommunity_seqno) break;
      if (query("uid")=="")
      {
          set("uid", "VACMCOMMUNITY-".$vacmcommunity_seqno);
          $vacmcommunity_seqno++;
          $vacmcommunity_cnt++;
      }
  set("/snmp/vacmcommunity/entry:".$InDeX."/uid",           query("uid"));
  set("/snmp/vacmcommunity/entry:".$InDeX."/communityname", query("communityname"));
  set("/snmp/vacmcommunity/entry:".$InDeX."/communityviewname",   query("communityviewname"));
  set("/snmp/vacmcommunity/entry:".$InDeX."/communitytype",   query("communitytype"));

}
set("/snmp/vacmcommunity/seqno", $vacmcommunity_seqno);
set("/snmp/vacmcommunity/count", $vacmcommunity_cnt);
/*****************VACM access table*****************/
$vacmaccess_cnt = query($SETCFG_prefix."/snmp/vacmaccess/count");
$vacmaccess_seqno = query($SETCFG_prefix."/snmp/vacmaccess/seqno");
foreach ($SETCFG_prefix."/snmp/vacmaccess/entry")
{
      if ($InDeX > $vacmaccess_seqno) break;
      if (query("uid")=="")
      {
          set("uid", "VACMACCESS-".$vacmaccess_seqno);
          $vacmaccess_seqno++;
          $vacmaccess_cnt++;
      }
      set("/snmp/vacmaccess/entry:".$InDeX."/uid",           query("uid"));
      set("/snmp/vacmaccess/entry:".$InDeX."/accessgroupname",   query("accessgroupname"));
      set("/snmp/vacmaccess/entry:".$InDeX."/accesssecuritymodel",   query("accesssecuritymodel"));
      set("/snmp/vacmaccess/entry:".$InDeX."/accesssecuritylevel",   query("accesssecuritylevel"));
      set("/snmp/vacmaccess/entry:".$InDeX."/accessreadviewname",    query("accessreadviewname"));
      set("/snmp/vacmaccess/entry:".$InDeX."/accesswriteviewname",   query("accesswriteviewname"));
      set("/snmp/vacmaccess/entry:".$InDeX."/accessnotifyviewname",    query("accessnotifyviewname"));

}  
set("/snmp/vacmaccess/seqno", $vacmaccess_seqno);
set("/snmp/vacmaccess/count", $vacmaccess_cnt);
/*****************trap table*****************/
$trap_cnt = query($SETCFG_prefix."/snmp/snmptrap/count");
$trap_seqno = query($SETCFG_prefix."/snmp/snmptrap/seqno");

foreach ($SETCFG_prefix."/snmp/snmptrap/entry")
{
	    if ($InDeX > $trap_seqno) break;
      if (query("uid")=="")
      {
          set("uid", "TRAP-".$trap_seqno);
          $trap_seqno++;
          $trap_cnt++;
      }     

  set("/snmp/snmptrap/entry:".$InDeX."/uid",           query("uid"));
  set("/snmp/snmptrap/entry:".$InDeX."/hostip", query("hostip"));
  set("/snmp/snmptrap/entry:".$InDeX."/community",   query("community"));

}
set("/snmp/snmptrap/seqno", $trap_seqno);
set("/snmp/snmptrap/count", $trap_cnt);
/*****************USM table*****************/
$usm_cnt = query($SETCFG_prefix."/snmp/usm/count");
$usm_seqno = query($SETCFG_prefix."/snmp/usm/seqno");

foreach ($SETCFG_prefix."/snmp/usm/entry")
{
      if ($InDeX > $usm_seqno) break;
      if (query("uid")=="")
      {
          set("uid", "USM-".$usm_seqno);
          $usm_seqno++;
          $usm_cnt++;
      }
      set("/snmp/usm/entry:".$InDeX."/uid",           query("uid"));
      set("/snmp/usm/entry:".$InDeX."/securityname",   query("securityname"));
      set("/snmp/usm/entry:".$InDeX."/authprotocol",       query("authprotocol"));
      set("/snmp/usm/entry:".$InDeX."/authkey",           query("authkey"));
      set("/snmp/usm/entry:".$InDeX."/privacyprotocol",           query("privacyprotocol"));
      set("/snmp/usm/entry:".$InDeX."/privacykey",           query("privacykey"));
      set("/snmp/usm/entry:".$InDeX."/groupname",           query("groupname"));
}  

set("/snmp/usm/seqno", $usm_seqno);
set("/snmp/usm/count", $usm_cnt);
?>