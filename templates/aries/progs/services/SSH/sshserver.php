<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";
include "/htdocs/phplib/inf.php";
function sshdstart($name, $sshdpro)
{ 
    $DAEMON="/usr/sbin/sshd";
    $CONFIG="/var/etc/sshd_config";

    $infp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);
    $sshd	= query($infp."/sshd");
    $sshdpro = XNODE_getpathbytarget("/sshd", "entry", "uid", $sshd, 0);
    $sshactive = query($sshdpro."/active");
    $stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);
    $addrtype = query($stsp."/inet/addrtype");
 
    if($sshactive == 1)
    { 
         
     if ($addrtype=="ipv4") //ipv4
     {  
        fwrite("a",$_GLOBALS["START"],"xmldbc -P /etc/services/SSH/sshdconf4.php > ".$CONFIG."\n");
     }
     else //ipv6
     { 
        fwrite("a",$_GLOBALS["START"],"xmldbc -P /etc/services/SSH/sshdconf6.php > ".$CONFIG."\n");
     }
     fwrite("a",$_GLOBALS["START"],"sleep 1\n");
   //fwrite("a",$_GLOBALS["START"], $DAEMON." -f ".$CONFIG." & > /dev/console\n");	
     fwrite("a",$_GLOBALS["START"], $DAEMON." & > /dev/console\n");	
     fwrite("a",$_GLOBALS["START"], "exit 0\n");         
    }
 
}
function sshdstop($name, $sshdpro)
{      
    $PIDFILE="/var/run/sshd.pid";
    $CONFIG="/var/etc/sshd_config";

      fwrite("a",$_GLOBALS["STOP"], "if [ -f ".$PIDFILE." ]; then\n");
    	fwrite("a",$_GLOBALS["STOP"], "	PID=`cat ".$PIDFILE."`\n");
    	fwrite("a",$_GLOBALS["STOP"], "	if [ \"$PID\" != \"0\" ]; then\n");
    	fwrite("a",$_GLOBALS["STOP"], "		kill $PID\n");
    	fwrite("a",$_GLOBALS["STOP"], "		rm -f ".$PIDFILE."\n");
    	fwrite("a",$_GLOBALS["STOP"], "   rm -f ".$CONFIG."\n");
    	fwrite("a",$_GLOBALS["STOP"], "	fi\n");
    	fwrite("a",$_GLOBALS["STOP"], "fi\n");
    	fwrite("a",$_GLOBALS["STOP"], "exit 0\n");
}
function sshdsetup($name)
{

  	/* Get the interface */
	$stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);
	$infp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);

	
	if ($stsp=="" || $infp=="")
	{
		SHELL_info($_GLOBALS["START"], "sshdsetup: (".$name.") no interface.");
		SHELL_info($_GLOBALS["STOP"],  "sshdsetup: (".$name.") no interface.");
		return;
	}
		/* Is this interface active ? */
	$active	= query($infp."/active");
	$sshd	= query($infp."/sshd");

	
	if ($active!="1" || $sshd == "")
	{
		SHELL_info($_GLOBALS["START"], "sshdsetup: (".$name.") not active.");
		SHELL_info($_GLOBALS["STOP"],  "sshdsetup: (".$name.") not active.");
		return;
	}
	/* Get the profile */
	
	$sshdpro = XNODE_getpathbytarget("/sshd", "entry", "uid", $sshd, 0);
	
	if ($sshdpro=="")
	{
		SHELL_info($_GLOBALS["START"], "sshdsetup: (".$name.") no profile.");
		SHELL_info($_GLOBALS["STOP"],  "sshdsetup: (".$name.") no profile.");
		return;
	}

	sshdstart($name, $sshdpro);
	sshdstop($name, $sshdpro);
}
?>