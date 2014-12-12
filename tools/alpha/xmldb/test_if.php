-------------------------------
if test for embeded php :
<<?
set(/test/node1,1);
set(/test/node2,2);
set(/test/node3,3);
set(/test/nodenum, 3);

$var = query(/test/node1);

$var1 = 4123;
$var2 = 321;

if ($var1 < $var2) {
	echo "var1 is less then var2. var1=".$var1.", var2=".$var2;
} else {
	echo "var1 is greater than var2. var1=".$var1.", var2=".$var2;
}
echo "\n";

$var = 9;

echo "var=".$var."\n";
if      ($var == 0) { echo "var is zero.\n"; }
else if	($var == 1) { echo "var is one.\n"; }
else if ($var == 2) { echo "var is two.\n"; }
else if ($var == 3) { echo "var is three.\n"; }
else if ($var > 10) { echo "var is greater than ten.\n"; }
else                { echo "var is greater than three and less then 11\n"; }

if ($var % 2 == 0 && $var > 10) { echo "var is even and greater than ten.\n"; }
else { echo "var is odd.\n"; }

?>>
---------------------------------
