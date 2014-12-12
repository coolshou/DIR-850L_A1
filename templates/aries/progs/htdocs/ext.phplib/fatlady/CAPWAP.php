<?
include "/htdocs/phplib/trace.php";
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";


function set_result($result, $node, $message)
{
    $_GLOBALS["FATLADY_result"] = $result;
    $_GLOBALS["FATLADY_node"]   = $node;
    $_GLOBALS["FATLADY_message"]= $message;
    return $result;
}
set_result("FAILED","","");
$rlt = "0";
$cnt = query($FATLADY_prefix."/capwap/aclist/count");
TRACE_debug("FATLADY: CAPWAP.ACCOUNT : ".$cnt);

$i = 0;

while ($i < $cnt)
{
    $i++;
    $staticip = query($FATLADY_prefix."/capwap/aclist/entry:".$i."/staticip");
    $acname = query($FATLADY_prefix."/capwap/aclist/entry:".$i."/acname");
    if (ipv4networkid($staticip,32)=="" && $staticip!="")
    { 
       set_result("FAILED",$FATLADY_prefix."/capwap/aclist/entry:".$i,"The AC address is invalid.");
       $rlt = "-1";
       break;
    }

    $j =0;    
    foreach ("/capwap/aclist/entry")
    {
      $j++;
      $num = query("/capwap/aclist/entry#");
      if (query("uid")=="")
      {
          break;
      } 

      /*staticip */
      $db_staticip = query("/capwap/aclist/entry:".$j."/staticip");          
      if ($db_staticip == $staticip)
      {
        set_result("FAILED",$FATLADY_prefix."/capwap/aclist/entry:".$j,"The AC address is the same.");
        $rlt = "-1";
        break;

      }  
      /*acname */ 
      $db_acname = query("/capwap/aclist/entry:".$j."/acname");    
      if ($db_acname == $acname)
      {
        set_result("FAILED",$FATLADY_prefix."/capwap/aclist/entry:".$j,"The AC name is the same.");
        $rlt = "-1";
        break;

      }     
    }      
}

if($rlt == "0")
{
    set($FATLADY_prefix."/valid", 1);
    set_result("OK", "", "");
}

?>
