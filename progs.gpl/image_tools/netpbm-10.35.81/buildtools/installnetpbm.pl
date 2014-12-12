#!/usr/bin/perl -w

require 5.000;

#Note: mkdir() must have 2 arguments as late as 5.005.

use strict;
use English;
use Fcntl;
use File::Basename;

my ($TRUE, $FALSE) = (1,0);

my $cpCommand;
#use vars qw($cpCommand);

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

    print("$prompt ($default) ==> ");

    my $response = <STDIN>;

    chomp($response);
    if ($response eq "") {
        $response = $default;
    }

    return $response;
}



sub getPkgdir() {

    my $pkgdir;

    while (!$pkgdir) {
    
        print("Where is the install package you created with " .
              "'make package'?\n");
        my $default = "/tmp/netpbm";
        
        my $response = prompt("package directory", $default);

        if (!-f("$response/pkginfo")) {
            print("This does not appear to be a Netpbm install package. \n");
            print("A file named $response/pkginfo does not exist.\n");
            print("\n");
        } else {
            $pkgdir = $response;
        }
    }
    print("\n");
    return $pkgdir;
}



sub makePrefixDirectory($) {

    my ($prefixDir) = @_;

    if ($prefixDir ne "" and !-d($prefixDir)) {
        print("No directory named '$prefixDir' exists.  Do you want " .
              "to create it?\n");

        my $done;
        while (!$done) {
            my $response = prompt("Y(es) or N(o)", "Y");
            if (uc($response) eq "Y") {
                my $success = mkdir($prefixDir, 0777);
                if (!$success) {
                print("Unable to create directory '$prefixDir'.  " .
                      "Error is $ERRNO\n");
            }
                $done = $TRUE;
            } elsif (uc($response) eq "N") {
                $done = $TRUE;
            } 
        }
    }
}





sub getPrefix() {

    print("Enter the default prefix for installation locations.  " .
          "I will use \n");
    print("this in generating defaults for the following prompts to " .
          "save you \n");
    print("typing.  If you plan to spread Netpbm across your system, \n" .
          "enter '/'.\n");
    print("\n");

    my $default;
    if ($OSNAME eq "cygwin") {
        $default = "/usr/local";
    } elsif ($ENV{OSTYPE} && $ENV{OSTYPE} eq "msdosdjgpp") {
        $default = "/dev/env/DJDIR";
    } else {
        $default = "/usr/local/netpbm";
    }

    my $response = prompt("install prefix", $default);

    my $prefix;

    # Remove possible trailing /
    if (substr($response,-1,1) eq "/") {
        $prefix = substr($response, 0, -1);
    } else {
        $prefix = $response;
    }
    print("\n");

    makePrefixDirectory($prefix);

    return $prefix;
}



sub getCpCommand() {
#-----------------------------------------------------------------------------
# compute the command + options need to do a recursive copy, preserving
# symbolic links and file attributes.
#-----------------------------------------------------------------------------
    my $cpCommand;

    # We definitely need more intelligence here, but we'll need input from
    # users to do it.  Maybe we should just bundle GNU Cp with Netpbm as an
    # install tool.  Maybe we should write a small recursive copy program
    # that uses more invariant tools, like buildtools/install.sh does for
    # single files.

    if (`cp --version 2>/dev/null` =~ m/GNU/) {
        # It's GNU Cp -- we have options galore, and they're readable.
        $cpCommand = "cp --recursive --preserve --no-dereference";
    } else {
        # This works on Cp from "4th Berkeley Distribution", July 1994.
        # Mac OSX has this.
        # -R means recursive with no dereferencing of symlinks
        # -p means preserve attributes
        $cpCommand = "cp -R -p";
    }
    return($cpCommand);
}



sub getBinDir($) {

    my ($prefix) = @_;

    print("Where do you want the programs installed?\n");
    print("\n");

    my $binDir;

    while (!$binDir) {
        my $default = "$prefix/bin";

        my $response = prompt("program directory", $default);
        
        if (-d($response)) {
            $binDir = $response;
        } else {
            my $succeeded = mkdir($response, 0777);
            
            if (!$succeeded) {
                print("Unable to create directory '$response'.  " .
                      "Error=$ERRNO\n");
            } else {
                $binDir = $response;
            }
        }
    }
    print("\n");

    return $binDir;
}



