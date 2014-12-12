#!/bin/csh -f
#
#	Uses ghostscript to translate an Encapsulated PostScript file to
#	Portable Anymap format file(s).
#	pstopnm will create as many files as the number of pages in 
#	the Postscript document.  The name of the files will be 
#	psfile001.ppm, psfile002.ppm, etc.
#	The ouput files will contain the area inside the BoundingBox.
#	If BoundingBox parameters are not found in the PostScript
#	document, default values are used.
#
#
#       Usage: pstopnm [-forceplain] [-help] [-llx s] [-lly s] 
#		       [-urx s] [-ury s] [-nocrop] [-pbm|-pgm|-ppm] 
#		       [-verbose] [-xborder n] [-xmax n] [-xsize n] 
#		       [-yborder n] [-ymax n] [-ysize n] 
#		       [-portrait] [-landscape] psfile[.ps]
# 
# 	Copyright (C) 1992 by Alberto Accomazzi, Smithsonian Astrophysical
#	Observatory (alberto@cfa.harvard.edu).
#
# 	Permission to use, copy, modify, and distribute this software and its
#	documentation for any purpose and without fee is hereby granted, 
#	provided that the above copyright notice appear in all copies and 
#	that both that copyright notice and this permission notice appear 
#	in supporting documentation.  This software is provided "as is" 
#	without express or implied warranty.
#
set noglob

set progname = $0
set progname = $progname:t
set filtertail = "raw"
set filterhead = "ppm"
set xsize = 0
set ysize = 0
set xres = ""
set yres = ""

# default values: max image x and y sizes
set xmax = 612
set ymax = 792
# default values: image area fits in a 8.5x11 sheet with 1 inch border
set llx = 72
set lly = 72
set urx = 540
set ury = 720
# default values: x and y borders are 10% of x and y size
set xborder = "0.1"
set yborder = "0.1"
# default values: orientation is unknown
set orient = 0

set psfile = ""
set USAGE = "Usage: $progname [-forceplain] [-help] [-llx s] [-lly s]\
[-urx s] [-ury s] [-landscape] [-portrait]\
[-nocrop] [-pbm|-pgm|-ppm] [-verbose] [-xborder s] [-xmax s]\
[-xsize s] [-yborder s] [-ymax s] [-ysize s] psfile[.ps]"
alias usage 'echo $USAGE; exit 1'

