/* obsolete.c --- Obsolete functions kept around for backwards compatibility.
 * Copyright (C) 2002, 2003, 2004, 2005  Simon Josefsson
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
 * License License along with GNU SASL Library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "internal.h"

/**
 * gsasl_client_listmech:
 * @ctx: libgsasl handle.
 * @out: output character array.
 * @outlen: input maximum size of output character array, on output
 * contains actual length of output array.
 *
 * Write SASL names, separated by space, of mechanisms supported by
 * the libgsasl client to the output array.  To find out how large the
 * output array must be, call this function with out=NULL.
 *
 * Return value: Returns GSASL_OK if successful, or error code.
 *
 * Deprecated: Use gsasl_client_mechlist() instead.
 **/
int
gsasl_client_listmech (Gsasl * ctx, char *out, size_t * outlen)
{
  char *tmp;
  int rc;

  rc = gsasl_client_mechlist (ctx, &tmp);

  if (rc == GSASL_OK)
    {
      size_t tmplen = strlen (tmp);

      if (tmplen >= *outlen)
	{
	  free (tmp);
	  return GSASL_TOO_SMALL_BUFFER;
	}

      if (out)
	strcpy (out, tmp);
      *outlen = tmplen + 1;
      free (tmp);
    }

  return rc;
}

/**
 * gsasl_server_listmech:
 * @ctx: libgsasl handle.
 * @out: output character array.
 * @outlen: input maximum size of output character array, on output
 * contains actual length of output array.
 *
 * Write SASL names, separated by space, of mechanisms supported by
 * the libgsasl server to the output array.  To find out how large the
 * output array must be, call this function with out=NULL.
 *
 * Return value: Returns GSASL_OK if successful, or error code.
 *
 * Deprecated: Use gsasl_server_mechlist() instead.
 **/
int
gsasl_server_listmech (Gsasl * ctx, char *out, size_t * outlen)
{
  char *tmp;
  int rc;

  rc = gsasl_server_mechlist (ctx, &tmp);

  if (rc == GSASL_OK)
    {
      size_t tmplen = strlen (tmp);

      if (tmplen >= *outlen)
	{
	  free (tmp);
	  return GSASL_TOO_SMALL_BUFFER;
	}

      if (out)
	strcpy (out, tmp);
      *outlen = tmplen + 1;
      free (tmp);
    }

  return rc;
}

static int
_gsasl_step (Gsasl_session * sctx,
	     const char *input, size_t input_len,
	     char *output, size_t * output_len)
{
  char *tmp;
  size_t tmplen;
  int rc;

  rc = gsasl_step (sctx, input, input_len, &tmp, &tmplen);

  if (rc == GSASL_OK || rc == GSASL_NEEDS_MORE)
    {
      if (tmplen >= *output_len)
	{
	  free (tmp);
	  /* XXX We lose the step token here, don't we? */
	  return GSASL_TOO_SMALL_BUFFER;
	}

      if (output)
	memcpy (output, tmp, tmplen);
      *output_len = tmplen;
      free (tmp);
    }

  return rc;
}

/**
 * gsasl_client_step:
 * @sctx: libgsasl client handle.
 * @input: input byte array.
 * @input_len: size of input byte array.
 * @output: output byte array.
 * @output_len: size of output byte array.
 *
 * Perform one step of SASL authentication in client.  This reads data
 * from server (specified with input and input_len), processes it
 * (potentially invoking callbacks to the application), and writes
 * data to server (into variables output and output_len).
 *
 * The contents of the output buffer is unspecified if this functions
 * returns anything other than GSASL_NEEDS_MORE.
 *
 * Return value: Returns GSASL_OK if authenticated terminated
 * successfully, GSASL_NEEDS_MORE if more data is needed, or error
 * code.
 *
 * Deprecated: Use gsasl_step() instead.
 **/
int
gsasl_client_step (Gsasl_session * sctx,
		   const char *input,
		   size_t input_len, char *output, size_t * output_len)
{
  return _gsasl_step (sctx, input, input_len, output, output_len);
}

/**
 * gsasl_server_step:
 * @sctx: libgsasl server handle.
 * @input: input byte array.
 * @input_len: size of input byte array.
 * @output: output byte array.
 * @output_len: size of output byte array.
 *
 * Perform one step of SASL authentication in server.  This reads data
 * from client (specified with input and input_len), processes it
 * (potentially invoking callbacks to the application), and writes
 * data to client (into variables output and output_len).
 *
 * The contents of the output buffer is unspecified if this functions
 * returns anything other than GSASL_NEEDS_MORE.
 *
 * Return value: Returns GSASL_OK if authenticated terminated
 * successfully, GSASL_NEEDS_MORE if more data is needed, or error
 * code.
 *
 * Deprecated: Use gsasl_step() instead.
 **/
int
gsasl_server_step (Gsasl_session * sctx,
		   const char *input,
		   size_t input_len, char *output, size_t * output_len)
{
  return _gsasl_step (sctx, input, input_len, output, output_len);
}

static int
_gsasl_step64 (Gsasl_session * sctx,
	       const char *b64input, char *b64output, size_t b64output_len)
{
  char *tmp;
  int rc;

  rc = gsasl_step64 (sctx, b64input, &tmp);

  if (rc == GSASL_OK || rc == GSASL_NEEDS_MORE)
    {
      if (b64output_len <= strlen (tmp))
	{
	  free (tmp);
	  /* XXX We lose the step token here, don't we? */
	  return GSASL_TOO_SMALL_BUFFER;
	}

      if (b64output)
	strcpy (b64output, tmp);
      free (tmp);
    }

  return rc;
}

