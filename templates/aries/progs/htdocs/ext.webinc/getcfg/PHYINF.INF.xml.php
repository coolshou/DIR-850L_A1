<module>
	<service>PHYINF.INF</service>
<?
	foreach ("/inf")	echo "\t<inf>\n".dump(2, "")."\t</inf>\n";
	foreach ("/phyinf")	echo "\t<phyinf>\n".dump(2, "")."\t</phyinf>\n";
?></module>
