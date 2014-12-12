/* 
 * This was adapted for Netpbm from bmpesoe.c in Dirk Krause's Bmeps package
 * by Bryan Henderson on 2005.01.05.
 *
 * Differences:
 *   - doesn't require Bmeps configuration stuff (bmepsco.h)
 *   - doesn't include pngeps.h
 *   - doesn't have test scaffold code
 *   - a few compiler warnings fixed
 *
 * Copyright (C) 2000 - Dirk Krause
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 * In this package the copy of the GNU Library General Public License
 * is placed in file COPYING.
 */

#include "bmepsoe.h"

#define RL_RUNLENGTH(i) (257 - (i))
#define RL_STRINGLENGTH(i) ((i) - 1)
#define RL_MAXLENGTH (127)

#define FINALOUTPUT(c) fputc((c),o->out)

void oe_init(Output_Encoder *o, FILE *out, int mode, int rate, int *buf,
  Bytef *flib, size_t flis, Bytef *flob, size_t flos
)
{
  
  o->out = out;
  o->mode = mode;
  o->textpos = 0;
  o->a85_value = 0UL; o->a85_consumed = 0;
  o->a85_0 = 1UL;
  o->a85_1 = 85UL;
  o->a85_2 = 85UL * 85UL;
  o->a85_3 = 85UL * o->a85_2;
  o->a85_4 = 85UL * o->a85_3;
  o->rl_lastbyte = 0;
  o->rl_buffer = buf;
  o->rl_bufused = 0;
  o->rl_state = 0;
  o->bit_value = 0;
  o->bit_consumed = 0;
  o->flate_rate = rate;
  if(o->flate_rate < 0) {
    o->mode &= (~OE_FLATE);
  }
  if(o->flate_rate > 9) {
    o->flate_rate = 9;
  }
  if((o->mode & OE_FLATE) && flib && flis && flob && flos) {
    (o->flate_stream).zfree = (free_func)0;
    (o->flate_stream).zalloc = (alloc_func)0;
    (o->flate_stream).opaque = (voidpf)0;
    if(deflateInit((&(o->flate_stream)),o->flate_rate) != Z_OK) {
      o->mode &= (~OE_FLATE);
    }
  }
  o->fl_i_buffer = flib;
  o->fl_o_buffer = flob;
  o->fl_i_size   = flis;
  o->fl_o_size   = flos;
  o->fl_i_used   = 0;
  
}

static char hexdigits[] = {
  "0123456789ABCDEF"
};

static void asciihex_add(Output_Encoder *o, int b)
{
  
  FINALOUTPUT(hexdigits[(b/16)]) ;
  FINALOUTPUT(hexdigits[(b%16)]) ;
  o->textpos = o->textpos + 2;
  if(o->textpos >= 64) {
    FINALOUTPUT('\n') ;
    o->textpos = 0;
  }
  
}

static void asciihex_flush(Output_Encoder *o)
{
  
  if(o->textpos > 0) {
    FINALOUTPUT('\n') ;
    o->textpos = 0;
  }
  
}

static char   ascii85_char(unsigned long x)
{
  unsigned u;
  int      i;
  char back;
  back = (char)0;
  u = (unsigned)x;
  i = (int)u;
  i += 33;
  back = (char)i;
  return back;
}



static void
ascii85_output(Output_Encoder * const o) {
    unsigned long value;

    value = o->a85_value;  /* initial value */

    if (value == 0 && o->a85_consumed == 4) {
        FINALOUTPUT('z');
        ++o->textpos;
    } else {
        unsigned int i;
        unsigned int j;
        char buffer[6];

        buffer[0] = ascii85_char(value / (o->a85_4));
        value = value % (o->a85_4);
        buffer[1] = ascii85_char(value / (o->a85_3));
        value = value % (o->a85_3);
        buffer[2] = ascii85_char(value / (o->a85_2));
        value = value % (o->a85_2);
        buffer[3] = ascii85_char(value / (o->a85_1));
        value = value % (o->a85_1);
        buffer[4] = ascii85_char(value);
        buffer[5] = '\0';

        i = o->a85_consumed + 1;
        o->textpos += i;

        for (j = 0; j < i; ++j)
            FINALOUTPUT(buffer[j]);
    }
    if (o->textpos >= 64) {
        FINALOUTPUT('\n');
        o->textpos = 0;
    }
}



static void ascii85_add(Output_Encoder *o, int b)
{
  unsigned u;
  unsigned long ul;
  
  u = (unsigned)b;
  ul = (unsigned long)u;
  o->a85_value = 256UL * (o->a85_value) + ul;
  o->a85_consumed = o->a85_consumed + 1;
  if(o->a85_consumed >= 4) {
    ascii85_output(o);
    o->a85_value = 0UL;
    o->a85_consumed = 0;
  }
  
}

