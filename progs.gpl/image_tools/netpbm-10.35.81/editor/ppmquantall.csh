#!/bin/csh -f
#
# ppmquantall - run ppmquant on a bunch of files all at once, so they share
#               a common colormap
#
# WARNING: overwrites the source files with the results!!!
#
# Verbose explanation: Let's say you've got a dozen pixmaps that you want
# to display on the screen all at the same time.  Your screen can only
# display 256 different colors, but the pixmaps have a total of a thousand
# or so different colors.  For a single pixmap you solve this problem with
# ppmquant; this script solves it for multiple pixmaps.  All it does is
# concatenate them together into one big pixmap, run ppmquant on that, and
# then split it up into little pixmaps again.

if ( $#argv < 3 ) then
    echo "usage:  ppmquantall <newcolors> <ppmfile> <ppmfile> ..."
    exit 1
endif

set newcolors=$argv[1]
set files=( $argv[2-] )

# Extract the width and height of each of the images.
# Here, we make the assumption that the width and height are on the
# second line, even though the PPM format doesn't require that.
# To be robust, we need to use Pnmfile to get that information, or 
# Put this program in C and use ppm_readppminit().

set widths=()
set heights=()
foreach i ( $files )
    set widths=( $widths `sed '1d; s/ .*//; 2q' $i` )
    set heights=( $heights `sed '1d; s/.* //; 2q' $i` )
end

set all=/tmp/pqa.all.$$
rm -f $all
pnmcat -topbottom -jleft -white $files | ppmquant -quiet $newcolors > $all
if ( $status != 0 ) exit $status

@ y = 0
@ i = 1
while ( $i <= $#files )
    pnmcut -left 0 -top $y -width $widths[$i] -height $heights[$i] $all \
       > $files[$i]
    if ( $status != 0 ) exit $status
    @ y = $y + $heights[$i]
    @ i++
end

rm -f $all





