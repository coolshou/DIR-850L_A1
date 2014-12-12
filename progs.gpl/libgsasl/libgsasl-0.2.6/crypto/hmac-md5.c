/* hmac-md5.c
 *
 * HMAC-MD5 message authentication code.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2002 Niels M�ller
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "hmac.h"

void
hmac_md5_set_key(struct hmac_md5_ctx *ctx,
		 unsigned key_length, const uint8_t *key)
{
  HMAC_SET_KEY(ctx, &nettle_md5, key_length, key);
}

void
hmac_md5_update(struct hmac_md5_ctx *ctx,
		unsigned length, const uint8_t *data)
{
  md5_update(&ctx->state, length, data);
}

void
hmac_md5_digest(struct hmac_md5_ctx *ctx,
		unsigned length, uint8_t *digest)
{
  HMAC_DIGEST(ctx, &nettle_md5, length, digest);
}
