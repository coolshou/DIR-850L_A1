<module><?
include "/htdocs/phplib/xnode.php";
$infp = XNODE_getpathbytarget("", "inf", "uid", "WAN-1", 0);
$infp_lan = XNODE_getpathbytarget("", "inf", "uid", "LAN-1", 0);
?>
	<service><?=$GETCFG_SVC?></service>
	<inf>
		<web><?echo query($infp."/web");?></web>
		<https_rport><?echo query($infp."/https_rport");?></https_rport>
		<stunnel><?echo query($infp_lan."/stunnel");?></stunnel>
		<weballow>
<?			echo dump(3, $infp."/weballow");
?>		</weballow>
		<inbfilter><?echo query($infp."/inbfilter");?></inbfilter>
	</inf>
	<ACTIVATE>ignore</ACTIVATE>
</module>
