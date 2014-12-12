<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";


$capwap_pid = "/var/run/capwap.pid";
$capwapactive = query("/capwap/active");

fwrite(w, $START, "#!/bin/sh\n");
fwrite(w, $STOP, "#!/bin/sh\n");
if($capwapactive=="1") // if capwap enable 
{
  fwrite(a, $START, '/usr/sbin/capwap & > /dev/console\n'.
                    'echo $! > '.$capwap_pid.'\n'.
                    'exit 0\n');
  

  fwrite(a, $STOP, 'if [ -f '.$capwap_pid.' ]; then\n'.
                   '  echo Stop WTP daemon ... > /dev/console\n'.
                   '  /etc/scripts/killpid.sh '.$capwap_pid.'\n'.
                   'fi\n\n'.
                   'exit 0\n');
}
else
{   
  fwrite(a, $STOP, 'if [ -f '.$capwap_pid.' ]; then\n'.
                   '  echo Stop WTP daemon ... > /dev/console\n'.
                   '  /etc/scripts/killpid.sh '.$capwap_pid.'\n'.
                   'fi\n\n'.
                   'exit 0\n');
}
?> 
