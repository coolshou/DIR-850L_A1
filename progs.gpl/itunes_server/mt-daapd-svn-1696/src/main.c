/*
 * $Id: main.c 1690 2007-10-23 04:23:50Z rpedde $
 * Driver for multi-threaded daap server
 *
 * Copyright (C) 2003 Ron Pedde (ron@pedde.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * @file main.c
 *
 * Driver for mt-daapd, including the main() function.  This
 * is responsible for kicking off the initial mp3 scan, starting
 * up the signal handler, starting up the webserver, and waiting
 * around for external events to happen (like a request to rescan,
 * or a background rescan to take place.)
 *
 * It also contains the daap handling callback for the webserver.
 * This should almost certainly be somewhere else, and is in
 * desparate need of refactoring, but somehow continues to be in
 * this files.
 */

/** @mainpage mt-daapd
 * @section about_section About
 *
 * This is mt-daapd, an attempt to create an iTunes server for
 * linux and other POSIXish systems.  Maybe even Windows with cygwin,
 * eventually.
 *
 * You might check these locations for more info:
 * - <a href="http://www.mt-daapd.org">Home page</a>
 * - <a href="http://sf.net/projects/mt-daapd">Project page on SourceForge</a>
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "daapd.h"

#include "conf.h"
#include "configfile.h"
#include "err.h"
#include "mp3-scanner.h"
#include "webserver.h"
#include "restart.h"
#include "db-generic.h"
#include "os.h"
#include "plugin.h"
#include "util.h"
#include "upnp.h"
#include "io.h"

char device_inf[10]="Any";

#ifdef HAVE_GETOPT_H
# include "getopt.h"
#endif

#ifndef WITHOUT_MDNS
# include "rend.h"
#endif

/** Seconds to sleep before checking for a shutdown or reload */
#define MAIN_SLEEP_INTERVAL  2

/** Let's hope if you have no atoll, you only have 32 bit inodes... */
#if !HAVE_ATOLL
#  define atoll(a) atol(a)
#endif

/* debug for scan path curtis@alpha 08_03_2009 */
#define PATH_SCAN
#ifdef PATH_SCAN
#define	DBG_PATH_SCAN(x)	x
	#else
#define	DBG_PATH_SCAN(x)
#endif

/*
 * Globals
 */
CONFIG config; /**< Main configuration structure, as read from configfile */

/*
 * Forwards
 */
static void usage(char *program);
static void main_handler(WS_CONNINFO *pwsc);
static int main_auth(WS_CONNINFO *pwsc, char *username, char *password);
static void txt_add(char *txtrecord, char *fmt, ...);
static void main_io_errhandler(int level, char *msg);
static void main_ws_errhandler(int level, char *msg);

/**
 * tranform win path to linux path
 * curtis@alpha 06_23_2009
 */
char *alpha_win_path_to_linux(char *pstr){
	char win_char=92;//ASCII 
	char linux_char=47;
	int i=0;
	while(pstr[i] != NULL){
		if( pstr[i] == win_char ) {
				DBG_PATH_SCAN(printf("pstr[%d]=%c\n",i,pstr[i]);)
				pstr[i]= linux_char; 
		}
		i++;
	}	
	
	return pstr;
}


int filter(const struct dirent *entry)
{
	char *last_dot = NULL;
	if(strstr(entry->d_name, ".") == entry->d_name)
		return 0;
	else if(entry->d_type & DT_DIR)
		return 1;
	else if((entry->d_type & DT_REG) && 
		(last_dot = strrchr(entry->d_name, '.')) &&
		(strcasecmp(last_dot, ".mp3") == 0))
		return 1;
	else if((entry->d_type & DT_REG) && 
		(last_dot = strrchr(entry->d_name, '.')) &&
		(strcasecmp(last_dot, ".ogg") == 0))
		return 1;		
	else if((entry->d_type & DT_REG) && 
		(last_dot = strrchr(entry->d_name, '.')) &&
		(strcasecmp(last_dot, ".flac") == 0))
		return 1;		
	else if((entry->d_type & DT_REG) && 
		(last_dot = strrchr(entry->d_name, '.')) &&
		(strcasecmp(last_dot, ".m4a") == 0))
		return 1;		
	else if((entry->d_type & DT_REG) && 
		(last_dot = strrchr(entry->d_name, '.')) &&
		(strcasecmp(last_dot, ".m4p") == 0))
		return 1;		
	else
		return 0;
}


