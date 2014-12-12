## Process this file with automake to produce Makefile.in
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

ACLOCAL_AMFLAGS = -I m4 -I gl/m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgsasl.pc

SUBDIRS = po gl crypto

# Plugins:
if EXTERNAL
SUBDIRS += external
endif

if ANONYMOUS
SUBDIRS += anonymous
endif

if PLAIN
SUBDIRS += plain
endif

if LOGIN
SUBDIRS += login
endif

if CRAM_MD5
SUBDIRS += cram-md5
endif

if DIGEST_MD5
SUBDIRS += digest-md5
endif

if NTLM
SUBDIRS += ntlm
endif

if GSSAPI
SUBDIRS += gssapi
endif			

if SECURID
SUBDIRS += securid
endif			

if KERBEROS_V5
SUBDIRS += kerberos_v5
endif

SUBDIRS += src

# Maintainer targets

POURL = ftp://ftp.unex.es/pub/gnu-i18n/po/maint/$(PACKAGE)
update-po:
	cd po && \
	wget --non-verbose --timestamping --retr-symlinks $(POURL)/*.po && \
	cvs add `ls *.po | grep -v quot.po` || true && \
	ls *.po | sed 's/.po//' > LINGUAS && \
	cvs commit -m "Sync with TP." LINGUAS `ls *.po | grep -v quot.po`

ChangeLog:
	cvs2cl --fsf --FSF --usermap ../.cvsusers -I ChangeLog -I .cvs \
		-I autogen.sh
	cat ../.cvscopying >> ChangeLog

htmldir = ../../www-gsasl

release:
	cvs upd -p -r HEAD NEWS > /dev/null
	rm -f ChangeLog && cvs upd ChangeLog
	cvs commit
	rm -f ChangeLog
	$(MAKE) ChangeLog distcheck
	cvs commit -m "" ChangeLog
	gpg -b $(distdir).tar.gz
	gpg --verify $(distdir).tar.gz.sig
	echo "directory: gsasl" > $(distdir).tar.gz.directive
	gpg --clearsign $(distdir).tar.gz.directive
	ncftpput ftp-upload.gnu.org /incoming/alpha \
		$(distdir).tar.gz{,.sig,.directive.asc}
	cp -v $(distdir).tar.gz{,.sig} $(htmldir)/releases/
	cd $(htmldir) && cvs add -kb releases/$(distdir).tar.gz{,.sig} && \
		cvs commit -m "Update." releases/
