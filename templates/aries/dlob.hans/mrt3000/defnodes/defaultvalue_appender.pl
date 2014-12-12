#!/usr/bin/perl

use strict;

#try to open default values, from first argument
my $defval_file_name = shift @ARGV;

open DEFAULT_VALUE_FILE , "<$defval_file_name" or die $!;

#read orginal defaultvalue.xml
my $file_content = join '' , <DEFAULT_VALUE_FILE>;

my $inserted_data = "";
if($#ARGV >= 0)
{
	#use diamond to get all inserted blocks
	$inserted_data = join '' , <>;
}

#fine insert point
$file_content =~ m/<\/SiGnAtUrE>/g or die "bad defaultvalue.xml file format\n";

#insert our default value blocks
my $rewriten_file_content = $`.$inserted_data.$&.$';

print $rewriten_file_content;
