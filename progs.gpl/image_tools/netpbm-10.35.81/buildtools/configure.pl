#!/usr/bin/perl -w

require 5.000;

use strict;
use English;
use File::Basename;
use Cwd 'abs_path';
use Fcntl;
use Config;
#use File::Temp "tempfile";   Not available before Perl 5.6.1
    

my ($TRUE, $FALSE) = (1,0);

# This program generates Makefile.config, which is included by all of the
# Netpbm makefiles.  You run this program as the first step in building 
# Netpbm.  (The second step is 'make').

# This program is only a convenience.  It is supported to create 
# Makefile.config any way you want.  In fact, an easy way is to copy
# Makefile.config.in and follow the instructions in the comments therein
# to uncomment certain lines and make other changes.

# Note that if you invoke 'make' without having first run 'configure',
# the make will call 'configure' itself when it finds
# 'Makefile.config' missing.  That might look a little messy to the
# user, but it isn't the normal build process.

# The argument to this program is the filepath of the Makefile.config.in
# file.  If unspecified, the default is 'Makefile.config.in' in the 
# Netpbm source directory.

# For explanations of the stuff we put in the make files, see the comments
# in Makefile.config.in.


# $testCc is the command we use to do test compiles.  Note that test
# compiles are never more than heuristics, because we may be configuring
# a build that will happen on a whole different system, which will build
# programs to run on a third system.

my $testCc;

#******************************************************************************
#
#  SUBROUTINES
#
#*****************************************************************************

sub autoFlushStdout() {
    my $oldFh = select(STDOUT);
    $OUTPUT_AUTOFLUSH = $TRUE;
    select($oldFh);
}



sub prompt($$) {

    my ($prompt, $default) = @_;

    my $defaultPrompt = defined($default) ? $default : "?";

    print("$prompt [$defaultPrompt] ==> ");

    my $response = <STDIN>;

    if (defined($response)) {
        chomp($response);
        if ($response eq "" && defined($default)) {
            $response = $default;
        }
    } else {
        print("\n");
        die("End of file on Standard Input when expecting response to prompt");
    }

    return $response;
}



sub tmpdir() {
# This is our approximation of File::Spec->tmpdir(), which became part of
# basic Perl some time after Perl 5.005_03.

    my $retval;
    
    if ($ENV{"TMPDIR"}) {
        $retval = $ENV{"TMPDIR"};
    } else {
        if ($Config{'osvers'} eq "djgpp") {
            $retval = "/dev/env/DJDIR/tmp";
        } else {
            $retval =  "/tmp";
        }
    }
    return $retval;
}



sub tempFile($) {

# Here's what we'd do if we could expect Perl 5.6.1 or later, instead
# of calling this subroutine:
#    my ($cFile, $cFileName) = tempfile("netpbmXXXX", 
#                                       SUFFIX=>".c", 
#                                       DIR=>File::Spec->tmpdir(),
#                                       UNLINK=>0);
    my ($suffix) = @_;
    my $fileName;
    local *file;  # For some inexplicable reason, must be local, not my
    my $i;
    $i = 0;
    do {
        $fileName = tmpdir() . "/netpbm" . $i++ . $suffix;
    } until sysopen(*file, $fileName, O_RDWR|O_CREAT|O_EXCL);

    return(*file, $fileName);
}



sub commandExists($) {
    my ($command) = @_;
#-----------------------------------------------------------------------------
#  Return TRUE iff a shell command $command exists.
#-----------------------------------------------------------------------------

# Note that it's significant that the redirection on the following
# causes it to be executed in a shell.  That makes the return code
# from system() a lot different than if system() were to try to
# execute the program directly.

    return(system("$command 1</dev/null 1>/dev/null 2>/dev/null")/256 != 127); 
}



sub chooseTestCompiler($$) {

    my ($compiler, $testCcR) = @_;

    my $cc;

    if ($ENV{'CC'}) {
        $cc = $ENV{'CC'};
    } else {
        if (commandExists('cc')) {
            $cc = 'cc';
        } elsif (commandExists("gcc")) {
            $cc = 'gcc';
        }
    }
    $$testCcR = $cc;
}



sub testCflags($) {
    my ($needLocal) = @_;

    my $cflags;

    $cflags = "";  # initial value 
    
    if ($ENV{"CPPFLAGS"}) {
        $cflags = $ENV{"CPPFLAGS"};
    } else {
        $cflags = "";
    }
    
    if ($ENV{"CFLAGS"}) {
        $cflags .= " " . $ENV{"CFLAGS"};
    }
    
    if ($needLocal) {
        $cflags .= " -I/usr/local/include";
    }
    return $cflags;
}    



sub testCompile($$$) {
    my ($cflags, $cSourceCodeR, $successR) = @_;
#-----------------------------------------------------------------------------
#  Do a test compile of the program in @{$cSourceCodeR}.
#  
#  Return $$successR == $TRUE iff the compile succeeds (exit code 0).
#-----------------------------------------------------------------------------
    my ($cFile, $cFileName) = tempFile(".c");

    print $cFile @{$cSourceCodeR};
    
    my ($oFile, $oFileName) = tempFile(".o");
    # Note: we tried using /dev/null for the output file and got complaints
    # from the Sun compiler that it has the wrong suffix.  2002.08.09.
    
    my $compileCommand = "$testCc -c -o $oFileName $cflags $cFileName";
    print ("Doing test compile: $compileCommand\n");
    my $rc = system($compileCommand);
    
    unlink($oFileName);
    close($oFile);
    unlink($cFileName);
    close($cFile);

    $$successR = ($rc == 0);
}



sub displayIntroduction() {
    print("This is the Netpbm configurator.  It is an interactive dialog " .
          "that\n");
    print("helps you build the file 'Makefile.config' and prepare to build ");
    print("Netpbm.\n");
    print("\n");

    print("Do not be put off by all the questions.  Configure gives you " .
          "the \n");
    print("opportunity to make a lot of choices, but you don't have to.  " .
          "If \n");
    print("you don't have reason to believe you're smarter than Configure,\n");
    print("just take the defaults (hit ENTER) and don't sweat it.\n");
    print("\n");

    print("If you are considering having a program feed answers to the " .
          "questions\n");
    print("below, please read doc/INSTALL, because that's probably the " .
          "wrong thing to do.\n");
    print("\n");

    print("Hit ENTER to begin.\n");
    my $response = <STDIN>;
}


sub askAboutCygwin() {

    print("Are you building in/for the Cygwin environment?\n");
    print("\n");
    
    my $default;
    if ($OSNAME eq "cygwin") {
        $default = "y";
    } else {
        $default = "n";
    }
    
    my $retval;

    while (!defined($retval)) {
        my $response = prompt("(y)es or (n)o", $default);
    
        if (uc($response) =~ /^(Y|YES)$/)  {
            $retval = $TRUE;
        } elsif (uc($response) =~ /^(N|NO)$/)  {
            $retval = $FALSE;
        } else {
            print("'$response' isn't one of the choices.  \n" .
                  "You must choose 'yes' or 'no' (or 'y' or 'n').\n");
        }
    }
    return $retval;
}



sub askAboutDjgpp() {

    print("Are you building in/for the DJGPP environment?\n");
    print("\n");
    
    my $default;
    if ($OSNAME eq "dos") {
        $default = "y";
    } else {
        $default = "n";
    }
    
    my $retval;

    while (!defined($retval)) {
        my $response = prompt("(y)es or (n)o", $default);
    
        if (uc($response) =~ /^(Y|YES)$/)  {
            $retval = $TRUE;
        } elsif (uc($response) =~ /^(N|NO)$/)  {
            $retval = $FALSE;
        } else {
            print("'$response' isn't one of the choices.  \n" .
                  "You must choose 'yes' or 'no' (or 'y' or 'n').\n");
        }
    }
}