#if 0
int filter(const struct dirent *entry)
{
	char *last_dot = NULL;
	if(strstr(entry->d_name, ".") == entry->d_name)
		return 0;
	else if(entry->d_type & DT_DIR)
		return 1;
	else if((entry->d_type & DT_REG) && 
		(last_dot = strrchr(entry->d_name, '.')) &&
		(strcasecmp(last_dot, ".mp3") == 0))
		return 1;
	else
		return 0;
}
#endif

int get_mp3_count(const char *file)
{
	struct dirent **namelist;
	char *fullname = NULL;
	int n, count = 0;

	if((n = scandir(file, &namelist, filter, alphasort)) < 0)
		return 0;
	
	while(n--)
	{
		if((fullname = malloc(strlen(file) + strlen(namelist[n]->d_name) + 2)))
		{
			sprintf(fullname, "%s%s%s", file, file[strlen(file) - 1] == '/' ? "" : "/", namelist[n]->d_name);
			if(namelist[n]->d_type & DT_DIR)
				count += get_mp3_count(fullname);
			else if(namelist[n]->d_type & DT_REG)
				count++;
			free(fullname);
		}
		free(namelist[n]);
	}
	free(namelist);
	return count;
}

/**
 * tranform Volume to real path
 * curtis@alpha 06_23_2009
 */
