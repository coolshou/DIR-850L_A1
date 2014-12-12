/*----------------------------------------------------------------------------
                                 libopt
------------------------------------------------------------------------------
  This is a program to convert link library filepaths to linker options
  that select them.  E.g. ../lib/libnetpbm.so becomes -L../lib -lnetpbm   .

  Each argument is a library filepath.  The option string to identify
  all of those library filepaths goes to Standard Output.

  If there is no slash in the library filepath, we assume it is just a
  filename to be searched for in the linker's default search path, and
  generate a -l option, but no -L.

  If an argument doesn't make sense as a library filespec, it is
  copied verbatim, blank delimited, to the output string.

  The "lib" part of the library name, which we call the prefix, may be
  other than "lib".  The list of recognized values is compiled in as
  the macro SHLIBPREFIXLIST (see below).
  
  There is no newline or null character or anything after the output.

  If you specify the option "-runtime", the output includes a -R
  option in addition, for every library after "-runtime" in the
  arguments.  -R tells the buildtime linker to include the specified
  library's directory in the search path that the runtime linker uses
  to find dynamically linked libraries.

  But if you compile this program with the EXPLICIT macro defined, and
  a library filepath contains a slash, then it skips all that -L/-l
  nonsense and just outputs the input library filepath verbatim (plus
  any -R option).
------------------------------------------------------------------------------
  Why would you want to use this?

  On some systems, the -L/-l output of this program has exactly the
  same effect as the filepath input when used in the arguments to a
  link command.  A GNU/Linux system, for example.  On others (Solaris,
  for example), if you include /tmp/lib/libnetpbm.so in the link as a
  link object, the executable gets built in such a way that the system
  accesses the shared library /tmp/lib/libnetpbm.so at run time.  On the
  other hand, if you instead put the options -L/tmp/lib -lnetpbm on the
  link command, the executable gets built so that the system accesses
  libnetpbm.so in its actual installed directory at runtime (that
  location might be determined by a --rpath linker option or a
  LD_LIBRARY_PATH environment variable at run time).

  In a make file, it is nice to use the same variable as the
  dependency of a rule that builds an executable and as the thing that
  the rule's command uses to identify its input.  Here is an example
  of using libopt for that:

     NETPBMLIB=../lib/libnetpbm.so
     ...
     pbmmake: pbmmake.o $(PBMLIB)
             ld -o pbmmake pbmmake.o `libopt $(PBMLIB)` --rpath=/lib/netpbm

  Caveat: "-L../lib -lnetpbm" is NOT exactly the same as
  "../pbm/libnetpbm.so" on any system.  All of the -l libraries are
  searched for in all of the -L directories.  So you just might get a
  different library with the -L/-l version than if you specify the
  library file explicitly.

  That's why you should compile with -DEXPLICIT if your linker can
  handle explicit file names.

-----------------------------------------------------------------------------*/
#define _BSD_SOURCE 1      /* Make sure strdup() is in stdio.h */
#define _XOPEN_SOURCE 500  /* Make sure strdup() is in string.h */

#define MAX_PREFIXES 10

/* Here's how to use SHLIBPREFIXLIST:  Use a -D compile option to pass in
   a value appropriate for the platform on which you are linking libraries.

   It's a blank-delimited list of prefixes that library names might
   have.  "lib" is traditionally the only prefix, as in libc or
   libnetpbm.  However, on Windows there is a convention of using
   different prefixes to distinguish different co-existent versions of
   the same library (kind of like a major number in some unices). 
   E.g. the value "cyg lib" is appropriate for a Cygwin system.
*/
#ifndef SHLIBPREFIXLIST
#  define SHLIBPREFIXLIST "lib"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef unsigned char bool;
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#ifdef DLLVERSTR
static const char * dllverstr = DLLVERSTR;
#else
static const char * dllverstr = "";
#endif

bool const explicit = 
#ifdef EXPLICIT
TRUE
#else
FALSE
#endif
;

static void
strfree(const char * const arg) {
    free((void*) arg);
}


