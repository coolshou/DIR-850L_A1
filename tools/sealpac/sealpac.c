/* vi: set sw=4 ts=4: */
/*
 *	sealpac - (SEA)ttle (L)anguage (PAC)k
 *	The language pack maker for Project Seattle.
 *
 *	Created by David Hsieh <david_hsieh@alphanetworks.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <dtrace.h>
#include <dlist.h>
#include <mem_helper.h>
#include <strobj.h>
#include <md5.h>

#include "globals.h"

/**************************************************************************/

#define MAX_FILE_TYPE	16

extern int optind;
extern char * optarg;

/* Options */
static int		o_verbose = 0;		/* verbose mode. */
static int		o_recursive = 0;	/* to subdirectories recursively */
static char *	o_targetpath = NULL;
static int		o_listfile = 0;		/* print the file list only. */
static char *	o_types[MAX_FILE_TYPE] = {NULL};
static size_t	o_typecount = 0;
static char *	o_langcode = "en";	/* language code. */
static char *	o_slpfile = NULL;	/* output file */

/* Global Variables */
static size_t	g_filecount = 0;	/* the global var for counting files. */

/**************************************************************************/

static void verbose(const char * format, ...)
{
	va_list marker;
	if (o_verbose)
	{
		va_start(marker, format);
		vfprintf(stdout, format, marker);
		va_end(marker);
	}
}

static void free_i18n_strings(void);

static void cleanup_exit(int exit_code)
{
	verbose("%s: exit with code %d\n", PROGNAME, exit_code);
	free_i18n_strings();
	mh_dump_used(stdout);
	mh_dump(stdout);
	mh_diagnostic(stdout);
	exit(exit_code);
}

static void show_usage(int exit_code)
{
	printf(	PROGNAME " version " VERSION "\n"
			"usage: " PROGNAME " [OPTIONS]\n"
			"  -h                 show this help message.\n"
			"  -v                 verbose mode.\n"
			"  -r                 recursive.\n"
			"  -l                 list files.\n"
			"  -c {langcode}      language code.\n"
			"  -f {file}          sealpac file.\n"
			"  -d {target}        target directory or file.\n"
			"  -t {type}          file type, file extension.\n"
			"\n"
			"  Sealpac will search the directory & files (specified with -d, -t & -r) for\n"
			"  i18n() functions of EPHP and extract the text. The text string will be save\n"
			"  in the Sealpac file (specified with -f).\n"
			"  ex:\n"
			"  To create sealpac: sealpac -d rootfs -t php -r -f sealpac.slt\n"
			"  To dump info:      sealpac -f sealpac.slt -v\n"
			);
	cleanup_exit(exit_code);
}

static int parse_args(int argc, char * argv[])
{
	int opt;

	while ((opt=getopt(argc, argv, "hvrlc:f:d:t:")) > 0)
	{
		switch (opt)
		{
		default:	show_usage(9); break;
		case 'h':	show_usage(0); break;
		case 'v':	o_verbose++; break;
		case 'r':	o_recursive++; break;
		case 'l':	o_listfile++; break;
		case 'c':	o_langcode = optarg; break;
		case 'f':	o_slpfile = optarg; break;
		case 'd':	o_targetpath = optarg; break;
		case 't':
			if (o_typecount < MAX_FILE_TYPE)
				o_types[o_typecount++] = optarg;
			break;
		}
	}
	return 0;
}

/**************************************************************************/
/* The functions are required by ephp.c. */
int client_puts(const char * str, int fd)
{
	return printf("%s", str);
}
int client_printf(int fd, const char * format, ...)
{
	int ret;
	va_list marker;

	va_start(marker, format);
	ret = vprintf(format, marker);
	va_end(marker);
	return ret;
}

/**************************************************************************/
/* Scan files */
static int match_type(const char * fname)
{
	char * ptr;
	size_t i;

	if (o_typecount == 0) return 1;

	ptr = strrchr(fname, '.');
	if (ptr)
	{
		ptr++;
		for (i=0; i<o_typecount; i++)
			if (strcmp(o_types[i], ptr)==0)
				return 1;
	}
	return 0;
}