sub computePlatformDefault($) {

    my ($defaultP) = @_;

    if ($OSNAME eq "linux") {
        $$defaultP = "gnu";
    } elsif ($OSNAME eq "cygwin") {
        $$defaultP = "win";
    } elsif ($OSNAME eq "dos") {
        # DJGPP says "dos"
        $$defaultP = "win";
    } elsif ($OSNAME eq "aix" || $OSNAME eq "freebsd" || $OSNAME eq "darwin" ||
             $OSNAME eq "amigaos") {
        $$defaultP = $OSNAME;
    } elsif ($OSNAME eq "solaris") {
        $$defaultP = "sun";
    } elsif ($OSNAME eq "dec_osf") {
        $$defaultP = "tru64";
    } else {
        print("Unrecognized OSNAME='$OSNAME'.  No default possible\n");
    }
    # OK - if you know what $OSNAME is on any other platform, send me a patch!
}



sub getPlatform() {

    my $platform;
    my $default;

    computePlatformDefault(\$default);

    print("Which of the following best describes your platform?\n");
 
    print("gnu      GNU/Linux\n");
    print("sun      Solaris or SunOS\n");
    print("hp       HP-UX\n");
    print("aix      AIX\n");
    print("win      Windows/DOS (Cygwin, DJGPP, Mingw32)\n");
    print("tru64    Tru64\n");
    print("irix     Irix\n");
    print("bsd      NetBSD, BSD/OS\n");
    print("openbsd  OpenBSD\n");
    print("freebsd  FreeBSD\n");
    print("darwin   Darwin or Mac OS X\n");
    print("amigaos  Amiga\n");
    print("unixware Unixware\n");
    print("sco      SCO OpenServer\n");
    print("beos     BeOS\n");
    print("none     none of these are even close\n");
    print("\n");

    my $response = prompt("Platform", $default);

    my %platform = ("gnu"      => "GNU",
                    "sun"      => "SOLARIS",
                    "hp"       => "HP-UX",
                    "aix"      => "AIX",
                    "tru64"    => "TRU64",
                    "irix"     => "IRIX",
                    "win"      => "WINDOWS",
                    "beos"     => "BEOS",
                    "bsd"      => "NETBSD",
                    "openbsd"  => "OPENBSD",
                    "freebsd"  => "FREEBSD",
                    "unixware" => "UNIXWARE",
                    "sco"      => "SCO",
                    "darwin"   => "DARWIN",
                    "amigaos"  => "AMIGA",
                    "none"     => "NONE"
                    );

    $platform = $platform{$response};
    if (!defined($platform)) {
        print("'$response' isn't one of the choices.\n");
        exit 8;
    }

    my $subplatform;

    if ($platform eq "WINDOWS") {
        my ($djgpp, $cygwin);

        if ($OSNAME eq "dos") {
            $djgpp = askAboutDjgpp();
            if ($djgpp) {
                $cygwin = $FALSE;
            } else {
                $cygwin = askAboutCygwin();
            }
        } else {
            $cygwin = askAboutCygwin();
            if ($cygwin) {
                $djgpp = $FALSE;
            } else {
                $djgpp = askAboutDjgpp();
            }
        }

        if ($cygwin) {
            $subplatform = "cygwin";
        } elsif ($djgpp) {
            $subplatform = "djgpp";
        } else {
            $subplatform = "other";
        }
    }

    return($platform, $subplatform);
}



sub getCompiler($$) {
    my ($platform, $compilerR) = @_;
#-----------------------------------------------------------------------------
#  Here are some of the issues surrounding choosing a compiler:
#
#  - It's not just the name of the program we need -- different compilers
#    need different options.
#
#  - There are basically two choices on any system:  native compiler or
#    GNU compiler.  That's all this program recognizes, anyway.  On some,
#    native _is_ GNU, and we return 'gcc'.
#
#  - A user may well have various compilers.  Different releases, using
#    different standard libraries, for different target machines, etc.
#
#  - The CC environment variable tells the default compiler.
#
#  - In the absence of a CC environment variable, 'cc' is the default
#    compiler.
#
#  - The user must be able to specify the compiler by overriding the CC
#    make variable (e.g. make CC=gcc2).
#
#  - Configure needs to do test compiles.  The test is best if it uses
#    the same compiler that the build eventually will use, but it's 
#    useful even if not.
#
# The value this subroutine returns is NOT the command name to invoke the
# compiler.  It is simply "cc" to mean native compiler or "gcc" to mean
# GNU compiler.
#-----------------------------------------------------------------------------
    my %gccOptionalPlatform = ("SOLARIS" => 1,
                               "TRU64"   => 1,
                               "SCO"     => 1,
                               "AIX"     => 1,
                               "HP-UX"   => 1);

    my %gccUsualPlatform = ("GNU"     => 1,
                            "NETBSD"  => 1,
                            "OPENBSD" => 1,
                            "FREEBSD" => 1,
                            "DARWIN"  => 1,
                            );

    if ($gccUsualPlatform{$platform}) {
        $$compilerR = "gcc";
    } elsif ($gccOptionalPlatform{$platform}) {
        print("GNU compiler or native operating system compiler (cc)?\n");
        print("\n");

        my $default;

        if ($platform eq "SOLARIS" || $platform eq "SCO" ) {
            $default = "gcc";
        } else {
            $default = "cc";
        }

        my $response = prompt("gcc or cc", $default);

        if ($response eq "gcc") {
            $$compilerR = "gcc";
        } elsif ($response eq "cc") {
            $$compilerR = "cc";
        } else {
            print("'$response' isn't one of the choices.  \n" .
                  "You must choose 'gcc' or 'cc'.\n");
            exit 12;
        }

        if ($$compilerR eq 'gcc' && !commandExists('gcc')) {
            print("WARNING: You selected the GNU compiler, " .
                  "but do not appear to have a program " .
                  "named 'gcc' in your PATH.  This may " .
                  "cause trouble later.  You may need to " .
                  "set the CC environment variable or CC " .
                  "makefile variable or install 'gcc'\n");
        }
        print("\n");
    } else {
        $$compilerR = 'cc';
    }
}



sub gccLinker() {
#-----------------------------------------------------------------------------
#  Determine what linker Gcc on this system appears to use.
#
#  Return either "gnu" or "sun"
#
#  For now, we assume it must be either a GNU linker or Sun linker and
#  that all Sun linkers are fungible.
#
#  If we can't tell, we assume it is the GNU linker.
#-----------------------------------------------------------------------------
    # First, we assume that the compiler calls 'collect2' as the linker
    # front end.  The specs file might specify some other program, but
    # it usually says 'collect2'.
    
    my $retval;

    my $collect2 = qx{gcc --print-prog-name=collect2};
    
    if (defined($collect2)) {
        chomp($collect2);
        my $linker = qx{$collect2 -v 2>&1};
        if (defined($linker) && $linker =~ m{GNU ld}) {
            $retval = "gnu";
        } else {
            $retval = "sun";
        }
    } else {
        $retval = "gnu";
    }
    return $retval;
}



sub getLinker($$$$) {

    my ($platform, $compiler, $baseLinkerR, $viaCompilerR) = @_;

    my $baseLinker;

    if ($platform eq "SOLARIS") {
        $$viaCompilerR = $TRUE;

        while (!defined($$baseLinkerR)) {
            print("GNU linker or SUN linker?\n");
            print("\n");

            my $default;
            
            if ($compiler eq "gcc") {
                $default = gccLinker();
            } else {
                $default = "sun";
            }
            my $response = prompt("sun or gnu", $default);
            
            if ($response eq "gnu") {
                $$baseLinkerR = "GNU";
            } elsif ($response eq "sun") {
                $$baseLinkerR = "SUN";
            } else {
                print("'$response' isn't one of the choices.  \n" .
                      "You must choose 'sun' or 'gnu'.\n");
            }
            print("\n");
        }
    } else {
        $$viaCompilerR = $TRUE;
        $$baseLinkerR = "?";
    }
}



sub libSuffix($) {
    my ($platform) = @_;
#-----------------------------------------------------------------------------
#  Return the traditional suffix for the link-time library on platform
#  type $platform.
#
#  Note that this information is used mainly for cosmetic purposes in this
#  this program and the Netpbm build, because the build typically removes
#  any suffix and uses link options such as "-ltiff" to link the library.
#  This leaves it up to the linker to supply the actual suffix.
#-----------------------------------------------------------------------------
    my $suffix;

    if ($platform eq 'WINDOWS') {
        $suffix = '.dll';
    } elsif ($platform eq 'AIX') {
        $suffix = '.a';
    } elsif ($platform eq 'DARWIN') {
        $suffix = '.dylib';
    } else {
        $suffix = '.so';
    }
}



