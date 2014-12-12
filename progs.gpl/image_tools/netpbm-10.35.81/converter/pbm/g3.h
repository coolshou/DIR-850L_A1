/* G3 fax format declarations */

#ifndef G3_H_INCLUDED
#define G3_H_INCLUDED

/* G3 is nearly universal as the format for fax transmissions in the
   US.  Its full name is CCITT Group 3 (G3).  It is specified in
   Recommendations T.4 and T.30 and in EIA Standards EIA-465 and
   EIA-466.  It dates to 1993.

   G3 faxes are 204 dots per inch (dpi) horizontally and 98 dpi (196
   dpi optionally, in fine-detail mode) vertically.  Since G3 neither
   assumes error free transmission nor retransmits when errors occur,
   the encoding scheme used is differential only over small segments
   never exceeding 2 lines at standard resolution or 4 lines for
   fine-detail. (The incremental G3 encoding scheme is called
   two-dimensional and the number of lines so encoded is specified by
   a parameter called k.)

   G3 specifies much more than the format of the bit stream, which is
   the subject of this header file.  It also specifies layers
   underneath the bit stream.

   There is also the newer G4.  
*/

typedef struct g3TableEntry {
    short int code;
    short int length;
} g3TableEntry;

static struct g3TableEntry ttable[] = {
/*    TERMWHITE           TERMBLACK   */
    { 0x35, 8 },    { 0x37, 10 },       /* white 0 , black 0 */
    { 0x07, 6 },    { 0x02,  3 },
    { 0x07, 4 },    { 0x03,  2 },
    { 0x08, 4 },    { 0x02,  2 },
    { 0x0b, 4 },    { 0x03,  3 },
    { 0x0c, 4 },    { 0x03,  4 },
    { 0x0e, 4 },    { 0x02,  4 },
    { 0x0f, 4 },    { 0x03,  5 },
    { 0x13, 5 },    { 0x05,  6 },
    { 0x14, 5 },    { 0x04,  6 },
    { 0x07, 5 },    { 0x04,  7 },
    { 0x08, 5 },    { 0x05,  7 },
    { 0x08, 6 },    { 0x07,  7 },
    { 0x03, 6 },    { 0x04,  8 },
    { 0x34, 6 },    { 0x07,  8 },
    { 0x35, 6 },    { 0x18,  9 },
    { 0x2a, 6 },    { 0x17, 10 },
    { 0x2b, 6 },    { 0x18, 10 },
    { 0x27, 7 },    { 0x08, 10 },
    { 0x0c, 7 },    { 0x67, 11 },
    { 0x08, 7 },    { 0x68, 11 },
    { 0x17, 7 },    { 0x6c, 11 },
    { 0x03, 7 },    { 0x37, 11 },
    { 0x04, 7 },    { 0x28, 11 },
    { 0x28, 7 },    { 0x17, 11 },
    { 0x2b, 7 },    { 0x18, 11 },
    { 0x13, 7 },    { 0xca, 12 },
    { 0x24, 7 },    { 0xcb, 12 },
    { 0x18, 7 },    { 0xcc, 12 },
    { 0x02, 8 },    { 0xcd, 12 },
    { 0x03, 8 },    { 0x68, 12 },
    { 0x1a, 8 },    { 0x69, 12 },
    { 0x1b, 8 },    { 0x6a, 12 },
    { 0x12, 8 },    { 0x6b, 12 },
    { 0x13, 8 },    { 0xd2, 12 },
    { 0x14, 8 },    { 0xd3, 12 },
    { 0x15, 8 },    { 0xd4, 12 },
    { 0x16, 8 },    { 0xd5, 12 },
    { 0x17, 8 },    { 0xd6, 12 },
    { 0x28, 8 },    { 0xd7, 12 },
    { 0x29, 8 },    { 0x6c, 12 },
    { 0x2a, 8 },    { 0x6d, 12 },
    { 0x2b, 8 },    { 0xda, 12 },
    { 0x2c, 8 },    { 0xdb, 12 },
    { 0x2d, 8 },    { 0x54, 12 },
    { 0x04, 8 },    { 0x55, 12 },
    { 0x05, 8 },    { 0x56, 12 },
    { 0x0a, 8 },    { 0x57, 12 },
    { 0x0b, 8 },    { 0x64, 12 },
    { 0x52, 8 },    { 0x65, 12 },
    { 0x53, 8 },    { 0x52, 12 },
    { 0x54, 8 },    { 0x53, 12 },
    { 0x55, 8 },    { 0x24, 12 },
    { 0x24, 8 },    { 0x37, 12 },
    { 0x25, 8 },    { 0x38, 12 },
    { 0x58, 8 },    { 0x27, 12 },
    { 0x59, 8 },    { 0x28, 12 },
    { 0x5a, 8 },    { 0x58, 12 },
    { 0x5b, 8 },    { 0x59, 12 },
    { 0x4a, 8 },    { 0x2b, 12 },
    { 0x4b, 8 },    { 0x2c, 12 },
    { 0x32, 8 },    { 0x5a, 12 },
    { 0x33, 8 },    { 0x66, 12 },
    { 0x34, 8 },    { 0x67, 12 },       /* white 63 , black 63 */

/* mtable */    
/*    MKUPWHITE           MKUPBLACK   */
    { 0x00, 0 },    { 0x00,  0 },   /* dummy to simplify pointer math */
    { 0x1b, 5 },    { 0x0f, 10 },   /* white 64 , black 64 */
    { 0x12, 5 },    { 0xc8, 12 },
    { 0x17, 6 },    { 0xc9, 12 },
    { 0x37, 7 },    { 0x5b, 12 },
    { 0x36, 8 },    { 0x33, 12 },
    { 0x37, 8 },    { 0x34, 12 },
    { 0x64, 8 },    { 0x35, 12 },
    { 0x65, 8 },    { 0x6c, 13 },
    { 0x68, 8 },    { 0x6d, 13 },
    { 0x67, 8 },    { 0x4a, 13 },
    { 0xcc, 9 },    { 0x4b, 13 },
    { 0xcd, 9 },    { 0x4c, 13 },
    { 0xd2, 9 },    { 0x4d, 13 },
    { 0xd3, 9 },    { 0x72, 13 },
    { 0xd4, 9 },    { 0x73, 13 },
    { 0xd5, 9 },    { 0x74, 13 },
    { 0xd6, 9 },    { 0x75, 13 },
    { 0xd7, 9 },    { 0x76, 13 },
    { 0xd8, 9 },    { 0x77, 13 },
    { 0xd9, 9 },    { 0x52, 13 },
    { 0xda, 9 },    { 0x53, 13 },
    { 0xdb, 9 },    { 0x54, 13 },
    { 0x98, 9 },    { 0x55, 13 },
    { 0x99, 9 },    { 0x5a, 13 },
    { 0x9a, 9 },    { 0x5b, 13 },
    { 0x18, 6 },    { 0x64, 13 },
    { 0x9b, 9 },    { 0x65, 13 },
    { 0x08, 11 },   { 0x08, 11 },        /* extable len = 1792 */
    { 0x0c, 11 },   { 0x0c, 11 },
    { 0x0d, 11 },   { 0x0d, 11 },
    { 0x12, 12 },   { 0x12, 12 },
    { 0x13, 12 },   { 0x13, 12 },
    { 0x14, 12 },   { 0x14, 12 },
    { 0x15, 12 },   { 0x15, 12 },
    { 0x16, 12 },   { 0x16, 12 },
    { 0x17, 12 },   { 0x17, 12 },
    { 0x1c, 12 },   { 0x1c, 12 },
    { 0x1d, 12 },   { 0x1d, 12 },
    { 0x1e, 12 },   { 0x1e, 12 },
    { 0x1f, 12 },   { 0x1f, 12 },
};

#define mtable ((ttable)+64*2)

#endif
