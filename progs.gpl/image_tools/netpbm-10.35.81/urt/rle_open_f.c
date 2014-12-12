/* 
 * rle_open_f.c - Open a file with defaults.
 * 
 * Author : 	Jerry Winters 
 * 		EECS Dept.
 * 		University of Michigan
 * Date:	11/14/89
 * Copyright (c) 1990, University of Michigan
 */

#define _XOPEN_SOURCE  /* Make sure fdopen() is in stdio.h */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef NO_OPEN_PIPES
/* Need to have a SIGCLD signal catcher. */
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "rle.h"

/* Count outstanding children.  Assume no more than 100 possible. */
#define MAX_CHILDREN 100
static int catching_children = 0;
static int pids[MAX_CHILDREN];



static FILE *
my_popen(const char * const cmd, 
         const char * const mode, 
         int  *       const pid) {

    FILE *retfile;
    int thepid = 0;
    int pipefd[2];
    int i;

    /* Check args. */
    if ( *mode != 'r' && *mode != 'w' )
    {
	errno = EINVAL;
	return NULL;
    }

    if ( pipe(pipefd) < 0 )
	return NULL;
    
    /* Flush known files. */
    fflush(stdout);
    fflush(stderr);
    if ( (thepid = fork()) < 0 )
    {
	close(pipefd[0]);
	close(pipefd[1]);
	return NULL;
    }
    else if (thepid == 0) {
	/* In child. */
	/* Rearrange file descriptors. */
	if ( *mode == 'r' )
	{
	    /* Parent reads from pipe, so reset stdout. */
	    close(1);
	    dup2(pipefd[1],1);
	} else {
	    /* Parent writing to pipe. */
	    close(0);
	    dup2(pipefd[0],0);
	}
	/* Close anything above fd 2. (64 is an arbitrary magic number). */
	for ( i = 3; i < 64; i++ )
	    close(i);

	/* Finally, invoke the program. */
	if ( execl("/bin/sh", "sh", "-c", cmd, NULL) < 0 )
	    exit(127);
	/* NOTREACHED */
    }	

    /* Close file descriptors, and gen up a FILE ptr */
    if ( *mode == 'r' )
    {
	/* Parent reads from pipe. */
	close(pipefd[1]);
	retfile = fdopen( pipefd[0], mode );
    } else {
	/* Parent writing to pipe. */
	close(pipefd[0]);
	retfile = fdopen( pipefd[1], mode );
    }

    /* Return the PID. */
    *pid = thepid;

    return retfile;
}
#endif  /* !NO_OPEN_PIPES */

/* 
 *  Purpose : Open a file for input or ouput as controlled by the mode
 *  parameter.  If no file name is specified (ie. file_name is null) then
 *  a pointer to stdin or stdout will be returned.  The calling routine may
 *  call this routine with a file name of "-".  For this case rle_open_f
 *  will return a pointer to stdin or stdout depending on the mode.
 *    If the user specifies a non-null file name and an I/O error occurs
 *  when trying to open the file, rle_open_f will terminate execution with
 *  an appropiate error message.
 *
 *  parameters
 *   input:
 *     prog_name: 	name of the calling program.
 *     file_name : 	name of the file to open
 *     mode : 		either "r" for read or input file or "w" for write or
 *            		output file
 *
 *   output:
 *     a file pointer
 * 
 */
