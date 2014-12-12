del() test for php .............
<?

$n = "\n";

set(/test/node/entry1, "value1");
set(/test/node/entry2, "value2");

echo "entry1 has value : ".query(/test/node/entry1).$n;
echo "entry2 has value : ".query(/test/node/entry2).$n;

echo "deleting node [/test/node]".$n;
del(/test/node);

echo "entry1 has value : ".query(/test/node/entry1).$n;
echo "entry2 has value : ".query(/test/node/entry2).$n;

?>
