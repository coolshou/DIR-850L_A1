HTTP/1.1 200 OK
Content-Type: text/html

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<?
include "/htdocs/phplib/langpack.php";
//fwrite("w", "/dev/console", "AUTHORIZED_GROUP=".$AUTHORIZED_GROUP."\n");
//if ($AUTHORIZED_GROUP>=0 && $AUTHORIZED_GROUP<100)
//{
	if		($_POST["multilanguage"]=="en")		$lcode="en";
	else if	($_POST["multilanguage"]=="de")		$lcode="de";
	else if	($_POST["multilanguage"]=="fr")		$lcode="fr";
	else if	($_POST["multilanguage"]=="it")		$lcode="it";
	else if	($_POST["multilanguage"]=="es")		$lcode="es";
	else if	($_POST["multilanguage"]=="zhcn")	$lcode="zhcn";
	else if	($_POST["multilanguage"]=="zhtw")	$lcode="zhtw";
	else if	($_POST["multilanguage"]=="ko")		$lcode="ko";
	else if	($_POST["multilanguage"]=="ja")		$lcode="ja";
	else $lcode="auto";

	set("/device/features/language", $lcode);
	LANGPACK_setsealpac();
	event("DBSAVE");
//}
?>
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
</html>