/**
 * gsasl_client_step_base64:
 * @sctx: libgsasl client handle.
 * @b64input: input base64 encoded byte array.
 * @b64output: output base64 encoded byte array.
 * @b64output_len: size of output base64 encoded byte array.
 *
 * This is a simple wrapper around gsasl_client_step() that base64
 * decodes the input and base64 encodes the output.
 *
 * Return value: See gsasl_client_step().
 *
 * Deprecated: Use gsasl_step64() instead.
 **/
int
gsasl_client_step_base64 (Gsasl_session * sctx,
			  const char *b64input,
			  char *b64output, size_t b64output_len)
{
  return _gsasl_step64 (sctx, b64input, b64output, b64output_len);
}

/**
 * gsasl_server_step_base64:
 * @sctx: libgsasl server handle.
 * @b64input: input base64 encoded byte array.
 * @b64output: output base64 encoded byte array.
 * @b64output_len: size of output base64 encoded byte array.
 *
 * This is a simple wrapper around gsasl_server_step() that base64
 * decodes the input and base64 encodes the output.
 *
 * Return value: See gsasl_server_step().
 *
 * Deprecated: Use gsasl_step64() instead.
 **/
int
gsasl_server_step_base64 (Gsasl_session * sctx,
			  const char *b64input,
			  char *b64output, size_t b64output_len)
{
  return _gsasl_step64 (sctx, b64input, b64output, b64output_len);
}

/**
 * gsasl_client_finish:
 * @sctx: libgsasl client handle.
 *
 * Destroy a libgsasl client handle.  The handle must not be used with
 * other libgsasl functions after this call.
 *
 * Deprecated: Use gsasl_finish() instead.
 **/
void
gsasl_client_finish (Gsasl_session * sctx)
{
  gsasl_finish (sctx);
}

/**
 * gsasl_server_finish:
 * @sctx: libgsasl server handle.
 *
 * Destroy a libgsasl server handle.  The handle must not be used with
 * other libgsasl functions after this call.
 *
 * Deprecated: Use gsasl_finish() instead.
 **/
void
gsasl_server_finish (Gsasl_session * sctx)
{
  gsasl_finish (sctx);
}

/**
 * gsasl_client_ctx_get:
 * @sctx: libgsasl client handle
 *
 * Return value: Returns the libgsasl handle given a libgsasl client handle.
 *
 * Deprecated: This function is not useful with the new 0.2.0 API.
 **/
Gsasl *
gsasl_client_ctx_get (Gsasl_session * sctx)
{
  return sctx->ctx;
}

/**
 * gsasl_client_application_data_set:
 * @sctx: libgsasl client handle.
 * @application_data: opaque pointer to application specific data.
 *
 * Store application specific data in the libgsasl client handle.  The
 * application data can be later (for instance, inside a callback) be
 * retrieved by calling gsasl_client_application_data_get().  It is
 * normally used by the application to maintain state between the main
 * program and the callback.
 *
 * Deprecated: Use gsasl_callback_hook_set() instead.
 **/
void
gsasl_client_application_data_set (Gsasl_session * sctx,
				   void *application_data)
{
  gsasl_appinfo_set (sctx, application_data);
}

/**
 * gsasl_client_application_data_get:
 * @sctx: libgsasl client handle.
 *
 * Retrieve application specific data from libgsasl client handle. The
 * application data is set using gsasl_client_application_data_set().
 * It is normally used by the application to maintain state between
 * the main program and the callback.
 *
 * Return value: Returns the application specific data, or NULL.
 *
 * Deprecated: Use gsasl_callback_hook_get() instead.
 **/
void *
gsasl_client_application_data_get (Gsasl_session * sctx)
{
  return gsasl_appinfo_get (sctx);
}

/**
 * gsasl_server_ctx_get:
 * @sctx: libgsasl server handle
 *
 * Return value: Returns the libgsasl handle given a libgsasl server handle.
 *
 * Deprecated: This function is not useful with the new 0.2.0 API.
 **/
Gsasl *
gsasl_server_ctx_get (Gsasl_session * sctx)
{
  return sctx->ctx;
}

/**
 * gsasl_server_application_data_set:
 * @sctx: libgsasl server handle.
 * @application_data: opaque pointer to application specific data.
 *
 * Store application specific data in the libgsasl server handle.  The
 * application data can be later (for instance, inside a callback) be
 * retrieved by calling gsasl_server_application_data_get().  It is
 * normally used by the application to maintain state between the main
 * program and the callback.
 *
 * Deprecated: Use gsasl_callback_hook_set() instead.
 **/
void
gsasl_server_application_data_set (Gsasl_session * sctx,
				   void *application_data)
{
  gsasl_appinfo_set (sctx, application_data);
}

/**
 * gsasl_server_application_data_get:
 * @sctx: libgsasl server handle.
 *
 * Retrieve application specific data from libgsasl server handle. The
 * application data is set using gsasl_server_application_data_set().
 * It is normally used by the application to maintain state between
 * the main program and the callback.
 *
 * Return value: Returns the application specific data, or NULL.
 *
 * Deprecated: Use gsasl_callback_hook_get() instead.
 **/
