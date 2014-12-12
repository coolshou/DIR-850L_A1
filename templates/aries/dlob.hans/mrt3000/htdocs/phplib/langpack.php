<?
function convert_lcode($primary, $subtag)
{
	$pri = tolower($primary);

	if ($pri=="zh")
	{
		$sub = tolower($subtag);
		if ($sub=="cn")	return "zhcn";
		else			return "zhtw";
	}
	return $pri;
}
function load_slp($lcode)
{
	$slp = "/etc/sealpac/".$lcode.".slp";
	if (isfile($slp)!="1") return 0;
	sealpac($slp);
	return 1;
}

function LANGPACK_setsealpac()
{
	$lcode = query("/device/features/language");
	if ($lcode=="auto" || $lcode=="")
	{
		$count = cut_count($_SERVER["HTTP_ACCEPT_LANGUAGE"], ',');
		$i = 0;
		while ($i < $count)
		{
			$tag = cut($_SERVER["HTTP_ACCEPT_LANGUAGE"], $i, ',');
			$pri = cut($tag, 0, '-');
			$sub = cut($tag, 1, '-');
			if (load_slp(convert_lcode($pri, $sub)) > 0) return;
			$i++;
		}
	}
	else if (load_slp($lcode) > 0) return;
	sealpac("/etc/sealpac/en.slp");	// Use system default language, en.
}
?>
