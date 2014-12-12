/* printer.h --- Convert DIGEST-MD5 token structures into strings.
 * Copyright (C) 2004  Simon Josefsson
 *
 * This file is part of GNU SASL Library.
 *
 * GNU SASL Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * GNU SASL Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GNU SASL Library; if not, write to the Free
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

/* Get prototypes. */
#include "printer.h"

/* Get free. */
#include <stdlib.h>

/* Get asprintf. */
#include <vasprintf.h>

/* Get token validator. */
#include "validate.h"

/* FIXME: The challenge/response functions may print "empty" fields,
   such as "foo=bar, , , bar=foo".  It is valid, but look ugly. */

char *
digest_md5_print_challenge (digest_md5_challenge * c)
{
  char *out = NULL;
  char *realm = NULL, *maxbuf = NULL;
  size_t i;

  /* Below we assume the mandatory fields are present, verify that
     first to avoid crashes. */
  if (digest_md5_validate_challenge (c) != 0)
    return NULL;

  for (i = 0; i < c->nrealms; i++)
    {
      char *tmp;
      if (asprintf (&tmp, "%s, realm=\"%s\"",
		    realm ? realm : "", c->realms[i]) < 0)
	goto end;
      if (realm)
	free (realm);
      realm = tmp;
    }

  if (c->servermaxbuf)
    if (asprintf (&maxbuf, "maxbuf=%lu", c->servermaxbuf) < 0)
      goto end;

  if (asprintf (&out, "%s, nonce=\"%s\", %s%s%s%s%s, %s, "
		"%s, %s, algorithm=md5-sess, %s%s%s%s%s%s%s%s",
		realm ? realm : "",
		c->nonce,
		c->qops ? "qop=\"" : "",
		(c->qops & DIGEST_MD5_QOP_AUTH) ? "auth, " : "",
		(c->qops & DIGEST_MD5_QOP_AUTH_INT) ? "auth-int, " : "",
		(c->qops & DIGEST_MD5_QOP_AUTH_CONF) ? "auth-conf" : "",
		c->qops ? "\"" : "",
		c->stale ? "stale=true" : "",
		maxbuf ? maxbuf : "",
		c->utf8 ? "charset=utf-8" : "",
		c->ciphers ? "cipher=\"" : "",
		(c->ciphers & DIGEST_MD5_CIPHER_3DES) ? "3des, " : "",
		(c->ciphers & DIGEST_MD5_CIPHER_DES) ? "des, " : "",
		(c->ciphers & DIGEST_MD5_CIPHER_RC4_40) ? "rc4-40, " : "",
		(c->ciphers & DIGEST_MD5_CIPHER_RC4) ? "rc4, " : "",
		(c->ciphers & DIGEST_MD5_CIPHER_RC4_56) ? "rc4-56, " : "",
		(c->ciphers & DIGEST_MD5_CIPHER_AES_CBC) ? "aes-cbc, " : "",
		c->ciphers ? "\"" : "") < 0)
    out = NULL;

end:
  if (realm)
    free (realm);
  if (maxbuf)
    free (maxbuf);

  return out;

}

char *
digest_md5_print_response (digest_md5_response * r)
{
  char *out = NULL;
  const char *qop = NULL;
  const char *cipher = NULL;
  char *maxbuf = NULL;

  /* Below we assume the mandatory fields are present, verify that
     first to avoid crashes. */
  if (digest_md5_validate_response (r) != 0)
    return NULL;

  if (r->qop & DIGEST_MD5_QOP_AUTH_CONF)
    qop = "qop=auth-conf";
  else if (r->qop & DIGEST_MD5_QOP_AUTH_INT)
    qop = "qop=auth-int";
  else if (r->qop & DIGEST_MD5_QOP_AUTH)
    qop = "qop=auth";
  else
    qop = "";

  if (r->clientmaxbuf)
    if (asprintf (&maxbuf, "maxbuf=%lu", r->clientmaxbuf) < 0)
      goto end;

  if (r->cipher & DIGEST_MD5_CIPHER_3DES)
    cipher = "cipher=3des";
  else if (r->cipher & DIGEST_MD5_CIPHER_DES)
    cipher = "cipher=des";
  else if (r->cipher & DIGEST_MD5_CIPHER_RC4_40)
    cipher = "cipher=rc4-40";
  else if (r->cipher & DIGEST_MD5_CIPHER_RC4)
    cipher = "cipher=rc4";
  else if (r->cipher & DIGEST_MD5_CIPHER_RC4_56)
    cipher = "cipher=rc4-56";
  else if (r->cipher & DIGEST_MD5_CIPHER_AES_CBC)
    cipher = "cipher=aes-cbc";
  else if (r->cipher & DIGEST_MD5_CIPHER_3DES)
    cipher = "cipher=3des";
  else
    cipher = "";

  if (asprintf (&out, "username=\"%s\", %s%s%s, nonce=\"%s\", cnonce=\"%s\", "
		"nc=%08lx, %s, digest-uri=\"%s\", response=%s, "
		"%s, %s, %s, %s%s%s",
		r->username,
		r->realm ? "realm=\"" : "",
		r->realm ? r->realm : "",
		r->realm ? "\"" : "",
		r->nonce,
		r->cnonce,
		r->nc,
		qop,
		r->digesturi,
		r->response,
		maxbuf ? maxbuf : "",
		r->utf8 ? "charset=utf-8" : "",
		cipher,
		r->authzid ? "authzid=\"" : "",
		r->authzid ? r->authzid : "", r->authzid ? "\"" : "") < 0)
    out = NULL;

end:
  if (maxbuf)
    free (maxbuf);

  return out;
}

char *
digest_md5_print_finish (digest_md5_finish * finish)
{
  char *out;

  /* Below we assume the mandatory fields are present, verify that
     first to avoid crashes. */
  if (digest_md5_validate_finish (finish) != 0)
    return NULL;

  if (asprintf (&out, "rspauth=%s", finish->rspauth) < 0)
    return NULL;

  return out;
}