void *
gsasl_server_application_data_get (Gsasl_session * sctx)
{
  return gsasl_appinfo_get (sctx);
}

/**
 * gsasl_randomize:
 * @strong: 0 iff operation should not block, non-0 for very strong randomness.
 * @data: output array to be filled with random data.
 * @datalen: size of output array.
 *
 * Store cryptographically random data of given size in the provided
 * buffer.
 *
 * Return value: Returns %GSASL_OK iff successful.
 *
 * Deprecated: Use gsasl_random() or gsasl_nonce() instead.
 **/
int
gsasl_randomize (int strong, char *data, size_t datalen)
{
  if (strong)
    return gsasl_random (data, datalen);
  return gsasl_nonce (data, datalen);
}

/**
 * gsasl_ctx_get:
 * @sctx: libgsasl session handle
 *
 * Return value: Returns the libgsasl handle given a libgsasl session handle.
 *
 * Deprecated: This function is not useful with the new 0.2.0 API.
 **/
Gsasl *
gsasl_ctx_get (Gsasl_session * sctx)
{
  return sctx->ctx;
}

/**
 * gsasl_encode_inline:
 * @sctx: libgsasl session handle.
 * @input: input byte array.
 * @input_len: size of input byte array.
 * @output: output byte array.
 * @output_len: size of output byte array.
 *
 * Encode data according to negotiated SASL mechanism.  This might mean
 * that data is integrity or privacy protected.
 *
 * Return value: Returns GSASL_OK if encoding was successful, otherwise
 * an error code.
 *
 * Deprecated: Use gsasl_encode() instead.
 *
 * Since: 0.2.0
 **/
int
gsasl_encode_inline (Gsasl_session * sctx,
		     const char *input, size_t input_len,
		     char *output, size_t * output_len)
{
  char *tmp;
  size_t tmplen;
  int res;

  res = gsasl_encode (sctx, input, input_len, &tmp, &tmplen);
  if (res == GSASL_OK)
    {
      if (*output_len < tmplen)
	return GSASL_TOO_SMALL_BUFFER;
      *output_len = tmplen;
      memcpy (output, tmp, tmplen);
      free (output);
    }

  return res;
}

/**
 * gsasl_decode_inline:
 * @sctx: libgsasl session handle.
 * @input: input byte array.
 * @input_len: size of input byte array.
 * @output: output byte array.
 * @output_len: size of output byte array.
 *
 * Decode data according to negotiated SASL mechanism.  This might mean
 * that data is integrity or privacy protected.
 *
 * Return value: Returns GSASL_OK if encoding was successful, otherwise
 * an error code.
 *
 * Deprecated: Use gsasl_decode() instead.
 *
 * Since: 0.2.0
 **/
int
gsasl_decode_inline (Gsasl_session * sctx,
		     const char *input, size_t input_len,
		     char *output, size_t * output_len)
{
  char *tmp;
  size_t tmplen;
  int res;

  res = gsasl_decode (sctx, input, input_len, &tmp, &tmplen);
  if (res == GSASL_OK)
    {
      if (*output_len < tmplen)
	return GSASL_TOO_SMALL_BUFFER;
      *output_len = tmplen;
      memcpy (output, tmp, tmplen);
      free (output);
    }

  return res;
}

/**
 * gsasl_application_data_set:
 * @ctx: libgsasl handle.
 * @appdata: opaque pointer to application specific data.
 *
 * Store application specific data in the libgsasl handle.  The
 * application data can be later (for instance, inside a callback) be
 * retrieved by calling gsasl_application_data_get().  It is normally
 * used by the application to maintain state between the main program
 * and the callback.
 *
 * Deprecated: Use gsasl_callback_hook_set() instead.
 **/
void
gsasl_application_data_set (Gsasl * ctx, void *appdata)
{
  ctx->application_hook = appdata;
}

/**
 * gsasl_application_data_get:
 * @ctx: libgsasl handle.
 *
 * Retrieve application specific data from libgsasl handle. The
 * application data is set using gsasl_application_data_set().  It is
 * normally used by the application to maintain state between the main
 * program and the callback.
 *
 * Return value: Returns the application specific data, or NULL.
 *
 * Deprecated: Use gsasl_callback_hook_get() instead.
 **/
void *
gsasl_application_data_get (Gsasl * ctx)
{
  return ctx->application_hook;
}

/**
 * gsasl_appinfo_set:
 * @sctx: libgsasl session handle.
 * @appdata: opaque pointer to application specific data.
 *
 * Store application specific data in the libgsasl session handle.
 * The application data can be later (for instance, inside a callback)
 * be retrieved by calling gsasl_appinfo_get().  It is normally used
 * by the application to maintain state between the main program and
 * the callback.
 *
 * Deprecated: Use gsasl_callback_hook_set() instead.
 **/
void
gsasl_appinfo_set (Gsasl_session * sctx, void *appdata)
{
  sctx->application_data = appdata;
}

/**
 * gsasl_appinfo_get:
 * @sctx: libgsasl session handle.
 *
 * Retrieve application specific data from libgsasl session
 * handle. The application data is set using gsasl_appinfo_set().  It
 * is normally used by the application to maintain state between the
 * main program and the callback.
 *
 * Return value: Returns the application specific data, or NULL.
 *
 * Deprecated: Use gsasl_callback_hook_get() instead.
 **/