FILE *
rle_open_f_noexit(const char * const prog_name, 
                  const char * const file_name, 
                  const char * const mode ) {

    FILE *fp;
    void perror();
    CONST_DECL char *err_str;
    const char *cp;
    char *combuf;

    if ( *mode == 'w' || *mode == 'a' )
        fp = stdout;     /* Set the default value */
    else
        fp = stdin;
    
    if ( file_name != NULL && strcmp( file_name, "-" ) != 0 )
    {
#ifndef NO_OPEN_PIPES
        /* Check for dead children. */
        if ( catching_children > 0 )
        {
            int i, j;

            /* Check all children to see if any are dead, reap them if so. */
            for ( i = 0; i < catching_children; i++ )
            {
                /* The assumption here is that if it's dead, the kill
                 * will fail, but, because we haven't waited for
                 * it yet, it's a zombie.
                 */
                if (kill(pids[i], 0) < 0) {
                    int opid = pids[i], pid = 0;
                    /* Wait for processes & delete them from the list,
                     * until we get the one we know is dead.
                     * When removing one earlier in the list than
                     * the one we found, decrement our loop index.
                     */
                    while (pid != opid) {
                        pid = wait( NULL );
                        for ( j = 0;
                              j < catching_children && pids[j] != pid;
                              j++ )
                            ;
                        if ( pid < 0 )
                            break;
                        if ( j < catching_children ) {
                            if ( i >= j )
                                i--;
                            for ( j++; j < catching_children; j++ )
                                pids[j-1] = pids[j];
                            catching_children--;
                        }
                    }
                }
            }
        }

        /*  Real file, not stdin or stdout.  If name ends in ".Z",
         *  pipe from/to un/compress (depending on r/w mode).
         *  
         *  If it starts with "|", popen that command.
         */
	    
        cp = file_name + strlen( (char*) file_name ) - 2;
        /* Pipe case. */
        if ( *file_name == '|' )
        {
            int thepid;		/* PID from my_popen */
            if ( (fp = my_popen( file_name + 1, mode, &thepid )) == NULL )
            {
                err_str = "%s: can't invoke <<%s>> for %s: ";
                goto err;
            }
            /* One more child to catch, eventually. */
            if (catching_children < MAX_CHILDREN) {
                pids[catching_children++] = thepid;
            }
        }

        /* Compress case. */
        else if ( cp > file_name && *cp == '.' && *(cp + 1) == 'Z' )
        {
            int thepid;		/* PID from my_popen. */
            combuf = (char *)malloc( 20 + strlen( file_name ) );
            if ( combuf == NULL )
            {
                err_str = "%s: out of memory opening (compressed) %s for %s";
                goto err;
            }

            if ( *mode == 'w' )
                sprintf( combuf, "compress > %s", file_name );
            else if ( *mode == 'a' )
                sprintf( combuf, "compress >> %s", file_name );
            else
                sprintf( combuf, "compress -d < %s", file_name );

            fp = my_popen( combuf, mode, &thepid );
            free( combuf );

            if ( fp == NULL )
            {
                err_str =
                    "%s: can't invoke 'compress' program, "
                    "trying to open %s for %s";
                goto err;
            }
            /* One more child to catch, eventually. */
            if (catching_children < MAX_CHILDREN) {
                pids[catching_children++] = thepid;
            }
        }
        else
#endif /* !NO_OPEN_PIPES */
        {
            /* Ordinary, boring file case. */
            /* In the original code, the code to add the "b" was
               conditionally included only if the macro STDIO_NEEDS_BINARY was
               defined.  But for Netpbm, there is no need make a distinction;
               we always add the "b".  -BJH 2000.07.20.  
            */
            char mode_string[32];	/* Should be enough. */

            /* Concatenate a 'b' onto the mode. */
            mode_string[0] = mode[0];
            mode_string[1] = 'b';
            strcpy( mode_string + 2, mode + 1 );
        
            if ( (fp = fopen(file_name, mode_string)) == NULL )
            {
                err_str = "%s: can't open %s for %s: ";
                goto err;
            }
        }
    }

    return fp;

err:
	fprintf( stderr, err_str,
             prog_name, file_name,
             (*mode == 'w') ? "output" :
             (*mode == 'a') ? "append" :
             "input" );
	perror( "" );
	return NULL;

}

FILE *
rle_open_f(const char * prog_name, const char * file_name, const char * mode)
{
    FILE *fp;

    if ( (fp = rle_open_f_noexit( prog_name, file_name, mode )) == NULL )
	exit( -1 );

    return fp;
}


/*****************************************************************
 * TAG( rle_close_f )
 * 
 * Close a file opened by rle_open_f.  If the file is stdin or stdout,
 * it will not be closed.
 * Inputs:
 * 	fd:	File to close.
 * Outputs:
 * 	None.
 * Assumptions:
 * 	fd is open.
 * Algorithm:
 * 	If fd is NULL, just return.
 * 	If fd is stdin or stdout, don't close it.  Otherwise, call fclose.
 */
void
rle_close_f( fd )
FILE *fd;
{
    if ( fd == NULL || fd == stdin || fd == stdout )
	return;
    else
	fclose( fd );
}