sub getLibTypes($$$$$$$$) {
    my ($platform, $subplatform, $default_target,
        $netpbmlibtypeR, $netpbmlibsuffixR, $shlibprefixlistR,
        $willBuildSharedR, $staticlib_tooR) = @_;

    print("Do you want libnetpbm to be statically linked or shared?\n");
    print("\n");

    my $default = ($default_target eq "merge") ? "static" : "shared";

    my ($netpbmlibtype, $netpbmlibsuffix, $shlibprefixlist, 
        $willBuildShared, $staticlib_too);
    
    my $response = prompt("static or shared", $default);
    
    if ($response eq "shared") {
        $willBuildShared = $TRUE;
        if ($platform eq "WINDOWS") {
            $netpbmlibtype = "dll";
            $netpbmlibsuffix = "dll";
            if ($subplatform eq "cygwin") {
                $shlibprefixlist = "cyg lib";
            } 
        } elsif ($platform eq "DARWIN") {
            $netpbmlibtype = "dylib";
            $netpbmlibsuffix = "dylib";
        } else {
	    if ($platform eq "IRIX") {
		$netpbmlibtype = "irixshared";
	    } else {
		$netpbmlibtype = "unixshared";
	    }
            if ($platform eq "AIX") {
                $netpbmlibsuffix = "a";
            } elsif ($platform eq "HP-UX") {
                $netpbmlibsuffix = "sl";
            } else {
                $netpbmlibsuffix = "so";
            }
        }
    } elsif ($response eq "static") {
        $willBuildShared = $FALSE;
        $netpbmlibtype = "unixstatic";
        $netpbmlibsuffix = "a";
        # targets, but needed for building
        # libopt 
    } else {
        print("'$response' isn't one of the choices.  \n" .
              "You must choose 'static' or 'shared'.\n");
        exit 12;
    }

    print("\n");

    # Note that we can't do both a static and shared library for AIX, because
    # they both have the same name: libnetpbm.a.
    
    if (($netpbmlibtype eq "unixshared" or 
         $netpbmlibtype eq "irixshared" or 
         $netpbmlibtype eq "dll") and $netpbmlibsuffix ne "a") {
        print("Do you want to build static libraries too (for linking to \n");
        print("programs not in the Netpbm package?\n");
        print("\n");
        
        my $default = "y";
        
        my $response = prompt("(y)es or (n)o", $default);
        
        if (uc($response) =~ /^(Y|YES)$/)  {
            $staticlib_too = "y";
        } elsif (uc($response) =~ /^(N|NO)$/)  {
            $staticlib_too = "n";
        } else {
            print("'$response' isn't one of the choices.  \n" .
              "You must choose 'yes' or 'no' (or 'y' or 'n').\n");
            exit 12;
        }
    } else {
        $staticlib_too = "n";
    }
    print("\n");

    $$netpbmlibtypeR   = $netpbmlibtype;
    $$netpbmlibsuffixR = $netpbmlibsuffix;
    $$shlibprefixlistR = $shlibprefixlist;
    $$willBuildSharedR = $willBuildShared;
    $$staticlib_tooR   = $staticlib_too;
}



sub inttypesDefault() {

    my $retval;

    if (defined($testCc)) {

        print("(Doing test compiles to choose a default for you -- " .
              "ignore errors)\n");

        my $cflags = testCflags($FALSE);

        my $works;

        # We saw a system (Irix 5.3 with native IDO, December 2005) on
        # which sys/types.h defines uint32_t, but not int32_t and other
        # similar types.  We saw a Mac OS X system (January 2006) on which
        # sys/types sometimes defines uint32_t, but sometimes doesn't.
        # So we make those last resorts.

        # int_fastXXX_t are the least likely of all the types to be
        # defined, so we look for that.

        # Solaris 8, for example, has a <sys/inttypes.h> that defines
        # int32_t and uint32_t, but nothing the defines int_fast32_t.

        my @candidateList = ("<inttypes.h>", "<sys/inttypes.h>",
                             "<types.h>", "<sys/types.h>");
        
        for (my $i = 0; $i < @candidateList && !$works; ++$i) {
            my $candidate = $candidateList[$i];
            my @cSourceCode = (
                               "#include $candidate\n",
                               "int_fast32_t testvar;\n"
                               );
            
            testCompile($cflags, \@cSourceCode, \my $success);
            
            if ($success) {
                $works = $candidate;
            }
        }
        if ($works) {
            $retval = $works;
        } else {
            testCompile($cflags, ["int_fast32_t testvar;"], \my $success);
            if ($success) {
                $retval = "NONE";
            } else {
                $retval = '"inttypes_netpbm.h"';
            }
        }
        print("\n");
    } else {
        $retval = '<inttypes.h>';
    }
    return $retval;
}



sub getInttypes($) {
    my ($inttypesHeaderFileR) = @_;

    my $gotit;

    print("What header file defines uint32_t, etc.?\n");
    print("\n");

    my $default = inttypesDefault();
    
    while (!$gotit) {
        my $response = prompt("'#include' argument or NONE", $default);

        if ($response eq "NONE") {
            $$inttypesHeaderFileR = '';
            $gotit = $TRUE;
        } else {
            if ($response !~ m{<.+>} &&
                $response !~ m{".+"}) {
                print("'$response' is not a legal argument of a C #include " .
                      "statement.  It must be something in <> or \"\".\n");
            } else {
                $gotit = $TRUE;
                $$inttypesHeaderFileR = $response;
            }
        }
    }
}


sub getInt64($$) {

    my ($inttypes_h, $haveInt64R) = @_;

    if (defined($testCc)) {

        print("(Doing test compiles to determine if you have int64 type -- " .
              "ignore errors)\n");

        my $cflags = testCflags($FALSE);

        my $works;

        my @cSourceCode = (
                           "#include $inttypes_h\n",
                           "int64_t testvar;\n"
                           );
            
        testCompile($cflags, \@cSourceCode, \my $success);
            
        if ($success) {
            print("You do.\n");
            $$haveInt64R = 'Y';
        } else {
            print("You do not.  64-bit code won't be built.\n");
            $$haveInt64R = 'N';
        }
        print("\n");
    } else {
        $$haveInt64R = "N";
    }
}



# TODO: These should do test compiles to see if the headers are in the
# default search path, both to create a default to offer and to issue a
# warning after user has chosen.  Also test links to test the link library.

# It looks like these should all be in the default search paths and were there
# just to override defaults in Makefile.config.in.  Since Configure now
# creates a default of "standard search path," I'm guessing we don't need
# to set these anymore.

sub getTiffLibrary($@) {

    my ($platform, @suggestedHdrDir) = @_;

    my $tifflib;
    {
        my $default = "libtiff" . libSuffix($platform);

        print("What is your TIFF (graphics format) library?\n");
        
        my $response = prompt("library filename or 'none'", $default);
        
        if ($response ne "none") {
            $tifflib = $response;
        }
        if (defined($tifflib) and $tifflib =~ m{/} and !-f($tifflib)) {
            printf("WARNING: No regular file named '$tifflib' exists.\n");
        }
    }
    my $tiffhdr_dir;
    if (defined($tifflib)) {
        my $default;

        if (-d("/usr/include/tiff")) {
            $default = "/usr/include/tiff";
        } elsif (-d("/usr/include/libtiff")) {
            $default = "/usr/include/libtiff";
        } else {
            $default = "default";
        }
        print("Where are the interface headers for it?\n");
        
        my $response = prompt("TIFF header directory", $default);
        
        if ($response ne "default") {
            $tiffhdr_dir = $response;
        }
        if (defined($tiffhdr_dir) and !-d($tiffhdr_dir)) {
            printf("WARNING: No directory named '$tiffhdr_dir' exists.\n");
        }
    }
    return($tifflib, $tiffhdr_dir);
}



