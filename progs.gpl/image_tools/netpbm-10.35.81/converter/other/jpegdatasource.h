#ifndef JPEGDATASOURCE_H_INCLUDED
#define JPEGDATASOURCE_H_INCLUDED

#include "pm_c_util.h"

struct sourceManager * 
dsCreateSource(const char * const fileName);

void
dsDestroySource(struct sourceManager * const srcP);

bool
dsDataLeft(struct sourceManager * const srcP);

struct jpeg_source_mgr *
dsJpegSourceMgr(struct sourceManager * const srcP);

#endif