sub installProgram($$$) {

    my ($pkgdir, $prefix, $bindirR) = @_;

    my $binDir = getBinDir($prefix);

    print("Installing programs...\n");

    my $rc = system("$cpCommand $pkgdir/bin/* $binDir/");

    if ($rc != 0) {
        print("Copy of programs from $pkgdir/bin to $binDir failed.\n");
        print("cp return code is $rc\n");
    } else {
        print("Done.\n");
    }
    $$bindirR = $binDir;
}



sub getLibDir($) {

    my ($prefix) = @_;

    print("Where do you want the shared library installed?\n");
    print("\n");

    my $libDir;

    while (!$libDir) {
        my $default = "$prefix/lib";

        my $response = prompt("shared library directory", $default);
        
        if (-d($response)) {
            $libDir = $response;
        } else {
            my $succeeded = mkdir($response, 0777);
            
            if (!$succeeded) {
                print("Unable to create directory '$response'.  " .
                      "Error=$ERRNO\n");
            } else {
                $libDir = $response;
            }
        }
    }
    print("\n");

    return $libDir;
}



sub 
execLdconfig() {
#-----------------------------------------------------------------------------
#  Run Ldconfig.  Try with the -X option first, and if that is an invalid
#  option (which we have seen on an openBSD system), try it without -X.
#
#  -X means "don't create any symlinks."  Any symlinks required should be
#  created as part of installing the library, so we don't need that function
#  from Ldconfig.  And we want to tread as lightly as possible on the 
#  system -- we don't want creating symlinks that have nothing to do with
#  Netpbm to be a hidden side effect of installing Netpbm.
#
#  Note that this Ldconfig works only if the user installed the Netpbm
#  library in a standard directory that Ldconfig searches.  Note that on
#  OpenBSD, Ldconfig is hardcoded to search only /usr/lib ever.  We could
#  also do 'ldconfig DIR' to scan the particular directory in which we
#  installed the Netpbm library.  But 1) the effects of this would disappear
#  the next time the user rebuilds the cache file; and 2) on OpenBSD, this
#  causes the cache file to be rebuilt from ONLY that directory.  On OpenBSD,
#  you can add the -m option to cause it to ADD the contents of DIR to the
#  existing cache file.
#  
#-----------------------------------------------------------------------------
# Implementation note:  We've seen varying completion codes and varying
# error messages from different versions of Ldconfig when it fails.

    my $ldconfigSucceeded;

    my $ldconfigXResp = `ldconfig -X 2>&1`;

    if (!defined($ldconfigXResp)) {
        print("Unable to run Ldconfig.\n");
        $ldconfigSucceeded = $FALSE;
    } elsif ($ldconfigXResp eq "") {
        $ldconfigSucceeded = $TRUE;
    } elsif ($ldconfigXResp =~ m{usage}i) {
        print("Trying Ldconfig again without the -X option...\n");

        my $rc = system("ldconfig");
        
        $ldconfigSucceeded = ($rc == 0);
    } else {
        print($ldconfigXResp);
        $ldconfigSucceeded = $FALSE;
    }
    
    if ($ldconfigSucceeded) {
        print("Ldconfig completed successfully.\n");
    } else {
        print("Ldconfig failed.  You will have to fix this later.\n");
    }
}



sub ldconfigExists() {

    return (system("ldconfig -? 2>/dev/null") >> 8) != 127;
}



sub
doLdconfig() {
#-----------------------------------------------------------------------------
#  Run Ldconfig where appropriate.
#-----------------------------------------------------------------------------
    if ($OSNAME eq "linux" || ldconfigExists()) {
        # This is a system where Ldconfig makes sense

        print("In order for the Netpbm shared library to be found when " .
              "you invoke \n");
        print("A Netpbm program, you must either set an environment " .
              "variable to \n");
        print("tell where to look for it, or you must put its location " .
              "in the shared \n");
        print("library location cache.  Do you want to run Ldconfig now " .
              "to put the \n");
        print("Netpbm shared library in the cache?  This works only if " .
              "you have\n");
        print("installed the library in a standard location.\n");
        print("\n");
        
        my $done;

        $done = $FALSE;

        while (!$done) {
            my $response = prompt("Y(es) or N(o)", "Y");

            if (uc($response) eq "Y") {
                execLdconfig();
                $done = $TRUE;
            } elsif (uc($response) eq "N") {
                $done = $TRUE;
            } else {
                print("Invalid response.  Enter 'Y' or 'N'\n");
            }
        }
    }
}



