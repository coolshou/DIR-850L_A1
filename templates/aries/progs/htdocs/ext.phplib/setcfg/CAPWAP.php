<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

set("/capwap/active",     query($SETCFG_prefix."/capwap/active"));
$active = query("/capwap/active");

/*******************AC List table*************************/
$aclist_cnt = query($SETCFG_prefix."/capwap/aclist/count");
$aclist_seqno = query($SETCFG_prefix."/capwap/aclist/seqno");
foreach ($SETCFG_prefix."/capwap/aclist/entry")
{

        if ($InDeX > $aclist_seqno) break;
		if (query("uid")=="")
		{
		     set("uid", "ACLIST-".$aclist_seqno);
		     $aclist_seqno++;
		     $aclist_cnt++;
		}
		    set("/capwap/aclist/entry:".$InDeX."/uid",           query("uid"));
		    set("/capwap/aclist/entry:".$InDeX."/staticip",  query("staticip"));
		    set("/capwap/aclist/entry:".$InDeX."/acname",   query("acname"));														
}
set("/capwap/aclist/seqno", $aclist_seqno);
set("/capwap/aclist/count", $aclist_cnt);
?>