static int read_file_list(const char * path, solist_t files, solist_t dirs)
{
	int ret;
	struct dirent **namelist;
	strobj_t sobj = NULL;
	const char * errmsg;

	struct stat st;
	char buf[1024];

	//d_dbg("%s: path = %s\n",__func__, path);
	ret = scandir(path, &namelist, 0, alphasort);
	if (ret < 0) perror("scandir");
	else
	{
		while (ret > 0)
		{
			ret--;
			errmsg = NULL;
#if 1//marco, use lstat to retrieve the file type of the files under 'path'
	//in previous version, we use d_type to know the file_type but some filesystem
	//doesn't support it, so the values will always 0 and become DT_UNKNOWN
			sprintf(buf,"%s/%s",path,namelist[ret]->d_name);
			lstat(buf, &st);
			if (S_ISLNK(st.st_mode))
			{
				errmsg = "link";
				goto ERR;
			}
			else if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))
			{
				if (namelist[ret]->d_name[0]=='.')
				{
					errmsg = "hidden";
					goto ERR;
					//	break;
				}
				if (S_ISREG(st.st_mode) && !match_type(namelist[ret]->d_name))
				{
					errmsg = "unmatch";
					goto ERR;
					//	break;
				}
				sobj = sobj_new();
				if (sobj)
				{
					sobj_add_string(sobj, path);
					sobj_add_char(sobj, '/');
					sobj_add_string(sobj, namelist[ret]->d_name);
					verbose("Found %s file - '%s'\n",
						( S_ISDIR(st.st_mode)) ? "DT_DIR":"DT_REG",
						sobj_get_string(sobj));
					solist_add( (S_ISDIR(st.st_mode))?dirs : files, sobj);
				}
				else
				{
					verbose("Memory allocation failed !!\n");
				}
			}
			else
			{
				errmsg = "UNKNOWN FILE TYPE";				
			}	
ERR:			
#else				
			switch (namelist[ret]->d_type)
			{
			case DT_UNKNOWN:	errmsg = "DT_UNKNOWN";	break;
			case DT_FIFO:		errmsg = "DT_FIFO";		break;
			case DT_CHR:		errmsg = "DT_CHR";		break;
			case DT_BLK:		errmsg = "DT_BLK";		break;
			case DT_LNK:		errmsg = "DT_LNK";		break;
			case DT_SOCK:		errmsg = "DT_SOCK";		break;
			case DT_WHT:		errmsg = "DT_WHT";		break;

			case DT_DIR:
			case DT_REG:
				if (namelist[ret]->d_name[0]=='.')
				{
					errmsg = "hidden";
					break;
				}
				if (namelist[ret]->d_type==DT_REG && !match_type(namelist[ret]->d_name))
				{
					errmsg = "unmatch";
					break;
				}
				sobj = sobj_new();
				if (sobj)
				{
					sobj_add_string(sobj, path);
					sobj_add_char(sobj, '/');
					sobj_add_string(sobj, namelist[ret]->d_name);
					verbose("Found %s file - '%s'\n",
						(namelist[ret]->d_type==DT_DIR) ? "DT_DIR":"DT_REG",
						sobj_get_string(sobj));
					solist_add((namelist[ret]->d_type == DT_DIR) ?
								dirs : files, sobj);
				}
				else
				{
					verbose("Memory allocation failed !!\n");
				}
				break;
			}
#endif			
			if (errmsg && o_verbose > 1)
			{
				verbose("Ignoring %s file - '%s'\n",
					errmsg, namelist[ret]->d_name);
			}
			free(namelist[ret]);
		}
		free(namelist);
	}
	//d_dbg("%s: return %d\n",__func__,ret);
	return ret;
}

static int read_file(const char * fname)
{
	g_filecount++;
	if (o_listfile) printf("%s\n", fname);
	else xmldb_ephp(1, fname, 0);
	return 0;
}