void *
gsasl_appinfo_get (Gsasl_session * sctx)
{
  return sctx->application_data;
}

/**
 * gsasl_server_suggest_mechanism:
 * @ctx: libgsasl handle.
 * @mechlist: input character array with SASL mechanism names,
 *   separated by invalid characters (e.g. SPC).
 *
 * Return value: Returns name of "best" SASL mechanism supported by
 * the libgsasl server which is present in the input string.
 *
 * Deprecated: This function was never useful, since it is the client
 * that chose which mechanism to use.
 **/
const char *
gsasl_server_suggest_mechanism (Gsasl * ctx, const char *mechlist)
{
  return NULL;			/* This function is just silly. */
}

/**
 * gsasl_client_callback_authentication_id_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to set the
 * authentication identity.  The function can be later retrieved using
 * gsasl_client_callback_authentication_id_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_authentication_id_set (Gsasl * ctx,
					     Gsasl_client_callback_authentication_id
					     cb)
{
  ctx->cbc_authentication_id = cb;
}

/**
 * gsasl_client_callback_authentication_id_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_authentication_id_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_authentication_id
gsasl_client_callback_authentication_id_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_authentication_id : NULL;
}

/**
 * gsasl_client_callback_authorization_id_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to set the
 * authorization identity.  The function can be later retrieved using
 * gsasl_client_callback_authorization_id_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_authorization_id_set (Gsasl * ctx,
					    Gsasl_client_callback_authorization_id
					    cb)
{
  ctx->cbc_authorization_id = cb;
}

/**
 * gsasl_client_callback_authorization_id_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_authorization_id_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_authorization_id
gsasl_client_callback_authorization_id_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_authorization_id : NULL;
}

/**
 * gsasl_client_callback_password_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to set the
 * password.  The function can be later retrieved using
 * gsasl_client_callback_password_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_password_set (Gsasl * ctx,
				    Gsasl_client_callback_password cb)
{
  ctx->cbc_password = cb;
}


/**
 * gsasl_client_callback_password_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_password_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_password
gsasl_client_callback_password_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_password : NULL;
}

/**
 * gsasl_client_callback_passcode_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to set the
 * passcode.  The function can be later retrieved using
 * gsasl_client_callback_passcode_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_passcode_set (Gsasl * ctx,
				    Gsasl_client_callback_passcode cb)
{
  ctx->cbc_passcode = cb;
}


/**
 * gsasl_client_callback_passcode_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_passcode_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_passcode
gsasl_client_callback_passcode_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_passcode : NULL;
}

/**
 * gsasl_client_callback_pin_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to chose a new
 * pin, possibly suggested by the server, for the SECURID mechanism.
 * This is not normally invoked, but only when the server requests it.
 * The function can be later retrieved using
 * gsasl_client_callback_pin_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_pin_set (Gsasl * ctx, Gsasl_client_callback_pin cb)
{
  ctx->cbc_pin = cb;
}


/**
 * gsasl_client_callback_pin_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_pin_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_pin
gsasl_client_callback_pin_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_pin : NULL;
}

/**
 * gsasl_client_callback_service_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to set the name
 * of the service.  The service buffer should be a registered GSSAPI
 * host-based service name, hostname the name of the server.
 * Servicename is used by DIGEST-MD5 and should be the name of generic
 * server in case of a replicated service. The function can be later
 * retrieved using gsasl_client_callback_service_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_service_set (Gsasl * ctx,
				   Gsasl_client_callback_service cb)
{
  ctx->cbc_service = cb;
}

/**
 * gsasl_client_callback_service_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_service_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_service
gsasl_client_callback_service_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_service : NULL;
}

/**
 * gsasl_client_callback_anonymous_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to set the
 * anonymous token, which usually is the users email address.  The
 * function can be later retrieved using
 * gsasl_client_callback_anonymous_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_anonymous_set (Gsasl * ctx,
				     Gsasl_client_callback_anonymous cb)
{
  ctx->cbc_anonymous = cb;
}

/**
 * gsasl_client_callback_anonymous_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_anonymous_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_anonymous
gsasl_client_callback_anonymous_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_anonymous : NULL;
}

/**
 * gsasl_client_callback_qop_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to determine the
 * qop to use after looking at what the server offered.  The function
 * can be later retrieved using gsasl_client_callback_qop_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_qop_set (Gsasl * ctx, Gsasl_client_callback_qop cb)
{
  ctx->cbc_qop = cb;
}

/**
 * gsasl_client_callback_qop_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_qop_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_qop
gsasl_client_callback_qop_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_qop : NULL;
}

/**
 * gsasl_client_callback_maxbuf_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to inform the
 * server of the largest buffer the client is able to receive when
 * using the DIGEST-MD5 "auth-int" or "auth-conf" Quality of
 * Protection (qop). If this directive is missing, the default value
 * 65536 will be assumed.  The function can be later retrieved using
 * gsasl_client_callback_maxbuf_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_maxbuf_set (Gsasl * ctx,
				  Gsasl_client_callback_maxbuf cb)
{
  ctx->cbc_maxbuf = cb;
}

/**
 * gsasl_client_callback_maxbuf_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_maxbuf_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_maxbuf
gsasl_client_callback_maxbuf_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_maxbuf : NULL;
}

/**
 * gsasl_client_callback_realm_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the client to know which
 * realm it belongs to.  The realm is used by the server to determine
 * which username and password to use.  The function can be later
 * retrieved using gsasl_client_callback_realm_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_client_callback_realm_set (Gsasl * ctx, Gsasl_client_callback_realm cb)
{
  ctx->cbc_realm = cb;
}

/**
 * gsasl_client_callback_realm_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_client_callback_realm_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_client_callback_realm
gsasl_client_callback_realm_get (Gsasl * ctx)
{
  return ctx ? ctx->cbc_realm : NULL;
}

/**
 * gsasl_server_callback_validate_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server for deciding if
 * user is authenticated using authentication identity, authorization
 * identity and password.  The function can be later retrieved using
 * gsasl_server_callback_validate_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_validate_set (Gsasl * ctx,
				    Gsasl_server_callback_validate cb)
{
  ctx->cbs_validate = cb;
}

/**
 * gsasl_server_callback_validate_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_validate_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_validate
gsasl_server_callback_validate_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_validate : NULL;
}

/**
 * gsasl_server_callback_retrieve_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server for deciding if
 * user is authenticated using authentication identity, authorization
 * identity and password.  The function can be later retrieved using
 * gsasl_server_callback_retrieve_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_retrieve_set (Gsasl * ctx,
				    Gsasl_server_callback_retrieve cb)
{
  ctx->cbs_retrieve = cb;
}

/**
 * gsasl_server_callback_retrieve_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_retrieve_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_retrieve
gsasl_server_callback_retrieve_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_retrieve : NULL;
}

/**
 * gsasl_server_callback_cram_md5_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server for deciding if
 * user is authenticated using CRAM-MD5 challenge and response.  The
 * function can be later retrieved using
 * gsasl_server_callback_cram_md5_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_cram_md5_set (Gsasl * ctx,
				    Gsasl_server_callback_cram_md5 cb)
{
  ctx->cbs_cram_md5 = cb;
}

/**
 * gsasl_server_callback_cram_md5_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_cram_md5_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_cram_md5
gsasl_server_callback_cram_md5_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_cram_md5 : NULL;
}

/**
 * gsasl_server_callback_digest_md5_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server for retrieving
 * the secret hash of the username, realm and password for use in the
 * DIGEST-MD5 mechanism.  The function can be later retrieved using
 * gsasl_server_callback_digest_md5_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_digest_md5_set (Gsasl * ctx,
				      Gsasl_server_callback_digest_md5 cb)
{
  ctx->cbs_digest_md5 = cb;
}

/**
 * gsasl_server_callback_digest_md5_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Return the callback earlier set by calling
 * gsasl_server_callback_digest_md5_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_digest_md5
gsasl_server_callback_digest_md5_get (Gsasl * ctx)
{
  return ctx->cbs_digest_md5;
}

/**
 * gsasl_server_callback_external_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server for deciding if
 * user is authenticated out of band.  The function can be later
 * retrieved using gsasl_server_callback_external_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_external_set (Gsasl * ctx,
				    Gsasl_server_callback_external cb)
{
  ctx->cbs_external = cb;
}

/**
 * gsasl_server_callback_external_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_external_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_external
gsasl_server_callback_external_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_external : NULL;
}

/**
 * gsasl_server_callback_anonymous_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server for deciding if
 * user is permitted anonymous access.  The function can be later
 * retrieved using gsasl_server_callback_anonymous_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_anonymous_set (Gsasl * ctx,
				     Gsasl_server_callback_anonymous cb)
{
  ctx->cbs_anonymous = cb;
}

/**
 * gsasl_server_callback_anonymous_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_anonymous_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_anonymous
gsasl_server_callback_anonymous_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_anonymous : NULL;
}

/**
 * gsasl_server_callback_realm_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server to know which
 * realm it serves.  The realm is used by the user to determine which
 * username and password to use.  The function can be later retrieved
 * using gsasl_server_callback_realm_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_realm_set (Gsasl * ctx, Gsasl_server_callback_realm cb)
{
  ctx->cbs_realm = cb;
}

/**
 * gsasl_server_callback_realm_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_realm_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_realm
gsasl_server_callback_realm_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_realm : NULL;
}

/**
 * gsasl_server_callback_qop_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server to know which
 * quality of protection it accepts.  The quality of protection
 * eventually used is selected by the client though.  It is currently
 * used by the DIGEST-MD5 mechanism. The function can be later
 * retrieved using gsasl_server_callback_qop_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_qop_set (Gsasl * ctx, Gsasl_server_callback_qop cb)
{
  ctx->cbs_qop = cb;
}

/**
 * gsasl_server_callback_qop_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_qop_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_qop
gsasl_server_callback_qop_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_qop : NULL;
}

/**
 * gsasl_server_callback_maxbuf_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server to inform the
 * client of the largest buffer the server is able to receive when
 * using the DIGEST-MD5 "auth-int" or "auth-conf" Quality of
 * Protection (qop). If this directive is missing, the default value
 * 65536 will be assumed.  The function can be later retrieved using
 * gsasl_server_callback_maxbuf_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_maxbuf_set (Gsasl * ctx,
				  Gsasl_server_callback_maxbuf cb)
{
  ctx->cbs_maxbuf = cb;
}

/**
 * gsasl_server_callback_maxbuf_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_maxbuf_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_maxbuf
gsasl_server_callback_maxbuf_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_maxbuf : NULL;
}

/**
 * gsasl_server_callback_cipher_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server to inform the
 * client of the cipher suites supported.  The DES and 3DES ciphers
 * must be supported for interoperability.  It is currently used by
 * the DIGEST-MD5 mechanism.  The function can be later retrieved
 * using gsasl_server_callback_cipher_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_cipher_set (Gsasl * ctx,
				  Gsasl_server_callback_cipher cb)
{
  ctx->cbs_cipher = cb;
}

/**
 * gsasl_server_callback_cipher_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_cipher_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_cipher
gsasl_server_callback_cipher_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_cipher : NULL;
}

/**
 * gsasl_server_callback_securid_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server for validating a
 * user via the SECURID mechanism.  The function should return
 * GSASL_OK if user authenticated successfully,
 * GSASL_SECURID_SERVER_NEED_ADDITIONAL_PASSCODE if it wants another
 * passcode, GSASL_SECURID_SERVER_NEED_NEW_PIN if it wants a PIN
 * change, or an error.  When (and only when)
 * GSASL_SECURID_SERVER_NEED_NEW_PIN is returned, suggestpin can be
 * populated with a PIN code the server suggests, and suggestpinlen
 * set to the length of the PIN.  The function can be later retrieved
 * using gsasl_server_callback_securid_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_securid_set (Gsasl * ctx,
				   Gsasl_server_callback_securid cb)
{
  ctx->cbs_securid = cb;
}

/**
 * gsasl_server_callback_securid_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_securid_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_securid
gsasl_server_callback_securid_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_securid : NULL;
}

/**
 * gsasl_server_callback_gssapi_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server for checking if
 * a GSSAPI user is authorized for username (by, e.g., calling
 * krb5_userok()).  The function should return GSASL_OK if the user
 * should be permitted access, or an error code such as
 * GSASL_AUTHENTICATION_ERROR on failure.  The function can be later
 * retrieved using gsasl_server_callback_gssapi_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_gssapi_set (Gsasl * ctx,
				  Gsasl_server_callback_gssapi cb)
{
  ctx->cbs_gssapi = cb;
}

/**
 * gsasl_server_callback_gssapi_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_gssapi_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_gssapi
gsasl_server_callback_gssapi_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_gssapi : NULL;
}

/**
 * gsasl_server_callback_service_set:
 * @ctx: libgsasl handle.
 * @cb: callback function
 *
 * Specify the callback function to use in the server to set the name
 * of the service.  The service buffer should be a registered GSSAPI
 * host-based service name, hostname the name of the server.  The
 * function can be later retrieved using
 * gsasl_server_callback_service_get().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
void
gsasl_server_callback_service_set (Gsasl * ctx,
				   Gsasl_server_callback_service cb)
{
  ctx->cbs_service = cb;
}

/**
 * gsasl_server_callback_service_get:
 * @ctx: libgsasl handle.
 *
 * Return value: Returns the callback earlier set by calling
 * gsasl_server_callback_service_set().
 *
 * Deprecated: This function is part of the old callback interface.
 * The new interface uses gsasl_callback_set() to set the application
 * callback, and uses gsasl_callback() or gsasl_property_get() to
 * invoke the callback for certain properties.
 **/