static void ascii85_flush(Output_Encoder *o)
{
  int i;
  
  if(o->a85_consumed > 0) {
    i = o->a85_consumed;
    while(i < 4) {
      o->a85_value = 256UL * o->a85_value;
      i++;
    }
    ascii85_output(o);
    o->a85_value = 0UL;
    o->a85_consumed = 0;
  }
  if(o->textpos > 0) {
    FINALOUTPUT('\n') ;
    o->textpos = 0;
  }
  
}

static void after_flate_add(Output_Encoder *o, int b)
{
  
  if(o->mode & OE_ASC85) {
    ascii85_add(o,b);
  } else {
    asciihex_add(o,b);
  }
  
}

static void do_flate_flush(Output_Encoder *o, int final)
{
  Bytef *iptr, *optr, *xptr;
  uLong  is, os, xs;
  int err, must_continue;
  
  iptr = o->fl_i_buffer; optr = o->fl_o_buffer;
  is = o->fl_i_size; os = o->fl_o_size;
  
  if(iptr && optr && is && os) {
    is = o->fl_i_used;
    if(is) {
      (o->flate_stream).next_in = iptr;
      (o->flate_stream).avail_in = is;
      if(final) { 
    must_continue = 1;
    while(must_continue) {
      (o->flate_stream).next_out = optr;
      (o->flate_stream).avail_out = os;
      must_continue = 0;
      err = deflate(&(o->flate_stream), Z_FINISH);
      switch(err) {
        case Z_STREAM_END: { 
              xptr = optr;
          
          xs = os - ((o->flate_stream).avail_out);
          while(xs--) {
        after_flate_add(o, (*(xptr++)));
          }
        } break;
        case Z_OK : {
          must_continue = 1;
              xptr = optr;
          
          xs = os - ((o->flate_stream).avail_out);
          while(xs--) {
        after_flate_add(o, (*(xptr++)));
          }
        } break;
        default : { 
        } break;
      }
    }
      } else { 
    must_continue = 1;
    while(must_continue) {
      must_continue = 0;
      (o->flate_stream).avail_out = os; (o->flate_stream).next_out = optr;
      err = deflate(&(o->flate_stream), 0);
      switch(err) {
        case Z_OK: {
          if((o->flate_stream).avail_in) {
        must_continue = 1;
          }
          
          xptr = optr; xs = os - ((o->flate_stream).avail_out);
          while(xs--) {
        after_flate_add(o, (*(xptr++)));
          }
        } break;
        default : { 
        } break;
      }
    }
      }
    }
  }
  
}

static void flate_add(Output_Encoder *o, int b)
{
  Byte bt;
  Bytef *btptr;
  
  btptr = o->fl_i_buffer;
  if(btptr) { 
    bt = (Byte)b; 
    btptr[(o->fl_i_used)] = bt;
    o->fl_i_used += 1UL;
    if(o->fl_i_used >= o->fl_i_size) {
      do_flate_flush(o, 0);
      o->fl_i_used = 0UL;
    }
  }
  
}

static void after_flate_flush(Output_Encoder *o)
{
  
  if(o->mode & OE_ASC85) {
    ascii85_flush(o);
  } else {
    asciihex_flush(o);
  }
  
}

static void flate_flush(Output_Encoder *o)
{
  do_flate_flush(o,1);
  deflateEnd(&(o->flate_stream));
  after_flate_flush(o);
}


static void after_rl_add(Output_Encoder *o, int b)
{
  
  if(o->mode & OE_FLATE) {
    flate_add(o,b);
  } else {
    after_flate_add(o,b);
  }
  
}