static void
parse_prefixlist(const char * const prefixlist, 
                 char * parsed_prefixes[MAX_PREFIXES+1],
                 bool * const errorP) {
/*----------------------------------------------------------------------------
   Given a space-separated list of tokens (suffixes), return an
   argv-style array of pointers, each to a newly malloc'ed storage
   area the prefix as a null-terminated string.  Return a null string
   for the unused entries at the end of the array

   We never return more than MAX_PREFIXES prefixes from the input, so
   there is guaranteed always to be one null string at the end of the
   array.

   In case of error, return *errorP == TRUE and don't allocate any
   storage.  Otherwise, return *errorP = FALSE.
-----------------------------------------------------------------------------*/
    char * prlist;

    prlist = strdup(prefixlist);
    if (prlist == NULL)
        *errorP = TRUE;
    else {
        if (strlen(prlist) <= 0) 
            *errorP = TRUE;
        else {
            /* NOTE: Mac OS X, at least, does not have strtok_r().
               2001.09.24
            */
            char * token;
            int num_tokens;
            int i;
            
            for (i=0; i < MAX_PREFIXES + 1; i++) {
                parsed_prefixes[i] = NULL;
            }
            num_tokens = 0;
            token = strtok(prlist, " ");
            *errorP = FALSE;  /* initial value */
            while (token != NULL && num_tokens < MAX_PREFIXES && !*errorP) {
                parsed_prefixes[num_tokens] = strdup (token);
                if (parsed_prefixes[num_tokens] == NULL) 
                    *errorP = TRUE;
                num_tokens++;
                token = strtok(NULL, " ");
            }
            for (i = num_tokens; i < MAX_PREFIXES + 1 && !*errorP;  i++) {
                parsed_prefixes[i] = strdup("");
                if (parsed_prefixes[i] == NULL) 
                    *errorP = TRUE;
            }
        }
        if (*errorP) {
            /* Deallocate any array entries we successfully did */
            int i;
            for (i = 0; i < MAX_PREFIXES + 1; i++)
                if (parsed_prefixes[i])
                    free(parsed_prefixes[i]);
        }
        free(prlist);
    }
}



static void
parse_prefix(const char * const filename, 
             bool * const prefix_good_p, unsigned int * const prefix_length_p,
             bool * const error_p) {
/*----------------------------------------------------------------------------
   Find the library name prefix (e.g. "lib") in the library filename
   'filename'.
   
   Return the length of the prefix, in characters, as *prefix_length_p.
   (The prefix always starts at the beginning of the filename).

   Iff we don't find a valid library name prefix, return *prefix_good_p
   == FALSE.  

   The list of valid prefixes is compiled in as the blank-delimited
   string which is the value of the SHLIBPREFIXLIST macro.
-----------------------------------------------------------------------------*/
    char * shlibprefixlist[MAX_PREFIXES+1];
        /* An argv-style array of prefix strings in the first entries, 
           null strings in the later entries.  At most MAX_PREFIXES prefixes,
           so at least one null string.
        */
    char * prefix;
        /* The prefix that the filename actually
           uses.  e.g. if shlibprefixlist = { "lib", "cyg", "", ... } and the
           filename is "path/to/cyg<something>.<extension>", then 
           prefix = "cyg".  String is in the same storage as pointed to by
           shlibprefixlist (shlibprefixlist[1] in this example).
        */
    bool prefix_good;
        /* The first part of the filename matched one of the prefixes
           in shlibprefixlist[].
        */
    int prefix_length;
    int i;

    parse_prefixlist(SHLIBPREFIXLIST , shlibprefixlist, error_p);
    if (!*error_p) {
        if (strcmp(shlibprefixlist[0], "") == 0) {
            fprintf(stderr, "libopt was compiled with an invalid value "
                    "of the SHLIBPREFIX macro.  It seems to have no "
                    "tokens.  SHLIBPREFIX = '%s'", 
                    SHLIBPREFIXLIST);
            exit(100);
        }

        i = 0;  /* start with the first entry in shlibprefixlist[] */
        prefix_length = 0;  /* initial value */
        prefix = shlibprefixlist[i];
        prefix_good = FALSE;  /* initial value */
        while ( (*prefix != '\0' ) && !prefix_good ) {
            /* stop condition: shlibprefixlist has MAX_PREFIXES+1 entries.
             * we only ever put tokens in the 0..MAX_PREFIXES-1 positions.
             * Then, we fill DOWN from the MAX_PREFIXES position with '\0'
             * so we insure that the shlibprefixlist array contains at 
             * least one final '\0' string, but probably many '\0' 
             * strings (depending on how many tokens there were).               
             */
            prefix_length = strlen(prefix);
            if (strncmp(filename, prefix, prefix_length) == 0) {
                prefix_good = TRUE;
                /* at this point, prefix is pointing to the correct
                 * entry, and prefix_length has the correct value.
                 * When we bail out of the while loop because of the
                 * !prefix_good clause, we can then use these 
                 * vars (prefix, prefix_length) 
                 */
            } else {
                prefix = shlibprefixlist[++i];
            }
        }
        *prefix_length_p = prefix_length;
        *prefix_good_p = prefix_good;
        { 
            int i;
            for (i=0; i < MAX_PREFIXES + 1; i++) 
                free (shlibprefixlist[i]);
        }
    }
}



