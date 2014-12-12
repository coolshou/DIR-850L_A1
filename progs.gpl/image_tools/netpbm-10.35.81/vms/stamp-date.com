$!# Copyright (C) 1993 by Oliver Trepte.
$!#
$!# Permission to use, copy, modify, and distribute this software and its
$!# documentation for any purpose and without fee is hereby granted, provided
$!# that the above copyright notice appear in all copies and that both that
$!# copyright notice and this permission notice appear in supporting
$!# documentation.  This software is provided "as is" without express or
$!# implied warranty.
$!#
$!# This shell script creates a file called "compile_time.h" which holds
$!# a define stating the compilation time. This is used for the -version
$!# flag.
$!#
$!# modified to a VMS command procedure by Rick Dyson 29-NOV-1993

$ OUTFILE   := "compile.h"
$ DATE      := "''F$Time ()'"
$ USER      := "''F$GetJPI (F$GetJPI ("", "PID"), "USERNAME")'"
$ Open /Write OUTFILE compile.h
$ Write OUTFILE "/* compile_time.h  This file tells the package when it was compiled */"
$ Write OUTFILE "/* DO NOT EDIT - THIS FILE IS MAINTAINED AUTOMATICALLY              */"
$ Write OUTFILE "#define COMPILE_TIME ""''DATE'"""
$ Write OUTFILE "#define COMPILED_BY ""''USER'"""
$ Close OUTFILE
$ Exit
