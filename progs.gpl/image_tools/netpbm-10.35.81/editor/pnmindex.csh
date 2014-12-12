#!/bin/csh -f
#
# pnmindex - build a visual index of a bunch of anymaps
#
# Copyright (C) 1991 by Jef Poskanzer.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.

# -title and -quant added by John Heidemann 13-Sep-00.

set size=100		# make the images about this big
set across=6		# show this many images per row
set colors=256		# quantize results to this many colors
set back="-white"	# default background color
set doquant=true	# quantize or not
set title=""		# default title (none)

while ( 1 )
    switch ( "$1" )

	case -s*:
	if ( $#argv < 2 ) goto usage
	set size="$2"
	shift
	shift
	breaksw

	case -a*:
	if ( $#argv < 2 ) goto usage
	set across="$2"
	shift
	shift
	breaksw

	case -t*:
	if ( $#argv < 2 ) goto usage
	set title="$2"
	shift
	shift
	breaksw

	case -c*:
	set colors="$2"
	shift
	shift
	breaksw

	case -noq*:
	set doquant=false
	shift
	breaksw

	case -q*:
	set doquant=true
	shift
	breaksw

	case -b*:
	set back="-black"
	shift
	breaksw

	case -w*:
	set back="-white"
	shift
	breaksw

	case -*:
	goto usage
	breaksw

	default:
	break
	breaksw

    endsw
end

if ( $#argv == 0 ) then
    goto usage
endif

set tmpfile=/tmp/pi.tmp.$$
rm -f $tmpfile
set maxformat=PBM

set rowfiles=()
set imagefiles=()
@ row = 1
@ col = 1

if ( "$title" != "" ) then
    set rowfile=/tmp/pi.${row}.$$
    rm -f $rowfile
    pbmtext "$title" > $rowfile
    set rowfiles=( $rowfiles $rowfile )
    @ row += 1
endif

foreach i ( $argv )

    set description=`pnmfile $i`
    if ( $description[4] <= $size && $description[6] <= $size ) then
	cat $i > $tmpfile
    else
	switch ( $description[2] )
	    case PBM:
	    pnmscale -quiet -xysize $size $size $i | pgmtopbm > $tmpfile
	    breaksw

	    case PGM:
	    pnmscale -quiet -xysize $size $size $i > $tmpfile
	    if ( $maxformat == PBM ) then
		set maxformat=PGM
	    endif
	    breaksw

	    default:
	    if ( $doquant == false ) then
	        pnmscale -quiet -xysize $size $size $i > $tmpfile
	    else
	        pnmscale -quiet -xysize $size $size $i | ppmquant -quiet $colors > $tmpfile
	    endif
	    set maxformat=PPM
	    breaksw
	endsw
    endif
    set imagefile=/tmp/pi.${row}.${col}.$$
    rm -f $imagefile
    if ( "$back" == "-white" ) then
	pbmtext "$i" | pnmcat $back -tb $tmpfile - > $imagefile
    else
	pbmtext "$i" | pnminvert | pnmcat $back -tb $tmpfile - > $imagefile
    endif
    rm -f $tmpfile
    set imagefiles=( $imagefiles $imagefile )

    if ( $col >= $across ) then
	set rowfile=/tmp/pi.${row}.$$
	rm -f $rowfile
	if ( $maxformat != PPM || $doquant == false ) then
	    pnmcat $back -lr -jbottom $imagefiles > $rowfile
	else
	    pnmcat $back -lr -jbottom $imagefiles | ppmquant -quiet $colors > $rowfile
	endif
	rm -f $imagefiles
	set imagefiles=()
	set rowfiles=( $rowfiles $rowfile )
	@ col = 1
	@ row += 1
    else
	@ col += 1
    endif

end

if ( $#imagefiles > 0 ) then
    set rowfile=/tmp/pi.${row}.$$
    rm -f $rowfile
    if ( $maxformat != PPM || $doquant == false ) then
	pnmcat $back -lr -jbottom $imagefiles > $rowfile
    else
	pnmcat $back -lr -jbottom $imagefiles | ppmquant -quiet $colors > $rowfile
    endif
    rm -f $imagefiles
    set rowfiles=( $rowfiles $rowfile )
endif

if ( $#rowfiles == 1 ) then
    cat $rowfiles
else
    if ( $maxformat != PPM || $doquant == false ) then
	pnmcat $back -tb $rowfiles
    else
	pnmcat $back -tb $rowfiles | ppmquant -quiet $colors
    endif
endif
rm -f $rowfiles

exit 0

usage:
echo "usage: $0 [-size N] [-across N] [-colors N] [-black] pnmfile ..."
exit 1
