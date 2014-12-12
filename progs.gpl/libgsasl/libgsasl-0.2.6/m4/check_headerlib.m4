# check_headerlib.m4 serial 2
dnl Copyright (C) 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl sj_CHECK_HEADERLIB(HEADER-FILE, LIBRARY, FUNCTION,
dnl                    [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
dnl                    [OTHER-LIBRARIES])
AC_DEFUN([sj_CHECK_HEADERLIB], [
	AC_CHECK_HEADER([$1], h=yes, l=no)
	AC_CHECK_LIB([$2], [$3], l=yes, l=no, [$6])
	if test "$h" = yes -a "$l" = yes; then
		LIBS="$LIBS -l$2"
		ifelse([$4], , :, [$4])
	else
		ifelse([$5], , :, [$5])
	fi])