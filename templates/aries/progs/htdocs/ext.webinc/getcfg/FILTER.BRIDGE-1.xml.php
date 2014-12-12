<module>
	<service><?=$GETCFG_SVC?></service>

<?
		include "/htdocs/phplib/xnode.php";
		$inf = XNODE_getpathbytarget("", "inf", "uid", cut($GETCFG_SVC,1,"."), 0);
?>	

	<phyinf>
<?
$target_phyinf=query($inf."/phyinf");

		$phyinf = XNODE_getpathbytarget("", "phyinf", "uid", $target_phyinf, 0);
		if ($phyinf!="") echo dump(2, $phyinf);

?>	</phyinf>

	<filter>
    	<remote_mnt>
<?
$target_filter=query($phyinf."/filter");

		$filter = XNODE_getpathbytarget("/filter", "remote_mnt", "uid", $target_filter, 0);
		if ($filter!="") echo dump(3, $filter);
?>	
		</remote_mnt>
    </filter>

</module>