static void
parse_filename(const char *  const filename,
               const char ** const libname_p,
               bool *        const valid_library_p,
               bool *        const static_p,
               bool *        const error_p) {
/*----------------------------------------------------------------------------
   Extract the library name root component of the filename 'filename'.  This
   is just a filename, not a whole pathname.

   Return it in newly malloc'ed storage pointed to by '*libname_p'.
   
   E.g. for "libxyz.so", return "xyz".

   return *valid_library_p == TRUE iff 'filename' validly names a library
   that can be expressed in a -l linker option.

   return *static_p == TRUE iff 'filename' indicates a static library.
   (but undefined if *valid_library_p != TRUE).

   return *error_p == TRUE iff some error such as out of memory prevents
   parsing.

   Do not allocate any memory if *error_p == TRUE or *valid_library_p == FALSE.
-----------------------------------------------------------------------------*/
    char *lastdot;  
    /* Pointer to last period in 'filename'.  Null if none */
    
    /* We accept any period-delimited suffix as a library type suffix.
       It's probably .so or .a, but is could be .kalamazoo for all we
       care. (HOWEVER, the double-suffixed import lib used on 
       cygwin (.dll.a) is NOT understood). 
    */
    char *p;

    lastdot = strrchr(filename, '.');
    if (lastdot == NULL) {
        /* This filename doesn't have any suffix, so we don't understand
           it as a library filename.
        */
        *valid_library_p = FALSE;
        *error_p = FALSE;
    } else {
        unsigned int prefix_length;
        bool prefix_good;

        if (strcmp(lastdot + 1, "a") == 0)
            *static_p = TRUE;
        else
            *static_p = FALSE;

        parse_prefix(filename, &prefix_good, &prefix_length, error_p);
        if (!*error_p) {
            if (!prefix_good) {
                *valid_library_p = FALSE;
            } else {
                /* Extract everything between <prefix> and "." as 
                   the library name root. 
                */
                char * libname;

                libname = strdup(filename + prefix_length);
                if (libname == NULL)
                    *error_p = TRUE;
                else {
                    libname[lastdot - filename - prefix_length] = '\0';
                    if (strlen(dllverstr) > 0) {
                        p = strstr(libname, dllverstr);
                        if (p) {
                            if (libname + strlen(libname) 
                                - strlen(dllverstr) == p) {
                                *p = '\0';
                            }
                        }
                    }
                    if (strlen(libname) == 0) {
                        *valid_library_p = FALSE;
                        strfree(libname);
                    } else
                        *valid_library_p = TRUE;
                }
                *libname_p = libname;
            }
        }
    }
}   



static void
parse_filepath(const char *  const filepath,
               const char ** const directory_p, 
               const char ** const filename_p,
               bool *        const error_p) {
/*----------------------------------------------------------------------------
   Extract the directory and filename components of the filepath 
   'filepath' and return them in newly malloc'ed storage pointed to by
   '*directory_p' and '*filename_p'.

   If there is no directory component, return a null string for it.
-----------------------------------------------------------------------------*/
    char *directory;
    char *lastslash; /* Pointer to last slash in 'filepath', or null if none */

    lastslash = strrchr(filepath, '/');

    if (lastslash == NULL) {
        /* There's no directory component; the filename starts at the
           beginning of the filepath 
        */
        *filename_p = strdup(filepath);
        if (*filename_p == NULL)
            *error_p = TRUE;
        else {
            directory = strdup("");
            if (directory == NULL) {
                *error_p = TRUE;
                strfree(*filename_p);
            } else
                *error_p = FALSE;
        }
    } else {
        /* Split the string at the slash we just found, into filename and 
           directory 
           */
        *filename_p = strdup(lastslash+1);
        if (*filename_p == NULL)
            *error_p = TRUE;
        else {
            directory = strdup(filepath);
            if (directory == NULL) {
                *error_p = TRUE;
                strfree(*filename_p);
            } else {
                *error_p = FALSE;
                directory[lastslash - filepath] = '\0';
            }
        }
    }
    *directory_p = directory;
}



