<?
include "/etc/services/PHYINF/phywifi.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

$CARD_CONF="/var/run/RT2860APCard.dat";
fwrite("w",$CARD_CONF,"Default\n");
fwrite("a",$CARD_CONF,"SELECT=CARDID\n");
fwrite("a",$CARD_CONF,"00CARDID=/var/run/RT3572.dat\n");
fwrite("a",$CARD_CONF,"01CARDID=/var/run/RT2860.dat\n");

echo "insmod /lib/modules/rt5592ap.ko\n";
?>