Gsasl_server_callback_service
gsasl_server_callback_service_get (Gsasl * ctx)
{
  return ctx ? ctx->cbs_service : NULL;
}

#if WITH_SASLPREP
# include <stringprep.h>
#endif

/**
 * gsasl_stringprep_nfkc:
 * @in: a UTF-8 encoded string.
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 *
 * Converts a string into canonical form, standardizing such issues as
 * whether a character with an accent is represented as a base
 * character and combining accent or as a single precomposed
 * character.
 *
 * The normalization mode is NFKC (ALL COMPOSE).  It standardizes
 * differences that do not affect the text content, such as the
 * above-mentioned accent representation. It standardizes the
 * "compatibility" characters in Unicode, such as SUPERSCRIPT THREE to
 * the standard forms (in this case DIGIT THREE). Formatting
 * information may be lost but for most text operations such
 * characters should be considered the same. It returns a result with
 * composed forms rather than a maximally decomposed form.
 *
 * Return value: Return a newly allocated string, that is the NFKC
 *   normalized form of @str, o %NULL on error.
 *
 * Deprecated: No replacement functionality in GNU SASL, use GNU
 * Libidn instead.  Note that in SASL, you most likely want to use
 * SASLprep and not bare NFKC, see gsasl_saslprep().
 **/