void alpha_transform_volume(char **path){
	
		int dir_index=0;
		/* define max length of Mp3Path as 128 */
		char mp3path[512]="/mnt/HD/";
		char mp3_tmp[512]="/mnt/HD/";
		char volume[12];
		char mnt[12];
		char sym[4];
		FILE *share_folder_number;
		/* support to 4-bay NAS */
		char volume_1[12];
		char volume_2[12];
		char volume_3[12];
		char volume_4[12];
		/* for scanf */
		int fe = 0;

		/* check and modify volume number */
		share_folder_number = NULL;
		memset(volume, 0, 12);
		memset(mnt, 0 , 12);
		memset(sym, 0 , 4);
		memset(volume_1, 0, 12);
		memset(volume_2, 0 ,12);
		memset(volume_3, 0 ,12);
		memset(volume_4, 0 ,12);
			    		
		/* open shared_name and test it */
		share_folder_number=fopen("/etc/shared_name", "r");
		if (share_folder_number == NULL)
			printf("main.c: open /etc/shared_name faild...\n");	
		else
		  printf("main.c: open /etc/shared_name successfully...\n");
			
		/* get Volume value from shared_name */
		while(feof(share_folder_number) == 0)	{
			fe = fscanf(share_folder_number, "%s %s %s", volume, sym, mnt);
			DBG_PATH_SCAN(printf("main.c: fscanf: fe=%d\n", fe);)
			DBG_PATH_SCAN(printf("main.c: fscanf: /etc/shared_name: %s %s %s\n", volume, sym, mnt);)
			 if(fe<0)
			 	break; 
			    
			 if( !strcmp(volume, "Volume_1")) {
			 	memcpy(volume_1, mnt, 12);
			 	DBG_PATH_SCAN(printf("main.c : Volume_1=%s\n", mnt);)
			 }
			 else if ( !strcmp(volume, "Volume_2")) {
			 	memcpy(volume_2, mnt, 12);
			 	DBG_PATH_SCAN(printf("main.c : Volume_2=%s\n", mnt);)
			 }
			 else if ( !strcmp(volume, "Volume_3")) { 
			 	memcpy(volume_3, mnt, 12);
			 	DBG_PATH_SCAN(printf("main.c : Volume_3=%s\n", mnt);)
			 }
			 else if ( !strcmp(volume, "Volume_4")) {   
			 	memcpy(volume_4, mnt, 12);   
			  DBG_PATH_SCAN(printf("main.c : Volume_4=%s\n", mnt);)
			 }
			   		   
			 dir_index=0;
			 while(dir_index < 1000) {/* break this loop when read last mp3_path */
			  if( path[dir_index] != NULL) {
					DBG_PATH_SCAN(printf("main.c: config_mp3_path[%d]=%s\n",dir_index,path[dir_index]);)

			   	/* curtis@alpha catch mp3_dir path 06_24_2009 */
			   	char *pstr;
					if( !strncmp(path[dir_index], "Volume_1", 8) && strlen(volume_1)){  
				  	strcat(mp3path, volume_1);
				  	DBG_PATH_SCAN(printf("main.c: Volume_1 and mp3path=%s\n", mp3path);)
					  if((pstr = strstr(path[dir_index], "Volume_")))
							pstr += strlen("Volume_")+1;
						
						DBG_PATH_SCAN(printf("main.c: catched_mp3_path=%s\n",pstr);)
						/* search "\" and replace it to "\" curtis@alpha 08_03 */
						pstr=alpha_win_path_to_linux(pstr);
			    	
			    	if(pstr!=NULL) {
				    	strcat(mp3path,pstr);
				    	DBG_PATH_SCAN(printf("main.c: mp3path=%s\n",mp3path);)
				   		path[dir_index]=strdup(mp3path);
				   		DBG_PATH_SCAN(printf("main.c: path[%d]=%s\n",dir_index,path[dir_index]);)
			   		}
				  }
				  else if ( !strncmp(path[dir_index], "Volume_2", 8) && strlen(volume_2) ){   
				  	strcat(mp3path,volume_2);
				  	DBG_PATH_SCAN(printf("main.c: Volume_2: mp3path=%s\n", mp3path);)
					  				 	 
						if((pstr = strstr(path[dir_index], "Volume_")))
							pstr += strlen("Volume_")+1;									
			    	DBG_PATH_SCAN(printf("main.c: catched_mp3_path=%s\n",pstr);)
			    	pstr=alpha_win_path_to_linux(pstr);
			    	if(pstr!=NULL) {
				   		strcat(mp3path,pstr);
				   		DBG_PATH_SCAN(printf("main.c: mp3path=%s\n",mp3path);)
			    		path[dir_index]=strdup(mp3path);
			    		DBG_PATH_SCAN(printf("main.c: path[%d]=%s\n",dir_index,path[dir_index]);)
		    		}					  				 	 
				  }
					else if ( !strncmp(path[dir_index], "Volume_3", 8) && strlen(volume_3)){  					  	
					 	strcat(mp3path,volume_3);
					  DBG_PATH_SCAN(printf("Volume_3: mp3path=%s\n", mp3path);)
				  				 	 
						if((pstr = strstr(path[dir_index], "Volume_")))
							pstr += strlen("Volume_")+1;
												
			    	DBG_PATH_SCAN(printf("main.c: catched_mp3_path=%s\n",pstr);)
			    	pstr=alpha_win_path_to_linux(pstr);
			    	if(pstr!=NULL) {
				   		strcat(mp3path,pstr);
				   		DBG_PATH_SCAN(printf("main.c: mp3path=%s\n",mp3path);)
			    		path[dir_index]=strdup(mp3path);
			    		DBG_PATH_SCAN(printf("main.c: path[%d]=%s\n",dir_index,path[dir_index]);)
		    		}				  				   
				  }
					else if ( !strncmp(path[dir_index], "Volume_4", 8) && strlen(volume_4)) { 
					 	strcat(mp3path,volume_4);			    					
					 	DBG_PATH_SCAN(printf("Volume_4: mp3path=%s\n", mp3path);)
				  				 	 
						if((pstr = strstr(path[dir_index], "Volume_")))
							pstr += strlen("Volume_")+1;
												
			   		DBG_PATH_SCAN(printf("main.c: catched_mp3_path=%s\n",pstr);)
			   		pstr=alpha_win_path_to_linux(pstr);
			   		if(pstr!=NULL) {
				    	strcat(mp3path,pstr);
				    	DBG_PATH_SCAN(printf("main.c: mp3path=%s\n",mp3path);)
				   		path[dir_index]=strdup(mp3path);
				   		DBG_PATH_SCAN(printf("main.c: path[%d]=%s\n",dir_index,path[dir_index]);)
			   		}			    						
			   	}
			    						
					memcpy(mp3path, mp3_tmp, 128);			    					
					DBG_PATH_SCAN(printf("main.c: clean: mp3path=%s\n",mp3path);)
					DBG_PATH_SCAN(printf("\n");)
			   	dir_index++;
			   }
			   else {
			   	DBG_PATH_SCAN(printf("main.c: path[%d]=NULL\n",dir_index);)
			   	dir_index=1000;
			   }
			  }/* dir_index */			
			}/* feof(shared_name) */
			/* close the file pointer whenever it is done */
			fclose(share_folder_number);  	
	
}
/**
 * build a dns text string
 *
 * @param txtrecord buffer to append text record string to
 * @param fmt sprintf-style format
 */
