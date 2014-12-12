/* vi: set sw=4 ts=4: */
#ifndef __LIB_RGBIN_HEADER__
#define __LIB_RGBIN_HEADER__

#define MAX_CMD_LEN		1024

#define IS_WHITE(c)		(((c)==' ' || (c)=='\t') ? 1 : 0)

/* pointer array */
typedef struct ptr_array	parray_t;
struct ptr_array
{
	size_t growsby;
	size_t size;
	void ** array;
};

void lrgbin_pa_init(parray_t * pa, size_t growsby);
void lrgbin_pa_destroy(parray_t * pa, int do_free);
int  lrgbin_pa_grows(parray_t * pa, size_t size);
void * lrgbin_pa_get_nth(parray_t * pa, size_t index);
int  lrgbin_pa_set_nth(parray_t * pa, size_t index, void * pointer, int do_free);

/* function prototypes */
int lrgbin_run_shell(char * buf, int size, const char * format, ...);
int lrgbin_system(const char * format, ...);
char * lrgbin_eatwhite(char * string);
void lrgbin_reatwhite(char * ptr);
const char * lrgbin_getdaystring(int start, int end);
char * lrgbin_get_line_from_file(FILE * file, int strip_new_line);

/* function to access xmldb */
int lrgdb_open(const char * sockname);
void lrgdb_close(int fd);
int lrgdb_get(int fd, unsigned long flags, const char * node, FILE * out);
int lrgdb_getwb(int fd, char * buff, size_t size, const char * format, ...);
int lrgdb_patch(int fd, unsigned long flags, const char * tempfile, FILE * out);
int lrgdb_ephp(int fd, unsigned long flags, const char * phpfile, FILE * out);
int lrgdb_patch_buffer(int fd, unsigned long flags, const char * buffer, FILE * out);
int lrgdb_set(int fd, unsigned long flags, const char * node, const char * value);
int lrgdb_del(int fd, unsigned long flags, const char * node);
int lrgdb_reload(int fd, unsigned long flags, const char * file);
int lrgdb_dump(int fd, unsigned long flags, const char * file);
int lrgdb_set_notify(int fd, unsigned long flags, const char * node, const char * message);

#endif