static void
doOptions(const char *  const filepath, 
          const char *  const directory,
          const char *  const libname,
          bool          const runtime,
          bool          const explicit,
          bool          const staticlib,
          const char ** const optionsP) {

    char * options;
    char * linkopt;

    if (strlen(directory) == 0) {
        linkopt = malloc(strlen(libname) + 10);
        sprintf(linkopt, "-l%s", libname);
    } else {
        if (explicit)
            linkopt = strdup(filepath);
        else {
            linkopt = malloc(strlen(directory) + strlen(libname) + 10);
            sprintf(linkopt, "-L%s -l%s", directory, libname);
        }
    }
    if (runtime && !staticlib && strlen(directory) > 0) {
        options = malloc(strlen(linkopt) + strlen(directory) + 10);
        sprintf(options, "%s -R%s", linkopt, directory); 
    } else
        options = strdup(linkopt);
    
    strfree(linkopt);
    
    *optionsP = options;
}



static void
processOneLibrary(const char *  const filepath, 
                  bool          const runtime, 
                  bool          const explicit,
                  const char ** const optionsP,
                  bool *        const errorP) {
/*----------------------------------------------------------------------------
   Process the library with filepath 'filepath'.  Return the resulting
   linker option string as a newly malloced null-terminated string at
   *optionsP.
-----------------------------------------------------------------------------*/
    const char * directory;
        /* Directory component of 'filepath' */
    const char * filename;
        /* Filename component of 'filepath' */

    parse_filepath(filepath, &directory, &filename, errorP);
    if (!*errorP) {
        const char *libname;
            /* Library name component of 'filename'.  e.g. xyz in
               libxyz.so 
            */
        bool valid_library;  
            /* Our argument is a valid library filepath that can be
               converted to -l/-L notation.  
            */
        bool staticlib;
            /* Our argument appears to name a static library. */

        parse_filename(filename, 
                       &libname, &valid_library, &staticlib, errorP);
        
        if (!*errorP) {
            if (valid_library) {
                doOptions(filepath, directory, libname, 
                          runtime, explicit, staticlib, optionsP);

                strfree(libname);
            } else
                *optionsP = strdup(filepath);
        }
        strfree(directory); 
        strfree(filename);
    }
}



int
main(int argc, char **argv) {

    bool error;
    bool runtime;  /* -runtime option has been seen */
    bool quiet;    /* -quiet option has been seen */
    int retval;
    unsigned int arg;  /* Index into argv[] of argument we're processing */
    char outputLine[1024];

    strcpy(outputLine, "");  /* initial value */
    runtime = FALSE;  /* initial value */
    quiet = FALSE;   /* initial value */
    error = FALSE;  /* no error yet */
    for (arg = 1; arg < argc && !error; arg++) {
        if (strcmp(argv[arg], "-runtime") == 0)
            runtime = TRUE;
        else if (strcmp(argv[arg], "-quiet") == 0)
            quiet = TRUE;
        else {
            const char * options;
            processOneLibrary(argv[arg], runtime, explicit, 
                              &options, &error);
            if (!error) {
                if (strlen(outputLine) + strlen(options) + 1 + 1 > 
                    sizeof(outputLine))
                    error = TRUE;
                else {
                    strcat(outputLine, " ");
                    strcat(outputLine, options);
                }
                strfree(options);
            }
        }
    }
    if (error) {
        fprintf(stderr, "serious libopt error prevented parsing library "
                "names.  Invalid input to libopt is NOT the problem.\n");
        retval = 10;
    } else {
        fputs(outputLine, stdout);
        retval = 0;
    }
    return retval;
}