void txt_add(char *txtrecord, char *fmt, ...) {
    va_list ap;
    char buff[256];
    int len;
    char *end;

    va_start(ap, fmt);
    vsnprintf(buff, sizeof(buff), fmt, ap);
    va_end(ap);

    len = (int)strlen(buff);
    if(len + strlen(txtrecord) > 255) {
        DPRINTF(E_FATAL,L_MAIN,"dns-sd text string too long.  Try a shorter "
                "share name.\n");
    }

    end = txtrecord + strlen(txtrecord);
    *end = len;
    strcpy(end+1,buff);
}

void main_handler(WS_CONNINFO *pwsc) {
    DPRINTF(E_DBG,L_MAIN,"in main_handler\n");
    if(plugin_url_candispatch(pwsc)) {
        DPRINTF(E_DBG,L_MAIN,"Dispatching %s to plugin\n",ws_uri(pwsc));
        plugin_url_handle(pwsc);
        return;
    }

    DPRINTF(E_DBG,L_MAIN,"Dispatching %s to config handler\n",ws_uri(pwsc));
    config_handler(pwsc);
}

int main_auth(WS_CONNINFO *pwsc, char *username, char *password) {
    DPRINTF(E_DBG,L_MAIN,"in main_auth\n");
    if(plugin_url_candispatch(pwsc)) {
        DPRINTF(E_DBG,L_MAIN,"Dispatching auth for %s to plugin\n",ws_uri(pwsc));
        return plugin_auth_handle(pwsc,username,password);
    }

    DPRINTF(E_DBG,L_MAIN,"Dispatching auth for %s to config auth\n",ws_uri(pwsc));
    return config_auth(pwsc, username, password);
}


/**
 * Print usage information to stdout
 *
 * \param program name of program (argv[0])
 */
void usage(char *program) {
    printf("Usage: %s [options]\n\n",program);
    printf("Options:\n");
    printf("  -a             Set cwd to app dir before starting\n");
    printf("  -d <number>    Debuglevel (0-9)\n");
    printf("  -D <mod,mod..> Debug modules\n");
    printf("  -m             Disable mDNS\n");
    printf("  -c <file>      Use configfile specified\n");
    printf("  -P <file>      Write the PID ot specified file\n");
    printf("  -f             Run in foreground\n");
    printf("  -y             Yes, go ahead and run as non-root user\n");
    printf("  -b <id>        ffid to be broadcast\n");
    printf("  -V             Display version information\n");
    printf("  -k             Kill a running daemon (based on pidfile)\n");
    printf("  -u             The codepage of the file\n");//marco
    printf("  -i             Interface that will bind to it\n");//marco
    printf("\n\n");
    printf("Valid debug modules:\n");
    printf(" config,webserver,database,scan,query,index,browse\n");
    printf(" playlist,art,daap,main,rend,misc\n");
    printf("\n\n");
}

/**
 * process a directory for plugins
 *
 * @returns TRUE if at least one plugin loaded successfully
 */
int load_plugin_dir(char *plugindir) {
    DIR *d_plugin;
    char de[sizeof(struct dirent) + MAXNAMLEN + 1]; /* ?? solaris  */
    struct dirent *pde;
    char *pext;
    char *perr=NULL;
    int loaded=FALSE;
    char plugin[PATH_MAX];

    if((d_plugin=opendir(plugindir)) == NULL) {
        DPRINTF(E_LOG,L_MAIN,"Error opening plugin dir %s.  Ignoring\n",
                plugindir);
        return FALSE;

    } else {
        while((readdir_r(d_plugin,(struct dirent *)de,&pde) != 1) && pde) {
                pext = strrchr(pde->d_name,'.');
                if((pext) && ((strcasecmp(pext,".so") == 0) ||
                   (strcasecmp(pext,".dylib") == 0) ||
                   (strcasecmp(pext,".dll") == 0))) {
                    /* must be a plugin */
                    snprintf(plugin,PATH_MAX,"%s%c%s",plugindir,
                             PATHSEP,pde->d_name);
                    if(plugin_load(&perr,plugin) != PLUGIN_E_SUCCESS) {
                        DPRINTF(E_LOG,L_MAIN,"Error loading plugin %s: %s\n",
                                plugin,perr);
                        free(perr);
                        perr = NULL;
                    } else {
                        loaded = TRUE;
                    }
                }
        }
        closedir(d_plugin);
    }

    return loaded;
}