sub getJpegLibrary($@) {

    my ($platform, @suggestedHdrDir) = @_;

    my $jpeglib;
    {
        my $default = "libjpeg" . libSuffix($platform);

        print("What is your JPEG (graphics format) library?\n");
        
        my $response = prompt("library filename or 'none'", $default);
        
        if ($response ne "none") {
            $jpeglib = $response;
        }
    }
    my $jpeghdr_dir;
    if (defined($jpeglib)) {
        my $default;

        if (-d("/usr/include/jpeg")) {
            $default = "/usr/include/jpeg";
        } else {
            $default = "default";
        }
        print("Where are the interface headers for it?\n");
        
        my $response = prompt("JPEG header directory", $default);
        
        if ($response ne "default") {
            $jpeghdr_dir = $response;
        }
        if (defined($jpeghdr_dir) and !-d($jpeghdr_dir)) {
            printf("WARNING: No directory named '$jpeghdr_dir' exists.\n");
        }
    }
    return($jpeglib, $jpeghdr_dir);
}



sub getPngLibrary($@) {

    my ($platform, @suggestedHdrDir) = @_;

    my ($pnglib, $pnghdr_dir);

    if (commandExists('libpng-config')) {
        # We don't need to ask where Libpng is; there's a 'libpng-config'
        # That tells exactly how to access it, and the make files will use
        # that.
    } else {
        {
            my $default = "libpng" . libSuffix($platform);

            print("What is your PNG (graphics format) library?\n");
            
            my $response = prompt("library filename or 'none'", $default);
            
            if ($response ne "none") {
                $pnglib = $response;
            }
        }
        if (defined($pnglib)) {
            my $default;

            if (-d("/usr/include/png")) {
                $default = "/usr/include/libpng";
            } else {
                $default = "default";
            }
            
            print("Where are the interface headers for it?\n");
            
            my $response = prompt("PNG header directory", $default);

            if ($response ne "default") {
                $pnghdr_dir = $response;
            }
        }
    }
    return($pnglib, $pnghdr_dir);
}



sub getZLibrary($@) {

    my ($platform, @suggestedHdrDir) = @_;

    my ($zlib, $zhdr_dir);

    {
        my $default = "libz" . libSuffix($platform);

        print("What is your Z (compression) library?\n");
        
        my $response = prompt("library filename or 'none'", $default);
        
        if ($response ne "none") {
            $zlib = $response;
        }
    }
    if (defined($zlib)) {
        my $default;

        if (-d("/usr/include/zlib")) {
            $default = "/usr/include/zlib";
        } else {
            $default = "default";
        }
        
        print("Where are the interface headers for it?\n");
        
        my $response = prompt("Z header directory", $default);
        
        if ($response ne "default") {
            $zhdr_dir = $response;
        }
    }
    return($zlib, $zhdr_dir);
}



sub getX11Library($@) {

    my ($platform, @suggestedHdrDir) = @_;

    my $x11lib;
    {
        my $default;

        if (-d('/usr/link/X11')) {
            $default = '/usr/link/X11/libX11' . libSuffix($platform);
        } elsif (-d('/usr/X11R6/lib')) {
            $default = '/usr/X11R6/lib/libX11' . libSuffix($platform);
        } else {
            $default = "libX11" . libSuffix($platform);
        }
        print("What is your X11 (X client) library?\n");
        
        my $response = prompt("library filename or 'none'", $default);
        
        if ($response ne "none") {
            $x11lib = $response;
        }
    }
    my $x11hdr_dir;
    if (defined($x11lib)) {
        my $default;

        $default = "default";

        print("Where are the interface headers for it?\n");
        
        my $response = prompt("X11 header directory", $default);
        
        if ($response ne "default") {
            $x11hdr_dir = $response;
        }
        if (defined($x11hdr_dir)) {
            if (!-d($x11hdr_dir)) {
                printf("WARNING: No directory named '$x11hdr_dir' exists.\n");
            } elsif (!-d("$x11hdr_dir/X11")) {
                printf("WARNING: Directory '$x11hdr_dir' does not contain " .
                       "the requisite 'X11' subdirectory\n");
            }
        }
    }
    return($x11lib, $x11hdr_dir);
}



sub getLinuxsvgaLibrary($@) {

    my ($platform, @suggestedHdrDir) = @_;

    my ($svgalib, $svgalibhdr_dir);

    if ($platform eq "GNU") {
        my $default;

        if (-d('/usr/link/svgalib')) {
            $default = '/usr/link/svgalib/libvga.so';
        } elsif (-d('/usr/lib/svgalib')) {
            $default = '/usr/lib/svgalib/libvga.so';
        } elsif (system('ldconfig -p | grep libvga &>/dev/null') == 0) {
            $default = 'libvga.so';
        } else {
            $default = 'none';
        }
            
        print("What is your Svgalib library?\n");
        
        my $response = prompt("library filename or 'none'", $default);
            
        if ($response ne 'none') {
            $svgalib = $response;
        }
    }
    if (defined($svgalib) && $svgalib ne 'none') {
        my $default;
        
        if (-d('/usr/include/svgalib')) {
            $default = '/usr/include/svgalib';
        } else {
            $default = "default";
        }
        print("Where are the interface headers for it?\n");
        
        my $response = prompt("Svgalib header directory", $default);
        
        if ($response ne "default") {
            $svgalibhdr_dir = $response;
        }
        if (defined($svgalibhdr_dir)) {
            if (!-d($svgalibhdr_dir)) {
                printf("WARNING: No directory named " .
                       "'$svgalibhdr_dir' exists.\n");
            }
        }
    }
    return($svgalib, $svgalibhdr_dir);
}



sub symlink_command() {

    my $retval;

    # Some Windows environments don't have symbolic links (or even a
    # simulation via a "ln" command, but have a "cp" command which works
    # in a pinch.  Some Windows environments have "ln", but it won't do
    # symbolic links.
    
    if (commandExists("ln")) {
        # We assume if Perl can do symbolic links, so can Ln, and vice
        # versa.

        my $symlink_exists = eval { symlink("",""); 1 };
        
        if ($symlink_exists) {
            $retval = "ln -s";
        } else {
            $retval = "ln";
        }
    } elsif (commandExists("cp")) {
        $retval = "cp";
    } else {
        # Well, maybe we just made a mistake.
        $retval = "ln -s";
    }
    return $retval;
}



sub help() {

    print("This is the Netpbm custom configuration program.  \n");
    print("It is not GNU Configure.\n");
    print("\n");
    print("There is one optional argument to this program:  The " .
          "name of the file to use as the basis for the Makefile.config " .
          "file.  Default is 'Makefile.config.in'\n");
    print("\n");
    print("Otherwise, the program is interactive.\n");
}



sub gnuOptimizeOpt($) {
    my ($gccCommandName) = @_;
#-----------------------------------------------------------------------------
#  Compute the -O compiler flag appropriate for a GNU system.  Ordinarily,
#  this is just -O3.  But many popular GNU systems have a broken compiler
#  that causes -O3 to generate incorrect code (symptom: pnmtojpeg --quality=95
#  generates a syntax error message from shhopt).
#-----------------------------------------------------------------------------
# I don't know what are exactly the cases that Gcc is broken.  I know 
# Red Hat 7.1 and 7.2 and Mandrake 8.2, running gcc 2.96.1, commonly have
# the problem.  But it may be limited to a certain subrelease level or
# CPU type or other environment.  People who have reported the problem have
# reported that Gcc 3.0 doesn't have it.  Gcc 2.95.3 doesn't have it.

# Note that automatic inlining happens at -O3 level, but there are some
# subroutines in Netpbm marked for explicit inlining, and we need to disable
# that inlining too, so we must go all the way down to -O0.

    my @gccVerboseResp = `$gccCommandName --verbose 2>&1`;

    my $brokenCompiler;
    
    if (@gccVerboseResp ==2) {
        if ($gccVerboseResp[1] =~ m{gcc version 2.96}) {
            $brokenCompiler = $TRUE;
        } else {
            $brokenCompiler = $FALSE;
        }
    } else {
        $brokenCompiler = $FALSE;
    }

    my $oOpt;

    if ($brokenCompiler) {
        print("You appear to have a broken compiler which would produce \n");
        print("incorrect code if requested to do inline optimization.\n");
        print("Therefore, I am configuring the build to not do inline \n");
        print("optimization.  This will make some Netpbm programs \n");
        print("noticeably slower.  If I am wrong about your compiler, just\n");
        print("edit Makefile.config and change -O0 to -O3 near the bottom.\n");
        print("\n");
        print("The problem is known to exist in the GNU Compiler \n");
        print("release 2.96.  If you upgrade, you will not have this \n");
        print("problem.\n");
        print("---------------------------------------------\n");
        print("\n");
        $oOpt = "-O0";
    } else {
        $oOpt = "-O3";
    }
    return $oOpt;
}



