map() function test ......
<?

set(/test/node/entry1, "node1");
set(/test/node/entry2, "node2");
set(/test/node/entry3, "node3");

echo "entry 1 value is ".query(/test/node/entry1)."\n";
echo "entry 2 value is ".query(/test/node/entry2)."\n";
echo "entry 3 value is ".query(/test/node/entry3)."\n";

$message = "You got the right message !!!!";

map(/test/node/entry1,node1,"Value 1",node2,"Value 2",*,"Unknown Value"); echo "\n";
map(/test/node/entry2,node1,"Value 1",node2,"Value 2",*,"Unknown Value"); echo "\n";
map(/test/node/entry3,node1,"Value 1",node2,"Value 2",*,"Unknown Value"); echo "\n";
map(/test/node/entry1, node1 , $message , node2 , "Value 2" , * , "Unknown Value"  ); echo "\n";
map(/test/node/entry2, node1 , $message , node2 , "Value 2" , * , "Unknown Value"  ); echo "\n";
map(/test/node/entry3, node1 , $message , node2 , "Value 2" , * , "Unknown Value"  ); echo "\n";

?>

show message test <?=$message?>
test done !