/**
 * set up an errorhandler for io errors
 *
 * @param int level of the error (0=fatal, 9=debug)
 * @param msg the text error
 */
void main_io_errhandler(int level, char *msg) {
    DPRINTF(level,L_MAIN,"%s",msg);
}

/**
 * set up an errorhandler for webserver errors
 *
 * @param int level of the error (0=fatal, 9=debug)
 * @param msg the text error
 */
void main_ws_errhandler(int level, char *msg) {
    DPRINTF(level,L_WS,"%s",msg);
}
/**
 * Kick off the daap server and wait for events.
 *
 * This starts the initial db scan, sets up the signal
 * handling, starts the webserver, then sits back and waits
 * for events, as notified by the signal handler and the
 * web interface.  These events are communicated via flags
 * in the config structure.
 *
 * \param argc count of command line arguments
 * \param argv command line argument pointers
 * \returns 0 on success, -1 otherwise
 *
 * \todo split out a ws_init and ws_start, so that the
 * web space handlers can be registered before the webserver
 * starts.
 *
 */
int main(int argc, char *argv[]) {
    int option;
    char *configfile=CONFFILE;
    WSCONFIG ws_config;
    int reload=0;
    int start_time;
    int end_time;
    int rescan_counter=0;
    int old_song_count, song_count;
    int force_non_root=0;
    int skip_initial=1;
    int kill_server=0;
    int convert_conf=0;
    char *db_type,*db_parms,*web_root,*runas, *tmp;
    char **mp3_dir_array;
    char *servername, *iface;
    char *ffid = NULL;
    int appdir = 0;
    char *perr=NULL;
    char txtrecord[255];
    void *phandle;
    char *plugindir;
    int err;
    char *apppath;
    int debuglevel=0;
    int plugins_loaded = 0; 
    int total_mp3 =0;
    int scan_finish=0;
    config.use_mdns=1;
    err_setlevel(2);
    config.foreground=0;
    int clean_IPC=0;
    int server_ready=0;
    int server_flag_test=0;
	char *code_page=NULL;//marco
                                
    while((option=getopt(argc,argv,"D:d:c:P:i:mfrysi:u:vab:Vk")) != -1) {
        switch(option) {
        case 'a':
            appdir = 1;
            break;

        case 'b':
            ffid=optarg;
            break;

        case 'd':
            debuglevel = atoi(optarg);
            err_setlevel(debuglevel);
            break;

        case 'D':
            if(err_setdebugmask(optarg)) {
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'f':
            config.foreground=1;
            err_setdest(err_getdest() | LOGDEST_STDERR);
            break;

        case 'c':
            configfile=optarg;
            break;

        case 'm':
            config.use_mdns=0;
            break;

#ifndef WIN32
        case 'P':
            os_set_pidfile(optarg);
            break;
#endif
        case 'r':
            reload=1;
            break;

        case 's':
            skip_initial=0;
            break;

        case 'y':
            force_non_root=1;
            break;

#ifdef WIN32
        case 'i':
            os_register();
            exit(EXIT_SUCCESS);
            break;

        case 'u':
            os_unregister();
            exit(EXIT_SUCCESS);
            break;
#else//marco            
        case 'u':
			code_page=optarg;
            break;
#endif
        case 'v':
            convert_conf=1;
            break;

        case 'k':
            kill_server=1;
            break;

        case 'V':
            fprintf(stderr,"Firefly Media Server: Version %s\n",VERSION);
            exit(EXIT_SUCCESS);
            break;
            
		case 'i':
			memset(device_inf, 0, sizeof(device_inf));
			strncpy(device_inf, optarg, 9);		
			printf("%s %d\n",device_inf,__LINE__);	
			break;
		
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    if((getuid()) && (!force_non_root) && (!convert_conf)) {
        fprintf(stderr,"You are not root.  This is almost certainly wrong.  "
                "If you are\nsure you want to do this, use the -y "
                "command-line switch\n");
        exit(EXIT_FAILURE);
    }


    if(kill_server) {
        os_signal_server(S_STOP);
        exit(0);
    }

    io_init();
    io_set_errhandler(main_io_errhandler);
    ws_set_errhandler(main_ws_errhandler);

    /* read the configfile, if specified, otherwise
     * try defaults */
    config.stats.start_time=start_time=(int)time(NULL);
    config.stop=0;

    /* set appdir first, that way config resolves relative to appdir */
    if(appdir) {
        apppath = os_apppath(argv[0]);
        DPRINTF(E_INF,L_MAIN,"Changing cwd to %s\n",apppath);
        chdir(apppath);
        free(apppath);
        configfile="mt-daapd.conf";
    }

    if(CONF_E_SUCCESS != conf_read(configfile)) {
        fprintf(stderr,"Error reading config file (%s)\n",configfile);
        exit(EXIT_FAILURE);
    }

    if(debuglevel) /* was specified, should override the config file */
        err_setlevel(debuglevel);

    if(convert_conf) {
        fprintf(stderr,"Converting config file...\n");
        if(CONF_E_SUCCESS != conf_write()) {
            fprintf(stderr,"Error writing config file.\n");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    DPRINTF(E_LOG,L_MAIN,"Firefly Version %s: Starting with debuglevel %d\n",
            VERSION,err_getlevel());


    /* load plugins before we drop privs?  Maybe... let the
     * plugins do stuff they might need to */
    plugin_init();
    if((plugindir=conf_alloc_string("plugins","plugin_dir",NULL)) != NULL) {
        /* instead of specifying plugins, let's walk through the directory
         * and load each of them */
        if(!load_plugin_dir(plugindir)) {
            DPRINTF(E_LOG,L_MAIN,"Warning: Could not load plugins\n");
        } else {
            plugins_loaded = TRUE;
        }
        free(plugindir);
    }

    if(!plugins_loaded) {
        if((!load_plugin_dir("/usr/lib/firefly/plugins")) &&
           (!load_plugin_dir("/usr/lib/mt-daapd/plugins")) &&
           (!load_plugin_dir("/lib/mt-daapd/plugins")) &&
           (!load_plugin_dir("/lib/mt-daapd/plugins")) &&
           (!load_plugin_dir("/usr/local/lib/mt-daapd/plugins")) &&
           (!load_plugin_dir("/usr/local/lib/mt-daapd/plugins")) &&
           (!load_plugin_dir("/opt/share/firefly/plugins")) &&
           (!load_plugin_dir("/opt/share/mt-daapd/plugins")) &&
           (!load_plugin_dir("/opt/lib/firefly/plugins")) &&
           (!load_plugin_dir("/opt/lib/mt-daapd/plugins")) &&
           (!load_plugin_dir("plugins/.libs"))) {
            DPRINTF(E_FATAL,L_MAIN,"plugins/plugin_dir not specified\n");
        }
    }

    phandle=NULL;
    while((phandle=plugin_enum(phandle))) {
        DPRINTF(E_LOG,L_MAIN,"Plugin loaded: %s\n",plugin_get_description(phandle));
    }

    runas = conf_alloc_string("general","runas","nobody");

#ifndef WITHOUT_MDNS
    if(config.use_mdns) {
        DPRINTF(E_LOG,L_MAIN,"Starting rendezvous daemon\n");
        if(rend_init(runas)) {
            DPRINTF(E_FATAL,L_MAIN|L_REND,"Error in rend_init: %s\n",
                    strerror(errno));
        }
    }
#endif

    if(!os_init(config.foreground,runas)) {
        DPRINTF(E_LOG,L_MAIN,"Could not initialize server\n");
        os_deinit();
        exit(EXIT_FAILURE);
    }
    free(runas);

#ifdef UPNP
    upnp_init();
#endif

    /* this will require that the db be readable by the runas user */
    db_type = conf_alloc_string("general","db_type","sqlite");
    db_parms = conf_alloc_string("general","db_parms","/var/cache/mt-daapd");
    err=db_open(&perr,db_type,db_parms);

    if(err) {
        DPRINTF(E_LOG,L_MAIN|L_DB,"Error opening db: %s\n",perr);
#ifndef WITHOUT_MDNS
        if(config.use_mdns) {
            rend_stop();
        }
#endif
        os_deinit();
        exit(EXIT_FAILURE);
    }

    free(db_type);
    free(db_parms);

    /* Initialize the database before starting */
    DPRINTF(E_LOG,L_MAIN|L_DB,"Initializing database\n");
    if(db_init(reload)) {
        DPRINTF(E_FATAL,L_MAIN|L_DB,"Error in db_init: %s\n",strerror(errno));
    }

    err=db_get_song_count(&perr,&song_count);
    if(err != DB_E_SUCCESS) {
        DPRINTF(E_FATAL,L_MISC,"Error getting song count: %s\n",perr);
    }
#if 1
	if(!song_count)
		reload = 1;
#else 
    /* do a full reload if the db is empty */
    if(!song_count){
    	/* compute total number of mp3 */
			conf_get_array("general","mp3_dir",&mp3_dir_array);		
			total_mp3 = get_mp3_count(mp3_dir_array[0]);			
			printf("total mp3=%d\n",total_mp3);
      if(total_mp3 == 0){
		  	scan_finish = 1;                                                                              
      }
      else{
 
      	reload = 1;
      }
    }
    else{
			/* already scan finished */                                                                    	
		  scan_finish=1;                                                                              

		}    
#endif	  
    if(conf_get_array("general","mp3_dir",&mp3_dir_array)) {   																	
        if((!skip_initial) || (reload)) {
            DPRINTF(E_LOG,L_MAIN|L_SCAN,"Starting mp3 scan\n");
            plugin_event_dispatch(PLUGIN_EVENT_FULLSCAN_START,0,NULL,0);
            start_time=(int) time(NULL);
            if(scan_init(mp3_dir_array,code_page)) {
                DPRINTF(E_LOG,L_MAIN|L_SCAN,"Error scanning MP3 files: %s\n",strerror(errno));
            }
            if(!config.stop) { /* don't send popup when shutting down */
                plugin_event_dispatch(PLUGIN_EVENT_FULLSCAN_END,0,NULL,0);
                err=db_get_song_count(&perr,&song_count);
                end_time=(int) time(NULL);
                DPRINTF(E_LOG,L_MAIN|L_SCAN,"Scanned %d songs in %d seconds\n",
                        song_count,end_time - start_time);
                                                                    
		            /* scanning is finish */
		            scan_finish=1;
						//		LIB_Set_IPC_int(MIPC_ITUNES,ITUNES_FINISH,ITUNES_OK, MIPC_TIME_ALWAYS,(int)scan_finish);                       
            }
        }
        conf_dispose_array(mp3_dir_array);
    }

    /* start up the web server */
    web_root = conf_alloc_string("general","web_root",NULL);
    ws_config.web_root=web_root;
    ws_config.port=conf_get_int("general","port",0);

    DPRINTF(E_LOG,L_MAIN|L_WS,"Starting web server from %s on port %d\n",
            ws_config.web_root, ws_config.port);

    config.server=ws_init(&ws_config);
    if(!config.server) {
        /* pthreads or malloc error */
        DPRINTF(E_FATAL,L_MAIN|L_WS,"Error initializing web server\n");
    }

    if(E_WS_SUCCESS != ws_start(config.server)) {
        /* listen or pthread error */
        DPRINTF(E_FATAL,L_MAIN|L_WS,"Error starting web server\n");
    }

    ws_registerhandler(config.server, "/",main_handler,main_auth,
                       0,1);

#ifndef WITHOUT_MDNS
    if(config.use_mdns) { /* register services */
        servername = conf_get_servername();

        memset(txtrecord,0,sizeof(txtrecord));
        txt_add(txtrecord,"txtvers=1");
        txt_add(txtrecord,"Database ID=%0X",util_djb_hash_str(servername));
        txt_add(txtrecord,"Machine ID=%0X",util_djb_hash_str(servername));
        txt_add(txtrecord,"Machine Name=%s",servername);
        txt_add(txtrecord,"mtd-version=" VERSION);
        txt_add(txtrecord,"iTSh Version=131073"); /* iTunes 6.0.4 */
        txt_add(txtrecord,"Version=196610");      /* iTunes 6.0.4 */
        tmp = conf_alloc_string("general","password",NULL);
        if(tmp && (strlen(tmp)==0)) tmp=NULL;

        txt_add(txtrecord,"Password=%s",tmp ? "true" : "false");
        if(tmp) free(tmp);

        srand((unsigned int)time(NULL));

        if(ffid) {
            txt_add(txtrecord,"ffid=%s",ffid);
        } else {
            txt_add(txtrecord,"ffid=%08x",rand());
        }

        DPRINTF(E_LOG,L_MAIN|L_REND,"Registering rendezvous names\n");
        iface = conf_alloc_string("general","interface","");

        rend_register(servername,"_http._tcp",ws_config.port,iface,txtrecord);

        plugin_rend_register(servername,ws_config.port,iface,txtrecord);

        free(servername);
        free(iface);
    }
#endif

    end_time=(int) time(NULL);

    err=db_get_song_count(&perr,&song_count);
    if(err != DB_E_SUCCESS) {
        DPRINTF(E_FATAL,L_MISC,"Error getting song count: %s\n",perr);
    }

    DPRINTF(E_LOG,L_MAIN,"Serving %d songs.  Startup complete in %d seconds\n",
            song_count,end_time-start_time);
		
		/* 08-04-2010 add server_ready flag */
		server_ready = 1;
		
    if(conf_get_int("general","rescan_interval",0) && (!reload) &&
       (!conf_get_int("scanning","skip_first",0)))
        config.reload = 1; /* force a reload on start */
			
    while(!config.stop) {
        if( (conf_get_int("general", "rescan_interval", 0) && 
        		(rescan_counter > conf_get_int("general", "rescan_interval", 0)) )) {
        	if((conf_get_int("general","always_scan",0)) || (config_get_session_count())) {
          	config.reload=1;
          } 
          else{
          	DPRINTF(E_DBG,L_MAIN|L_SCAN|L_DB,"Skipped bground scan... no users\n");
          }
          rescan_counter=0;
    		}
        if(config.reload) {
        		/* start scanning */
            old_song_count = song_count;
            start_time=(int) time(NULL);
            DPRINTF(E_LOG,L_MAIN|L_DB|L_SCAN,"Rescanning database\n");
			db_init(1);//marco
            if(conf_get_array("general","mp3_dir",&mp3_dir_array)) {
            //alpha_transform_volume(mp3_dir_array);
						//curtis@alpha everytime restart server do force_scan 12_24_2009  
						 #if 0  
             if(config.full_reload) {
              	config.full_reload=0;
            		db_force_rescan(NULL);
		   					config.reload=0;
            	}
						 #endif
						 
             	if(scan_init(mp3_dir_array,code_page)) {
             		DPRINTF(E_LOG,L_MAIN|L_DB|L_SCAN,"Error rescanning... bad path?\n");
             	}
              conf_dispose_array(mp3_dir_array);
            }
            config.reload=0;
            db_get_song_count(NULL,&song_count);
            DPRINTF(E_LOG,L_MAIN|L_DB|L_SCAN,"Scanned %d songs (was %d) in "
                    "%d seconds\n",song_count,old_song_count,
                    time(NULL)-start_time);
                    
            /* scanning is finish */
            scan_finish=1;
					//	LIB_Set_IPC_int(MIPC_ITUNES,ITUNES_FINISH,ITUNES_OK,MIPC_TIME_ALWAYS,(int)scan_finish);                           
        }

        os_wait(MAIN_SLEEP_INTERVAL);
        rescan_counter += MAIN_SLEEP_INTERVAL;
    }

    DPRINTF(E_LOG,L_MAIN,"Stopping gracefully\n");

#ifndef WITHOUT_MDNS
    if(config.use_mdns) {
        DPRINTF(E_LOG,L_MAIN|L_REND,"Stopping rendezvous daemon\n");
        rend_stop();
    }
#endif

#ifdef UPNP
    upnp_deinit();
#endif


    /* Got to find a cleaner way to stop the web server.
     * Closing the fd of the socking accepting doesn't necessarily
     * cause the accept to fail on some libcs.
     *
    DPRINTF(E_LOG,L_MAIN|L_WS,"Stopping web server\n");
    ws_stop(config.server);
    */
    free(web_root);
    conf_close();

    DPRINTF(E_LOG,L_MAIN|L_DB,"Closing database\n");
    db_deinit();

    DPRINTF(E_LOG,L_MAIN,"Done!\n");

    os_deinit();
    io_deinit();
    mem_dump();
    return EXIT_SUCCESS;
}

