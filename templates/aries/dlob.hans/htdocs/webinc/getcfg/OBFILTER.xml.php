<module>
	<service><?=$GETCFG_SVC?></service>
	<acl>
<?
if($GETCFG_SVC=="OBFILTER-2")
{
	$target ="obfilter2";
}
else
{
	$target = tolower($GETCFG_SVC);
}
echo "\t\t<".$target.">\n";
echo dump(3, "/acl/".$target);
echo "\t\t</".$target.">";
?>
	</acl>
</module>
