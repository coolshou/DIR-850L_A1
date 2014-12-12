<?
include "/htdocs/phplib/xnode.php";
include "/htdocs/phplib/trace.php";

/* I save the channel list into /runtime/freqrule/channellist/a & runtime/freqrule/channellist/g */
$path_a = "/runtime/freqrule/channellist/a";
$path_g = "/runtime/freqrule/channellist/g";

$c = query("/runtime/devdata/countrycode");
if ($c == "")
{
	TRACE_error("phplib/getchlist.php - GETCHLIST() ERROR: no Country Code!!! Please check if you board is initialized.");
	return;
}
if (isdigit($c)==1)
{
	TRACE_error("phplib/getchlist.php - GETCHLIST() ERROR: Country Code (".$c.") is not in ISO Name!! Please use ISO name insteads of Country Number.");
	return;
}

/* never set the channel list, so do it.*/
if (query($path_a)=="" || query($path_g)=="")
{
	/* map the region by country ISO name */
	if		($c == "AL")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "DZ")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "AR")	{$region_a =  "3";	$region_g = "1";}
	else if ($c == "AM")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "AU")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "AT")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "AZ")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "BH")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "BY")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "BE")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "BZ")	{$region_a =  "4";	$region_g = "1";}
	else if ($c == "BO")	{$region_a =  "4";	$region_g = "1";}
	else if ($c == "BR")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "BN")	{$region_a =  "4";	$region_g = "1";}
	else if ($c == "BG")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "CA")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "CL")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "CN")	{$region_a =  "4";	$region_g = "1";}
	else if ($c == "CO")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "CR")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "HR")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "CY")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "CZ")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "DK")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "DO")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "EC")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "EG")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "SV")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "EE")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "FI")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "FR")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "GE")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "DE")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "GR")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "GT")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "HN")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "HK")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "HU")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "IS")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "IN")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "ID")	{$region_a =  "4";	$region_g = "1";}
	else if ($c == "IR")	{$region_a =  "4";	$region_g = "1";}
	else if ($c == "IE")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "IL")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "IT")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "JP")	{$region_a =  "9";	$region_g = "1";}
	else if ($c == "JO")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "KZ")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "KP")	{$region_a =  "5";	$region_g = "1";}
	else if ($c == "KR")	{$region_a =  "5";	$region_g = "1";}
	else if ($c == "KW")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "LV")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "LB")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "LI")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "LT")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "LU")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "MO")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "MK")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "MY")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "MX")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "MC")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "MA")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "NL")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "NZ")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "NO")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "OM")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "PK")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "PA")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "PE")	{$region_a =  "4";	$region_g = "1";}
	else if ($c == "PH")	{$region_a =  "4";	$region_g = "1";}
	else if ($c == "PL")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "PT")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "PR")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "QA")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "RO")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "RU")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "SA")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "SG")	{$region_a =  "0";	$region_g = "1";}
	else if ($c == "SK")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "SI")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "ZA")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "ES")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "SE")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "CH")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "SY")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "TW")	{$region_a =  "3";	$region_g = "0";}
	else if ($c == "TH")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "TT")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "TN")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "TR")	{$region_a =  "2";	$region_g = "1";}
	else if ($c == "UA")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "AE")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "GB")	{$region_a =  "1";	$region_g = "1";}
	else if ($c == "US")	{$region_a =  "0";	$region_g = "0";}
	else if ($c == "UY")	{$region_a =  "5";	$region_g = "1";}
	else if ($c == "UZ")	{$region_a =  "1";	$region_g = "0";}
	else if ($c == "VE")	{$region_a =  "5";	$region_g = "1";}
	else if ($c == "VN")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "YE")	{$region_a = "-1";	$region_g = "1";}
	else if ($c == "ZW")	{$region_a = "-1";	$region_g = "1";}
	else /* match no ISO name! return ERROR message. */
	{
		return "phplib/getchlist.php - GETCHLIST() ERROR: countrycode (".$c.") doesn't match any list in GETCHLIST(). Please check it.";
	}

	/* map the A band channels */
	if		($region_a == "-1")	$list_a = "This country DOES NOT support 802.11A";
	else if	($region_a == "0")	$list_a = "36,40,44,48,52,56,60,64,149,153,157,161,165";
	else if ($region_a == "1")	$list_a = "36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140";
	else if ($region_a == "2")	$list_a = "36,40,44,48,52,56,60,64";
	else if ($region_a == "3")	$list_a = "52,56,60,64,149,153,157,161";
	else if ($region_a == "4")	$list_a = "149,153,157,161,165";
	else if ($region_a == "5")	$list_a = "149,153,157,161";
	else if ($region_a == "6")	$list_a = "36,40,44,48";
	else if ($region_a == "8")	$list_a = "52,56,60,64";
	else if ($region_a == "9")	$list_a = "36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165";
	else if ($region_a == "10")	$list_a = "36,40,44,48,149,153,157,161,165";
	else if ($region_a == "11")	$list_a = "36,40,44,48,52,56,60,64,100,104,108,112,116,120,149,153,157,161";

	/* map the A band channels */
	if		($region_g == "0")	$list_g = "1,2,3,4,5,6,7,8,9,10,11";
	else if	($region_g == "1")	$list_g = "1,2,3,4,5,6,7,8,9,10,11,12,13";
	else if	($region_g == "2")	$list_g = "10,11";
	else if	($region_g == "3")	$list_g = "10,11,12,13";
	else if	($region_g == "4")	$list_g = "14";
	else if	($region_g == "5")	$list_g = "1,2,3,4,5,6,7,8,9,10,11,12,13,14";
	else if	($region_g == "6")	$list_g = "3,4,5,6,7,8,9";
	else if	($region_g == "7")	$list_g = "5,6,7,8,9,10,11,12,13";

	set($path_a, $list_a);
	set($path_g, $list_g);
}
?>