char *
gsasl_stringprep_nfkc (const char *in, ssize_t len)
{
  char *out = NULL;

#if WITH_SASLPREP
  out = stringprep_utf8_nfkc_normalize (in, len);
#endif

  return out;
}

/**
 * gsasl_stringprep_saslprep:
 * @in: input ASCII or UTF-8 string with data to prepare according to SASLprep.
 * @stringprep_rc: pointer to output variable with stringprep error code,
 *   or %NULL to indicate that you don't care about it.
 *
 * Process a Unicode string for comparison, according to the
 * "SASLprep" stringprep profile.  This function is intended to be
 * used by Simple Authentication and Security Layer (SASL) mechanisms
 * (such as PLAIN, CRAM-MD5, and DIGEST-MD5) as well as other
 * protocols exchanging user names and/or passwords.
 *
 * Return value: Return a newly allocated string that is the
 *   "SASLprep" processed form of the input string, or %NULL on error,
 *   in which case @stringprep_rc contain the stringprep library error
 *   code.
 *
 * Deprecated: Use gsasl_saslprep() instead.
 **/
char *
gsasl_stringprep_saslprep (const char *in, int *stringprep_rc)
{
  char *out = NULL;
  int rc;

#if WITH_SASLPREP
  rc = stringprep_profile (in, &out, "SASLprep", 0);
  if (stringprep_rc)
    *stringprep_rc = rc;
  if (rc != STRINGPREP_OK)
    out = NULL;
#endif

  return out;
}