sub installSharedLib($$$) {

    my ($pkgdir, $prefix, $libdirR) = @_;

    if (-d("$pkgdir/lib")) {
        my $libDir = getLibDir($prefix);

        print("Installing shared libraries...\n");

        my $rc = system("$cpCommand $pkgdir/lib/* $libDir/");

        if ($rc != 0) {
            print("Copy of libraries from $pkgdir/lib to $libDir failed.\n");
            print("cp return code is $rc\n");
        } else {
            print("done.\n");
            print("\n");
            doLdconfig();
        }
        $$libdirR = $libDir;
    } else {
        print("You did not build a shared library, so I will not " .
              "install one.\n");
    }
    print("\n");
}



sub getLinkDir($) {

    my ($prefix) = @_;

    print("Where do you want the static link library installed?\n");
    print("\n");

    my $linkDir;

    while (!$linkDir) {
        my $default = "$prefix/lib";

        my $response = prompt("static library directory", $default);
        
        if (-d($response)) {
            $linkDir = $response;
        } else {
            my $succeeded = mkdir($response, 0777);
            
            if (!$succeeded) {
                print("Unable to create directory '$response'.  " .
                      "Error=$ERRNO\n");
            } else {
                $linkDir = $response;
            }
        }
    }
    print("\n");

    return $linkDir;
}



sub installStaticLib($$$) {

    my ($pkgdir, $prefix, $linkdirR) = @_;

    if (-d("$pkgdir/link")) {
        my $linkDir = getLinkDir($prefix);

        print("Installing link libraries.\n");

        my $rc = system("$cpCommand $pkgdir/link/* $linkDir/");

        if ($rc != 0) {
            print("Copy of files from $pkgdir/link to $linkDir failed.\n");
            print("cp return code is $rc\n");
        } else {
            print("done.\n");
        }
        $$linkdirR = $linkDir;
    } else {
        print("You did not build a static library, so I will not " .
              "install one \n");
    }
}



sub getDataDir($) {

    my ($prefix) = @_;

    print("Where do you want the data files installed?\n");
    print("\n");

    my $dataDir;

    while (!$dataDir) {
        my $default = "$prefix/lib";

        my $response = prompt("data file directory", $default);
        
        if (-d($response)) {
            $dataDir = $response;
        } else {
            my $succeeded = mkdir($response, 0777);
            
            if (!$succeeded) {
                print("Unable to create directory '$response'.  " .
                      "Error=$ERRNO\n");
            } else {
                $dataDir = $response;
            }
        }
    }
    print("\n");

    return $dataDir;
}



sub getHdrDir($) {

    my ($prefix) = @_;

    print("Where do you want the library interface header files installed?\n");
    print("\n");

    my $hdrDir;

    while (!$hdrDir) {
        my $default = "$prefix/include";

        my $response = prompt("header directory", $default);
        
        if (-d($response)) {
            $hdrDir = $response;
        } else {
            my $succeeded = mkdir($response, 0777);
            
            if (!$succeeded) {
                print("Unable to create directory '$response'.  " .
                      "Error=$ERRNO\n");
            } else {
                $hdrDir = $response;
            }
        }
    }
    print("\n");

    return $hdrDir;
}



sub installDataFile($$$) {

    my ($pkgdir, $prefix, $datadirR) = @_;

    my $dataDir = getDataDir($prefix);

    print("Installing data files...\n");

    my $rc = system("$cpCommand $pkgdir/misc/* $dataDir/");

    if ($rc != 0) {
        print("copy of data files from $pkgdir/misc to $dataDir " .
              "failed.\n");
        print("cp exit code is $rc\n");
    } else {
        $$datadirR = $dataDir;
        print("done.\n");
    }
}



sub installHeader($$$) {

    my ($pkgdir, $prefix, $includedirR) = @_;

    my $hdrDir = getHdrDir($prefix);

    print("Installing interface header files...\n");

    my $rc = system("$cpCommand $pkgdir/include/* $hdrDir/");

    if ($rc != 0) {
        print("copy of header files from $pkgdir/include to $hdrDir " .
              "failed.\n");
        print("cp exit code is $rc\n");
    } else {
        print("done.\n");
    }
    $$includedirR = $hdrDir;
}



sub getManDir($) {

    my ($prefix) = @_;

    print("Where do you want the man pages installed?\n");

    print("\n");

    my $manDir;

    while (!$manDir) {
        my $default = "$prefix/man";

        my $response = prompt("man page directory", $default);

        if (-d($response)) {
            $manDir = $response;
        } else {
            my $succeeded = mkdir($response, 0777);
            
            if (!$succeeded) {
                print("Unable to create directory '$response'.  " .
                      "Error=$ERRNO\n");
            } else {
                $manDir = $response;
            }
        }
    }
    print("\n");

    return $manDir;
}



