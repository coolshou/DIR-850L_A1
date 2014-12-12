/*
 *  error.h
 *  
 *  Written by:     Stefan Frank
 *          Ullrich Hafner
 *
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

void
set_error (const char *format, ...);
void
error (const char *format, ...);
void
file_error (const char *filename);
void
message (const char *format, ...);
void 
debug_message (const char *format, ...);
void
warning (const char *format, ...);
void 
info (const char *format, ...);
const char *
get_system_error (void);

#include <setjmp.h>
extern jmp_buf env;

#define try         if (setjmp (env) == 0)
#define catch           else

#include <assert.h>

#endif