static void rl_add(Output_Encoder *o, int b)
{
  int lgt, i;
  int *buffer;
  /* ##### */
  
  buffer = o->rl_buffer;
  lgt = o->rl_bufused;
  if(buffer) {
    
    if(lgt > 0) {
      if(o->rl_lastbyte == b) {
    switch(o->rl_state) {
      case 2: {
        buffer[lgt++] = b;
        o->rl_bufused = lgt;
        o->rl_state = 2;
        o->rl_lastbyte = b;
        if(lgt >= RL_MAXLENGTH) {
          after_rl_add(o, RL_RUNLENGTH(lgt));
          after_rl_add(o, b);
          o->rl_bufused = 0;
          o->rl_state = 0;
          o->rl_lastbyte = b;
        }
      } break;
      case 1: {
        buffer[lgt++] = b;
        o->rl_bufused = lgt;
        o->rl_state = 2;
        o->rl_lastbyte = b;
        lgt = lgt - 3;
        if(lgt > 0) {
          after_rl_add(o, RL_STRINGLENGTH(lgt));
          for(i = 0; i < lgt; i++) {
        after_rl_add(o, buffer[i]);
          }
          buffer[0] = buffer[1] = buffer[2] = b;
          o->rl_bufused = 3;
          o->rl_state = 2;
          o->rl_lastbyte = b;
        }
      } break;
      default: {
        buffer[lgt++] = b;
        o->rl_bufused = lgt;
        o->rl_state = 1;
        o->rl_lastbyte = b;
        if(lgt >= RL_MAXLENGTH) {
          lgt = lgt - 2;
          after_rl_add(o, RL_STRINGLENGTH(lgt));
          for(i = 0; i < lgt; i++) {
        after_rl_add(o, buffer[i]);
          }
          buffer[0] = buffer[1] = b;
          o->rl_bufused = 2;
          o->rl_state = 1;
          o->rl_lastbyte = b;
        }
      } break;
    }
      } else {
    if(o->rl_state == 2) {
      after_rl_add(o, RL_RUNLENGTH(lgt));
      after_rl_add(o, (o->rl_lastbyte));
      buffer[0] = b; o->rl_bufused = 1; o->rl_lastbyte = b;
      o->rl_state = 0;
    } else {
      buffer[lgt++] = b;
      o->rl_bufused = lgt;
      o->rl_lastbyte = b;
      if(lgt >= RL_MAXLENGTH) {
        after_rl_add(o, RL_STRINGLENGTH(lgt));
        for(i = 0; i < lgt; i++) {
          after_rl_add(o, buffer[i]);
        }
        o->rl_bufused = 0;
      }
      o->rl_state = 0;
    }
      }
    } else {
      buffer[0] = b;
      o->rl_bufused = 1;
      o->rl_lastbyte = b;
    }
    o->rl_lastbyte = b;
    
  } else { 
    after_rl_add(o,0);
    after_rl_add(o,b);
  }
  
}

static void after_rl_flush(Output_Encoder *o)
{
  
  if(o->mode & OE_FLATE) {
    flate_flush(o);
  } else {
    after_flate_flush(o);
  }
  
}

static void rl_flush(Output_Encoder *o)
{
  int lgt;
  int *buffer;
  int i;
  
  buffer = o->rl_buffer;
  lgt = o->rl_bufused;
  if(lgt > 0) {
    if(o->rl_state == 2) {
      i = o->rl_lastbyte;
      after_rl_add(o,RL_RUNLENGTH(lgt));
      after_rl_add(o,i);
    } else {
      after_rl_add(o,RL_STRINGLENGTH(lgt));
      for(i = 0; i < lgt; i++) {
    after_rl_add(o,buffer[i]);
      }
    }
  }
  after_rl_flush(o);
  
}

static void internal_byte_add(Output_Encoder *o, int b)
{
  
  if((o->mode) & OE_RL) {
    rl_add(o,b);
  } else {
    after_rl_add(o,b);
  }
  
}

static void internal_byte_flush(Output_Encoder *o)
{
  
  if((o->mode) & OE_RL) {
    rl_flush(o);
  } else {
    after_rl_flush(o);
  }
  
}

void oe_bit_add(Output_Encoder *o, int b)
{
  
  o->bit_value = 2 * o->bit_value + (b ? 1 : 0);
  o->bit_consumed = o->bit_consumed + 1;
  if(o->bit_consumed >= 8) {
    o->bit_consumed = 0;
    internal_byte_add(o, (o->bit_value));
    o->bit_value = 0;
  }
  
}

void oe_bit_flush(Output_Encoder *o)
{
  
  if(o->bit_consumed) {
    int v, i;
    v = o->bit_value;
    i = o->bit_consumed;
    while(i < 8) {
      i++;
      v = v * 2;
    }
    internal_byte_add(o,v);
    o->bit_value = 0;
    o->bit_consumed = 0;
  }
  internal_byte_flush(o);
  
}

void oe_byte_add(Output_Encoder *o, int b)
{
  
  if(o->bit_consumed) {
    int testval,i;
    testval = 128;
    for(i = 0; i < 8; i++) {
      if(b & testval) {
    oe_bit_add(o,1);
      } else {
    oe_bit_add(o,0);
      }
      testval = testval / 2;
    }
  } else {
    internal_byte_add(o,b);
  }
  
}

void oe_byte_flush(Output_Encoder *o)
{
  
  oe_bit_flush(o);
  
}