sub removeObsoleteManPage($) {

    my ($mandir) = @_;

    unlink("$mandir/man1/pgmoil");
    unlink("$mandir/man1/pgmnorm");
    unlink("$mandir/man1/ppmtojpeg");
    unlink("$mandir/man1/bmptoppm");
    unlink("$mandir/man1/ppmtonorm");
    unlink("$mandir/man1/ppmtouil");
    unlink("$mandir/man1/pnmnoraw");
    unlink("$mandir/man1/gemtopbm");
    unlink("$mandir/man1/pnminterp");
}



sub tryToCreateManwebConf($) {

    my ($manweb_conf_filename) = $@;

    print("You don't have a /etc/manweb.conf, which is the " .
          "configuration\n");
    print("file for the 'manweb' program, which is a quick way to " .
          "get to Netpbm\n");
    print("documentation.  Would you like to create one now?\n");
        
    my $done;
    
    while (!$done) {
        my $response = prompt("create /etc/manweb.conf", "Y");
        
        if (uc($response) eq "Y") {
            my $successful = open(MANWEB_CONF, ">/etc/manweb.conf");
            if (!$successful) {
                print("Unable to create file /etc/manweb.conf.  " .
                          "error = $ERRNO\n");
            } else {
                print(MANWEB_CONF "#Configuration file for Manweb\n");
                print(MANWEB_CONF "webdir=/usr/man/web\n");
                close(MANWEB_CONF);
                $done = $TRUE;
            }
        } else {
            $done = $TRUE;
        }
    }
}



sub getWebdir($) {
    my ($manweb_conf_filename) = @_;
#-----------------------------------------------------------------------------
#  Return the value of the Manweb "web directory," as indicated by the
#  Manweb configuration file $manweb_conf_filename.
#
#  If that file doesn't exist, or doesn't have a 'webdir' value, or
#  the 'webdir' value is a chain of directories instead of just one,
#  we return an undefined value.
#-----------------------------------------------------------------------------
    my $webdir;

    my $success = open(MANWEB_CONF, "<$manweb_conf_filename");
    if (!$success) {
        print("Unable to open file '$manweb_conf_filename' for reading.  " .
              "error is $ERRNO\n");
    } else {
        while (<MANWEB_CONF>) {
            chomp();
            if (/^\s*#/) {
                #It's comment - ignore
            } elsif (/^\s*$/) {
                #It's a blank line - ignore
            } elsif (/\s*(\S+)\s*=\s*(\S+)/) {
                #It looks like "keyword=value"
                my ($keyword, $value) = ($1, $2);
                if ($keyword eq "webdir") {
                    # We can't handle a multi-directory path; we're looking
                    # only for a webdir statement naming a sole directory.
                    if ($value !~ m{:}) {
                        $webdir = $value;
                    }
                }
            }
        }
        close(MANWEB_CONF);
    }              

    return $webdir
}



sub userWantsManwebSymlink($$) {

    my ($webdir, $netpbmWebdir) = @_;

    print("Your manweb.conf file says top level documentation " .
          "is in $webdir, \n");
    print("but you installed netpbm.url in $netpbmWebdir.\n");
    print("Do you want to create a symlink in $webdir now?\n");

    my $wants;
    my $done;
    
    while (!$done) {
        my $response = prompt("create symlink (Y/N)", "Y");
        
        if (uc($response) eq "Y") {
            $wants = $TRUE;
            $done = $TRUE;
        } elsif (uc($response) eq "N") {
            $wants = $FALSE;
            $done = $TRUE;
        }
    }
    return $wants;
}



sub makeInManwebPath($) {

    my ($netpbmWebdir) = @_;

    # Now we need /etc/manweb.conf to point to the directory in which we
    # just installed netpbm.url.

    if (!-f("/etc/manweb.conf")) {
        tryToCreateManwebConf("/etc/manweb.conf");
    }
    if (-f("/etc/manweb.conf")) {
        my $webdir = getWebdir("/etc/manweb.conf");
        if (defined($webdir)) {
            if ($webdir ne $netpbmWebdir) {
                if (userWantsManwebSymlink($webdir, $netpbmWebdir)) {
                    my $old = "$netpbmWebdir/netpbm.url";
                    my $new = "$webdir/netpbm.url";
                    mkdir($webdir, 0777);
                    my $success = symlink($old, $new);
                    if (!$success) {
                        print("Failed to create symbolic link from $new to " .
                              "$old.  Error is $ERRNO\n");
                    }
                }
            }
        }
    }
}



