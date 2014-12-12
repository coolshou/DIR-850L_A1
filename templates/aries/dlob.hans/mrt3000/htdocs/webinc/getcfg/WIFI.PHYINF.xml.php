<module>
	<service><?=$GETCFG_SVC?></service>
	<device>
		<radio24gonoff><? echo query("/device/radio24gonoff");?></radio24gonoff>
		<radio24gcfged><? echo query("/device/radio24gcfged");?></radio24gcfged>
		<wiz_freset><? echo query("/device/wiz_freset");?></wiz_freset>
		<wiz_clonemac><? echo query("/device/wiz_clonemac");?></wiz_clonemac>
	</device>
	<wifi>
<?		echo dump(2, "/wifi");
?>	</wifi>
<?
	foreach("/phyinf")
	{
		if (query("type")== "wifi")
		{
			echo '\t<phyinf>\n';
			echo dump(2, "/phyinf:".$InDeX);
			echo '\t</phyinf>\n';
		}
	}
?></module>
