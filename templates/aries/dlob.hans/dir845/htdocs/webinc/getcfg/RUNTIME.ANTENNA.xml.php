<module>
	<service><?=$GETCFG_SVC?></service>
	<runtime>
		<antenna>
<?		
			include "/htdocs/webinc/config.php";
			include "/htdocs/phplib/xnode.php";
			setattr("/runtime/antenna/script", "get", "sh /etc/scripts/antenna.sh");
			get("s", "/runtime/antenna/script");
			$i=1;
			$wifin=1;
			while($wifin<=4)
			{
				if($wifin==1) $WLAN = $WLAN1;
				else if($wifin==2) $WLAN = $WLAN1_GZ;
				else if($wifin==3) $WLAN = $WLAN2;
				else if($wifin==4) $WLAN = $WLAN2_GZ;
				$WLAN_rphy = XNODE_getpathbytarget("/runtime", "phyinf", "uid", $WLAN);
				foreach ($WLAN_rphy."/media/clients/entry")
				{
					$mac = get("", "macaddr");
					set("/runtime/antenna/entry:".$i."/macaddr", $mac);
					setattr("/runtime/antenna/entry:".$i."/antenna", "get", "scut -p ".$mac." -f 8 -n 3 /var/antenna_info");
					$i++;
				}
				$wifin++;
			}
			echo dump(3, "/runtime/antenna");
?>		</antenna>
	</runtime>
	<FATLADY>ignore</FATLADY>
	<SETCFG>ignore</SETCFG>
	<ACTIVATE>ignore</ACTIVATE>
</module>
