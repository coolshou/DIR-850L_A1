<module>
	<service><?=$GETCFG_SVC?></service>
	<dns><? echo "\n".dump(2, "/dns")."\t";?></dns>
<?  foreach("/inf") if (cut(query("uid"),0,'-')!="WAN") echo "\t<inf>\n".dump(2,"")."\t</inf>\n";
?></module>
