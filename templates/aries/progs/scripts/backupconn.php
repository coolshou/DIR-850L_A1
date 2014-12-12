<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";

echo "#!/bin/sh\n";

$wanp		= XNODE_getpathbytarget("", "inf", "uid", $INF);
$backup		= query($wanp."/backup");
$chkinterval= query($wanp."/chkinterval");
if ($chkinterval == "") $chkinterval=10;

$run_infp	= XNODE_getpathbytarget("/runtime", "inf", "uid", $INF, 0);
$addrtype	= query($run_infp."/inet/addrtype");
//$dns		= query($run_infp."/inet/".$addrtype."/dns");
$dns		= "";
$run_phyinf	= query($run_infp."/phyinf");
$run_phyinfp= XNODE_getpathbytarget("/runtime", "phyinf", "uid", $run_phyinf, 0);
$name		= query($run_phyinfp."/name");


if ($backup != "")
{
	set($run_infp.'/inet/conn', $CONN);
	if ($CONN == "connected")
	{
		$run_bkwanp = XNODE_getpathbytarget("/runtime", "inf", "uid", $backup, 0);
		if ($run_bkwanp != "")
		{
			$pppdsta = query($run_bkwanp."/pppd/status");
			if($pppdsta == "connected")
			{
				echo	'event INFSVCS.'.$backup.'.DOWN add '.
						'"sh /etc/scripts/conntrack_flush.sh; '.
						'event INFSVCS.'.$backup.'.DOWN add true"\n';
				echo	'event '.$backup.'.PPP.HANGUP\n';
			}
		}
	}
	else if ($CONN == "disconnected")
	{
		$run_bkwanp = XNODE_getpathbytarget("/runtime", "inf", "uid", $backup, 0);
		if ($run_bkwanp != "")
		{
			$pppdsta = query($run_bkwanp."/pppd/status");

			$inf_con_sta	= query($run_infp."/pppd/status");
			echo 'echo \"######inf_con_sta value is '.$inf_con_sta.' > /dev/console\"\n';
			//follow is WAN-3 ppp sta judge.
			//WAN-3 disconnected.
			if ($pppdsta == "disconnected" || $pppdsta == "")
			{
				if($inf_con_sta != "connected")
				{
					if($addrtype == "ppp4")
					{
						echo 'event '.$INF.'.PPP.HANGUP\n';
					}
					echo	'event INFSVCS.'.$backup.'.UP add '.
							'"sh /etc/scripts/conntrack_flush.sh; '.
							'event INFSVCS.'.$backup.'.UP add true"\n';
					echo	'event '.$backup.'.PPP.DIALUP\n';
				}
				else if($inf_con_sta == "connected")
				{
                                	echo    'event INFSVCS.'.$backup.'.DOWN add '.
                                                	'"sh /etc/scripts/conntrack_flush.sh; '.
                                                	'event INFSVCS.'.$backup.'.DOWN add true"\n';
                                	echo    'event '.$backup.'.PPP.HANGUP\n';
				}
			}
			//WAN-3 connected.
			else if($pppdsta == "connected")
			{
				if($inf_con_sta == "connected")
				{
					echo	'echo \"####wan-1 and wan-3 both connected,hangup wan-1\"\n';
					echo    'event INFSVCS.'.$backup.'.DOWN add '.
						'"sh /etc/scripts/conntrack_flush.sh; '.
						'event INFSVCS.'.$backup.'.DOWN add true"\n';
					echo    'event '.$backup.'.PPP.HANGUP\n';
				}
			}
		}
		else
		{
			$inf_con_sta	= query($run_infp."/pppd/status");
			if($inf_con_sta != "connected")
			{
				if($addrtype == "ppp4")
	                        {
	       	                 	echo 'event '.$INF.'.PPP.HANGUP\n';
   	    	                 }
				echo	'event INFSVCS.'.$backup.'.UP add '.
						'"sh /etc/scripts/conntrack_flush.sh; '.
						'event INFSVCS.'.$backup.'.UP add true"\n';
				echo	'event '.$backup.'.PPP.DIALUP\n';
			}
                        else if($inf_con_sta == "connected")
                        {
                        	echo    'event INFSVCS.'.$backup.'.DOWN add '.
                       			'"sh /etc/scripts/conntrack_flush.sh; '.
                        		'event INFSVCS.'.$backup.'.DOWN add true"\n';
                        	echo    'event '.$backup.'.PPP.HANGUP\n';
                        }			
		}
	}
	if ($name == "")
	{
			echo 'xmldbc -t chkconn.'.$INF.':'.$chkinterval.':"chkconn -n '.$INF.'"\n';
	}
	else
	{
			if ($dns == "")
				echo 'xmldbc -t chkconn.'.$INF.':'.$chkinterval.':"chkconn -i '.$name.' -n '.$INF.'"\n';
			else
				echo 'xmldbc -t chkconn.'.$INF.':'.$chkinterval.':"chkconn -i '.$name.' -n '.$INF.' -d '.$dns.'"\n';
	}

}
echo "exit 0\n";
?>