/**
 * gsasl_stringprep_trace:
 * @in: input ASCII or UTF-8 string with data to prepare according to "trace".
 * @stringprep_rc: pointer to output variable with stringprep error code,
 *   or %NULL to indicate that you don't care about it.
 *
 * Process a Unicode string for use as trace information, according to
 * the "trace" stringprep profile.  The profile is designed for use
 * with the SASL ANONYMOUS Mechanism.
 *
 * Return value: Return a newly allocated string that is the "trace"
 *   processed form of the input string, or %NULL on error, in which
 *   case @stringprep_rc contain the stringprep library error code.
 *
 * Deprecated: No replacement functionality in GNU SASL, use GNU
 * Libidn instead.
 **/
char *
gsasl_stringprep_trace (const char *in, int *stringprep_rc)
{
  char *out = NULL;
  int rc;

#if WITH_SASLPREP
  rc = stringprep_profile (in, &out, "trace", 0);
  if (stringprep_rc)
    *stringprep_rc = rc;
  if (rc != STRINGPREP_OK)
    out = NULL;
#endif

  return out;
}

/**
 * gsasl_md5pwd_get_password:
 * @filename: filename of file containing passwords.
 * @username: username string.
 * @key: output character array.
 * @keylen: input maximum size of output character array, on output
 * contains actual length of output array.
 *
 * Retrieve password for user from specified file.  To find out how
 * large the output array must be, call this function with out=NULL.
 *
 * The file should be on the UoW "MD5 Based Authentication" format,
 * which means it is in text format with comments denoted by # first
 * on the line, with user entries looking as username\tpassword.  This
 * function removes \r and \n at the end of lines before processing.
 *
 * Return value: Return GSASL_OK if output buffer contains the
 * password, GSASL_AUTHENTICATION_ERROR if the user could not be
 * found, or other error code.
 *
 * Deprecated: Use gsasl_simple_getpass() instead.
 **/
int
gsasl_md5pwd_get_password (const char *filename,
			   const char *username, char *key, size_t * keylen)
{
  char matchbuf[BUFSIZ];
  char line[BUFSIZ];
  FILE *fh;

  fh = fopen (filename, "r");
  if (fh == NULL)
    return GSASL_FOPEN_ERROR;

  sprintf (matchbuf, "%s\t", username);

  while (!feof (fh))
    {
      if (fgets (line, BUFSIZ, fh) == NULL)
	break;

      if (line[0] == '#')
	continue;

      while (strlen (line) > 0 && (line[strlen (line) - 1] == '\n' ||
				   line[strlen (line) - 1] == '\r'))
	line[strlen (line) - 1] = '\0';

      if (strlen (line) <= strlen (matchbuf))
	continue;

      if (strncmp (matchbuf, line, strlen (matchbuf)) == 0)
	{
	  if (*keylen < strlen (line) - strlen (matchbuf))
	    {
	      fclose (fh);
	      return GSASL_TOO_SMALL_BUFFER;
	    }

	  *keylen = strlen (line) - strlen (matchbuf);

	  if (key)
	    memcpy (key, &line[strlen (matchbuf)], *keylen);

	  fclose (fh);

	  return GSASL_OK;
	}
    }

  if (fclose (fh) != 0)
    return GSASL_FCLOSE_ERROR;

  return GSASL_AUTHENTICATION_ERROR;
}


/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Portions Copyright (c) 1995 by International Business Machines, Inc.
 *
 * International Business Machines, Inc. (hereinafter called IBM) grants
 * permission under its copyrights to use, copy, modify, and distribute this
 * Software with or without fee, provided that the above copyright notice and
 * all paragraphs of this notice appear in all copies, and that the name of IBM
 * not be used in connection with the marketing of any product incorporating
 * the Software or modifications thereof, without specific, written prior
 * permission.
 *
 * To the extent it has a right to do so, IBM grants an immunity from suit
 * under its patents, if any, for the use, sale or manufacture of products to
 * the extent that such products are used for performing Domain Name System
 * dynamic updates in TCP/IP networks by means of the Software.  No immunity is
 * granted for any product per se or for any other function of any product.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", AND IBM DISCLAIMS ALL WARRANTIES,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL,
 * DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, EVEN
 * IF IBM IS APPRISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#include <ctype.h>
#include <stdio.h>

#define Assert(Cond) if (!(Cond)) abort()

