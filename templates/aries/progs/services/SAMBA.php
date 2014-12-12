<?
include "/htdocs/phplib/inf.php";

$SAMBAP		= "/var/etc/samba";
$SAMBACFG	= $SAMBAP."/smb.conf";
$MNTROOT	= "/var/tmp/storage";

fwrite("w",$START, "#!/bin/sh\n");
fwrite("a",$START, "if [ ! -d ".$SAMBAP." ]; then mkdir -p ".$SAMBAP."; fi\n");
fwrite("a",$START, "if [ ! -f ".$SAMBAP."/smbpasswd ]; then adduser nobody; smbpasswd -a nobody -n; fi\n");
fwrite("a",$START, "smbd -D\n");
fwrite("a",$START, "nmbd -D\n");
fwrite("w",$STOP,  "#!/bin/sh\n");
fwrite("a",$STOP,  "killall nmbd\n");
fwrite("a",$STOP,  "killall smbd\n");

fwrite("w",$SAMBACFG, "[global]\n");
fwrite("a",$SAMBACFG, "\tworkgroup = ALPS\n");
fwrite("a",$SAMBACFG, "\tserver string = ".query("/runtime/device/modelname")." Samba Server\n");
fwrite("a",$SAMBACFG, "\tnetbios name = ".query("/device/hostname")."\n");
//fwrite("a",$SAMBACFG, "\tkernel change notify = no\n");
fwrite("a",$SAMBACFG, "\twinbind nested groups = no\n");
fwrite("a",$SAMBACFG, "\tdomain master = no\n");
//fwrite("a",$SAMBACFG, "\tpublic = yes\n");
fwrite("a",$SAMBACFG, "\tinterfaces = ".INF_getcfgipaddr("LAN-1")."/".INF_getcfgmask("LAN-1")."\n");
//fwrite("a",$SAMBACFG, "\tload printers = no\n");
//fwrite("a",$SAMBACFG, "\tprinting = bsd\n");
//fwrite("a",$SAMBACFG, "\tprintcap name = /dev/null\n");
fwrite("a",$SAMBACFG, "\tsecurity = share\n");
fwrite("a",$SAMBACFG, "\tsocket options = IPTOS_LOWDELAY TCP_NODELAY SO_KEEPALIVE SO_RCVBUF=65536 SO_SNDBUF=65536\n");
fwrite("a",$SAMBACFG, "\tdns proxy = no\n");
//fwrite("a",$SAMBACFG, "[Storage]\n");
//fwrite("a",$SAMBACFG, "\tcomment = Temporary file space\n");
//fwrite("a",$SAMBACFG, "\tpath = /var/tmp/storage\n");
fwrite("a",$SAMBACFG, "\twriteable = yes\n");
fwrite("a",$SAMBACFG, "\tpublic = yes\n");
fwrite("a",$SAMBACFG, "\toplocks = no\n");
fwrite("a",$SAMBACFG, "\tcreate mask = 0777\n");
fwrite("a",$SAMBACFG, "\tdirectory mask = 0777\n");

foreach("/runtime/device/storage/disk")
{
	$disk_n=$InDeX;
	foreach("entry")
	{
		$mntpath = query("/runtime/device/storage/disk:".$disk_n."/entry:".$InDeX."/mntp");
		$mntname = cut($mntpath, 4, "/");
		fwrite("a",$SAMBACFG, "[".$mntname."]\n");
		fwrite("a",$SAMBACFG, "\tcomment = Temporary file space\n");
		fwrite("a",$SAMBACFG, "\tpath = ".$mntpath."\n");
	}
}
/*
fwrite(a,$SAMBACFG,
	"[USBDISK]\n".
	"	comment = USB flash driver\n".
	"	path = /var/tmp/storage/sda_UDISK_PDU014G84H20_1\n".
	"	browseable = yes\n".
	"	writeable = yes\n".
	"	public = yes\n"
	);
*/
?>