sub needLocal($) {
#-----------------------------------------------------------------------------
#  Return wether or not /usr/local paths must be added to compiles and
#  links.  In a properly configured system, those paths should be in
#  the compiler and linker default search paths, e.g. the compiler
#  should search /usr/local/include and then /usr/include without any
#  -I options needed.  But we've seen some systems where it isn't.
#
#  Actually, I have doubts now as to whether these misconfigured systems
#  really exist.  This subroutine was apparently always broken, because
#  before 04.03.15, it had "netbsd", etc. in lower case.  So it always
#  returned false.  I never had a complaint.  Plus, we had a bug in 
#  Makefile.config.in wherein it wiped out the user's setting of the LDFLAGS
#  environment variable.  This could explain /usr/local/lib not being in
#  the path when it should have been.
#
#  So I've disabled this function; we'll see if we encounter any truly
#  misconfigured systems.  04.03.15.
#-----------------------------------------------------------------------------
    my ($platform) = @_;

    return $FALSE;  # See comments above.

    my $needLocal;
    
    if ($platform eq "NETBSD" || $platform eq "OPENBSD" || 
        $platform eq "FREEBSD") {
        $needLocal = $TRUE;
    } else {
        $needLocal = $FALSE;
    }
    return $needLocal;
}



sub findProcessManagement($) {
    my ($dontHaveProcessMgmtR) = @_;
#-----------------------------------------------------------------------------
#  Return $TRUE iff the system does not have <sys/wait.h> in its default
#  search path.
#-----------------------------------------------------------------------------
    my $cflags = testCflags($FALSE);

    my @cSourceCode = (
                       "#include <sys/wait.h>\n",
                       );
    
    testCompile($cflags, \@cSourceCode, \my $success);

    if (!$success) {
        print("Your system does not appear to have <sys/wait.h> in its " .
              "standard compiler include path.  Therefore, we will build " .
              "Netpbm with some function missing (e.g. the pm_system() " .
              "function in libnetpbm and most of 'pamlookup'\n");
        $$dontHaveProcessMgmtR = $TRUE;
    } else {
        $$dontHaveProcessMgmtR = $FALSE;
    }
}



sub validateLibraries(@) {
    my @libList = @_;
#-----------------------------------------------------------------------------
#  Check each library name in the list @libList for viability.
#-----------------------------------------------------------------------------
    foreach my $libname (@libList) {
        if (defined($libname)) {
            if ($libname =~ m{/} and !-f($libname)) {
                print("WARNING: No regular file named '$libname' exists.\n");
            } elsif (!($libname =~ m{ .* \. (so|a|sa|sl|dll|dylib) $ }x)) {
                print("WARNING: The library name '$libname' does not have " .
                      "a conventional suffix (.so, .a, .dll, etc.)\n");
            }
        }
    }
}



sub warnJpegTiffDependency($$) {
    my ($jpeglib, $tifflib) = @_;

    if (defined($tifflib) && !defined($jpeglib)) {
        print("WARNING: You say you have a Tiff library, " .
              "but no Jpeg library.\n");
        print("Sometimes the Tiff library prerequires the " .
              "Jpeg library.  If \n");
        print("that is the case on your system, you will " .
              "have some links fail with\n");
        print("missing 'jpeg...' symbols.  If so, rerun " .
              "Configure and say you\n");
        print("have no Tiff library either.\n");
        print("\n");
    }
}



sub testCompileJpeglibH($$) {
    my ($cflags, $successR) = @_;
#-----------------------------------------------------------------------------
#  Do a test compile to see if we can see jpeglib.h.
#-----------------------------------------------------------------------------
    my @cSourceCode = (
                       "#include <ctype.h>\n",
                       "#include <stdio.h>\n",
                       "#include <jpeglib.h>\n",
                       );
    
    testCompile($cflags, \@cSourceCode, $successR);
}



sub testCompileJpegMarkerStruct($$) {
    my ($cflags, $successR) = @_;
#-----------------------------------------------------------------------------
#  Do a test compile to see if struct jpeg_marker_struct is defined in 
#  jpeglib.h.  Assume it is already established that the compiler works
#  and can find jpeglib.h.
#-----------------------------------------------------------------------------
    my @cSourceCode = (
                       "#include <ctype.h>\n",
                       "#include <stdio.h>\n",
                       "#include <jpeglib.h>\n",
                       "struct jpeg_marker_struct test;\n",
                       );

    testCompile($cflags, \@cSourceCode, $successR);
}



sub printMissingHdrWarning($$) {

    my ($name, $hdr_dir) = @_;

    print("WARNING: You said the compile-time part of the $name library " .
          "(the header\n");
    print("files) is in ");

    if (defined($hdr_dir)) {
        print("directory '$hdr_dir', ");
    } else {
        print("the compiler's default search path, ");
    }
    print("but a test compile failed\n");
    print("to confirm that.  If your configuration is exotic, the test " .
          "compile might\n");
    print("just be wrong, but otherwise the Netpbm build will fail.\n");
    print("\n");
    print("To fix this, either install the $name library there \n");
    print("or re-run Configure and answer the question about the $name " .
          "library\n");
    print("differently.\n");
    print("\n");
}



sub printOldJpegWarning() {
    print("WARNING: Your JPEG library appears to be too old for Netpbm.\n");
    print("We base this conclusion on the fact that jpeglib.h apparently\n");
    print("does not define struct jpeg_marker_struct.\n");
    print("If the JPEG library is not Independent Jpeg Group's Version 6b\n");
    print("or better, the Netpbm build will fail when it attempts to build\n");
    print("the parts that use the JPEG library.\n");
    print("\n");
    print("If your configuration is exotic, this test may just be wrong.\n");
    print("Otherwise, either upgrade your JPEG library or re-run Configure\n");
    print("and say you don't have a JPEG library.\n");
    print("\n");
}



sub testJpegHdr($$) {

    my ($needLocal, $jpeghdr_dir) = @_;

    if (defined($testCc)) {

        my $generalCflags = testCflags($needLocal);

        my $jpegIOpt = $jpeghdr_dir ? "-I$jpeghdr_dir" : "";

        testCompileJpeglibH("$generalCflags $jpegIOpt", \my $success);

        if (!$success) {
            print("\n");
            printMissingHdrWarning("JPEG", $jpeghdr_dir);
        } else {
            # We can get to something named jpeglib.h, but maybe it's an old
            # version we can't use.  Check it out.
            testCompileJpegMarkerStruct("$generalCflags $jpegIOpt", 
                                        \my $success);
            if (!$success) {
                print("\n");
                printOldJpegWarning();
            }
        }
    }
}



sub testCompileZlibH($$) {
    my ($cflags, $successR) = @_;
#-----------------------------------------------------------------------------
#  Do a test compile to see if we can see zlib.h.
#-----------------------------------------------------------------------------
    my @cSourceCode = (
                       "#include <zlib.h>\n",
                       );
    
    testCompile($cflags, \@cSourceCode, $successR);
}



sub testCompilePngH($$) {
    my ($cflags, $successR) = @_;
#-----------------------------------------------------------------------------
#  Do a test compile to see if we can see png.h, assuming we can see
#  zlib.h, which png.h #includes.
#-----------------------------------------------------------------------------
    my @cSourceCode = (
                       "#include <png.h>\n",
                       );
    
    testCompile($cflags, \@cSourceCode, $successR);
}