static int read_directory(const char * dname)
{
	int ret = 0;
	solist_t files;
	solist_t dirs;
	strobj_t sobj;

	//d_dbg("%s: dname = '%s'\n",__func__,dname);

	do
	{
		files = solist_new();
		dirs = solist_new();
		if (!files || !dirs)
		{
			ret = -1;
			verbose("solist_new() return error!\n");
			break;
		}

		ret = read_file_list(dname, files, dirs);
		if (ret < 0)
		{
			verbose("error reading file list @ '%s' !\n", dname);
			break;
		}

		/* handle files first */
		ret = 0;
		while ((sobj=solist_get_next(files)) != NULL)
		{
			ret = read_file(sobj_get_string(sobj));
			if (ret < 0)
			{
				verbose("error handling file - '%s'\n",sobj_get_string(sobj));
				break;
			}
		}
		if (ret < 0) break;

		/* handle diretories if recursive. */
		if (o_recursive)
		{
			ret = 0;
			while ((sobj=solist_get_next(dirs)) != NULL)
			{
				ret = read_directory(sobj_get_string(sobj));
				if (ret < 0)
				{
					verbose("error handling directory - '%s'\n",sobj_get_string(sobj));
					break;
				}
			}
			if (ret < 0) break;
		}

	} while (0);
	if (files) solist_del(files);
	if (dirs) solist_del(dirs);
	return ret;
}

/**************************************************************************/
/* Write language pack file. */

#define SEALPAC_MAGIC	0x5ea19ac	/* The magic of sealpac */

/* I did not pack this structure. It should be packed already. */
/* All the header entries should be stored in network byte order. */
typedef struct sealpac_header sealpac_hdr;
struct sealpac_header
{
	uint32_t	magic;	/* SEALPAC_MAGIC */
	uint32_t	num;	/* number of string in this pack. */
	uint32_t	reserved[2];
	char		langcode[16];
	uint8_t		checksum[16];
};

/* string header */
typedef struct sealpac_string sealpac_str;
struct sealpac_string
{
	uint8_t		hash[16];
	uint32_t	offset;
};

/* i18n string entry */
typedef struct i18n_string_entry i18nstr_t;
struct i18n_string_entry
{
	struct dlist_head	list;
	uint8_t				digest[16];
	char *				string;
};
static DLIST_HEAD(g_i18n_strings);

/**************************************************************************/
static void free_i18n_strings(void)
{
	struct dlist_head * entry;
	i18nstr_t * istr;

	while (!dlist_empty(&g_i18n_strings))
	{
		entry = g_i18n_strings.next;
		dlist_del(entry);
		istr = dlist_entry(entry, i18nstr_t, list);

		if (istr->string) FREE(istr->string);
		FREE(istr);
	}
}

static size_t count_i18n_strings(void)
{
	size_t i = 0;
	struct dlist_head * entry = NULL;

	while ((entry = dlist_get_next(entry, &g_i18n_strings))) i++;
	return i;
}

/**************************************************************************/
#define ERRBREAK(fmt, args...) { printf(fmt, ##args); break;  }

