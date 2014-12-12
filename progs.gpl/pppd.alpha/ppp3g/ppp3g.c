/* vi: set sw=4 ts=4: */

/***********************************************************************
*
* ppp3g.c
*
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include "ppp3g.h"
#include "ppp_fcs.h"
#include "eloop.h"
#include "dtrace.h"
#include "pppd.h"

#include "pathnames.h"

static int chat_script(char *, int, int );


static int conn_running;				/* we have a [dis]connector running */

/* function located in chat.c for porcessing the script */
extern int do_file(char *, int );

extern void modem_hungup(void);
extern int hungup;
extern uid_t uid;
extern mode_t tty_mode;
//extern int fd_devnull;

int *real_ttyfd;				/* fd for actual serial port (not pty) */
extern int pty_pid;

static int chat_script(char * program, int ttyfd, int dont_wait)
{
	int pid;
	int status = -1;
	int errfd;

	++conn_running;
#ifdef EMBED
	pid = vfork();
#else
	pid = safe_fork();
#endif
	d_dbg("%s: after the fork pid = %d\n",__func__, pid);
	if (pid < 0)
	{
		--conn_running;
		error("Failed to create child process: %m");
		return -1;
    }

	if (pid != 0)
	{
		if (dont_wait)
		{
//#ifndef EMBED
			record_child(pid, program, NULL, NULL);
//#endif
			fprintf(stderr, "pppd: %s %d\n", program, pid);
			pty_pid = pid; //what is this for?
			status = 0;
		}
		else
		{
			while (waitpid(pid, &status, 0) < 0)
			{
				if (errno == EINTR) continue;
				fatal("error waiting for (dis)connection process: %m");
			}
			--conn_running;
		}
		d_dbg("%s: before exiting\n",__func__);
		return (status == 0 ? 0 : -1);
	}

    /* here we are executing in the child */
    
    /* dup log_to_fd to 2 for debug printing */
	d_dbg("%s: log_to_fd = %d\n",__func__,log_to_fd);
	errfd = dup(log_to_fd);
	close( log_to_fd );
	close(2);
	dup2(errfd, 2);
	close(errfd);

	setuid(uid);
	if (getuid() != uid)
	{
		error("setuid failed");
		exit(1);
	}
	setgid(getgid());

	d_dbg("%s: program equals to: %s\n",__func__, program);
	if (do_file(program, ttyfd) == 0) return 0;
	d_error("%s: chat.c failed\n",__func__);
	return 1;
}

int ppp3g_module_connect(int *tty_fd, char *connector)
{
	real_ttyfd = tty_fd;
	int fdflags;
	struct stat statbuf;
	int ttyfd;				/* Serial port file descriptor */
	int retry = 10;
	int err;

	d_dbg("%s: Hello !!\n",__func__);

	/* We need a tty device */
	if (devnam[0] == '\0')
	{
		d_error("%s: no device specified !\n",__func__);
		return -1;
	}

	while (retry > 0)
	{
		/* Become the user before opening the Device. */
		//seteuid(uid);
		ttyfd = open(devnam, O_NONBLOCK | O_RDWR, 0);
		err = errno;
		//seteuid(0);
		if (ttyfd >= 0) break;
		errno = err;
		if (err != EINTR)
		{
			d_error("%s: Failed to open %s\n",__func__,devnam);
			status = EXIT_OPEN_FAILED;
		}
		if (!persist || err != EINTR) return -1;
	}
	*real_ttyfd = ttyfd;
	d_dbg("%s: real ttyfd becomes %d\n",__func__, *real_ttyfd);
	if ((fdflags = fcntl(ttyfd, F_GETFL)) == -1 || fcntl(ttyfd, F_SETFL, fdflags & ~O_NONBLOCK) < 0)
		warn("Couldn't reset non-blocking mode on device: %m");

	/* Do the equivalent of `mesg n' to stop broadcast messages. */
	if (fstat(ttyfd, &statbuf) < 0 || fchmod(ttyfd, statbuf.st_mode & ~(S_IWGRP | S_IWOTH)) < 0)
		warn("Couldn't restrict write permissions to %s: %m", devnam);
	else
		tty_mode = statbuf.st_mode;

	/*
	 * Set line speed, flow control, etc.
	 * If we have a non-null connection or initializer script,
	 * on most systems we set CLOCAL for now so that we can talk
	 * to the modem before carrier comes up.  But this has the
	 * side effect that we might miss it if CD drops before we
	 * get to clear CLOCAL below.  On systems where we can talk
	 * successfully to the modem with CLOCAL clear and CD down,
	 * we could clear CLOCAL at this point.
	 */
	set_up_tty(ttyfd, (ppp3g_chat_file != NULL && ppp3g_chat_file[0] != 0));

	d_dbg("%s: connection script!!\n",__func__);

	/* run connection script */
	if (ppp3g_chat_file != NULL && ppp3g_chat_file[0] != 0)
	{
		if (*real_ttyfd != -1) 
		{	/* XXX do this if doing_callback == CALLBACK_DIALIN? */
			if (!default_device && modem) 
			{
				setdtr(*real_ttyfd, 0);	/* in case modem is off hook */
				sleep(1);
				setdtr(*real_ttyfd, 1);
			}
		}
		if (ppp3g_chat_file && ppp3g_chat_file[0]) 
		{
			fflush(stdout);
			d_dbg("%s: running chatscript - %s\n",__func__,ppp3g_chat_file);
			if (chat_script(ppp3g_chat_file, ttyfd, 0) < 0)
			{
				error("Connect script failed");
				status = EXIT_CONNECT_FAILED;
				return -1;
			}
			info("Serial connection established.");
		}

		/* set line speed, flow control, etc.;
		 * clear CLOCAL if modem option */
		/* Mark: these two lines are likely for call back and can be erased, but not sure */
/*		if (*real_ttyfd != -1)
			set_up_tty(*real_ttyfd, 0);
		connector = ppp3g_chat_file; */
	}
	
	return 0;
}

void ppp3g_module_disconnect(void)
{
	if (ppp3g_dc_chat_file == NULL || hungup)
		return;
	if (*real_ttyfd >= 0)
		set_up_tty(*real_ttyfd, 1);
	if (chat_script(ppp3g_dc_chat_file, *real_ttyfd, 0) < 0) 
		warn("disconnect script failed");
	else 
		info("Serial link disconnected.");
}