sub testPngHdr($$$) {
#-----------------------------------------------------------------------------
#  Issue a warning if the compiler can't find png.h.
#-----------------------------------------------------------------------------
    my ($needLocal, $pnghdr_dir, $zhdr_dir) = @_;

    if (defined($testCc)) {

        my $generalCflags = testCflags($needLocal);

        my $zlibIOpt = $zhdr_dir ? "-I$zhdr_dir" : "";

        testCompileZlibH("$generalCflags $zlibIOpt", \my $success);
        if (!$success) {
            print("\n");
            printMissingHdrWarning("Zlib", $zhdr_dir);
        } else {
            my $pngIOpt = $pnghdr_dir ? "-I$pnghdr_dir" : "";

            testCompilePngH("$generalCflags $zlibIOpt $pngIOpt", 
                            \my $success);

            if (!$success) {
                print("\n");
                printMissingHdrWarning("PNG", $pnghdr_dir);
            }
        }
    }
}



sub testLibpngConfig($) {
    my ($needLocal) = @_;
#-----------------------------------------------------------------------------
#  Issue a warning if the instructions 'libpng-config' give for compiling
#  with Libpng don't work.
#-----------------------------------------------------------------------------
    my $generalCflags = testCflags($needLocal);

    my $pngCflags = qx{libpng-config --cflags};
    chomp($pngCflags);

    testCompilePngH("$generalCflags $pngCflags", \my $success);

    if (!$success) {
        print("\n");
        print("Unable to compile a test PNG program using the compiler " .
              "flags that libpng-config says to use: '$pngCflags'.\n");
        print("This indicates that you have Libpng installed incorrectly.\n");
        print("libpng-config gets installed as part of the Libpng package.\n");
    } else {
        


    }
}



sub testConfiguration($$$$$$$) {

    my ($needLocal, $jpeglib, $jpeghdr_dir,
        $pnglib, $pnghdr_dir, $zlib, $zhdr_dir) = @_;

    if (defined($jpeglib)) {
        testJpegHdr($needLocal, $jpeghdr_dir);
    }
    if (defined($pnglib) && defined($zlib)) {
        testPngHdr($needLocal, $pnghdr_dir, $zhdr_dir);
    } elsif (commandExists('libpng-config')) {
        testLibpngConfig($needLocal);
    }

    # TODO: We ought to validate other libraries too.  But it's not
    # that important, because in the vast majority of cases where the
    # user incorrectly identifies any library, it affects all the
    # libraries and if the user can get a handle on the JPEG library
    # problem, he will also solve problems with any other library.
}



#******************************************************************************
#
#  MAINLINE
#
#*****************************************************************************

autoFlushStdout();

my $configInPathArg;
if (@ARGV > 0) {
    if ($ARGV[0] =~ "^-") {
        if ($ARGV[0] eq "--help") {
            help();
            exit(0);
        } else {
            die("Unrecognized option: $ARGV[0]");
        }
    } 
    $configInPathArg = $ARGV[0];
}

if (stat("Makefile.config")) {
    print("Discard existing Makefile.config?\n");
    print("Y or N (N) ==> ");

    my $answer = <STDIN>;
    if (!defined($answer)) {
        die("\nEnd of file on Standard Input");
    }
    chomp($answer);
    if (uc($answer) ne "Y") {
        print("Aborting at user request.\n");
        exit(1);
    }
}

print("\n");

displayIntroduction();

my ($platform, $subplatform) = getPlatform();

print("\n");

if ($platform eq "NONE") {
    print("You will have to construct Makefile.config manually.  To do \n");
    print("this, copy Makefile.config.in as Makefile.config, and then \n");
    print("edit it.  Follow the instructions and examples in the file. \n");
    print("Please report your results to the Netpbm maintainer so he \n");
    print("can improve the configure program. \n");
    exit;
}

getCompiler($platform, \my $compiler);
getLinker($platform, $compiler, \my $baseLinker, \my $linkViaCompiler);

chooseTestCompiler($compiler, \$testCc);

my $netpbmlib_runtime_path;
    # Undefined if the default from Makefile.config.in is acceptable.

if ($platform eq "SOLARIS" or $platform eq "IRIX" or
    $platform eq "DARWIN" or $platform eq "NETBSD" or
    $platform eq "AMIGA") {
    print("Where will the Netpbm shared library reside once installed?\n");
    print("Enter 'default' if it will reside somewhere that the shared\n");
    print("library loader will find it automatically.  Otherwise, \n");
    print("this directory will get built into the Netpbm programs.\n");
    print("\n");

    my $default = "default";
    my $response = prompt("Netpbm shared library directory", $default);

    if ($response eq "default") {
        $netpbmlib_runtime_path = "";
    } else {
        $netpbmlib_runtime_path = $response;
    }
}

my $default_target;

print("Do you want a regular build or a merge build?\n");
print("If you don't know what this means, " .
      "take the default or see doc/INSTALL\n");
print("\n");

{
    my $default = "regular";
    my $response = prompt("regular or merge", $default);
    
    if ($response eq "regular") {
        $default_target = "nonmerge";
    } elsif ($response eq "merge") {
        $default_target = "merge";
    } else {
        print("'$response' isn't one of the choices.  \n" .
              "You must choose 'regular' or 'merge'.\n");
        exit 12;
    }
}

print("\n");

getLibTypes($platform, $subplatform, $default_target,
            \my $netpbmlibtype, \my $netpbmlibsuffix, \my $shlibprefixlist,
            \my $willBuildShared, \my $staticlib_too);


getInttypes(\my $inttypesHeaderFile);

getInt64($inttypesHeaderFile, \my $haveInt64);

findProcessManagement(\my $dontHaveProcessMgmt);

#******************************************************************************
#
#  FIND THE PREREQUISITE LIBRARIES
#
#*****************************************************************************

print("\n\n");
print("The following questions concern the subroutine libraries that are " .
      "Netpbm\n");
print("prerequisites.  Every library has a compile-time part (header " .
      "files)\n");
print("and a link-time part.  In the case of a shared library, these are " .
      "both\n");
print("part of the \"development\" component of the library, which may be " .
      "separately\n");
print("installable from the runtime shared library.  For each library, you " .
      "must\n");
print("give the filename of the link library.  If it is not in your " .
      "linker's\n");
print("default search path, give the absolute pathname of the file.  In " .
      "addition,\n");
print("you will be asked for the directory in which the library's interface " .
      "headers\n");
print("reside, and you can respond 'default' if they are in your compiler's " .
      "default\n");
print("search path.\n");
print("\n");
print("If you don't have the library on your system, you can enter 'none' " .
      "as the\n");
print("library filename and the builder will skip any part that requires " .
      "that ");
print("library.\n");
print("\n");

my ($jpeglib, $jpeghdr_dir) = getJpegLibrary($platform);
print("\n");
my ($tifflib, $tiffhdr_dir) = getTiffLibrary($platform, $jpeghdr_dir);
print("\n");
my ($pnglib, $pnghdr_dir)   = getPngLibrary($platform, 
                                            $tiffhdr_dir, $jpeghdr_dir);
print("\n");
my ($zlib, $zhdr_dir)       = getZLibrary($platform, 
                                          $pnghdr_dir,
                                          $tiffhdr_dir,
                                          $jpeghdr_dir);
print("\n");
my ($x11lib, $x11hdr_dir) = getX11Library($platform); 

print("\n");
my ($linuxsvgalib, $linuxsvgahdr_dir) = getLinuxsvgaLibrary($platform); 

print("\n");

# We should add the JBIG and URT libraries here too.  They're a little
# more complicated because there are versions shipped with Netpbm.


#******************************************************************************
#
#  CONFIGURE DOCUMENTATION
#
#*****************************************************************************

print("What URL will you use for the main Netpbm documentation page?\n");
print("This information does not get built into any programs or libraries.\n");
print("It does not make anything actually install that web page.\n");
print("It is just for including in legacy man pages.\n");
print("\n");

my $default = "http://netpbm.sourceforge.net/doc/";

my $netpbm_docurl = prompt("Documentation URL", $default);

print("\n");




#******************************************************************************
#
#  VALIDATE THE CONFIGURATION USER HAS SELECTED
#
#*****************************************************************************

validateLibraries($jpeglib, $tifflib, $pnglib, $zlib);

