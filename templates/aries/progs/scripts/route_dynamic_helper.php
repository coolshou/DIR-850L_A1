#!/bin/sh
<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
  if($FAMILY == 2)//ipv4
  {   
    if($MASK == "255.0.0.0") 
        $prefixlen = 8;
    if($MASK == "255.255.0.0") 
        $prefixlen = 16;
    if($MASK == "255.255.255.0") 
        $prefixlen = 24;
    if($MASK == "255.255.255.255") 
        $prefixlen = 32;  
      
    $base	= "/runtime/dynamic/route";   
    if($TYPE == 24) //RTM_NEWROUTE
    {  
      $cnt = query($base."/entry#");
      if ($cnt=="") $cnt=0;
      $cnt++; 

      $network = query($base."/entry:".$cnt."/network");
      if($network =="")
      {
        set($base."/entry:".$cnt."/network", $DEST);
        set($base."/entry:".$cnt."/mask", $MASK);
        set($base."/entry:".$cnt."/via",$GATE);
        set($base."/entry:".$cnt."/metric",$METRIC);
        set($base."/entry:".$cnt."/inf",$INF);
      }
      echo 'ip route add '.$DEST.'/'.$prefixlen.' via '.$GATE.' dev '.$INF.' metric '.$METRIC.' table DYNAMIC\n';
      echo 'exit 0\n';
    }
    else  //RTM_DELROUTE
    {
      $i = 1;   
      $cnt = query($base."/entry#");
      while($i <= $cnt)
      {
        $network = query($base."/entry:".$i."/network");
        $mask = query($base."/entry:".$i."/mask");
        $via = query($base."/entry:".$i."/via");
        if($network == $DEST && $mask == $MASK && $via == $GATE)
        {
          del($base."/entry:".$i.);
        }
        $i++;
     }
      echo 'ip route del '.$DEST.'/'.$prefixlen.' via '.$GATE.' dev '.$INF.' metric '.$METRIC.' table DYNAMIC\n';
      echo 'exit 0\n';
    }
  }
  else if($FAMILY == 10)//ipv6
  {
    $base	= "/runtime/dynamic/route6";      
    if($TYPE == 24) //RTM_NEWROUTE
    {  
      $cnt = query($base."/entry#");
      if ($cnt=="") $cnt=0;
      $cnt++;
     // $max =64;

      $ipaddr = query($base."/entry:".$cnt."/ipaddr");
      if($ipaddr =="")
      {
        set($base."/entry:".$cnt."/ipaddr", $DEST);
        set($base."/entry:".$cnt."/prefix", $MASK);
        set($base."/entry:".$cnt."/gateway",$GATE);
        set($base."/entry:".$cnt."/metric",$METRIC);
        set($base."/entry:".$cnt."/inf",$INF);
      }
      /*
      while ($cnt >= $max)
  		{
  			$cnt--;
  			del($base."/entry:1");
  		} */  
  		if($GATE != "")
            echo 'ip -6 route add '.$DEST.'/'.$MASK. ' via '.$GATE.' dev '.$INF.' metric '.$METRIC.' table DYNAMIC\n';     
      else  echo 'ip -6 route add '.$DEST.'/'.$MASK.' dev '.$INF.' metric '.$METRIC.' table DYNAMIC\n';
      echo 'exit 0\n';
    }
    else  //RTM_DELROUTE
    {
      $i = 1;   
      $cnt = query($base."/entry#");
      while($i <= $cnt)
      {
        $ipaddr = query($base."/entry:".$i."/ipaddr");
        $mask = query($base."/entry:".$i."/prefix");
        $via = query($base."/entry:".$i."/gateway");
        if($ipaddr == $DEST && $mask == $MASK && $via == $GATE)
        {
          del($base."/entry:".$i.);
        }
        $i++;
     }
     if($GATE != "")
            echo 'ip -6 route del '.$DEST.'/'.$MASK. ' via '.$GATE.' dev '.$INF.' metric '.$METRIC.' table DYNAMIC\n';
     else   echo 'ip -6 route del '.$DEST.'/'.$MASK. ' dev '.$INF.' metric '.$METRIC.' table DYNAMIC\n';
      echo 'exit 0\n';
    }
  }

?>
