php echo test:

<?

set("/test/echo/node1",1);

$var1=0;
$var1 = $var1 + 1;
$var2 = query("/test/echo/node1");

$string = "123456;7890\n";
echo $string;

echo "var1 = ".$var1."\n";
echo "var2 = ".$var2."\n";
exit;
echo "test; test;"."\n";

?>