static void dump_i18n_strings(void)
{
	struct dlist_head * entry;
	i18nstr_t * istr;
	size_t i, num;
	uint32_t offset;
	sealpac_hdr sphdr;
	sealpac_str * spstr = NULL;
	FILE * fh = NULL;
	MD5_CTX ctx;

	/* dump the string list. */
	if (o_verbose > 2)
	{
		entry = NULL;
		while ((entry = dlist_get_next(entry, &g_i18n_strings)))
		{
			istr = dlist_entry(entry, i18nstr_t, list);
			verbose("[");
			for (i=0; i<16; i++) verbose("%02x", istr->digest[i]);
			verbose("] : [%s]\n", istr->string);
		}
	}

	num = count_i18n_strings();
	printf("Number of strings : %d\n", num);

	do
	{
		if (num == 0) break;
		if (o_targetpath == NULL) break;

		/* Setup sealpac header */
		memset(&sphdr, 0, sizeof(sphdr));
		sphdr.magic	= htonl(SEALPAC_MAGIC);
		sphdr.num	= htonl(num);
		strncpy(sphdr.langcode, o_langcode, sizeof(sphdr.langcode));

		/* Allocate sealpac string head */
		spstr = MALLOC(sizeof(sealpac_str) * num);
		if (!spstr) ERRBREAK("%s: Memory Allocation Failed!\n",__func__);
		memset(spstr, 0, sizeof(sealpac_str) * num);

		/* Walk through all the string to prepare sealpac string heads */
		i = 0;
		offset = sizeof(sealpac_hdr) + sizeof(sealpac_str) * num;
		entry = NULL;
		while ((entry = dlist_get_next(entry, &g_i18n_strings)))
		{
			istr = dlist_entry(entry, i18nstr_t, list);
			memcpy(spstr[i].hash, istr->digest, 16);
			spstr[i].offset = htonl(offset);
			offset += (strlen(istr->string) + 1);
			i++;
		}

		/* Calculate the checksum (MD5 digest) */
		MD5Init(&ctx);
		MD5Update(&ctx, (uint8_t *)spstr, sizeof(sealpac_str) * num);
		entry = NULL;
		while ((entry = dlist_get_next(entry, &g_i18n_strings)))
		{
			istr = dlist_entry(entry, i18nstr_t, list);
			MD5Update(&ctx, (uint8_t *)istr->string, strlen(istr->string)+1);
		}
		MD5Final(sphdr.checksum, &ctx);

		/* Open the file to write */
		if (o_slpfile == NULL) ERRBREAK("No output file !\n");
		fh = fopen(o_slpfile, "w+");
		if (fh == NULL) ERRBREAK("ERROR!!! Unable to open output file '%s', %s\n",
							o_slpfile, strerror(errno));

		/* write to the langpack file. */
		fwrite(&sphdr, sizeof(sealpac_hdr), 1, fh);
		fwrite(spstr, sizeof(sealpac_str), num, fh);
		entry = NULL;
		while ((entry = dlist_get_next(entry, &g_i18n_strings)))
		{
			istr = dlist_entry(entry, i18nstr_t, list);
			fwrite(istr->string, sizeof(char), strlen(istr->string)+1, fh);
		}

	} while (0);
	if (spstr) FREE(spstr);
	if (fh) fclose(fh);
}

static int compare_i18nstr(i18nstr_t * str1, i18nstr_t * str2)
{
	return memcmp(str1->digest, str2->digest, 16);
}

/**************************************************************************/

static void dump_sealpac_hdr(sealpac_hdr * hdr)
{
	int i;

	printf("==========================================================\n");
	printf("Magic    : %08x\n", ntohl(hdr->magic));
	printf("Num      : %d\n", ntohl(hdr->num));
	printf("langcode : %s\n", hdr->langcode);
	printf("Checksum : ");
	for (i=0; i<16; i++) printf("%02x ", hdr->checksum[i]);
	printf("\n");
	printf("==========================================================\n");
}

static int read_sealpac(const char * filename)
{
	int ret = 0;
	size_t i, num;
	sealpac_hdr sphdr;
	sealpac_str * spstr = NULL;
	FILE * fh = NULL;
	uint8_t digest[16];
	uint8_t buf[256];
	MD5_CTX ctx;
	int c;
	strobj_t sobj = NULL;

	do
	{
		/* open file for reading */
		fh = fopen(filename, "r");
		if (!fh) ERRBREAK("Unable to open file '%s' for reading. %s. Creating a new one.\n",filename, strerror(errno));

		/* Read sealpac head */
		if (fread(&sphdr, sizeof(sealpac_hdr), 1, fh) != 1) ERRBREAK("Unable to read sealpac header!\n");
		if (sphdr.magic != htonl(SEALPAC_MAGIC)) ERRBREAK("Invalid MAGIC!\n");

		/* Read & calculate the MD5 digest */
		MD5Init(&ctx);
		while (!ferror(fh) && !feof(fh))
		{
			i = fread(buf, sizeof(uint8_t), 256, fh);
			if (i > 0) MD5Update(&ctx, (uint8_t *)buf, i);
		}
		MD5Final(digest, &ctx);
		if (memcmp(digest, sphdr.checksum, 16)) ERRBREAK("Invalid Checksum !\n");

		/* dump the header */
		if (o_verbose) dump_sealpac_hdr(&sphdr);

		/* read sealpac_str */
		if ((num = ntohl(sphdr.num)) == 0) break;
		fseek(fh, sizeof(sealpac_hdr), SEEK_SET);
		spstr = MALLOC(sizeof(sealpac_str) * num);
		if (!spstr) ERRBREAK("Memory Allocation Failed !\n");
		if (fread(spstr, sizeof(sealpac_str), num, fh) != num)
			ERRBREAK("Unable to read sealpac string head!\n");

		/* We need a string oject */
		sobj = sobj_new();
		if (!sobj) ERRBREAK("Memory Allocation Failed!\n");

		for (i=0; i<num; i++)
		{
			if (fseek(fh, ntohl(spstr[i].offset), SEEK_SET) < 0) ERRBREAK("Read file error @ %d\n", i);
			sobj_free(sobj);
			while ((c = fgetc(fh)) != EOF) { if (c) sobj_add_char(sobj, c); else break; }
			sealpac_puts(sobj_get_string(sobj));
		}
		
	} while (0);
	if (sobj) sobj_del(sobj);
	if (fh) fclose(fh);
	if (spstr) FREE(spstr);
	return ret;
}

