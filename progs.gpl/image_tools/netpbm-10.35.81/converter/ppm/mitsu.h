#ifndef MITSU_H_INCLUDED
#define MITSU_H_INCLUDED

/* static char SCCSid[] = "@(#)mitsu.h\t\t1.3\t(SPZ)\t3/11/92\n"; */
#define MAXLUTCOL   255

#define A4_MAXCOLS  1184
#define A4_MAXROWS  1452
#define A4S_MAXROWS 1754
#define A_MAXCOLS   1216
#define A_MAXROWS   1350
#define AS_MAXROWS  1650

#define ONLINE         cmd('\021')
#define CLRMEM         cmd('\033'), cmd('Z')

struct mediasize {
        char size;
        int  maxcols, maxrows;
};

const struct mediasize MSize_User={' ',1184,1350};
const struct mediasize MSize_A4  ={'0',1184,1452};
const struct mediasize MSize_A   ={'1',1216,1350};
const struct mediasize MSize_A4S ={'2',1184,1754};
const struct mediasize MSize_AS  ={'3',1216,1650};
#define MEDIASIZE(chr) cmd('\033'), cmd('#'), cmd('P'), cmd((chr).size)

#define HENLARGE(enl)  cmd('\033'), cmd('&'), cmd('P'), cmd(enl), cmd('\001')
#define VENLARGE(enl)  cmd('\033'), cmd('&'), cmd('Q'), cmd(enl), cmd('\001')
#define NOENLARGE '\001'
#define ENLARGEx2 '\002'
#define ENLARGEx3 '\003'

#define COLREVERSION(arg) cmd('\033'), cmd('&'), cmd('W'), cmd(arg)
#define DONTREVERTCOLOR '0'
#define REVERTCOLOR   '2'

#define NUMCOPY(num)   cmd('\033'), cmd('#'), cmd('C'), cmd((num) & 0xff)

#define HOFFINCH(off)  cmd('\033'), cmd('&'), cmd('S'), cmd((off) & 0xff)
#define VOFFINCH(off)  cmd('\033'), cmd('&'), cmd('T'), cmd((off) & 0xff)

#define CENTERING(cen) cmd('\033'), cmd('&'), cmd('C'), cmd(cen)
#define DONTCENTER '0'
#define DOCENTER   '1'

#define TRANSFERFORMAT(fmt) cmd('\033'), cmd('&'), cmd('A'), cmd(fmt)
#define FRAMEORDER  '0'
#define LINEORDER   '1'
#define LOOKUPTABLE '3'

#define COLORSYSTEM(cs) cmd('\033'), cmd('&'), cmd('I'), cmd(cs)
#define RGB '0'
#define YMC '1'

#define SHARPNESS(spn) cmd('\033'), cmd('#'), cmd('E'), cmd(spn)
#define SP_USER ' '
#define SP_NONE '0'
#define SP_LOW  '1'
#define SP_MIDLOW '2'
#define SP_MIDHIGH '3'
#define SP_HIGH '4'

#define COLORDES(col) cmd('\033'), cmd('C'), cmd(col)
#define RED   '1'
#define GREEN '2'
#define BLUE  '3'
#define YELLOW  '1'
#define MAGENTA '2'
#define CYAN    '3'

#define HPIXELS(hpix) cmd('\033'), cmd('&'), cmd('H'),\
                                                        cmd(((hpix) >> 8) & 0xff), cmd((hpix) & 0xff)
#define VPIXELS(vpix) cmd('\033'), cmd('&'), cmd('V'),\
                                                        cmd(((vpix) >> 8) & 0xff), cmd((vpix) & 0xff)
#define HPIXELSOFF(hoff) cmd('\033'), cmd('&'), cmd('J'),\
                                                        cmd(((hoff) >> 8) & 0xff), cmd((hoff) & 0xff)
#define VPIXELSOFF(voff) cmd('\033'), cmd('&'), cmd('K'),\
                                                        cmd(((voff) >> 8) & 0xff), cmd((voff) & 0xff)

#define GRAYSCALELVL(lvl) cmd('\033'), cmd('#'), cmd('L'), cmd(lvl)
#define BIT_6 '\006'
#define BIT_8 '\010'

#define LOADLOOKUPTABLE cmd('\033'), cmd('&'), cmd('L')
#define DONELOOKUPTABLE cmd('\004')

#define ROTATEIMG(rot)  cmd('\033'), cmd('#'), cmd('R'), cmd(rot)
#define DONTROTATE '0'
#define DOROTATE   '1'

#define DATASTART cmd('\033'), cmd('O')
#define PRINTIT cmd('\014')
#define OFFLINE cmd('\023')

#endif