warnJpegTiffDependency($jpeglib, $tifflib);

testConfiguration(needLocal($platform), 
                  $jpeglib, $jpeghdr_dir,
                  $pnglib, $pnghdr_dir,
                  $zlib, $zhdr_dir,
                  );

#******************************************************************************
#
#  FIND THE NETPBM SOURCE TREE AND INITIALIZE BUILD TREE
#
#*****************************************************************************

my $defaultConfigInPath;

if (-f("GNUmakefile")) {
    # He's apparently running us in the source tree or an already set up
    # build directory.
    $defaultConfigInPath = "Makefile.config.in";
} else {
    my $srcdir;
    my $done;

    $done = $FALSE;
    while (!$done) {
        print("Where is the Netpbm source code?\n");

        $srcdir = prompt("Netpbm source directory", 
                         abs_path(dirname($0) . "/.."));

        if (-f("$srcdir/GNUmakefile")) {
            $done = $TRUE;
        } else {
            print("That doesn't appear to contain Netpbm source code.\n");
            print("There is no file named 'GNUmakefile' in it.\n");
            print("\n");
        }    
    }
    unlink("GNUmakefile");
    symlink("$srcdir/GNUmakefile", "GNUmakefile");
    unlink("Makefile");
    symlink("$srcdir/Makefile", "Makefile");

    open(SRCDIR, ">Makefile.srcdir");
    print(SRCDIR "SRCDIR = $srcdir\n");
    close(SRCDIR);
    
    $defaultConfigInPath = "$srcdir/Makefile.config.in";
}

sub makeCompilerGcc($) {
    my ($Makefile_configR) = @_;
    my $compileCommand = 'gcc';
    push(@{$Makefile_configR}, "CC = $compileCommand\n");
    push(@{$Makefile_configR}, gnuCflags($compileCommand));
}


#******************************************************************************
#
#  BUILD Makefile.config
#
#*****************************************************************************

sub gnuCflags($) {
    my ($gccCommandName) = @_;

    return("CFLAGS = " . gnuOptimizeOpt($gccCommandName) . " -ffast-math " .
           " -pedantic -fno-common " . 
           "-Wall -Wno-uninitialized -Wmissing-declarations -Wimplicit " .
           "-Wwrite-strings -Wmissing-prototypes -Wundef\n");
}

my @Makefile_config;
    # This is the complete Makefile.config contents.  We construct it here
    # and ultimately write the whole thing out as Makefile.config.

# First, we just read the 'Makefile.config.in' in

my $configInPath;
if (defined($configInPathArg)) {
    $configInPath = $configInPathArg;
} else {
    $configInPath = $defaultConfigInPath;
}
open (CONFIG_IN,"<$configInPath") or
    die("Unable to open file '$configInPath' for input.");

@Makefile_config = <CONFIG_IN>;

unshift(@Makefile_config, 
        "####This file was automatically created by 'configure.'\n",
        "####Many variables are set twice -- a generic setting, then \n",
        "####a system-specific override at the bottom of the file.\n",
        "####\n");

close(CONFIG_IN);

# Now, add the variable settings that override the default settings that are
# done by the Makefile.config.in contents.

push(@Makefile_config, "\n\n\n\n");
push(@Makefile_config, "####Lines above were copied from Makefile.config.in " .
     "by 'configure'.\n");
push(@Makefile_config, "####Lines below were added by 'configure' based on " .
     "the $platform platform.\n");
if (defined($subplatform)) {
    push(@Makefile_config, "####subplatform '$subplatform'\n");
}

push(@Makefile_config, "DEFAULT_TARGET = $default_target\n");

push(@Makefile_config, "NETPBMLIBTYPE=$netpbmlibtype\n");
push(@Makefile_config, "NETPBMLIBSUFFIX=$netpbmlibsuffix\n");
if (defined($shlibprefixlist)) {
    push(@Makefile_config, "SHLIBPREFIXLIST=$shlibprefixlist\n");
}
push(@Makefile_config, "STATICLIB_TOO=$staticlib_too\n");

if (defined($netpbmlib_runtime_path)) {
    push(@Makefile_config, "NETPBMLIB_RUNTIME_PATH=$netpbmlib_runtime_path\n");
}

if ($platform eq "GNU") {
    my $compileCommand;
    if (!commandExists("cc") && commandExists("gcc")) {
        $compileCommand = "gcc";
        push(@Makefile_config, "CC = $compileCommand\n");
    } else {
        $compileCommand = "cc";
    }
    push(@Makefile_config, gnuCflags($compileCommand));
# The merged programs have a main_XXX subroutine instead of main(),
# which would cause a warning with -Wmissing-declarations or 
# -Wmissing-prototypes.
    push(@Makefile_config, "CFLAGS_MERGE = " .
         "-Wno-missing-declarations -Wno-missing-prototypes\n");
    push(@Makefile_config, "LDRELOC = ld --reloc\n");
    push(@Makefile_config, "LINKER_CAN_DO_EXPLICIT_LIBRARY=Y\n");
} elsif ($platform eq "SOLARIS") {
    push(@Makefile_config, 'LDSHLIB = -Wl,-Bdynamic,-G,-h,$(SONAME)', "\n");

    push(@Makefile_config, 'NEED_RUNTIME_PATH = Y', "\n");
    if ($compiler eq "cc") {
        push(@Makefile_config, "CFLAGS = -O\n");
        push(@Makefile_config, "CFLAGS_SHLIB = -Kpic\n");
    } else {
        makeCompilerGcc(\@Makefile_config);
    }
    # Before Netpbm 10.20 (January 2004), we set this to -R for 
    # $compiler == cc and -rpath otherwise.  But now we know that the GNU
    # compiler can also invoke a linker that needs -R, so we're more flexible.
    if ($baseLinker eq "GNU") {
        push(@Makefile_config, "RPATHOPTNAME = -rpath\n");
    } else {
        push(@Makefile_config, "RPATHOPTNAME = -R\n");
    }
    push(@Makefile_config, "NETWORKLD = -lsocket -lnsl\n");
} elsif ($platform eq "HP-UX") {
    if ($compiler eq "gcc") {
        makeCompilerGcc(\@Makefile_config);
        push(@Makefile_config, "CFLAGS += -fPIC\n");
        push(@Makefile_config, "LDSHLIB = -shared -fPIC\n");
        push(@Makefile_config, 'LDFLAGS += -Wl,+b,/usr/pubsw/lib', "\n");
    } else {
        # We don't know what to do here.  We used to (before 10.20) just
        # just assume the compiler was gcc.  We know that the gcc stuff
        # above does NOT work for HP native compiler.
        push(@Makefile_config, "LDSHLIB =\n");
    }
} elsif ($platform eq "AIX") {
    push(@Makefile_config, 'LDFLAGS = -L /usr/pubsw/lib', "\n");
    if ($compiler eq "cc") {
        # Yes, the -L option implies the runtime as well as linktime library
        # search path.  There's no way to specify runtime path independently.
        push(@Makefile_config, "RPATHOPTNAME = -L\n");
        push(@Makefile_config, "LDSHLIB = -qmkshrobj\n");
    } else {
        makeCompilerGcc(\@Makefile_config);
        push(@Makefile_config, "LDSHLIB = -shared\n");
    }
} elsif ($platform eq "TRU64") {
#    push(@Makefile_config, "INSTALL = installbsd\n");
    if ($compiler eq "cc") {
        push(@Makefile_config, 'CFLAGS = -O2 -std1', "\n");
        push(@Makefile_config, "LDFLAGS = -call_shared -oldstyle_liblookup " .
             "-L/usr/local/lib\n");
        push(@Makefile_config, "LDSHLIB = -shared -expect_unresolved \"*\"\n");
    } else {
        # We've never tested this.  This is just here to give a user a 
        # headstart on submitting to us the necessary information.  2002.07.04.
        push(@Makefile_config, "CC = gcc\n");
        push(@Makefile_config, 'CFLAGS = -O3', "\n");
        push(@Makefile_config, "LDSHLIB = -shared\n");
    }
    # Between May 2000 and July 2003, we had -DLONG_32 in these options.
    # We took it out because it generated bad code for a TRU64 user in
    # July 2003 whose system has 64 bit long and 32 bit int.  It affects
    # only Ppmtompeg and it isn't clear that using long instead of int is
    # ever right anyway.

    push(@Makefile_config, "OMIT_NETWORK = y\n");
    push(@Makefile_config, "LINKER_CAN_DO_EXPLICIT_LIBRARY=Y\n");
} elsif ($platform eq "IRIX") {
#    push(@Makefile_config, "INSTALL = install\n");
    push(@Makefile_config, "MANPAGE_FORMAT = cat\n");
    push(@Makefile_config, "RANLIB = true\n");
    push(@Makefile_config, "CFLAGS = -n32 -O3 -fullwarn\n");
    push(@Makefile_config, "LDFLAGS = -n32\n");
    push(@Makefile_config, "LDSHLIB = -shared -n32\n");
} elsif ($platform eq "WINDOWS") {
    if ($subplatform eq "cygwin") {
        makeCompilerGcc(\@Makefile_config);
    }
    push(@Makefile_config, "EXE = .exe\n");
    push(@Makefile_config, "OMIT_NETWORK = y\n");
#    # Though it may not have the link as "ginstall", "install" in a Windows
#    # Unix environment is usually GNU install.
#    my $ginstall_result = `ginstall --version 2>/dev/null`;
#    if (!$ginstall_result) {
#        # System doesn't have 'ginstall', so use 'install' instead.
#        push(@Makefile_config, "INSTALL = install\n");
#    }
    push(@Makefile_config, 'SYMLINK = ', symlink_command(), "\n");
    push(@Makefile_config, 'DLLVER=$(NETPBM_MAJOR_RELEASE)', "\n");
    push(@Makefile_config, "LDSHLIB = " . 
         '-shared -Wl,--image-base=0x10000000 -Wl,--enable-auto-import', "\n");
} elsif ($platform eq "BEOS") {
    push(@Makefile_config, "LDSHLIB = -nostart\n");
} elsif ($platform eq "OPENBSD") {
    # vedge@vedge.com.ar says on 2001.04.29 that there are a ton of 
    # undefined symbols in the Fiasco stuff on OpenBSD.  So we'll just
    # cut it out of the build until someone feels like fixing it.
    push(@Makefile_config, "BUILD_FIASCO = N\n");
} elsif ($platform eq "FREEBSD") {
} elsif ($platform eq "AMIGA") {
    push(@Makefile_config, "CFLAGS = -m68020-60 -ffast-math -mstackextend\n");
} elsif ($platform eq "UNIXWARE") {
    # Nothing to do.
} elsif ($platform eq "SCO") {
    # Got this from "John H. DuBois III" <spcecdt@armory.com> 2002.09.27:
    push(@Makefile_config, "RANLIB = true\n");
    if ($compiler eq "cc") {
        push(@Makefile_config, "CFLAGS = -O\n");
        push(@Makefile_config, "CFLAGS_SHLIB = -O -K pic\n");
        push(@Makefile_config, "LDSHLIB = -G\n");
        push(@Makefile_config, "SHLIB_CLIB =\n");
    } else {
        makeCompilerGcc(\@Makefile_config);
        push(@Makefile_config, "LDSHLIB = -shared\n"); 
    }
    push(@Makefile_config, "NETWORKLD = -lsocket -lresolve\n");
} elsif ($platform eq "DARWIN") {
    push(@Makefile_config, "CC = cc -no-cpp-precomp\n");
    push(@Makefile_config, 'CFLAGS_SHLIB = -fno-common', "\n");
    push(@Makefile_config, "LDSHLIB = ",
         "-dynamiclib ",
         '-install_name $(NETPBMLIB_RUNTIME_PATH)/libnetpbm.$(MAJ).dylib', 
         "\n");
#    push(@Makefile_config, "INSTALL = install\n");
} else {
    die ("Internal error: invalid value for \$platform: '$platform'\n");
}

