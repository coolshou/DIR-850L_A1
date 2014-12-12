<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/inf.php";

$enable		= query("/device/samba/enable");
$SAMBAP		= "/var/etc/samba";
$SAMBACFG	= $SAMBAP."/smb.conf";
$MNTROOT	= "/var/tmp/storage";

fwrite("w",$START, "#!/bin/sh\n");
if($enable=="0")
{
		fwrite("a", $START, "#samba: samba not enable.\n");
		fwrite("a", $STOP,  "#samba: samba not enable.\n");
		fwrite("a", $START, "exit;\n");
		fwrite("a", $STOP, "exit;\n");
}
else
{
	//$mntp = query("/runtime/device/storage/disk/entry:1/mntp");	
	$passwd = query("/device/account/entry:1/password");//password of admin
	fwrite("a",$START, "#passwd is ".$passwd."\n");
	fwrite("a",$START, "#passwd_cmd is ".$passwd_cmd."\n");
	fwrite("a",$START, "if [ ! -d ".$SAMBAP." ]; then mkdir -p ".$SAMBAP."; fi\n");
	//fwrite("a",$START, "if [ ! -f ".$SAMBAP."/smbpasswd ]; then adduser nobody; smbpasswd -a nobody -n; fi\n");
	fwrite("a",$START, "if [ ! -f ".$SAMBAP."/smbpasswd ]; then ( echo \"".$passwd."\"; echo \"".$passwd."\" ) | smbpasswd -s -a admin; fi\n");
	fwrite("a",$START, "smbd -D\n");
	fwrite("a",$START, "nmbd -D\n");
	fwrite("w",$STOP,  "#!/bin/sh\n");
	fwrite("a",$STOP,  "killall nmbd\n");
	fwrite("a",$STOP,  "killall smbd\n");
	fwrite("a",$STOP,  "rm -rf ".$SAMBAP."\n");	

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
	fwrite("a",$SAMBACFG, "\tsecurity = user\n");
	fwrite("a",$SAMBACFG, "\tsocket options = IPTOS_LOWDELAY TCP_NODELAY SO_KEEPALIVE SO_RCVBUF=65536 SO_SNDBUF=65536\n");
	fwrite("a",$SAMBACFG, "\tdns proxy = no\n");
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
			fwrite("a",$SAMBACFG, "\tvalid users = admin\n");
		}
	}
}
?>
