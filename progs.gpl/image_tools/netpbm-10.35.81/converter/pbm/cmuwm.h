/* cmuwm.h - definitions for the CMU window manager format
*/

#ifndef CMUWM_H_INCLUDED
#define CMUWM_H_INCLUDED

struct cmuwm_header
    {
    long magic;
    long width;
    long height;
    short depth;
    };

#define CMUWM_MAGIC 0xf10040bbL

#endif