/**************************************************************************/

void sealpac_puts(const char * string)
{
	MD5_CTX ctx;
	i18nstr_t * istr;
	i18nstr_t * newstr;
	struct dlist_head * entry;
	int comp;

	if (!string) return;
	if (strlen(string)<=0) return;

	/* Allocate a new entry */
	newstr = MALLOC(sizeof(i18nstr_t));
	if (!newstr)
	{
		printf("%s: memory allocation failed!\n",__func__);
		return;
	}

	/* fill up this entry */
	newstr->string = STRDUP(string);
	if (!newstr->string)
	{
		printf("%s: memory allocation failed!\n",__func__);
		return;
	}
	MD5Init(&ctx);
	MD5Update(&ctx, (uint8_t *)newstr->string, strlen(newstr->string));
	MD5Final(newstr->digest, &ctx);

	/* Look for insert point */
	entry = NULL;
	while ((entry = dlist_get_next(entry, &g_i18n_strings)))
	{
		istr = dlist_entry(entry, i18nstr_t, list);
		comp = compare_i18nstr(istr, newstr);
		if (comp > 0) /* istr > newstr */
		{
			dlist_add_tail(&newstr->list, entry);
			if (o_verbose > 1) verbose("Add string '%s'\n",newstr->string);
			return;
		}
		else if (comp == 0) /* digest conflict */
		{
			if (strcmp(newstr->string, istr->string)==0)
			{
				if (o_verbose > 1) verbose("Duplicated string : '%s'\n", string);
			}
			else
			{
				printf("ERROR!! Digest Conflict. Please change the string '%s'\n",string);
			}
			FREE(newstr->string);
			FREE(newstr);
			return;
		}
	}
	dlist_add_tail(&newstr->list, &g_i18n_strings);
	if (o_verbose > 1) verbose("Add string '%s'\n",newstr->string);
	return;
}

/**************************************************************************/

int main(int argc, char * argv[], char * env[])
{
	struct stat st;
	int ret = -1;

    /* strobj.c need memory helper */
	mh_init_all();
	parse_args(argc, argv);

	do
	{
		/* If we have output, read the output first. */
		if (o_slpfile) read_sealpac(o_slpfile);

		/* If we have files to read, read the files. */
		if (o_targetpath != NULL)
		{
			if (stat(o_targetpath, &st) < 0)
			{
				printf("stat() error, %s !\n",strerror(errno));
				break;
			}
			if		(S_ISREG(st.st_mode))	ret = read_file(o_targetpath);
			else if	(S_ISDIR(st.st_mode))	ret = read_directory(o_targetpath);
			else
			{
				printf("Unable to handle '%s'.\n",o_targetpath);
				break;
			}
			printf("Number of files read : %d\n",g_filecount);
		}

		/* Dump to output */
		dump_i18n_strings();
		ret = 0;
	} while (0);
	cleanup_exit(ret);
	return ret;
}