while ($#argv > 0)
    switch ($argv[1])
    case -h*:   # -help
        usage
        breaksw
    case -pbm:
    case -pgm:
    case -ppm:
    	set filterhead = `echo "$argv[1]" | sed "s/-//1"`
	breaksw
    case -llx:
        shift argv
        if ($#argv == 0) eval usage
    	set llx = `(echo "scale=4";echo "$argv[1] * 72")|bc -l`
	set nobb
	breaksw
    case -lly:
        shift argv
        if ($#argv == 0) eval usage
	set lly = `(echo "scale=4";echo "$argv[1] * 72")|bc -l`
	set nobb
	breaksw
    case -urx:
        shift argv
        if ($#argv == 0) eval usage
	set urx = `(echo "scale=4";echo "$argv[1] * 72")|bc -l`
	set nobb
	breaksw
    case -ury:
        shift argv
        if ($#argv == 0) eval usage
	set ury = `(echo "scale=4";echo "$argv[1] * 72")|bc -l`
	set nobb
	breaksw
    case -no*:	# -nocrop
	set nocrop
	breaksw
    case -xs*:	# -xsize
        shift argv
        if ($#argv == 0) eval usage
	@ xsize = $argv[1]
	breaksw
    case -ys*:	# -ysize
        shift argv
        if ($#argv == 0) eval usage
	@ ysize = $argv[1]
	breaksw
    case -xm*:	# -xmax
        shift argv
        if ($#argv == 0) eval usage
	@ xmax = $argv[1]
	breaksw
    case -ym*:	# -ymax
        shift argv
        if ($#argv == 0) eval usage
	@ ymax = $argv[1]
	breaksw
    case -xb*:	# -xborder
        shift argv
        if ($#argv == 0) eval usage
	set xborder = $argv[1]
	breaksw
    case -yb*:	# -yborder
        shift argv
        if ($#argv == 0) eval usage
	set yborder = $argv[1]
	breaksw
    case -f*:	# -forceplain
	set filtertail = ""
	breaksw
    case -s*:	# -stdout
	set goto_stdout
	breaksw
    case -v*:	# -verbose
	set verb
	breaksw
    case -po*:	# -portrait
	set orient = 1
	breaksw
    case -la*:	# -landscape
	set orient = 2
	breaksw
    case -*:
        echo "${progname}: Unknown option $argv[1]"
        usage
        breaksw
    default:	# input file
	set psfile = $argv[1]
	set ppmfile = `basename $argv[1] .ps`
        breaksw
    endsw
    shift argv
end

if ($psfile =~ "") eval usage
if (! -f $psfile) then
    echo "${progname}: file $psfile not found"
    usage
endif

set bb = `grep "%%BoundingBox" $psfile`
if ($?nobb == 0 && $#bb == 5) then
    set llx = $bb[2]
    set lly = $bb[3]
    set urx = $bb[4]
    set ury = $bb[5]
else
    if ($?nobb == 0) \
    	echo "${progname}: warning: BoundingBox not found in input file"
endif

set tmpsx = `(echo "scale=4";echo "$urx - $llx")|bc -l`
set tmpsy = `(echo "scale=4";echo "$ury - $lly")|bc -l`

# see if orientation was specified 
if ($orient == 0) then
    # no orientation was specified; compute default orientation
    set tmpx = 0
    set tmpy = 0
    set tmpsx1 = $tmpsx:r
    set tmpsy1 = $tmpsy:r
    # default is landscape mode
    set orient = 2
    if ($xsize == 0 && $ysize == 0) then
	set tmpx = $xmax
	set tmpy = $ymax
    else
	if ($xsize != 0) set tmpx = $xsize
	if ($ysize != 0) set tmpy = $ysize
    endif
    if ($tmpx == 0 || $tmpy == 0) then
	# only one size was specified
	if ($tmpsy1 > $tmpsx1) set orient = 1
    else
	# no size or both sizes were specified
	if ($tmpsy1 > $tmpsx1 && $tmpy > $tmpx) set orient = 1
	if ($tmpsx1 > $tmpsy1 && $tmpx > $tmpy) set orient = 1
    endif
endif

# now reset BoundingBox llc and total size to take into account margin
set llx = `(echo "scale=4";echo "$llx - $tmpsx * $xborder")|bc -l`
set lly = `(echo "scale=4";echo "$lly - $tmpsy * $yborder")|bc -l`
set urx = `(echo "scale=4";echo "$urx + $tmpsx * $xborder")|bc -l`
set ury = `(echo "scale=4";echo "$ury + $tmpsy * $yborder")|bc -l`
# compute image area size 
set sx = `(echo "scale=4";echo "$tmpsx + 2 * $xborder * $tmpsx")|bc -l`
set sy = `(echo "scale=4";echo "$tmpsy + 2 * $yborder * $tmpsy")|bc -l`
    
if ($orient != 1) then
    # render image in landscape mode
    set tmpsx = $sx
    set sx = $sy
    set sy = $tmpsx
endif

# if xsize or ysize was specified, compute resolution from them
if ($xsize != 0) set xres = `(echo "scale=4";echo "$xsize *72 / $sx")|bc -l`
if ($ysize != 0) set yres = `(echo "scale=4";echo "$ysize *72 / $sy")|bc -l`

if ($xres =~ "" && $yres !~ "") then
    # ysize was specified, xsize was not; compute xsize based on ysize
    set xres = $yres 
    set xsize = `(echo "scale=4";echo "$sx * $xres /72 + 0.5")|bc -l`
    set xsize = $xsize:r
else 
    if ($yres =~ "" && $xres !~ "") then
	# xsize was specified, ysize was not; compute ysize based on xsize
        set yres = $xres
    	set ysize = `(echo "scale=4";echo "$sy * $yres /72 + 0.5")|bc -l`
    	set ysize = $ysize:r
    else
	if ($xres =~ "" && $yres =~ "") then
    	    # neither xsize nor ysize was specified; compute them from
	    # xmax and ymax
	    set xres = `(echo "scale=4";echo "$xmax *72/$sx")|bc -l`
	    set yres = `(echo "scale=4";echo "$ymax *72/$sy")|bc -l`
	    set xres = `(echo "scale=4";echo "if($xres>$yres)$yres";echo "if($yres>$xres)$xres";echo "if($xres==$yres)$xres")|bc -l`
	    set yres = $xres
	    if ($?nocrop) then
		# keep output file dimensions equal to xmax and ymax
		set xsize = $xmax
		set ysize = $ymax
	    else
    	    	set xsize = `(echo "scale=4";echo "$sx * $xres /72+0.5")|bc -l`
    	    	set ysize = `(echo "scale=4";echo "$sy * $yres /72+0.5")|bc -l`
	    endif
    	    set xsize = $xsize:r
    	    set ysize = $ysize:r
	endif
    endif
endif

# translate + rotate image, if necessary
if ($orient == 1) then
    # portrait mode
    # adjust offsets
    set llx = `(echo "scale=4";echo "$llx - ($xsize *72/$xres - $sx)/2")|bc -l`
    set lly = `(echo "scale=4";echo "$lly - ($ysize *72/$yres - $sy)/2")|bc -l`
    set pstrans = "$llx neg $lly neg translate"
else
    # landscape mode
    # adjust offsets
    set llx = `(echo "scale=4";echo "$llx - ($ysize *72/$yres - $sy)/2")|bc -l`
    set ury = `(echo "scale=4";echo "$ury + ($xsize *72/$xres - $sx)/2")|bc -l`
    set pstrans = "90 rotate $llx neg $ury neg translate"
endif
   
if ($?goto_stdout) then
    set outfile = "-"
else
    set outfile = $ppmfile%03d.$filterhead 
endif

if ($?verb) then
    echo "sx = $sx" 
    echo "sy = $sy"
    echo "xres  = $xres"
    echo "yres  = $yres"
    echo "xsize = $xsize"
    echo "ysize = $ysize"
    echo -n "orientation "
    if ($orient == 1) then 
	echo "portrait"
    else
	echo "landscape"
    endif
    echo "PS header: $pstrans"
endif

echo "${progname}: writing $filterhead file(s)" 1>&2

echo $pstrans | \
	gs -sDEVICE=${filterhead}${filtertail} \
	   -sOutputFile=$outfile \
	   -g${xsize}x${ysize} \
	   -r${xres}x${yres} \
	   -q - $psfile 



