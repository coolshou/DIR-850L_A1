#!/usr/bin/perl
while (defined($line = <STDIN>))
{
	$_ = $line;
	if (/^diff -urN/)
	{
		print $line;
		$line = <STDIN>;
		@val = split(/\s+/,$line);
		print "$val[0] $val[1]\n";
		$line = <STDIN>;
		@val = split(/\s+/,$line);
		print "$val[0] $val[1]\n";
	}
	else
	{
		print $line;
	}
}