static const char Base64[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

/**
 * gsasl_base64_encode:
 * @src: input byte array
 * @srclength: size of input byte array
 * @target: output byte array
 * @targsize: size of output byte array
 *
 * Encode data as base64.  Converts characters, three at a time,
 * starting at src into four base64 characters in the target area
 * until the entire input buffer is encoded.
 *
 * Return value: Returns the number of data bytes stored at the
 * target, or -1 on error.
 *
 * Deprecated: Use gsasl_base64_to() instead.
 **/
int
gsasl_base64_encode (char const *src,
		     size_t srclength, char *target, size_t targsize)
{
  size_t datalength = 0;
  unsigned char input[3];
  unsigned char output[4];
  size_t i;

  while (2 < srclength)
    {
      input[0] = *src++;
      input[1] = *src++;
      input[2] = *src++;
      srclength -= 3;

      output[0] = input[0] >> 2;
      output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
      output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
      output[3] = input[2] & 0x3f;
      Assert (output[0] < 64);
      Assert (output[1] < 64);
      Assert (output[2] < 64);
      Assert (output[3] < 64);

      if (datalength + 4 > targsize)
	return (-1);
      target[datalength++] = Base64[output[0]];
      target[datalength++] = Base64[output[1]];
      target[datalength++] = Base64[output[2]];
      target[datalength++] = Base64[output[3]];
    }

  /* Now we worry about padding. */
  if (0 != srclength)
    {
      /* Get what's left. */
      input[0] = input[1] = input[2] = '\0';
      for (i = 0; i < srclength; i++)
	input[i] = *src++;

      output[0] = input[0] >> 2;
      output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
      output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
      Assert (output[0] < 64);
      Assert (output[1] < 64);
      Assert (output[2] < 64);

      if (datalength + 4 > targsize)
	return (-1);
      target[datalength++] = Base64[output[0]];
      target[datalength++] = Base64[output[1]];
      if (srclength == 1)
	target[datalength++] = Pad64;
      else
	target[datalength++] = Base64[output[2]];
      target[datalength++] = Pad64;
    }
  if (datalength >= targsize)
    return (-1);
  target[datalength] = '\0';	/* Returned value doesn't count \0. */
  return (datalength);
}

/**
 * gsasl_base64_decode:
 * @src: input byte array
 * @target: output byte array
 * @targsize: size of output byte array
 *
 * Decode Base64 data.  Skips all whitespace anywhere.  Converts
 * characters, four at a time, starting at (or after) src from Base64
 * numbers into three 8 bit bytes in the target area.
 *
 * Return value: Returns the number of data bytes stored at the
 * target, or -1 on error.
 *
 * Deprecated: Use gsasl_base64_from() instead.
 **/
int
gsasl_base64_decode (char const *src, char *target, size_t targsize)
{
  int tarindex, state, ch;
  char *pos;

  state = 0;
  tarindex = 0;

  while ((ch = *src++) != '\0')
    {
      if (isspace (ch))		/* Skip whitespace anywhere. */
	continue;

      if (ch == Pad64)
	break;

      pos = strchr (Base64, ch);
      if (pos == 0)		/* A non-base64 character. */
	return (-1);

      switch (state)
	{
	case 0:
	  if (target)
	    {
	      if ((size_t) tarindex >= targsize)
		return (-1);
	      target[tarindex] = (pos - Base64) << 2;
	    }
	  state = 1;
	  break;
	case 1:
	  if (target)
	    {
	      if ((size_t) tarindex + 1 >= targsize)
		return (-1);
	      target[tarindex] |= (pos - Base64) >> 4;
	      target[tarindex + 1] = ((pos - Base64) & 0x0f) << 4;
	    }
	  tarindex++;
	  state = 2;
	  break;
	case 2:
	  if (target)
	    {
	      if ((size_t) tarindex + 1 >= targsize)
		return (-1);
	      target[tarindex] |= (pos - Base64) >> 2;
	      target[tarindex + 1] = ((pos - Base64) & 0x03) << 6;
	    }
	  tarindex++;
	  state = 3;
	  break;
	case 3:
	  if (target)
	    {
	      if ((size_t) tarindex >= targsize)
		return (-1);
	      target[tarindex] |= (pos - Base64);
	    }
	  tarindex++;
	  state = 0;
	  break;
	default:
	  abort ();
	}
    }

  /*
   * We are done decoding Base-64 chars.  Let's see if we ended
   * on a byte boundary, and/or with erroneous trailing characters.
   */

  if (ch == Pad64)
    {				/* We got a pad char. */
      ch = *src++;		/* Skip it, get next. */
      switch (state)
	{
	case 0:		/* Invalid = in first position */
	case 1:		/* Invalid = in second position */
	  return (-1);

	case 2:		/* Valid, means one byte of info */
	  /* Skip any number of spaces. */
	  for ((void) NULL; ch != '\0'; ch = *src++)
	    if (!isspace (ch))
	      break;
	  /* Make sure there is another trailing = sign. */
	  if (ch != Pad64)
	    return (-1);
	  ch = *src++;		/* Skip the = */
	  /* Fall through to "single trailing =" case. */
	  /* FALLTHROUGH */

	case 3:		/* Valid, means two bytes of info */
	  /*
	   * We know this char is an =.  Is there anything but
	   * whitespace after it?
	   */
	  for ((void) NULL; ch != '\0'; ch = *src++)
	    if (!isspace (ch))
	      return (-1);

	  /*
	   * Now make sure for cases 2 and 3 that the "extra"
	   * bits that slopped past the last full byte were
	   * zeros.  If we don't check them, they become a
	   * subliminal channel.
	   */
	  if (target && target[tarindex] != 0)
	    return (-1);
	}
    }
  else
    {
      /*
       * We ended by seeing the end of the string.  Make sure we
       * have no partial bytes lying around.
       */
      if (state != 0)
	return (-1);
    }

  return (tarindex);
}
