<? /* vi: set sw=4 ts=4: */
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/phyinf.php";

function strip_char($original, $char)
{
        $cnt = cut_count($original, $char);
        if ($cnt == 0) return $original;

        $i = 0;
        $strip = "";
        while ($i < $cnt)
        {
                $strip = $strip.cut($original, $i, $char);
                $i++;
        }
        return $strip;
}

function iobb_user_agent($model)
{
        $model = strip_char($model, '-');
        $model = strip_char($model, '/');
        $model = strip_char($model, '.');
        return $model."/1.2";
}
function ddns4generate($inf, $phyinf, $ddnsp)
{
        /* the interface status */
        $stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $inf, 0);

        /* Get the network info. */
        $addrtype       = query($stsp."/inet/addrtype");
        if ($addrtype == "ipv4")        $ipaddr = query($stsp."/inet/ipv4/ipaddr");
        else                                            $ipaddr = query($stsp."/inet/ppp4/local");

        /* Get the DDNS service index*/
        anchor($ddnsp);
        $provider       = query("provider");
        $username       = get("s", "username");
        $password       = get("s", "password");
        $hostname       = get("s", "hostname");
        $interval       = get("s", "interval");
        if ($provider == "IOBB")
        {
                $model          = query("/runtime/device/modelname");
                $hostname       = $hostname.".iobb.net";
                if($interval=="")       $interval       = 14400; /* 10 days */
                $useragent      = iobb_user_agent($model);
        }
        else
        {
                $vendor         = query("/runtime/device/vendor");
                $model          = query("/runtime/device/modelname");
                if($interval=="")       $interval       = 21600; /* 15 days */
                $useragent      = '"'.$vendor.' '.$model.'"';
        }

        set($stsp."/ddns4/valid",               "1");
        set($stsp."/ddns4/provider",    $provider);
        if( $provider != "ORAY")
        {
                $cmd = "susockc /var/run/ddnsd.susock DUMP ".$provider;
                setattr($stsp."/ddns4/uptime",  "get", $cmd." | scut -p uptime:");
                setattr($stsp."/ddns4/ipaddr",  "get", $cmd." | scut -p ipaddr:");
                setattr($stsp."/ddns4/status",  "get", $cmd." | scut -p state:");
                setattr($stsp."/ddns4/result",  "get", $cmd." | scut -p result:");

                $set = 'SET '.$provider.' "'.$ipaddr.'" "'.$username.'" "'.$password.'" "'.$hostname.'" '.$interval;
                /* start the application */
                fwrite("a",$_GLOBALS["START"],
                        'event DDNS4.'.$inf.'.UPDATE add "susockc /var/run/ddnsd.susock UPDATE '.$provider.'"\n'.
                        'susockc /var/run/ddnsd.susock USERAGENT '.$useragent.'\n'.
                        'susockc /var/run/ddnsd.susock '.$set.'\n'.
                        'xmldbc -s '.$stsp.'/ddns4/valid 1\n'.
                        'xmldbc -s '.$stsp.'/ddns4/provider '.$provider.'\n'.
                        'exit 0\n');

                fwrite("a", $_GLOBALS["STOP"],
                        'event DDNS4.'.$inf.'.UPDATE add true\n'.
                        'xmldbc -s '.$stsp.'/ddns4/valid 0\n'.
                        'xmldbc -s '.$stsp.'/ddns4/provider ""\n'.
                        'susockc /var/run/ddnsd.susock DEL '.$provider.'\n'.
                        'exit 0\n');
        }
        else
        {
                setattr($stsp."/ddns4/uptime",  "get", "scut -p uptime: /var/run/peanut.info");
                setattr($stsp."/ddns4/ipaddr",  "get", "scut -p ip:     /var/run/peanut.info");
                setattr($stsp."/ddns4/status",  "get", "scut -p status: /var/run/peanut.info");
                setattr($stsp."/ddns4/usertype","get", "cat /var/run/peanut_user_type");

                fwrite("a",$_GLOBALS["START"],
                        // 'event DDNS4.'.$inf.'.UPDATE add "susockc /var/run/ddnsd.susock UPDATE '.$provider.'"\n'.
                        // 'susockc /var/run/ddnsd.susock USERAGENT '.$useragent.'\n'.
                        // 'susockc /var/run/ddnsd.susock '.$set.'\n'.
                        'echo Start peanut...\n'.
                        'peanut -S "5" -u "'.$username.'" -p "'.$password.'" -i '.$ipaddr.' -q "/var/run/peanut.info" > /dev/console &\n'.
                        'xmldbc -s '.$stsp.'/ddns4/valid 1\n'.
                        'xmldbc -s '.$stsp.'/ddns4/provider '.$provider.'\n'.
                        'exit 0\n');

                fwrite("a", $_GLOBALS["STOP"],
                        'echo Stop peanut...\n'.
                        'killall -16 peanut\n'.
                        // 'event DDNS4.'.$inf.'.UPDATE add true\n'.
                        'xmldbc -s '.$stsp.'/ddns4/valid 0\n'.
                        'xmldbc -s '.$stsp.'/ddns4/provider ""\n'.
                        'rm -rf /var/run/peanut.info\n'.
                        'rm -rf /var/run/peanut_user_type'. 
                        'rm -rf /var/run/all_ddns_domain_name'. 
                        // 'susockc /var/run/ddnsd.susock DEL '.$provider.'\n'.
                        'exit 0\n');
        }
}

function ddns_error($errno)
{
        fwrite("a", $_GLOBALS["START"], "exit ".$errno."\n");
        fwrite("a", $_GLOBALS["STOP"],  "exit ".$errno."\n");
}

function ddns4setup($v6name,$name)
{
        /* Get the interface */
        $stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $name, 0);
        $infp = XNODE_getpathbytarget("", "inf", "uid", $name, 0);
        if ($stsp=="" || $infp=="")                     { ddns_error("9"); return; }
        /* Is this interface active ? */

        $active = query($infp."/active");
        $ddns   = query($infp."/ddns4");
        if ($active!="1" || $ddns == "")        { ddns_error("8"); return; }
        /* Check runtime status */
        $addrtype = query($stsp."/inet/addrtype");
        if ($addrtype != "ipv4" && $addrtype != "ppp4") { ddns_error("7"); return; }
        if (query($stsp."/inet/".$addrtype."/valid")!="1") { ddns_error("6"); return; }
        /* Get the physical interface */
        $phyinf = query($infp."/phyinf");
        if ($phyinf == "")                                      { ddns_error("9"); return; }
        /* Get the profile */
        $ddnsp = XNODE_getpathbytarget("/ddns4", "entry", "uid", $ddns, 0);
        if ($ddnsp=="")                                         { ddns_error("9"); return; }

        $v6_stsp = XNODE_getpathbytarget("/runtime", "inf", "uid", $v6name, 0);
        $v6_infp = XNODE_getpathbytarget("", "inf", "uid", $v6name, 0);
        if ($v6_stsp=="" || $v6_infp=="")                       {$v6enable=0; }
        else {$v6enable=1; }

        $v6active       = query($v6_infp."/active");
        if ($v6active!="1" )    { $v6enable=0;}
        else {$v6enable=1; }

        $v6phyinf = query($v6_infp."/phyinf");
        if ($v6phyinf == "")                                    { $v6enable=0;}
        else {$v6enable=1; }

        ddns4generate($name, $phyinf, $ddnsp,$v6name,$v6enable,$v6phyinf);
}

/*************************************************************************/
?>