if (needLocal($platform)) {
    push(@Makefile_config, "CFLAGS += -I/usr/local/include\n");
    push(@Makefile_config, "LDFLAGS += -L/usr/local/lib\n");
}

if ($linkViaCompiler) {
    push(@Makefile_config, "LINKERISCOMPILER = Y\n");
}

my $flex_result = `flex --version`;
if (!$flex_result) {
    # System doesn't have 'flex'.  Maybe 'lex' will work.  See the
    # make rules for Thinkjettopbm for information on our experiences
    # with Lexes besides Flex.

    my $systemRc = system('lex </dev/null &>/dev/null');

    if ($systemRc >> 8 == 127) {
        print("\n");
        print("You do not appear to have the 'flex' or 'lex' pattern \n");
        print("matcher generator on your system, so we will not build \n");
        print("programs that need it (Thinkjettopbm)\n");
        
        print("\n");
        print("Press ENTER to continue.\n");
        my $key = <STDIN>;
        print("\n");

        push(@Makefile_config, "LEX=\n");
    } else {
        print("\n");
        print("Using 'lex' as the pattern matcher generator, " .
              "since we cannot\n");
        print("find 'flex' on your system.\n");
        print("\n");

        push(@Makefile_config, "LEX = lex\n"); 
    }
}

if ($compiler eq 'gcc') {
    push(@Makefile_config, "CFLAGS_SHLIB += -fPIC\n");
}

if (defined($tiffhdr_dir)) {
    push(@Makefile_config, "TIFFHDR_DIR = $tiffhdr_dir\n");
}
if (defined($tifflib)) {
    push(@Makefile_config, "TIFFLIB = $tifflib\n");
}

if (defined($jpeghdr_dir)) {
    push(@Makefile_config, "JPEGHDR_DIR = $jpeghdr_dir\n");
}
if (defined($jpeglib)) {
    push(@Makefile_config, "JPEGLIB = $jpeglib\n");
}

if (defined($pnghdr_dir)) {
    push(@Makefile_config, "PNGHDR_DIR = $pnghdr_dir\n");
}
if (defined($pnglib)) {
    push(@Makefile_config, "PNGLIB = $pnglib\n");
}

if (defined($zhdr_dir)) {
    push(@Makefile_config, "ZHDR_DIR = $zhdr_dir\n");
}
if (defined($zlib)) {
    push(@Makefile_config, "ZLIB = $zlib\n");
}

if (defined($x11hdr_dir)) {
    push(@Makefile_config, "X11HDR_DIR = $x11hdr_dir\n");
}
if (defined($x11lib)) {
    push(@Makefile_config, "X11LIB = $x11lib\n");
}

if (defined($linuxsvgahdr_dir)) {
    push(@Makefile_config, "LINUXSVGAHDR_DIR = $linuxsvgahdr_dir\n");
}
if (defined($linuxsvgalib)) {
    push(@Makefile_config, "LINUXSVGALIB = $linuxsvgalib\n");
}

if (defined($netpbm_docurl)) {
    push(@Makefile_config, "NETPBM_DOCURL = $netpbm_docurl\n");
}

if ($inttypesHeaderFile ne '<inttypes.h>') {
    push(@Makefile_config, "INTTYPES_H = $inttypesHeaderFile\n");
}

if ($haveInt64 ne 'Y') {
    push(@Makefile_config, "HAVE_INT64 = $haveInt64\n");
}

if ($dontHaveProcessMgmt) {
    push(@Makefile_config, "DONT_HAVE_PROCESS_MGMT = Y\n");
}

#******************************************************************************
#
#  WRITE OUT THE FILE
#
#*****************************************************************************

open(MAKEFILE_CONFIG, ">Makefile.config") or
    die("Unable to open Makefile.config for writing in the current " .
        "directory.");

print MAKEFILE_CONFIG @Makefile_config;

close(MAKEFILE_CONFIG) or
    die("Error:  Close of Makefile.config failed.\n");

print("\n");
print("We have created the file 'Makefile.config'.  Note, however, that \n");
print("we guessed a lot at your configuration and you may want to look \n");
print("at Makefile.config and edit it to your requirements and taste \n");
print("before doing the make.\n");
print("\n");


print("Now you may proceed with 'make'\n");
print("\n");


exit 0;          
