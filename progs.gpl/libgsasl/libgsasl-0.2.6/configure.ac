dnl Process this file with autoconf to produce a configure script.

# Copyright (C) 2002, 2003, 2004, 2005 Simon Josefsson.
#
# This file is part of GNU SASL Library.
#
# GNU SASL Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
#
# GNU SASL Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with GNU SASL Library; if not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.

AC_INIT([libgsasl], [0.2.6], [bug-gsasl@gnu.org])

# Library code modified:                              REVISION++
# Interfaces changed/added/removed:   CURRENT++       REVISION=0
# Interfaces added:                             AGE++
# Interfaces removed:                           AGE=0
AC_SUBST(LT_CURRENT, 9)
AC_SUBST(LT_REVISION, 2)
AC_SUBST(LT_AGE, 2)

AC_PREREQ(2.59)
AM_INIT_AUTOMAKE([1.9 gnits])
AC_CONFIG_HEADERS(config.h)

# Checks for programs.
AM_GNU_GETTEXT(external)
AM_GNU_GETTEXT_VERSION(0.14.1)
AC_PROG_CC
gl_EARLY
AC_PROG_LIBTOOL

# Checks for header files.
AC_CHECK_HEADERS(sys/types.h netinet/in.h)

# ANONYMOUS
AC_ARG_ENABLE(anonymous,
  AS_HELP_STRING([--disable-anonymous], [don't use the ANONYMOUS mechanism]),
  anonymous=$enableval)
if test "$anonymous" != "no" ; then
	anonymous=yes
	AC_DEFINE(USE_ANONYMOUS, 1, [Define to 1 if you want ANONYMOUS.])
fi
AC_MSG_CHECKING([if ANONYMOUS should be used])
AC_MSG_RESULT($anonymous)
AM_CONDITIONAL(ANONYMOUS, test x$anonymous = xyes)

# EXTERNAL
AC_ARG_ENABLE(external,
  AS_HELP_STRING([--disable-external], [don't use the EXTERNAL mechanism]),
  external=$enableval)
if test "$external" != "no" ; then
	external=yes
	AC_DEFINE(USE_EXTERNAL, 1, [Define to 1 if you want EXTERNAL.])
fi
AC_MSG_CHECKING([if EXTERNAL should be used])
AC_MSG_RESULT($external)
AM_CONDITIONAL(EXTERNAL, test x$external = xyes)

# PLAIN
AC_ARG_ENABLE(plain,
  AS_HELP_STRING([--disable-plain], [don't use the PLAIN mechanism]),
  plain=$enableval)
if test "$plain" != "no" ; then
	plain=yes
	AC_DEFINE(USE_PLAIN, 1, [Define to 1 if you want PLAIN.])
fi
AC_MSG_CHECKING([if PLAIN should be used])
AC_MSG_RESULT($plain)
AM_CONDITIONAL(PLAIN, test x$plain = xyes)

# LOGIN
AC_ARG_ENABLE(login,
  AS_HELP_STRING([--disable-login], [don't use the LOGIN mechanism]),
  login=$enableval)
if test "$login" != "no" ; then
	login=yes
	AC_DEFINE(USE_LOGIN, 1, [Define to 1 if you want LOGIN.])
fi
AC_MSG_CHECKING([if LOGIN should be used])
AC_MSG_RESULT($login)
AM_CONDITIONAL(LOGIN, test x$login = xyes)

# SECURID
AC_ARG_ENABLE(securid,
  AS_HELP_STRING([--disable-securid], [don't use the SECURID mechanism]),
  securid=$enableval)
if test "$securid" != "no" ; then
	securid=yes
	AC_DEFINE(USE_SECURID, 1, [Define to 1 if you want SECURID.])
fi
AC_MSG_CHECKING([if SECURID should be used])
AC_MSG_RESULT($securid)
AM_CONDITIONAL(SECURID, test x$securid = xyes)

# NTLM
AC_ARG_ENABLE(ntlm,
  AS_HELP_STRING([--disable-ntlm], [don't use the NTLM mechanism]),
  ntlm=$enableval)
if test "$ntlm" != "no"; then
	sj_CHECK_HEADERLIB(ntlm.h, ntlm, ntlm_des_set_key,
		[ntlm=yes
		AC_DEFINE(USE_NTLM, 1, [Define to 1 if you want NTLM.])],
		[ntlm=no
		AC_MSG_WARN([Libntlm >= 0.3.5 not found, disabling NTLM.])])
fi
AC_MSG_CHECKING([if NTLM should be used])
AC_MSG_RESULT($ntlm)
AM_CONDITIONAL(NTLM, test x$ntlm = xyes)

# Check for libgcrypt
AC_ARG_WITH(libgcrypt,
  AS_HELP_STRING([--with-libgcrypt], [use libgcrypt for low-level crypto]),
  libgcrypt=$withval, libgcrypt=no)
if test "$libgcrypt" != "no" ; then
	AM_PATH_LIBGCRYPT(1.1.42, :, [
		libgcrypt=no
		AC_MSG_WARN([Libgcrypt >= 1.1.42 not found.])
	])
fi
AC_MSG_CHECKING([whether to use libgcrypt])
AC_MSG_RESULT($libgcrypt)
if test "$libgcrypt" != "no" ; then
	AC_DEFINE(USE_GCRYPT, 1, [Define to 1 if you want to use libgcrypt.])
fi
AM_CONDITIONAL(LIBGCRYPT, test "$libgcrypt" != "no")
AM_CONDITIONAL(NETTLE, test "$libgcrypt" = "no")

# Generic Crypto stuff.
if test "$libgcrypt" = "no" ; then
  # Devices with randomness.
  # FIXME: Are these the best defaults?
  case "${target}" in
    *-openbsd*)
	NAME_OF_RANDOM_DEVICE="/dev/srandom"
	NAME_OF_PSEUDO_RANDOM_DEVICE="/dev/prandom"
	NAME_OF_NONCE_DEVICE="/dev/urandom"
        ;;

    *-netbsd*)
	NAME_OF_RANDOM_DEVICE="/dev/srandom"
	NAME_OF_PSEUDO_RANDOM_DEVICE="/dev/urandom"
	NAME_OF_NONCE_DEVICE="/dev/urandom"
        ;;

    *-solaris* | *-irix* | *-dec-osf* )
	NAME_OF_RANDOM_DEVICE="/dev/random"
	NAME_OF_PSEUDO_RANDOM_DEVICE="/dev/random"
	NAME_OF_NONCE_DEVICE="/dev/random"
        ;;

    *)
	NAME_OF_RANDOM_DEVICE="/dev/random"
	NAME_OF_PSEUDO_RANDOM_DEVICE="/dev/urandom"
	NAME_OF_NONCE_DEVICE="/dev/urandom"
        ;;
  esac

  AC_MSG_CHECKING([device with (strong) random data...])
  AC_ARG_ENABLE(random-device,
	AC_HELP_STRING([--enable-random-device],
		[device with (strong) randomness (for Nettle)]),
	NAME_OF_RANDOM_DEVICE=$enableval)
  AC_MSG_RESULT($NAME_OF_RANDOM_DEVICE)

  AC_MSG_CHECKING([device with pseudo random data...])
  AC_ARG_ENABLE(pseudo-random-device,
	AC_HELP_STRING([--enable-pseudo-random-device],
		[device with pseudo randomness (for Nettle)]),
	NAME_OF_PSEUDO_RANDOM_DEVICE=$enableval)
  AC_MSG_RESULT($NAME_OF_PSEUDO_RANDOM_DEVICE)

  AC_MSG_CHECKING([device with unpredictable data for nonces...])
  AC_ARG_ENABLE(nonce-device,
	AC_HELP_STRING([--enable-nonce-device],
		[device with unpredictable nonces (for Nettle)]),
	NAME_OF_NONCE_DEVICE=$enableval)
  AC_MSG_RESULT($NAME_OF_NONCE_DEVICE)

  if test "$cross_compiling" != yes; then
    AC_CHECK_FILE($NAME_OF_RANDOM_DEVICE,, AC_MSG_ERROR([[
      *** Device for (strong) random data $NAME_OF_RANDOM_DEVICE does not exist
    ]]))
    AC_CHECK_FILE($NAME_OF_PSEUDO_RANDOM_DEVICE,, AC_MSG_ERROR([[
      *** Device for pseudo-random data $NAME_OF_PSEUDO_RANDOM_DEVICE does not exist
    ]]))
    AC_CHECK_FILE($NAME_OF_NONCE_DEVICE,, AC_MSG_ERROR([[
      *** Device for unpredictable nonces $NAME_OF_NONCE_DEVICE does not exist
    ]]))
  else
    AC_MSG_NOTICE([[Cross compiling, assuming random devices exists...]])  
  fi

  # FIXME: Open+read 42 bytes+close twice and compare data.  Should differ.

  AC_DEFINE_UNQUOTED(NAME_OF_RANDOM_DEVICE, "$NAME_OF_RANDOM_DEVICE",
                   [defined to the name of the (strong) random device])
  AC_DEFINE_UNQUOTED(NAME_OF_PSEUDO_RANDOM_DEVICE,
			 "$NAME_OF_PSEUDO_RANDOM_DEVICE",
                   [defined to the name of the pseudo random device])
  AC_DEFINE_UNQUOTED(NAME_OF_NONCE_DEVICE, "$NAME_OF_NONCE_DEVICE",
                   [defined to the name of the unpredictable nonce device])
fi

# CRAM-MD5
AC_ARG_ENABLE(cram-md5,
  AS_HELP_STRING([--disable-cram-md5], [don't use the CRAM-MD5 mechanism]),
  cram_md5=$enableval)
if test "$cram_md5" != "no" ; then
	cram_md5=yes
	AC_DEFINE(USE_CRAM_MD5, 1, [Define to 1 if you want CRAM-MD5.])
fi
AC_MSG_CHECKING([if CRAM-MD5 should be used])
AC_MSG_RESULT($cram_md5)
AM_CONDITIONAL(CRAM_MD5, test x$cram_md5 = xyes)

# DIGEST-MD5
AC_ARG_ENABLE(digest-md5,
  AS_HELP_STRING([--disable-digest-md5], [don't use the DIGEST-MD5 mechanism]),
  digest_md5=$enableval)
if test "$digest_md5" != "no" ; then
	digest_md5=yes
	AC_DEFINE(USE_DIGEST_MD5, 1, [Define to 1 if you want DIGEST-MD5.])
fi
AC_MSG_CHECKING([if DIGEST-MD5 should be used])
AC_MSG_RESULT($digest_md5)
AM_CONDITIONAL(DIGEST_MD5, test x$digest_md5 = xyes)

# GSSAPI.
AC_ARG_ENABLE(gssapi,
  AS_HELP_STRING([--disable-gssapi], [don't use the GSSAPI mechanism]),
  gssapi=$enableval, gssapi=yes)
AC_MSG_NOTICE([checking for GSS implementation])
if test "$gssapi" = "gss"; then
	AC_MSG_NOTICE([trying GSS])
	sj_CHECK_HEADERLIB(gss.h, gss, gss_check_version,
		[gssapi=yes
		AC_DEFINE(USE_GSS, 1, [Define to 1 if you want GSS.])],
		[gssapi=no
		AC_MSG_WARN([Libgss not found.])])
elif test "$gssapi" = "mit"; then
	AC_MSG_NOTICE([trying MIT])
	AC_PATH_PROG(KRB5_CONFIG, krb5-config, no)
	if test "$KRB5_CONFIG" != "no" ; then
		CPPFLAGS="$CPPFLAGS `$KRB5_CONFIG --cflags`"
		LIBS="$LIBS `$KRB5_CONFIG --libs gssapi`"
		AC_CHECK_HEADERS(gssapi/gssapi.h gssapi/gssapi_generic.h)
		AC_CHECK_DECL(GSS_C_NT_HOSTBASED_SERVICE,,
			[AC_DEFINE(GSS_C_NT_HOSTBASED_SERVICE,
				gss_nt_service_name,
				[Work around buggy MIT library])],[
#ifdef HAVE_GSSAPI_GSSAPI_H
# include <gssapi/gssapi.h>
#endif
#ifdef HAVE_GSSAPI_GSSAPI_GENERIC_H
# include <gssapi/gssapi_generic.h>
#endif
])
		gssapi=yes
	else
		AC_MSG_WARN([krb5-config not found, disabling GSSAPI])
		gssapi=no
	fi
elif test "$gssapi" = "heimdal"; then
	AC_MSG_NOTICE([trying Heimdal])
	AC_PATH_PROG(KRB5_CONFIG, krb5-config, no)
	if test "$KRB5_CONFIG" != "no" ; then
		CPPFLAGS="$CPPFLAGS `$KRB5_CONFIG --cflags`"
		LIBS="$LIBS `$KRB5_CONFIG --libs gssapi`"
		AC_CHECK_HEADERS(gssapi.h)
		gssapi=yes
	else
		AC_MSG_WARN([krb5-config not found, disabling GSSAPI])
		gssapi=no
	fi
elif test "$gssapi" != "no" ; then
	AC_MSG_NOTICE([auto-detecing GSS/MIT/Heimdal])
	AC_MSG_NOTICE([use --enable-gssapi=IMPL to override])
	AC_MSG_NOTICE([where IMPL is `gss', `mit', or `heimdal'])
	sj_CHECK_HEADERLIB(gss.h, gss, gss_check_version, gss=yes, gss=no)
	if test "$gss" = "yes" ; then
		AC_MSG_NOTICE([GSS found])
		AC_DEFINE(USE_GSS, 1, [Define to 1 if you want GSS.])
		gssapi=yes
	else
		AC_MSG_WARN([GSS not found (see http://josefsson.org/gss/)...])
		AC_PATH_PROG(KRB5_CONFIG, krb5-config, no)
		if test "$KRB5_CONFIG" != "no" ; then
			AC_MSG_NOTICE([MIT/Heimdal found])
			CPPFLAGS="$CPPFLAGS `$KRB5_CONFIG --cflags`"
			LIBS="$LIBS `$KRB5_CONFIG --libs gssapi`"
			AC_CHECK_HEADERS(gssapi.h gssapi/gssapi.h gssapi/gssapi_generic.h)
			if test "$ac_cv_header_gssapi_h" = "yes" && \
			   test	"$ac_cv_header_gssapi_gssapi_h" = "yes"; then
				AC_MSG_WARN([Both Heimdal and MIT GSSAPI header files found.  Will use Heimdal.])
				AC_MSG_WARN([Use --enable-gssapi=mit to override.])
			fi
			if test "$ac_cv_header_gssapi_h" != "no"; then
				AC_MSG_NOTICE([Heimdal found])
			else
				AC_MSG_NOTICE([MIT found])
				AC_CHECK_DECL(GSS_C_NT_HOSTBASED_SERVICE,,
					[AC_DEFINE(GSS_C_NT_HOSTBASED_SERVICE,
					gss_nt_service_name,
					[Work around buggy MIT library])],[
#ifdef HAVE_GSSAPI_GSSAPI_H
# include <gssapi/gssapi.h>
#endif
#ifdef HAVE_GSSAPI_GSSAPI_GENERIC_H
# include <gssapi/gssapi_generic.h>
#endif
])
			fi
			gssapi=yes
		else
			AC_MSG_WARN([krb5-config not found, disabling GSSAPI])
			gssapi=no
		fi
	fi
fi
if test "$gssapi" = "yes"; then
	AC_DEFINE(USE_GSSAPI, 1, [Define to 1 if you want GSSAPI.])
fi
AC_MSG_CHECKING([if GSSAPI should be used])
AC_MSG_RESULT($gssapi)
AM_CONDITIONAL(GSSAPI, test x$gssapi = xyes)

# KERBEROS_V5
AC_ARG_ENABLE(kerberos_v5,
	AS_HELP_STRING([--disable-kerberos_v5],
                       [don't use the KERBEROS_V5 mechanism]),
	kerberos_v5=$enableval, kerberos_v5=no)
if test "$kerberos_v5" != "no" ; then
	sj_CHECK_HEADERLIB(shishi.h, shishi, shishi_check_version,
		[kerberos_v5=yes
		AC_DEFINE(USE_KERBEROS_V5, 1,
                          [Define to 1 if you want KERBEROS_V5.])],
		[kerberos_v5=no
		AC_MSG_WARN([shishi not found, disabling KERBEROS_V5])])
fi
AC_MSG_CHECKING([if KERBEROS_V5 should be used])
AC_MSG_RESULT($kerberos_v5)
AM_CONDITIONAL(KERBEROS_V5, test x$kerberos_v5 = xyes)

# Check for libidn
AC_ARG_WITH(stringprep,
  AS_HELP_STRING([--without-stringprep], [disable non-ASCII support]),
	stringprep=$withval, stringprep=yes)
if test "$stringprep" != "no"; then
  sj_CHECK_HEADERLIB(stringprep.h, idn, stringprep_check_version, [
    stringprep=yes
    AC_CHECK_FUNCS(pr29_8z)
    AC_CHECK_HEADERS(pr29.h)
    AC_DEFINE(WITH_STRINGPREP, 1,
	[Define to 1 if you want to use string preparation from Libidn.])
  ], [
    stringprep=no
    AC_MSG_WARN([Libidn not found.  Stringprep disabled.])
  ])
fi
AC_MSG_CHECKING([if non-ASCII support via Libidn should be built])
AC_MSG_RESULT($stringprep)

# Allow disabling of client or server.
AC_ARG_ENABLE(client,
              AS_HELP_STRING([--disable-client], [disable client code]),
	      client=$enableval)
AC_ARG_ENABLE(server,
              AS_HELP_STRING([--disable-server], [disable server code]),
	      server=$enableval)
if test "$client" != "no"; then
  AC_DEFINE(USE_CLIENT, 1, [Define to 1 if you want client code.])
  client=yes
fi
if test "$server" != "no"; then
  AC_DEFINE(USE_SERVER, 1, [Define to 1 if you want server code.])
  server=yes
fi
if test "$server" = "no" && test "$client" = "no"; then
  AC_MSG_ERROR([Disabling both client and server mode is meaningless])
fi
AC_MSG_CHECKING([if client code should be built])
AC_MSG_RESULT($client)
AC_MSG_CHECKING([if server code should be built])
AC_MSG_RESULT($server)
AM_CONDITIONAL(CLIENT, test x$client = xyes)
AM_CONDITIONAL(SERVER, test x$server = xyes)

# Allow disabling of obsolete stuff.
AC_ARG_ENABLE(obsolete,
  AS_HELP_STRING([--disable-obsolete], [disable backwards compatibility code]),
    obsolete=$enableval)
if test "$obsolete" = "no"; then
  AC_DEFINE(GSASL_NO_OBSOLETE, 1,
    [Define to 1 if you don't want backwards compatibility code.])
else
  obsolete=yes
fi
AC_MSG_CHECKING([if backwards compatibility code should be present])
AC_MSG_RESULT($obsolete)
AM_CONDITIONAL(OBSOLETE, test "$obsolete" = "yes")

# For gnulib stuff in gl/.
gl_SOURCE_BASE(gl)
gl_M4_BASE(gl/m4)
gl_LIB(libgl)
gl_MODULES(strdup gettext base64 vasprintf getline strverscmp)
gl_LGPL
gl_INIT

# We need uint8_t etc.
AX_CREATE_STDINT_H(crypto/nettle-types.h, sys/types.h)

AC_CONFIG_FILES(Makefile libgsasl.pc po/Makefile.in \
		gl/Makefile crypto/Makefile \
		src/gsasl.h src/Makefile \
		anonymous/Makefile cram-md5/Makefile \
		digest-md5/Makefile external/Makefile \
		gssapi/Makefile kerberos_v5/Makefile \
		login/Makefile ntlm/Makefile plain/Makefile \
		securid/Makefile)
AC_OUTPUT
