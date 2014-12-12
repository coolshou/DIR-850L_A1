/* ppp_fcs.h ... header file for PPP-HDLC FCS 
 *               C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: ppp_fcs.h,v 1.1.1.1 2005/05/19 10:53:06 r01122 Exp $
 */

#define PPPINITFCS16    0xffff	/* Initial FCS value */
#define PPPGOODFCS16    0xf0b8	/* Good final FCS value */

u_int16_t pppfcs16(u_int16_t fcs, void *cp, int len);
