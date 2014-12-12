/* defaults.h
 * Default printer values.  Edit these and recompile if so desired.
 * [Note: a /etc/pbm2ppa.conf file will override these]
 */
#ifndef _DEFAULTS_H
#define _DEFAULTS_H

/* Refer to CALIBRATION file about these settings */

#define HP1000_PRINTER        ( HP1000 )

#define HP1000_MARG_DIFF      (  0x62 )
#define HP1000_BUFSIZE        ( 100*1024 )

#define HP1000_X_OFFSET       (   100 )
#define HP1000_Y_OFFSET       (  -650 )

#define HP1000_TOP_MARGIN     (   150 )
#define HP1000_LEFT_MARGIN    (   150 )
#define HP1000_RIGHT_MARGIN   (   150 )
#define HP1000_BOTTOM_MARGIN  (   150 )


#define HP720_PRINTER        ( HP720 )

#define HP720_MARG_DIFF      (     2 )
#define HP720_BUFSIZE        ( 200*1024 )

#define HP720_X_OFFSET       (   169 )
#define HP720_Y_OFFSET       (  -569 )

#define HP720_TOP_MARGIN     (   150 )
#define HP720_LEFT_MARGIN    (   150 )
#define HP720_RIGHT_MARGIN   (   150 )
#define HP720_BOTTOM_MARGIN  (   150 )


#define HP820_PRINTER        ( HP820 )

#define HP820_MARG_DIFF      (  0x62 )
#define HP820_BUFSIZE        ( 100*1024 )

#define HP820_X_OFFSET       (    75 )
#define HP820_Y_OFFSET       (  -500 )

#define HP820_TOP_MARGIN     (    80 )
#define HP820_LEFT_MARGIN    (    80 )
#define HP820_RIGHT_MARGIN   (    80 )
#define HP820_BOTTOM_MARGIN  (   150 )



#define DEFAULT_PRINTER        HP1000_PRINTER

#define DEFAULT_X_OFFSET       HP1000_X_OFFSET
#define DEFAULT_Y_OFFSET       HP1000_Y_OFFSET

#define DEFAULT_TOP_MARGIN     HP1000_TOP_MARGIN
#define DEFAULT_LEFT_MARGIN    HP1000_LEFT_MARGIN
#define DEFAULT_RIGHT_MARGIN   HP1000_RIGHT_MARGIN
#define DEFAULT_BOTTOM_MARGIN  HP1000_BOTTOM_MARGIN

#define DEFAULT_DPI ( 600 )

#endif