sub installManPage($$$) {


# Note: This installs the pointer man pages and the netpbm.url file for Manweb.

    my ($pkgdir, $prefix, $mandirR) = @_;

    my $manDir = getManDir($prefix);

    print("Installing man pages...\n");

    my $rc = system("$cpCommand $pkgdir/man/* $manDir/");

    if ($rc != 0) {
        print("copy of man pages from $pkgdir/man to $manDir failed.\n");
        print("cp exit code is $rc\n");
    } else {
        print("done.\n");
    }

    print("\n");

    removeObsoleteManPage($manDir);

    makeInManwebPath("$manDir/web");
    
    $$mandirR = $manDir;
}



sub 
processTemplate($$$$$$$$$) {
    my ($templateR, $version, $bindir, $libdir, $linkdir, $datadir, 
        $includedir, $mandir, $outputR) = @_;

    my @output;

    foreach (@{$templateR}) {
        if (m{^@}) {
            # Comment -- ignore it.
        } else {
            if (defined($version)) {
                s/\@VERSION\@/$version/;
            }
            if (defined($bindir)) {
                s/\@BINDIR@/$bindir/;
            }
            if (defined($libdir)) {
                s/\@LIBDIR@/$libdir/;
            }
            if (defined($linkdir)) {
                s/\@LINKDIR@/$linkdir/;
            }
            if (defined($datadir)) {
                s/\@DATADIR@/$datadir/;
            }
            if (defined($includedir)) {
                s/\@INCLUDEDIR@/$includedir/;
            }
            if (defined($mandir)) {
                s/\@MANDIR@/$mandir/;
            }
            push(@output, $_);
        }
    }
    $$outputR = \@output;
}



sub
installConfig($$$$$$$) {
    my ($pkgdir, 
        $bindir, $libdir, $linkdir, $datadir, $includedir, $mandir) = @_;

    my $error;

    my $configTemplateFilename = dirname($0) . "/config_template";

    my $templateOpened = open(TEMPLATE, "<$configTemplateFilename");
    if (!$templateOpened) {
        $error = "Can't open template file '$configTemplateFilename'.\n";
    } else {
        my @template = <TEMPLATE>;

        close(TEMPLATE);

        my $versionOpened = open(VERSION, "<$pkgdir/VERSION");

        my $version;
        if (!$versionOpened) {
            $error = "Unable to open $pkgdir/VERSION for reading.  " .
                "Errno=$ERRNO\n";
        } else {
            $version = <VERSION>;
            chomp($version);
            close(VERSION);

            processTemplate(\@template, $version, $bindir, $libdir,
                            $linkdir, $datadir, $includedir, $mandir,
                            \my $fileContentsR);

            # TODO: Really, this ought to go in an independent directory,
            # because you might want to have the Netpbm executables in
            # some place not in the PATH and use this program, via the
            # PATH, to find them.
            
            my $filename = "$bindir/netpbm-config";
            
            my $success = open(NETPBM_CONFIG, ">$filename");
            if ($success) {
                chmod(0755, $filename);
                foreach (@{$fileContentsR}) { print NETPBM_CONFIG; }
                close(NETPBM_CONFIG);
            } else {
                $error = "Unable to open the file " .
                    "'$filename' for writing.  Errno=$ERRNO\n";
            }
        }
    }
    if ($error) {
        print(STDERR "Failed to create the Netpbm configuration program.  " .
              "$error\n");
    }
}



#******************************************************************************
#
#  MAINLINE
#
#*****************************************************************************

autoFlushStdout();

print("Welcome to the Netpbm install dialogue.  We will now proceed \n");
print("to interactively install Netpbm on this system.\n");
print("\n");
print("You must have already built Netpbm and then packaged it for \n");
print("installation by running 'make package'.  See the INSTALL file.\n");
print("\n");

my $pkgdir = getPkgdir();

my $prefix = getPrefix();

$cpCommand = getCpCommand();

installProgram($pkgdir, $prefix, \my $bindir);
print("\n");

installSharedLib($pkgdir, $prefix, \my $libdir);
print("\n");

installStaticLib($pkgdir, $prefix, \my $linkdir);
print("\n");

installDataFile($pkgdir, $prefix, \my $datadir);
print("\n");

installHeader($pkgdir, $prefix, \my $includedir);
print("\n");

installManPage($pkgdir, $prefix, \my $mandir);
print("\n");

installConfig($pkgdir, 
              $bindir, $libdir, $linkdir, $datadir, $includedir, $mandir);

print("Installation is complete (except where previous error messages have\n");
print("indicated otherwise).\n");

exit(0);
