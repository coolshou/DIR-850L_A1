<?
/* flash programming speed */
$flashspeed = query("/runtime/devdata/flashspeed");
if ($flashspeed == "")  { $flashspeed=1800; }
if ($flashspeed < 1000) { $flashspeed=1000; }
$flashspeed = $flashspeed * 12;
$flashspeed = $flashspeed / 10;
set("/runtime/device/fptime", $flashspeed);
set("/runtime/device/bootuptime", 100);
?>
